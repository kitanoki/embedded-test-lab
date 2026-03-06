cmake_minimum_required(VERSION 3.16)

project(Rk3576CodecConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(Rk3576CodecConsole
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

target_link_libraries(Rk3576CodecConsole PRIVATE Qt6::Widgets)
