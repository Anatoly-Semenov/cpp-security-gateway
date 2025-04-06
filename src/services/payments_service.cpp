#include "services/payments_service.hpp"
#include <spdlog/spdlog.h>

namespace gateway {
namespace services {

PaymentsService::PaymentsService(std::shared_ptr<LoadBalancer> load_balancer)
    : load_balancer_(load_balancer),
      retry_policy_(3, 100, 5000) {
}

payments::CreatePaymentGatewayResponse PaymentsService::CreatePaymentGateway(
    const payments::CreatePaymentGatewayRequest& request) {
    
    payments::CreatePaymentGatewayResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available payments service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = payments::PaymentsService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const payments::CreatePaymentGatewayRequest& req,
                             payments::CreatePaymentGatewayResponse* resp) {
        return stub->CreatePaymentGateway(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to create payment gateway: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

payments::CheckPaymentStatusResponse PaymentsService::CheckPaymentStatus(
    const payments::CheckPaymentStatusRequest& request) {
    
    payments::CheckPaymentStatusResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available payments service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = payments::PaymentsService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const payments::CheckPaymentStatusRequest& req,
                             payments::CheckPaymentStatusResponse* resp) {
        return stub->CheckPaymentStatus(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to check payment status: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

payments::ProcessPaymentResponse PaymentsService::ProcessPayment(
    const payments::ProcessPaymentRequest& request) {
    
    payments::ProcessPaymentResponse response;
    
    auto client = load_balancer_->GetNextClient();
    if (!client) {
        spdlog::error("No available payments service clients");
        response.set_success(false);
        response.set_error_message("Service unavailable");
        return response;
    }
    
    auto stub = payments::PaymentsService::NewStub(client->GetChannel());
    
    auto rpc_function = [&stub](grpc::ClientContext* context, 
                             const payments::ProcessPaymentRequest& req,
                             payments::ProcessPaymentResponse* resp) {
        return stub->ProcessPayment(context, req, resp);
    };
    
    grpc::Status status = client->ExecuteWithRetry(rpc_function, request, &response);
    
    if (!status.ok() && !response.success()) {
        spdlog::error("Failed to process payment: {}", status.error_message());
        response.set_success(false);
        if (response.error_message().empty()) {
            response.set_error_message(status.error_message());
        }
    }
    
    return response;
}

}
}