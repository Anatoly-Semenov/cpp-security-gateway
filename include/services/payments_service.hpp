#pragma once

#include <memory>
#include <string>
#include "gateway/grpc_client.hpp"
#include "gateway/load_balancer.hpp"
#include "gateway/retry_policy.hpp"
#include "payments.grpc.pb.h"

namespace gateway {
namespace services {

class PaymentsService {
public:
    explicit PaymentsService(std::shared_ptr<LoadBalancer> load_balancer);

    payments::CreatePaymentGatewayResponse CreatePaymentGateway(
        const payments::CreatePaymentGatewayRequest& request);

    payments::CheckPaymentStatusResponse CheckPaymentStatus(
        const payments::CheckPaymentStatusRequest& request);

    payments::ProcessPaymentResponse ProcessPayment(
        const payments::ProcessPaymentRequest& request);

private:
    std::shared_ptr<LoadBalancer> load_balancer_;
    RetryPolicy retry_policy_;
};

}
}