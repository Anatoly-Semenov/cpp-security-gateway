#pragma once

#include <memory>
#include <string>
#include "gateway/grpc_client.hpp"
#include "gateway/load_balancer.hpp"
#include "gateway/retry_policy.hpp"
#include "balance.grpc.pb.h"

namespace gateway {
namespace services {

class BalanceService {
public:
    explicit BalanceService(std::shared_ptr<LoadBalancer> load_balancer);

    balance::BalanceResponse DecreaseBalance(const balance::DecreaseBalanceRequest& request);

    balance::BalanceResponse IncreaseBalance(const balance::IncreaseBalanceRequest& request);

    balance::BalanceResponse GetBalance(const balance::GetBalanceRequest& request);

private:
    std::shared_ptr<LoadBalancer> load_balancer_;
    RetryPolicy retry_policy_;
};

}
}