# cpp-security-gateway
C++ implementation of a gRPC gateway designed for scalable and secure microservice architectures on Kubernetes

## Security Features

The gateway includes the following security features:

### DDoS Attack Protection
- Request rate limiting for each IP address
- Customizable limits for different API endpoints
- Automatic blocking of IP addresses that exceed limits

### Redis Integration
- Distributed storage of blocked IP addresses data
- Real-time tracking of request counts

### IP Blacklist
- Predefined list of known malicious IP addresses
- REST API for blacklist management (adding/removing IPs)
- Automatic blocking of requests from blacklisted IP addresses

## Launch

```bash
# Run with Docker Compose
docker-compose up -d
```

## IP Blacklist Management

### Get list of blocked IPs
```
GET /api/v1/admin/blacklist
```

### Add IP to blacklist
```
POST /api/v1/admin/blacklist/add
{
  "ip": "192.168.1.1",
  "ban_time_seconds": 3600
}
```

### Remove IP from blacklist
```
POST /api/v1/admin/blacklist/remove
{
  "ip": "192.168.1.1"
}
```

> Note: To access the blacklist management API, you must include the `X-API-Key` header with the correct API key.

## Project Structure

```
.
├── CMakeLists.txt           # CMake build configuration
├── Dockerfile               # Docker image build instructions
├── README.md                # Project documentation
├── blacklist.txt            # Predefined list of blocked IP addresses
├── docker-compose.yml       # Docker Compose configuration
├── include/                 # Header files (*.h, *.hpp)
│   ├── gateway/             # Gateway module headers
│   ├── security/            # Security module headers
│   └── services/            # Service integration headers
├── proto/                   # gRPC service definitions (Protocol Buffers)
│   ├── balance.proto        # User balance service
│   ├── payments.proto       # Payments service
│   └── users.proto          # Users service
└── src/                     # C++ source code (*.cpp)
    ├── gateway/             # Gateway functionality implementation
    ├── main.cpp             # Application entry point
    ├── security/            # Security functionality implementation
    └── services/            # External services integration implementation
```
