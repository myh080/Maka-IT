cmake_minimum_required(VERSION 3.16)
project(Maka_IT LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

add_executable(Maka_IT
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    materielspage.cpp
    materielspage.h
    addmaterieldialog.cpp
    addmaterieldialog.h
    empruntspage.cpp
    empruntspage.h
    retourspage.cpp
    retourspage.h
    datastore.cpp
    datastore.h
    theme.cpp
    theme.h
    zoombackground.cpp
    zoombackground.h
    toggleswitch.cpp
    toggleswitch.h
    dashboardpage.h dashboardpage.cpp
    studentspage.h studentspage.cpp
)

target_link_libraries(Maka_IT PRIVATE Qt6::Widgets Qt6::Sql)