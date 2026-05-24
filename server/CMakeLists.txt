cmake_minimum_required(VERSION 3.21)

include(FetchContent)

# We define the minimum set of features we need to speed up the compilation
set(USERVER_FEATURE_MONGODB OFF CACHE BOOL "Disable MongoDB" FORCE)
set(USERVER_FEATURE_REDIS OFF CACHE BOOL "Disable Redis" FORCE)
set(USERVER_FEATURE_CLICKHOUSE OFF CACHE BOOL "Disable ClickHouse" FORCE)
set(USERVER_FEATURE_RABBITMQ OFF CACHE BOOL "Disable RabbitMQ" FORCE)
set(USERVER_FEATURE_GRPC OFF CACHE BOOL "Disable gRPC" FORCE)
set(USERVER_FEATURE_MYSQL OFF CACHE BOOL "Disable MySQL" FORCE)
set(USERVER_FEATURE_UTEST OFF CACHE BOOL "Disable UTest" FORCE)

FetchContent_Declare(
  userver
  GIT_REPOSITORY https://github.com/userver-framework/userver.git
  GIT_TAG develop
)
FetchContent_MakeAvailable(userver)

find_package(userver REQUIRED COMPONENTS core postgres clients-http)
find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

add_executable(Server
    main.cpp
    handlers.cpp
    auth.hpp
    smtp_client.cpp
    smtp_client.h
)

target_link_libraries(Server PRIVATE 
    userver::core
    userver::postgres
    userver::clients-http
    Qt6::Core 
    Qt6::Network 
)

