cmake_minimum_required(VERSION 3.16)
project(adb_shell LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)

set(SOURCES
    main.cpp
    mainwindow.cpp
    commandexecutor.cpp
    settingsdialog.cpp
)

set(HEADERS
    mainwindow.h
    commandexecutor.h
    settingsdialog.h
)

add_executable(adb_shell ${SOURCES} ${HEADERS})
target_include_directories(adb_shell PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(adb_shell PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
