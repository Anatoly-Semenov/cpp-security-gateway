#include "security/ddos_protection.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace security {

DDoSProtection::DDoSProtection() {
    default_config_ = {
        100,    // 100 запросов
        60,     // за 60 секунд
        1800    // бан на 30 минут (1800 секунд)
    };
}

DDoSProtection::~DDoSProtection() {}

DDoSProtection& DDoSProtection::getInstance() {
    static DDoSProtection instance;
    return instance;
}

void DDoSProtection::init() {
    std::lock_guard<std::mutex> lock(mutex_);

    setRateLimit("/api/v1/auth/login", 5, 60, 1800);           // 5 запросов в минуту
    setRateLimit("/api/v1/auth/register", 3, 60, 1800);        // 3 запроса в минуту
    setRateLimit("/api/v1/auth/refresh", 10, 60, 1800);        // 10 запросов в минуту
    setRateLimit("/api/v1/auth/reset-password", 3, 300, 1800); // 3 запроса в 5 минут

    setRateLimit("/api/v1/balance/decrease", 30, 60, 1800);    // 30 запросов в минуту
    setRateLimit("/api/v1/balance/increase", 30, 60, 1800);    // 30 запросов в минуту

    setRateLimit("/api/v1/payments/create", 20, 60, 1800);     // 20 запросов в минуту
    
    spdlog::info("DDoS protection initialized");
}

void DDoSProtection::setupMiddleware(crow::SimpleApp& app) {
    struct RateLimitMiddleware {
        RateLimitMiddleware() {}
        
        struct context {};
        
        template <typename AllContext>
        void before_handle(crow::request& req, crow::response& res, context& ctx, AllContext& all_ctx) {
            auto& protection = DDoSProtection::getInstance();
            auto response = protection.handleRequest(req);
            
            if (response.code != 200) {
                res = std::move(response);
                all_ctx.skip_all();
            }
        }
        
        void after_handle(crow::request& req, crow::response& res, context& ctx) {
        }
    };
    
    app.add_middleware<RateLimitMiddleware>();
    spdlog::info("DDoS protection middleware added");
}

bool DDoSProtection::checkRequestLimit(const std::string& ip, const std::string& endpoint) {
    auto& redis = RedisClient::getInstance();

    if (redis.isBlacklisted(ip)) {
        spdlog::warn("IP {} is blacklisted, request to {} rejected", ip, endpoint);
        return false;
    }

    auto config = findConfigForPath(endpoint);

    redis.incrementRequestCount(ip, endpoint, config.windowSeconds);

    int requestCount = redis.getRequestCount(ip, endpoint, config.windowSeconds);

    if (requestCount > config.requestLimit) {
        spdlog::warn("Rate limit exceeded for IP {} on {}: {} requests in {} seconds", 
                     ip, endpoint, requestCount, config.windowSeconds);
        
        redis.addToBlacklist(ip, config.banTimeSeconds);
        return false;
    }
    
    return true;
}

void DDoSProtection::setRateLimit(const std::string& endpoint, int requestLimit, int windowSeconds, int banTimeSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    path_configs_[endpoint] = {requestLimit, windowSeconds, banTimeSeconds};
}

RateLimitConfig DDoSProtection::getRateLimitConfig(const std::string& endpoint) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return findConfigForPath(endpoint);
}

crow::response DDoSProtection::handleRequest(const crow::request& req) {
    std::string ip = getClientIP(req);
    std::string path = req.url;

    if (!checkRequestLimit(ip, path)) {
        nlohmann::json error = {
            {"success", false},
            {"error", "Too many requests"},
            {"message", "Rate limit exceeded. Please try again later."}
        };
        
        crow::response res(429, error.dump());
        res.add_header("Content-Type", "application/json");
        res.add_header("Retry-After", "60");
        return res;
    }
    
    return crow::response(200);
}

RateLimitConfig DDoSProtection::findConfigForPath(const std::string& path) const {
    auto it = path_configs_.find(path);
    if (it != path_configs_.end()) {
        return it->second;
    }

    for (const auto& [prefix, config] : path_configs_) {
        if (path.rfind(prefix, 0) == 0) {
            return config;
        }
    }
    
    return default_config_;
}

std::string DDoSProtection::getClientIP(const crow::request& req) const {
    std::string ip;

    if (req.headers.count("X-Forwarded-For") > 0) {
        ip = req.headers.find("X-Forwarded-For")->second;
        auto pos = ip.find(',');
        if (pos != std::string::npos) {
            ip = ip.substr(0, pos);
        }
    }
    else if (req.headers.count("X-Real-IP") > 0) {
        ip = req.headers.find("X-Real-IP")->second;
    }
    else {
        ip = req.remote_ip;
    }
    
    return ip;
}

}