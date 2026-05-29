set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    config/*.h
    ui/*.cpp ui/*.h
    network/*.cpp network/*.h
    windows/*.cpp windows/*.h
    dialogs/*.cpp dialogs/*.h
    tabs/*.cpp tabs/*.h
)

add_executable(Client ${CLIENT_SOURCES})

target_include_directories(Client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/windows
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/tabs
)

target_link_libraries(Client PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Pdf
    Qt6::PdfWidgets
)
