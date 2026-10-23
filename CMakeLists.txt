cmake_minimum_required(VERSION 3.16)
project(Maka_IT LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Maka_IT
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    zoombackground.h
    zoombackground.cpp
    toggleswitch.h
    toggleswitch.cpp
    materielspage.h
    materielspage.cpp
    addmaterieldialog.h addmaterieldialog.cpp
    datastore.h datastore.cpp
    empruntspage.h empruntspage.cpp




)

target_link_libraries(Maka_IT PRIVATE Qt6::Widgets)