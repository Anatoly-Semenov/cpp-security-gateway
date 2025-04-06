#include "gateway/gateway_server.hpp"
#include "config/environment.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include "security/ddos_protection.hpp"
#include "security/blacklist_manager.hpp"
#include "security/redis_client.hpp"

namespace gateway {

GatewayServer::GatewayServer(
    const std::vector<std::string>& users_addresses,
    const std::vector<std::string>& balance_addresses,
    const std::vector<std::string>& payments_addresses,
    uint16_t port
) : port_(port), is_running_(false) {
    app_ = std::make_unique<crow::SimpleApp>();
    
    users_lb_ = std::make_unique<LoadBalancer>(users_addresses);
    balance_lb_ = std::make_unique<LoadBalancer>(balance_addresses);
    payments_lb_ = std::make_unique<LoadBalancer>(payments_addresses);

    auto& env = config::Environment::getInstance();
    auto& redis = security::RedisClient::getInstance();
    
    if (!redis.init(env.getRedisHost(), env.getRedisPort())) {
        spdlog::error("Failed to initialize Redis client");
    } else {
        spdlog::info("Redis client initialized successfully");
    }

    auto& ddos_protection = security::DDoSProtection::getInstance();
    ddos_protection.init();

    auto& blacklist_manager = security::BlacklistManager::getInstance();
    try {
        blacklist_manager.loadPredefinedBlacklist("/etc/grpc_gateway/blacklist.txt");
    } catch (const std::exception& e) {
        spdlog::warn("Failed to load predefined blacklist: {}", e.what());
    }
    
    SetupMiddleware();
    SetupRoutes();
}

void GatewayServer::SetupMiddleware() {
    security::DDoSProtection::getInstance().setupMiddleware(*app_);
    
    app_->after_handle([](const crow::response& res) {
        spdlog::info("Response status: {}", res.code);
        return res;
    });

    app_->on_error([](int code, const std::string& message) {
        spdlog::error("Error {}: {}", code, message);
        return crow::response(code, message);
    });
}

void GatewayServer::SetupRoutes() {
    security::BlacklistManager::getInstance().setupAPI(*app_);

    CROW_ROUTE(app_->handle, "/api/v1/auth/register")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto& redis = security::RedisClient::getInstance();
                std::string ip = req.remote_ip;

                int registration_count = redis.getRequestCount(ip, "/api/v1/auth/register", 3600);
                if (registration_count >= 5) {
                    spdlog::warn("IP {} blocked due to too many registration attempts", ip);
                    return crow::response(403, nlohmann::json{
                        {"success", false},
                        {"error_message", "Too many registration attempts. IP blocked for 1 hour."}
                    }.dump());
                }

                auto json = nlohmann::json::parse(req.body);

                users::RegisterRequest request;
                request.set_username(json["username"].get<std::string>());
                request.set_email(json["email"].get<std::string>());
                request.set_password(json["password"].get<std::string>());

                services::UsersService users_service(users_lb_);
                auto response = users_service.Register(request);

                if (response.success()) {
                    redis.incrementRequestCount(ip, "/api/v1/auth/register", 3600);
                }

                nlohmann::json result = {
                    {"success", response.success()},
                    {"user_id", response.user_id()},
                    {"access_token", response.access_token()},
                    {"refresh_token", response.refresh_token()},
                    {"error_message", response.error_message()}
                };
                
                return crow::response(response.success() ? 200 : 400, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process register request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app_->handle, "/api/v1/auth/login")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto& redis = security::RedisClient::getInstance();
                std::string ip = req.remote_ip;

                int failed_attempts = redis.getFailedLoginAttempts(ip);
                if (failed_attempts >= 15) {
                    spdlog::warn("IP {} blocked due to too many failed login attempts", ip);
                    return crow::response(403, nlohmann::json{
                        {"success", false},
                        {"error_message", "Too many failed login attempts. IP blocked for 1 hour."}
                    }.dump());
                }

                auto json = nlohmann::json::parse(req.body);
                
                users::LoginRequest request;
                request.set_username_or_email(json["username_or_email"].get<std::string>());
                request.set_password(json["password"].get<std::string>());
                
                services::UsersService users_service(users_lb_);
                auto response = users_service.Login(request);
                
                if (!response.success()) {
                    redis.incrementFailedLoginAttempts(ip);
                    
                    nlohmann::json result = {
                        {"success", false},
                        {"error_message", response.error_message()}
                    };
                    
                    return crow::response(401, result.dump());
                }

                redis.resetFailedLoginAttempts(ip);
                
                nlohmann::json result = {
                    {"success", true},
                    {"user_id", response.user_id()},
                    {"access_token", response.access_token()},
                    {"refresh_token", response.refresh_token()}
                };
                
                return crow::response(200, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process login request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app_->handle, "/api/v1/auth/refresh")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto json = nlohmann::json::parse(req.body);
                
                users::RefreshTokenRequest request;
                request.set_refresh_token(json["refresh_token"].get<std::string>());
                
                services::UsersService users_service(users_lb_);
                auto response = users_service.RefreshToken(request);
                
                nlohmann::json result = {
                    {"success", response.success()},
                    {"access_token", response.access_token()},
                    {"refresh_token", response.refresh_token()},
                    {"error_message", response.error_message()}
                };
                
                return crow::response(response.success() ? 200 : 401, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process refresh token request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app_->handle, "/api/v1/auth/reset-password")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto json = nlohmann::json::parse(req.body);
                
                users::ResetPasswordRequest request;
                request.set_email(json["email"].get<std::string>());
                
                services::UsersService users_service(users_lb_);
                auto response = users_service.ResetPassword(request);
                
                nlohmann::json result = {
                    {"success", response.success()},
                    {"error_message", response.error_message()}
                };
                
                return crow::response(response.success() ? 200 : 400, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process reset password request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app_->handle, "/api/v1/balance/decrease")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto json = nlohmann::json::parse(req.body);
                
                balance::DecreaseBalanceRequest request;
                request.set_user_id(json["user_id"].get<std::string>());
                request.set_amount(json["amount"].get<double>());
                request.set_transaction_id(json["transaction_id"].get<std::string>());
                
                if (json.contains("description")) {
                    request.set_description(json["description"].get<std::string>());
                }
                
                services::BalanceService balance_service(balance_lb_);
                auto response = balance_service.DecreaseBalance(request);
                
                nlohmann::json result = {
                    {"success", response.success()},
                    {"balance", response.balance()},
                    {"transaction_id", response.transaction_id()},
                    {"error_message", response.error_message()}
                };
                
                return crow::response(response.success() ? 200 : 400, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process decrease balance request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app_->handle, "/api/v1/balance/increase")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto json = nlohmann::json::parse(req.body);
                
                balance::IncreaseBalanceRequest request;
                request.set_user_id(json["user_id"].get<std::string>());
                request.set_amount(json["amount"].get<double>());
                request.set_transaction_id(json["transaction_id"].get<std::string>());
                
                if (json.contains("description")) {
                    request.set_description(json["description"].get<std::string>());
                }
                
                services::BalanceService balance_service(balance_lb_);
                auto response = balance_service.IncreaseBalance(request);
                
                nlohmann::json result = {
                    {"success", response.success()},
                    {"balance", response.balance()},
                    {"transaction_id", response.transaction_id()},
                    {"error_message", response.error_message()}
                };
                
                return crow::response(response.success() ? 200 : 400, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process increase balance request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app_->handle, "/api/v1/payments/create")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            try {
                auto json = nlohmann::json::parse(req.body);
                
                payments::CreatePaymentGatewayRequest request;
                request.set_user_id(json["user_id"].get<std::string>());
                request.set_amount(json["amount"].get<double>());
                request.set_currency(json["currency"].get<std::string>());
                
                if (json.contains("description")) {
                    request.set_description(json["description"].get<std::string>());
                }
                
                if (json.contains("return_url")) {
                    request.set_return_url(json["return_url"].get<std::string>());
                }
                
                if (json.contains("cancel_url")) {
                    request.set_cancel_url(json["cancel_url"].get<std::string>());
                }
                
                services::PaymentsService payments_service(payments_lb_);
                auto response = payments_service.CreatePaymentGateway(request);
                
                nlohmann::json result = {
                    {"success", response.success()},
                    {"payment_id", response.payment_id()},
                    {"payment_url", response.payment_url()},
                    {"error_message", response.error_message()}
                };
                
                return crow::response(response.success() ? 200 : 400, result.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to process create payment request: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error_message", e.what()}}.dump());
            }
        });
}

void GatewayServer::Start() {
    if (is_running_) {
        spdlog::warn("Server is already running");
        return;
    }

    spdlog::info("Starting gateway server on port {}", port_);
    is_running_ = true;
    
    app_->port(port_).multithreaded().run();
}

void GatewayServer::Stop() {
    if (!is_running_) {
        spdlog::warn("Server is not running");
        return;
    }

    spdlog::info("Stopping gateway server");
    is_running_ = false;
    app_->stop();
}

}