cmake_minimum_required(VERSION 3.14)
project(grpc_gateway)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим необходимые пакеты
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenTelemetry REQUIRED)
find_package(redis++ REQUIRED)
find_package(hiredis REQUIRED)

# Генерация кода из Proto файлов
set(PROTO_FILES
    proto/users.proto
    proto/balance.proto
    proto/payments.proto
)

# Находим protoc и плагин для gRPC
set(PROTOC_PATH ${Protobuf_PROTOC_EXECUTABLE})
set(GRPC_CPP_PLUGIN_PATH $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Устанавливаем пути для сгенерированных файлов
set(PROTO_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})

# Генерируем код для каждого proto файла
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    
    # Генерация .pb.h и .pb.cc файлов
    add_custom_command(
        OUTPUT "${PROTO_SRC_DIR}/${PROTO_NAME}.pb.h" "${PROTO_SRC_DIR}/${PROTO_NAME}.pb.cc"
        COMMAND ${PROTOC_PATH}
        ARGS --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
             --cpp_out=${PROTO_SRC_DIR}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
    )
    
    # Генерация .grpc.pb.h и .grpc.pb.cc файлов
    add_custom_command(
        OUTPUT "${PROTO_SRC_DIR}/${PROTO_NAME}.grpc.pb.h" "${PROTO_SRC_DIR}/${PROTO_NAME}.grpc.pb.cc"
        COMMAND ${PROTOC_PATH}
        ARGS --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
             --grpc_out=${PROTO_SRC_DIR}
             --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_PATH}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE} "${PROTO_SRC_DIR}/${PROTO_NAME}.pb.h" "${PROTO_SRC_DIR}/${PROTO_NAME}.pb.cc"
    )
    
    list(APPEND PROTO_GENERATED_FILES 
        "${PROTO_SRC_DIR}/${PROTO_NAME}.pb.h" 
        "${PROTO_SRC_DIR}/${PROTO_NAME}.pb.cc"
        "${PROTO_SRC_DIR}/${PROTO_NAME}.grpc.pb.h" 
        "${PROTO_SRC_DIR}/${PROTO_NAME}.grpc.pb.cc"
    )
endforeach()

# Добавляем исходные файлы
set(SOURCES
    src/main.cpp
    src/gateway/gateway_server.cpp
    src/gateway/grpc_client.cpp
    src/gateway/load_balancer.cpp
    src/gateway/retry_policy.cpp
    src/services/users_service.cpp
    src/services/balance_service.cpp
    src/services/payments_service.cpp
    src/security/redis_client.cpp
    src/security/ddos_protection.cpp
    src/security/blacklist_manager.cpp
    ${PROTO_GENERATED_FILES}
)

# Добавляем заголовочные файлы
set(HEADERS
    include/gateway/gateway_server.hpp
    include/gateway/grpc_client.hpp
    include/gateway/load_balancer.hpp
    include/gateway/retry_policy.hpp
    include/services/users_service.hpp
    include/services/balance_service.hpp
    include/services/payments_service.hpp
    include/security/redis_client.hpp
    include/security/ddos_protection.hpp
    include/security/blacklist_manager.hpp
)

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Устанавливаем пути для включения заголовочных файлов
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${PROTO_SRC_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

# Линкуем библиотеки
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Protobuf_LIBRARIES}
    ${gRPC_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    Boost::thread
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    OpenTelemetry::opentelemetry
    redis++::redis++
    hiredis::hiredis
) 