#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <crow.h>

#include "security/redis_client.hpp"

namespace security {

struct RateLimitConfig {
    int requestLimit;
    int windowSeconds;
    int banTimeSeconds;
};

class DDoSProtection {
public:
    static DDoSProtection& getInstance();

    void init();

    void setupMiddleware(crow::SimpleApp& app);

    bool checkRequestLimit(const std::string& ip, const std::string& endpoint);

    void setRateLimit(const std::string& endpoint, int requestLimit, int windowSeconds, int banTimeSeconds);

    RateLimitConfig getRateLimitConfig(const std::string& endpoint) const;
    
private:
    DDoSProtection();
    ~DDoSProtection();
    
    DDoSProtection(const DDoSProtection&) = delete;
    DDoSProtection& operator=(const DDoSProtection&) = delete;

    crow::response handleRequest(const crow::request& req);

    RateLimitConfig findConfigForPath(const std::string& path) const;

    std::string getClientIP(const crow::request& req) const;
    
    std::unordered_map<std::string, RateLimitConfig> path_configs_;
    RateLimitConfig default_config_;
    mutable std::mutex mutex_;
};

}