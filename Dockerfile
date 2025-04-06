FROM ubuntu:20.04 AS builder

# Установка необходимых пакетов
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    libssl-dev \
    libboost-all-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    curl \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Создаем рабочую директорию
WORKDIR /app

# Копируем исходный код
COPY . /app/

# Скачиваем и устанавливаем OpenTelemetry
RUN git clone https://github.com/open-telemetry/opentelemetry-cpp.git /tmp/opentelemetry && \
    cd /tmp/opentelemetry && \
    mkdir build && cd build && \
    cmake -DBUILD_TESTING=OFF .. && \
    make && make install

# Скачиваем и устанавливаем Crow
RUN git clone https://github.com/CrowCpp/Crow.git /tmp/crow && \
    cd /tmp/crow && \
    mkdir build && cd build && \
    cmake .. && \
    make && make install

# Скачиваем и устанавливаем hiredis
RUN git clone https://github.com/redis/hiredis.git /tmp/hiredis && \
    cd /tmp/hiredis && \
    make && make install && \
    ldconfig

# Скачиваем и устанавливаем redis-plus-plus
RUN git clone https://github.com/sewenew/redis-plus-plus.git /tmp/redis-plus-plus && \
    cd /tmp/redis-plus-plus && \
    mkdir build && cd build && \
    cmake -DREDIS_PLUS_PLUS_CXX_STANDARD=17 .. && \
    make && make install

# Настраиваем и собираем проект
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Создаем финальный образ
FROM ubuntu:20.04

# Установка runtime зависимостей
RUN apt-get update && apt-get install -y \
    libprotobuf-dev \
    libgrpc++-dev \
    libssl-dev \
    libboost-system-dev \
    libboost-thread-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Копируем собранное приложение из образа-билдера
COPY --from=builder /app/build/grpc_gateway /usr/local/bin/grpc_gateway
COPY --from=builder /usr/local/lib/libhiredis* /usr/local/lib/
COPY --from=builder /usr/local/lib/libredis++* /usr/local/lib/

# Обновляем кеш библиотек
RUN ldconfig

# Открываем порт для HTTP
EXPOSE 8080

# Запускаем приложение
CMD ["grpc_gateway"] 