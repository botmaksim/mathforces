set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Server
    main.cpp
    database.cpp
    database.h
    llm_client.cpp
    llm_client.h
    contest_handler.cpp
    contest_handler.h
    auth_middleware.h
    smtp_client.cpp
    smtp_client.h
)

target_link_libraries(Server PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Sql 
    Qt6::HttpServer 
    Qt6::Concurrent
)
