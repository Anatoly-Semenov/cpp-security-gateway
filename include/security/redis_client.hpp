#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <sw/redis++/redis++.h>

namespace security {

class RedisClient {
public:
    static RedisClient& getInstance();

    bool init(const std::string& host, int port);
    bool isConnected() const;

    bool addToBlacklist(const std::string& ip, int banTimeSeconds = 3600);
    bool isBlacklisted(const std::string& ip);
    bool removeFromBlacklist(const std::string& ip);

    bool incrementRequestCount(const std::string& ip, const std::string& endpoint, int windowSeconds = 60);
    int getRequestCount(const std::string& ip, const std::string& endpoint, int windowSeconds = 60);
    void resetRequestCount(const std::string& ip, const std::string& endpoint);

private:
    RedisClient();
    ~RedisClient();

    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    std::unique_ptr<sw::redis::Redis> redis_;
    bool connected_;
    std::mutex mutex_;

    std::string getBlacklistKey(const std::string& ip);
    std::string getRequestCountKey(const std::string& ip, const std::string& endpoint, int windowSeconds);
};

}