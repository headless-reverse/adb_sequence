cmake_minimum_required(VERSION 3.16)
project(adb_sequence VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann/json.hpp")
    message(FATAL_ERROR "Brak pliku nagłówkowego nlohmann/json.hpp.")
endif()
include_directories(nlohmann)

add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    commandexecutor.cpp
    settingsdialog.cpp
    sequencerunner.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
