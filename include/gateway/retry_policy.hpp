#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <grpcpp/grpcpp.h>

namespace gateway {

class RetryPolicy {
public:
    RetryPolicy(int max_retries = 3, 
               int base_delay_ms = 100,
               int max_delay_ms = 5000);

    template<typename Func, typename... Args>
    auto Execute(Func&& func, Args&&... args);

    bool ShouldRetry(const grpc::Status& status);

    std::chrono::milliseconds GetBackoffDelay(int retry_count);

private:
    int max_retries_;
    int base_delay_ms_;
    int max_delay_ms_;
};

template<typename Func, typename... Args>
auto RetryPolicy::Execute(Func&& func, Args&&... args) {
    using ResultType = std::invoke_result_t<Func, Args...>;
    
    int retry_count = 0;
    ResultType result;
    
    while (true) {
        result = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        
        if (ShouldRetry(result) && retry_count < max_retries_) {
            retry_count++;
            auto delay = GetBackoffDelay(retry_count);
            std::this_thread::sleep_for(delay);
            continue;
        }
        
        break;
    }
    
    return result;
}

}