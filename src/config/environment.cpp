#include "config/environment.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <sstream>
#include <algorithm>

namespace config {

Environment& Environment::getInstance() {
    static Environment instance;
    return instance;
}

bool Environment::validate() {
    if (!validatePort()) {
        return false;
    }

    if (!validateServiceAddresses()) {
        return false;
    }

    if (!validateRedisConfig()) {
        return false;
    }

    return true;
}

std::string Environment::getErrorMessage() const {
    return error_message_;
}

bool Environment::validatePort() {
    const char* port_env = std::getenv("PORT");
    if (port_env) {
        try {
            int port = std::stoi(port_env);
            if (port < 1 || port > 65535) {
                error_message_ = "PORT must be between 1 and 65535";
                return false;
            }
            port_ = static_cast<uint16_t>(port);
        } catch (const std::exception& e) {
            error_message_ = "Invalid PORT value: " + std::string(e.what());
            return false;
        }
    }
    return true;
}

bool Environment::validateServiceAddresses() {
    auto validateAddresses = [this](const char* env_name, std::vector<std::string>& addresses) {
        const char* env_value = std::getenv(env_name);
        if (!env_value) {
            return true;
        }

        std::string value(env_value);
        if (value.empty()) {
            error_message_ = std::string(env_name) + " cannot be empty";
            return false;
        }

        std::stringstream ss(value);
        std::string address;
        addresses.clear();

        while (std::getline(ss, address, ',')) {
            address.erase(std::remove_if(address.begin(), address.end(), ::isspace), address.end());
            
            if (address.empty()) {
                continue;
            }

            size_t colon_pos = address.find(':');
            if (colon_pos == std::string::npos) {
                error_message_ = "Invalid address format in " + std::string(env_name) + ": " + address;
                return false;
            }

            std::string host = address.substr(0, colon_pos);
            std::string port = address.substr(colon_pos + 1);

            if (host.empty() || port.empty()) {
                error_message_ = "Invalid address format in " + std::string(env_name) + ": " + address;
                return false;
            }

            try {
                int port_num = std::stoi(port);
                if (port_num < 1 || port_num > 65535) {
                    error_message_ = "Invalid port number in " + std::string(env_name) + ": " + address;
                    return false;
                }
            } catch (const std::exception& e) {
                error_message_ = "Invalid port number in " + std::string(env_name) + ": " + address;
                return false;
            }

            addresses.push_back(address);
        }

        if (addresses.empty()) {
            error_message_ = std::string(env_name) + " must contain at least one valid address";
            return false;
        }

        return true;
    };

    if (!validateAddresses("USERS_SERVICE_ADDRESSES", users_service_addresses_)) {
        return false;
    }

    if (!validateAddresses("BALANCE_SERVICE_ADDRESSES", balance_service_addresses_)) {
        return false;
    }

    if (!validateAddresses("PAYMENTS_SERVICE_ADDRESSES", payments_service_addresses_)) {
        return false;
    }

    return true;
}

bool Environment::validateRedisConfig() {
    const char* redis_host = std::getenv("REDIS_HOST");
    if (redis_host) {
        std::string host(redis_host);
        if (host.empty()) {
            error_message_ = "REDIS_HOST cannot be empty";
            return false;
        }
        redis_host_ = host;
    }

    const char* redis_port = std::getenv("REDIS_PORT");
    if (redis_port) {
        try {
            int port = std::stoi(redis_port);
            if (port < 1 || port > 65535) {
                error_message_ = "REDIS_PORT must be between 1 and 65535";
                return false;
            }
            redis_port_ = port;
        } catch (const std::exception& e) {
            error_message_ = "Invalid REDIS_PORT value: " + std::string(e.what());
            return false;
        }
    }

    return true;
}

uint16_t Environment::getPort() const {
    return port_;
}

std::vector<std::string> Environment::getUsersServiceAddresses() const {
    return users_service_addresses_;
}

std::vector<std::string> Environment::getBalanceServiceAddresses() const {
    return balance_service_addresses_;
}

std::vector<std::string> Environment::getPaymentsServiceAddresses() const {
    return payments_service_addresses_;
}

std::string Environment::getRedisHost() const {
    return redis_host_;
}

int Environment::getRedisPort() const {
    return redis_port_;
}

}