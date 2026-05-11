set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Client
    main.cpp
    auth_dialog.cpp
    auth_dialog.h
    main_window.cpp
    main_window.h
    contests_tab.cpp
    contests_tab.h
    active_contest_tab.cpp
    active_contest_tab.h
    results_tab.cpp
    results_tab.h
    admin_tab.cpp
    admin_tab.h
    math_highlighter.cpp
    math_highlighter.h
)

target_link_libraries(Client PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Widgets 
    Qt6::Concurrent
)
