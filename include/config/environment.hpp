#pragma once

#include <string>
#include <vector>
#include <optional>

namespace config {

class Environment {
public:
    static Environment& getInstance();

    bool validate();
    std::string getErrorMessage() const;

    uint16_t getPort() const;
    std::vector<std::string> getUsersServiceAddresses() const;
    std::vector<std::string> getBalanceServiceAddresses() const;
    std::vector<std::string> getPaymentsServiceAddresses() const;
    std::string getRedisHost() const;
    int getRedisPort() const;

private:
    Environment() = default;
    ~Environment() = default;

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;

    bool validatePort();
    bool validateServiceAddresses();
    bool validateRedisConfig();

    std::string error_message_;
    uint16_t port_ = 8080;
    std::vector<std::string> users_service_addresses_;
    std::vector<std::string> balance_service_addresses_;
    std::vector<std::string> payments_service_addresses_;
    std::string redis_host_ = "localhost";
    int redis_port_ = 6379;
};

}