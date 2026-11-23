FROM ubuntu:26.04 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        cmake g++ libboost-json-dev libpq-dev libpqxx-dev ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt ./
COPY NetworkLogFilter/ ./NetworkLogFilter/
COPY receiver_service/ ./receiver_service/
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel 2

FROM ubuntu:26.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends libboost-json1.90.0 libpqxx-7.10 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/receiver_service/receiver ./receiver
COPY --from=build /src/build/NetworkLogFilter/NetworkLogFilter ./NetworkLogFilter
COPY --from=build /src/receiver_service/schema.sql ./receiver_service/schema.sql
