cmake_minimum_required(VERSION 3.20)
project(Mathforces VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск Qt6 (Core, Network, Sql, Widgets, HttpServer, Test)
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Widgets HttpServer Concurrent Pdf PdfWidgets Test)

add_subdirectory(server)
add_subdirectory(client)
