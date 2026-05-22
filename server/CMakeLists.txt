cmake_minimum_required(VERSION 3.20...3.30)

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql HttpServer)

set(CMAKE_AUTOMOC ON)

add_executable(Server
    main.cpp
    routes.cpp
    routes.hpp
    auth.hpp
    smtp_client.cpp
    smtp_client.h
)

target_link_libraries(Server PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Sql 
    Qt6::HttpServer
)
