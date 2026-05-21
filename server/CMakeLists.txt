cmake_minimum_required(VERSION 3.20)

# Попытка найти установленный в системе userver
find_package(userver QUIET COMPONENTS core postgres clients-http)

if(NOT userver_FOUND)
    message(STATUS "=== userver не найден в системе. Автоматическое скачивание и сборка из исходников... ===")
    include(FetchContent)
    FetchContent_Declare(
        userver
        GIT_REPOSITORY https://github.com/userver-framework/userver.git
        GIT_TAG        v1.0.0
    )
    # Отключение тестов, примеров и патчинга libpq для существенного ускорения и стабильной сборки на современных дистрибутивах
    set(USERVER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(USERVER_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(USERVER_FEATURE_PATCH_LIBPQ OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(userver)
else()
    message(STATUS "=== Найдена системная копия userver! ===")
endif()

find_package(OpenSSL REQUIRED)

add_executable(Server
    main.cpp
    handlers.cpp
    handlers.hpp
    auth.hpp
)

target_link_libraries(Server PRIVATE 
    userver::core 
    userver::postgres 
    userver::clients-http 
    OpenSSL::Crypto
)
