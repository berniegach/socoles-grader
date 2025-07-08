# 1) Build stage
FROM debian:bookworm-slim AS builder

# install build-time deps
RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    libpq-dev libpqxx-dev \
    nlohmann-json3-dev \
    libboost-test-dev \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libssl-dev zlib1g-dev \
    libpg-query-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Install Crow (header-only) from source
RUN git clone --depth 1 --branch v1.0+5 https://github.com/CrowCpp/Crow.git /tmp/Crow \
    && mkdir /tmp/Crow/build \
    && cd /tmp/Crow/build \
    && cmake -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCROW_BUILD_EXAMPLES=OFF \
    -DCROW_BUILD_TESTS=OFF \
    .. \
    && make -j"$(nproc)" install \
    && rm -rf /tmp/Crow

# Build & install DuckDB so we get libduckdb.so in /usr/local/lib
RUN git clone --depth 1 https://github.com/duckdb/duckdb.git /tmp/duckdb \
    && mkdir /tmp/duckdb/build \
    && cd /tmp/duckdb/build \
    && cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    .. \
    && make -j"$(nproc)" install \
    && rm -rf /tmp/duckdb                            



WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# 3) Runtime stage
FROM debian:bookworm-slim

# Install only the required runtime shared libraries (no .-dev packages)
RUN apt-get update && apt-get install -y \
    libpq5 \
    libpqxx-6.4 \
    libboost-program-options1.74.0 \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libssl3 \
    zlib1g \
    libpg-query1504.0 \
    && rm -rf /var/lib/apt/lists/*

# copy the compiled server
COPY --from=builder /app/build/socoles_server /usr/local/bin/socoles_server

# copy DuckDB’s shared libs from builder to ensure they load at runtime
COPY --from=builder /usr/local/lib/libduckdb.so* /usr/local/lib/
RUN ldconfig                                         # update linker cache 


ENTRYPOINT ["socoles_server"]
