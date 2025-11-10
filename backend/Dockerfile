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

## Note: We rely on the vendored DuckDB library and headers in the repository to avoid header/ABI mismatches.



WORKDIR /app
COPY . .
# remove any previous build directory to avoid conflicts
RUN rm -rf build 
RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# 3) Runtime stage
FROM debian:bookworm-slim

# Install only the required runtime shared libraries (no .-dev packages)
RUN apt-get update && apt-get install -y \
    libpq5 \
    libboost-program-options1.74.0 \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libssl3 \
    zlib1g \
    && rm -rf /var/lib/apt/lists/*

# copy the compiled server
COPY --from=builder /app/build/socoles_server /usr/local/bin/socoles_server

# copy DuckDB’s shared lib from the build context (vendored)
COPY --from=builder /app/duckdb/libduckdb.so* /usr/local/lib/
# copy required shared libs from builder to avoid version-locked runtime packages
COPY --from=builder /usr/lib/x86_64-linux-gnu/libpg_query.so* /usr/local/lib/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libpqxx.so* /usr/local/lib/
RUN ldconfig                                         # update linker cache 


EXPOSE 5000
ENTRYPOINT ["socoles_server"]
