#include "gateway/retry_policy.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <random>

namespace gateway {

RetryPolicy::RetryPolicy(int max_retries, int base_delay_ms, int max_delay_ms)
    : max_retries_(max_retries),
      base_delay_ms_(base_delay_ms),
      max_delay_ms_(max_delay_ms) {
}

bool RetryPolicy::ShouldRetry(const grpc::Status& status) {
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED ||
        status.error_code() == grpc::StatusCode::UNAVAILABLE ||
        status.error_code() == grpc::StatusCode::RESOURCE_EXHAUSTED ||
        status.error_code() == grpc::StatusCode::ABORTED) {
        
        spdlog::warn("Retryable gRPC error: [{}] {}", 
                    static_cast<int>(status.error_code()), 
                    status.error_message());
        return true;
    }

    if (!status.ok()) {
        spdlog::error("Non-retryable gRPC error: [{}] {}", 
                     static_cast<int>(status.error_code()), 
                     status.error_message());
    }
    
    return false;
}

std::chrono::milliseconds RetryPolicy::GetBackoffDelay(int retry_count) {

    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.8, 1.2);

    double delay = base_delay_ms_ * std::pow(2, retry_count - 1);

    delay *= dist(gen);

    delay = std::min(delay, static_cast<double>(max_delay_ms_));
    
    auto ms = std::chrono::milliseconds(static_cast<int>(delay));
    spdlog::debug("Retry #{} - backing off for {} ms", retry_count, ms.count());
    
    return ms;
}
}