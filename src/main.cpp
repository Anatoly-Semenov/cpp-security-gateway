#include "gateway/gateway_server.hpp"
#include "config/environment.hpp"
#include <spdlog/spdlog.h>
#include <signal.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>

std::unique_ptr<gateway::GatewayServer> g_server;

void SignalHandler(int signal) {
    spdlog::info("Received signal {}, shutting down...", signal);
    if (g_server) {
        g_server->Stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    spdlog::set_level(spdlog::level::debug);

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try {
        auto& env = config::Environment::getInstance();
        
        if (!env.validate()) {
            spdlog::error("Environment validation failed: {}", env.getErrorMessage());
            return 1;
        }

        spdlog::info("Starting grpc_gateway on port {}", env.getPort());
        spdlog::info("Users service addresses: {}", fmt::join(env.getUsersServiceAddresses(), ", "));
        spdlog::info("Balance service addresses: {}", fmt::join(env.getBalanceServiceAddresses(), ", "));
        spdlog::info("Payments service addresses: {}", fmt::join(env.getPaymentsServiceAddresses(), ", "));
        spdlog::info("Redis configuration: {}:{}", env.getRedisHost(), env.getRedisPort());

        g_server = std::make_unique<gateway::GatewayServer>(
            env.getUsersServiceAddresses(),
            env.getBalanceServiceAddresses(),
            env.getPaymentsServiceAddresses(),
            env.getPort()
        );

        g_server->Start();

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
} 