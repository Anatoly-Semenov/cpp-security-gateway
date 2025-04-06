#include "gateway/load_balancer.hpp"
#include <spdlog/spdlog.h>

namespace gateway {

LoadBalancer::LoadBalancer(const std::vector<std::string>& addresses)
    : current_index_(0) {
    for (const auto& address : addresses) {
        clients_.push_back(std::make_shared<GrpcClient>(address));
    }
}

std::shared_ptr<GrpcClient> LoadBalancer::GetNextClient() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (clients_.empty()) {
        spdlog::error("No available clients in load balancer");
        return nullptr;
    }

    auto client = clients_[current_index_];
    current_index_ = (current_index_ + 1) % clients_.size();
    return client;
}

void LoadBalancer::UpdateAddresses(const std::vector<std::string>& new_addresses) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<GrpcClient>> new_clients;
    for (const auto& address : new_addresses) {
        new_clients.push_back(std::make_shared<GrpcClient>(address));
    }
    
    clients_ = std::move(new_clients);
    current_index_ = 0;
    
    spdlog::info("Updated load balancer with {} new addresses", new_addresses.size());
}

}