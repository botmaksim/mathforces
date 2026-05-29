set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
)

add_executable(Server ${SERVER_SOURCES})

target_include_directories(Server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(Server PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::HttpServer
    Qt6::Concurrent
)
