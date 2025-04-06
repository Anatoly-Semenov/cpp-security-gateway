#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <crow.h>

#include "security/redis_client.hpp"

namespace security {

class BlacklistManager {
public:
    static BlacklistManager& getInstance();

    void setupAPI(crow::SimpleApp& app);

    bool addToBlacklist(const std::string& ip, int banTimeSeconds = 3600);

    bool isBlacklisted(const std::string& ip);

    bool removeFromBlacklist(const std::string& ip);

    void loadPredefinedBlacklist(const std::string& filename);

    std::vector<std::string> getBlacklistedIPs();
    
private:
    BlacklistManager();
    ~BlacklistManager();
    
    BlacklistManager(const BlacklistManager&) = delete;
    BlacklistManager& operator=(const BlacklistManager&) = delete;

    bool isAuthorized(const crow::request& req) const;
    
    mutable std::mutex mutex_;
    std::vector<std::string> predefined_blacklist_;
};

}