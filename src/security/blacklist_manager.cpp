#include "security/blacklist_manager.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <nlohmann/json.hpp>

namespace security {

BlacklistManager::BlacklistManager() {}

BlacklistManager::~BlacklistManager() {}

BlacklistManager& BlacklistManager::getInstance() {
    static BlacklistManager instance;
    return instance;
}

void BlacklistManager::setupAPI(crow::SimpleApp& app) {
    CROW_ROUTE(app.handle, "/api/v1/admin/blacklist")
        .methods("GET"_method)
        ([this](const crow::request& req) {
            if (!isAuthorized(req)) {
                return crow::response(403, R"({"success":false,"error":"Unauthorized"})");
            }
            
            auto blacklistedIPs = getBlacklistedIPs();
            
            nlohmann::json response = {
                {"success", true},
                {"blacklisted_ips", blacklistedIPs}
            };
            
            return crow::response(200, response.dump());
        });

    CROW_ROUTE(app.handle, "/api/v1/admin/blacklist/add")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            if (!isAuthorized(req)) {
                return crow::response(403, R"({"success":false,"error":"Unauthorized"})");
            }
            
            try {
                auto json = nlohmann::json::parse(req.body);
                std::string ip = json["ip"].get<std::string>();
                int banTime = 3600;
                
                if (json.contains("ban_time_seconds")) {
                    banTime = json["ban_time_seconds"].get<int>();
                }
                
                bool success = addToBlacklist(ip, banTime);
                
                nlohmann::json response = {
                    {"success", success},
                    {"ip", ip},
                    {"ban_time_seconds", banTime}
                };
                
                return crow::response(success ? 200 : 400, response.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to add IP to blacklist: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error", e.what()}}.dump());
            }
        });

    CROW_ROUTE(app.handle, "/api/v1/admin/blacklist/remove")
        .methods("POST"_method)
        ([this](const crow::request& req) {
            if (!isAuthorized(req)) {
                return crow::response(403, R"({"success":false,"error":"Unauthorized"})");
            }
            
            try {
                auto json = nlohmann::json::parse(req.body);
                std::string ip = json["ip"].get<std::string>();
                
                bool success = removeFromBlacklist(ip);
                
                nlohmann::json response = {
                    {"success", success},
                    {"ip", ip}
                };
                
                return crow::response(success ? 200 : 400, response.dump());
            } catch (const std::exception& e) {
                spdlog::error("Failed to remove IP from blacklist: {}", e.what());
                return crow::response(400, nlohmann::json{{"success", false}, {"error", e.what()}}.dump());
            }
        });
    
    spdlog::info("Blacklist management API initialized");
}

bool BlacklistManager::addToBlacklist(const std::string& ip, int banTimeSeconds) {
    auto& redis = RedisClient::getInstance();
    return redis.addToBlacklist(ip, banTimeSeconds);
}

bool BlacklistManager::isBlacklisted(const std::string& ip) {
    auto& redis = RedisClient::getInstance();
    return redis.isBlacklisted(ip);
}

bool BlacklistManager::removeFromBlacklist(const std::string& ip) {
    auto& redis = RedisClient::getInstance();
    return redis.removeFromBlacklist(ip);
}

void BlacklistManager::loadPredefinedBlacklist(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        spdlog::error("Failed to open blacklist file: {}", filename);
        return;
    }
    
    predefined_blacklist_.clear();
    std::string line;
    
    while (std::getline(file, line)) {

        if (line.empty() || line[0] == '#') {
            continue;
        }

        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (!line.empty()) {
            predefined_blacklist_.push_back(line);
            addToBlacklist(line);
        }
    }
    
    spdlog::info("Loaded {} IPs from predefined blacklist", predefined_blacklist_.size());
}

std::vector<std::string> BlacklistManager::getBlacklistedIPs() {


    std::lock_guard<std::mutex> lock(mutex_);
    return predefined_blacklist_;
}

bool BlacklistManager::isAuthorized(const crow::request& req) const {

    if (req.headers.count("X-API-Key") > 0) {
        std::string apiKey = req.headers.find("X-API-Key")->second;

        return apiKey == "your-secret-api-key";
    }
    
    return false;
}

}