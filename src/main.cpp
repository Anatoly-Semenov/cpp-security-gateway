#include "gateway/gateway_server.hpp"
#include <spdlog/spdlog.h>
#include <signal.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>

std::unique_ptr<gateway::GatewayServer> g_server;

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> getAddressesFromEnv(const std::string& envName, 
                                             const std::vector<std::string>& defaultAddresses) {
    const char* envValue = std::getenv(envName.c_str());
    if (envValue) {
        return split(std::string(envValue), ',');
    }
    return defaultAddresses;
}

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

        uint16_t port = 8080;
        const char* portEnv = std::getenv("PORT");
        if (portEnv) {
            port = static_cast<uint16_t>(std::stoi(portEnv));
        }

        std::vector<std::string> default_users_addresses = {
            "users-service-1:50051",
            "users-service-2:50051"
        };

        std::vector<std::string> default_balance_addresses = {
            "balance-service-1:50051",
            "balance-service-2:50051"
        };

        std::vector<std::string> default_payments_addresses = {
            "payments-service-1:50051",
            "payments-service-2:50051"
        };

        auto users_addresses = getAddressesFromEnv("USERS_SERVICE_ADDRESSES", default_users_addresses);
        auto balance_addresses = getAddressesFromEnv("BALANCE_SERVICE_ADDRESSES", default_balance_addresses);
        auto payments_addresses = getAddressesFromEnv("PAYMENTS_SERVICE_ADDRESSES", default_payments_addresses);

        spdlog::info("Starting grpc_gateway on port {}", port);
        spdlog::info("Users service addresses: {}", fmt::join(users_addresses, ", "));
        spdlog::info("Balance service addresses: {}", fmt::join(balance_addresses, ", "));
        spdlog::info("Payments service addresses: {}", fmt::join(payments_addresses, ", "));

        g_server = std::make_unique<gateway::GatewayServer>(
            users_addresses,
            balance_addresses,
            payments_addresses,
            port
        );

        g_server->Start();

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
} 