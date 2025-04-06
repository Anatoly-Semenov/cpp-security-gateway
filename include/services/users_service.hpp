#pragma once

#include <memory>
#include <string>
#include "gateway/grpc_client.hpp"
#include "gateway/load_balancer.hpp"
#include "gateway/retry_policy.hpp"
#include "users.grpc.pb.h"

namespace gateway {
namespace services {

class UsersService {
public:
    explicit UsersService(std::shared_ptr<LoadBalancer> load_balancer);

    users::RegisterResponse Register(const users::RegisterRequest& request);

    users::LoginResponse Login(const users::LoginRequest& request);

    users::RefreshTokenResponse RefreshToken(const users::RefreshTokenRequest& request);

    users::ResetPasswordResponse ResetPassword(const users::ResetPasswordRequest& request);

private:
    std::shared_ptr<LoadBalancer> load_balancer_;
    RetryPolicy retry_policy_;
};

}
}