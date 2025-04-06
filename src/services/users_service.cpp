#include "services/users_service.hpp"
#include <spdlog/spdlog.h>

namespace gateway {
namespace services {

UsersService::UsersService(std::shared_ptr<LoadBalancer> load_balancer)
    : load_balancer_(load_balancer),
      retry_policy_(3, 100, 5000) {
}

users::RegisterResponse UsersService::Register(const users::RegisterRequest& request) {
    users::RegisterResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available users service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = users::UsersService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const users::RegisterRequest& req,
                             users::RegisterResponse* resp) {
        return stub->Register(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to register user: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

users::LoginResponse UsersService::Login(const users::LoginRequest& request) {
    users::LoginResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available users service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = users::UsersService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const users::LoginRequest& req,
                             users::LoginResponse* resp) {
        return stub->Login(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to login user: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

users::RefreshTokenResponse UsersService::RefreshToken(const users::RefreshTokenRequest& request) {
    users::RefreshTokenResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available users service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = users::UsersService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const users::RefreshTokenRequest& req,
                             users::RefreshTokenResponse* resp) {
        return stub->RefreshToken(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to refresh token: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

users::ResetPasswordResponse UsersService::ResetPassword(const users::ResetPasswordRequest& request) {
    users::ResetPasswordResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available users service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = users::UsersService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const users::ResetPasswordRequest& req,
                             users::ResetPasswordResponse* resp) {
        return stub->ResetPassword(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to reset password: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

}
}