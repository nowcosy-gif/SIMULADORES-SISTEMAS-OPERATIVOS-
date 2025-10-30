cmake_minimum_required(VERSION 3.16)
project(SJF_Simulator)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(SJF_Simulator
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui


)

target_link_libraries(SJF_Simulator PRIVATE Qt6::Widgets)
