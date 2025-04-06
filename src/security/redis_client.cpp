#include "security/redis_client.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <ctime>

namespace security {

RedisClient::RedisClient() : connected_(false) {}

RedisClient::~RedisClient() {}

RedisClient& RedisClient::getInstance() {
    static RedisClient instance;
    return instance;
}

bool RedisClient::init(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connected_) {
        return true;
    }
    
    try {
        std::string connection_string = "tcp://" + host + ":" + std::to_string(port);
        sw::redis::ConnectionOptions options;
        options.host = host;
        options.port = port;
        options.socket_timeout = std::chrono::milliseconds(100);
        
        redis_ = std::make_unique<sw::redis::Redis>(options);

        auto pong = redis_->ping();
        if (pong && *pong == "PONG") {
            connected_ = true;
            spdlog::info("Connected to Redis at {}:{}", host, port);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to Redis: {}", e.what());
    }
    
    connected_ = false;
    return false;
}

bool RedisClient::isConnected() const {
    return connected_;
}

bool RedisClient::addToBlacklist(const std::string& ip, int banTimeSeconds) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string key = getBlacklistKey(ip);
        redis_->set(key, "1", std::chrono::seconds(banTimeSeconds));
        spdlog::info("IP {} added to blacklist for {} seconds", ip, banTimeSeconds);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error adding IP to blacklist: {}", e.what());
        return false;
    }
}

bool RedisClient::isBlacklisted(const std::string& ip) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string key = getBlacklistKey(ip);
        auto value = redis_->get(key);
        return value.has_value();
    } catch (const std::exception& e) {
        spdlog::error("Error checking blacklist: {}", e.what());
        return false;
    }
}

bool RedisClient::removeFromBlacklist(const std::string& ip) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string key = getBlacklistKey(ip);
        long long removed = redis_->del(key);
        return removed > 0;
    } catch (const std::exception& e) {
        spdlog::error("Error removing IP from blacklist: {}", e.what());
        return false;
    }
}

bool RedisClient::incrementRequestCount(const std::string& ip, const std::string& endpoint, int windowSeconds) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string key = getRequestCountKey(ip, endpoint, windowSeconds);

        redis_->incr(key);
        redis_->expire(key, std::chrono::seconds(windowSeconds));
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error incrementing request count: {}", e.what());
        return false;
    }
}

int RedisClient::getRequestCount(const std::string& ip, const std::string& endpoint, int windowSeconds) {
    if (!connected_) {
        return 0;
    }
    
    try {
        std::string key = getRequestCountKey(ip, endpoint, windowSeconds);
        auto value = redis_->get(key);
        
        if (value) {
            return std::stoi(*value);
        }
        
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("Error getting request count: {}", e.what());
        return 0;
    }
}

void RedisClient::resetRequestCount(const std::string& ip, const std::string& endpoint) {
    if (!connected_) {
        return;
    }
    
    try {
        std::string pattern = "rate_limit:" + ip + ":" + endpoint + ":*";
        auto keys = redis_->keys(pattern);
        
        for (const auto& key : keys) {
            redis_->del(key);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error resetting request count: {}", e.what());
    }
}

std::string RedisClient::getBlacklistKey(const std::string& ip) {
    return "blacklist:ip:" + ip;
}

std::string RedisClient::getRequestCountKey(const std::string& ip, const std::string& endpoint, int windowSeconds) {
    return "rate_limit:" + ip + ":" + endpoint + ":" + std::to_string(windowSeconds);
}

bool RedisClient::incrementFailedLoginAttempts(const std::string& ip) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string key = getFailedLoginAttemptsKey(ip);
        redis_->incr(key);
        redis_->expire(key, std::chrono::seconds(3600));
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Error incrementing failed login attempts: {}", e.what());
        return false;
    }
}

int RedisClient::getFailedLoginAttempts(const std::string& ip) {
    if (!connected_) {
        return 0;
    }
    
    try {
        std::string key = getFailedLoginAttemptsKey(ip);
        auto value = redis_->get(key);
        
        if (value) {
            return std::stoi(*value);
        }
        
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("Error getting failed login attempts: {}", e.what());
        return 0;
    }
}

void RedisClient::resetFailedLoginAttempts(const std::string& ip) {
    if (!connected_) {
        return;
    }
    
    try {
        std::string key = getFailedLoginAttemptsKey(ip);
        redis_->del(key);
    } catch (const std::exception& e) {
        spdlog::error("Error resetting failed login attempts: {}", e.what());
    }
}

std::string RedisClient::getFailedLoginAttemptsKey(const std::string& ip) {
    return "failed_login:" + ip;
}

}