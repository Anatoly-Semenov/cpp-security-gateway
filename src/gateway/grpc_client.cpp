#include "gateway/grpc_client.hpp"
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

namespace gateway {

GrpcClient::GrpcClient(const std::string& address) {
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
    args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    
    channel_ = grpc::CreateCustomChannel(
        address,
        grpc::InsecureChannelCredentials(),
        args
    );
}

std::shared_ptr<grpc::Channel> GrpcClient::GetChannel() const {
    return channel_;
}

template<typename Request, typename Response>
grpc::Status GrpcClient::ExecuteWithRetry(
    std::function<grpc::Status(grpc::ClientContext*, const Request&, Response*)> rpc_func,
    const Request& request,
    Response* response,
    int max_retries,
    int timeout_ms
) {
    int retry_count = 0;
    grpc::Status status;

    while (retry_count < max_retries) {
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + 
                           std::chrono::milliseconds(timeout_ms));

        status = rpc_func(&context, request, response);

        if (status.ok()) {
            return status;
        }

        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED ||
            status.error_code() == grpc::StatusCode::UNAVAILABLE) {
            
            retry_count++;
            if (retry_count < max_retries) {
                spdlog::warn("Retry attempt {} for request", retry_count);
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * retry_count));
                continue;
            }
        }
        break;
    }

    return status;
}

}