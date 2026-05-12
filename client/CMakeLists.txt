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
    users_tab.cpp
    users_tab.h
    profile_dialog.cpp
    profile_dialog.h
    friends_tab.cpp
    friends_tab.h
    archive_tab.cpp
    archive_tab.h
    ratings_tab.cpp
    ratings_tab.h
)

target_link_libraries(Client PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Widgets 
    Qt6::Concurrent
    Qt6::Pdf
    Qt6::PdfWidgets
)
