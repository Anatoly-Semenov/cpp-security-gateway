#pragma once

#include <memory>
#include <string>
#include <crow.h>
#include "gateway/load_balancer.hpp"
#include "services/users_service.hpp"
#include "services/balance_service.hpp"
#include "services/payments_service.hpp"

namespace gateway {

class GatewayServer {
public:
    GatewayServer(
        const std::vector<std::string>& users_addresses,
        const std::vector<std::string>& balance_addresses,
        const std::vector<std::string>& payments_addresses,
        uint16_t port = 8080
    );

    void Start();
    void Stop();

private:
    void SetupRoutes();
    void SetupMiddleware();

    std::unique_ptr<crow::SimpleApp> app_;
    std::unique_ptr<LoadBalancer> users_lb_;
    std::unique_ptr<LoadBalancer> balance_lb_;
    std::unique_ptr<LoadBalancer> payments_lb_;
    
    uint16_t port_;
    bool is_running_;
};

}