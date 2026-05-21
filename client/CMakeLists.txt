set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(Client
    main.cpp
    auth_dialog.cpp auth_dialog.h
    main_window.cpp main_window.h
    contests_tab.cpp contests_tab.h
    active_contest_tab.cpp active_contest_tab.h
    results_tab.cpp results_tab.h
    admin_tab.cpp admin_tab.h
    math_highlighter.cpp math_highlighter.h
    users_tab.cpp users_tab.h
    profile_dialog.cpp profile_dialog.h
    friends_tab.cpp friends_tab.h
    archive_tab.cpp archive_tab.h
    ratings_tab.cpp ratings_tab.h
    
    # Слой Model & Repository (MVC/MVP)
    mvc/LocalDb.cpp mvc/LocalDb.h
    mvc/ApiClient.cpp mvc/ApiClient.h
    mvc/ContestModel.cpp mvc/ContestModel.h
    mvc/ContestsPresenter.cpp mvc/ContestsPresenter.h
    mvc/RatingModel.cpp mvc/RatingModel.h
    mvc/RatingsPresenter.cpp mvc/RatingsPresenter.h
    mvc/ArchiveModel.cpp mvc/ArchiveModel.h
    mvc/ArchivePresenter.cpp mvc/ArchivePresenter.h
    mvc/UsersListModel.cpp mvc/UsersListModel.h
    mvc/FriendsPresenter.cpp mvc/FriendsPresenter.h
    mvc/AuthPresenter.cpp mvc/AuthPresenter.h
    mvc/UsersTableModel.cpp mvc/UsersTableModel.h
    mvc/UsersPresenter.cpp mvc/UsersPresenter.h
    mvc/ResultsModel.cpp mvc/ResultsModel.h
    mvc/ResultsPresenter.cpp mvc/ResultsPresenter.h
    mvc/AdminPresenter.cpp mvc/AdminPresenter.h
    mvc/ActiveContestPresenter.cpp mvc/ActiveContestPresenter.h
    mvc/ProfilePresenter.cpp mvc/ProfilePresenter.h
)

target_link_libraries(Client PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Widgets 
    Qt6::Sql
    Qt6::Concurrent
    Qt6::Pdf
    Qt6::PdfWidgets
)

# Юнит тесты к слою Model (как требуется: Qt Test)
add_executable(ClientTests tests/test_model.cpp mvc/ContestModel.cpp)
target_link_libraries(ClientTests PRIVATE Qt6::Core Qt6::Test)
