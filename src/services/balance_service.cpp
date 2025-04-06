#include "services/balance_service.hpp"
#include <spdlog/spdlog.h>

namespace gateway {
namespace services {

BalanceService::BalanceService(std::shared_ptr<LoadBalancer> load_balancer)
    : load_balancer_(load_balancer),
      retry_policy_(3, 100, 5000) {
}

balance::BalanceResponse BalanceService::DecreaseBalance(const balance::DecreaseBalanceRequest& request) {
    balance::BalanceResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available balance service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = balance::BalanceService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const balance::DecreaseBalanceRequest& req,
                             balance::BalanceResponse* resp) {
        return stub->DecreaseBalance(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to decrease balance: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

balance::BalanceResponse BalanceService::IncreaseBalance(const balance::IncreaseBalanceRequest& request) {
    balance::BalanceResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available balance service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = balance::BalanceService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const balance::IncreaseBalanceRequest& req,
                             balance::BalanceResponse* resp) {
        return stub->IncreaseBalance(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to increase balance: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

balance::BalanceResponse BalanceService::GetBalance(const balance::GetBalanceRequest& request) {
    balance::BalanceResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available balance service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = balance::BalanceService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const balance::GetBalanceRequest& req,
                             balance::BalanceResponse* resp) {
        return stub->GetBalance(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to get balance: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

}
}