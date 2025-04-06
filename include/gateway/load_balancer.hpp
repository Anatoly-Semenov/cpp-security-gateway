#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include "gateway/grpc_client.hpp"

namespace gateway {

class LoadBalancer {
public:
    LoadBalancer(const std::vector<std::string>& addresses);

    std::shared_ptr<GrpcClient> GetNextClient();

    void UpdateAddresses(const std::vector<std::string>& new_addresses);

private:
    std::vector<std::shared_ptr<GrpcClient>> clients_;
    size_t current_index_;
    std::mutex mutex_;
};

}