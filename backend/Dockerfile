# 1) Build stage
FROM debian:bookworm-slim AS builder

# install build-time deps
RUN apt-get update && \
    apt-get install -y \
    build-essential cmake git pkg-config \
    libpq-dev libpqxx-dev \
    nlohmann-json3-dev libcrow-dev \
    libboost-unit-test-framework-dev \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libssl-dev zlib1g-dev \
    libpg-query-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# 2) Runtime stage
FROM debian:bookworm-slim

# only runtime libs
RUN apt-get update && \
    apt-get install -y \
    libpq5 libpqxx-6.4 \
    libssl1.1 zlib1g \
    libpg-query1706.0 \
    && rm -rf /var/lib/apt/lists/*

# copy the compiled server
COPY --from=builder /app/build/socoles_server /usr/local/bin/socoles_server

ENTRYPOINT ["socoles_server"]
