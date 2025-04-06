#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <functional>

namespace gateway {

class GrpcClient {
public:
    GrpcClient(const std::string& address);
    virtual ~GrpcClient() = default;

    std::shared_ptr<grpc::Channel> GetChannel() const;

    template<typename Request, typename Response>
    grpc::Status ExecuteWithRetry(
        std::function<grpc::Status(grpc::ClientContext*, const Request&, Response*)> rpc_func,
        const Request& request,
        Response* response,
        int max_retries = 3,
        int timeout_ms = 5000
    );

protected:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<grpc::ClientContext> context_;
};

}