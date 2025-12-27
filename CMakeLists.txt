cmake_minimum_required(VERSION 3.16)
project(adb_sequence VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebSockets)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann/json.hpp")
    message(FATAL_ERROR "Brak pliku nagłówkowego nlohmann/json.hpp.")
endif()
include_directories(nlohmann)

set(SHARED_SOURCES
    argsparser.cpp
    commandexecutor.cpp
    sequencerunner.cpp
    adb_client.cpp
)

set(SHARED_HEADERS
    argsparser.h
    commandexecutor.h
    sequencerunner.h
    adb_client.h
)

add_library(adb_shared_components STATIC
    ${SHARED_SOURCES}
    ${SHARED_HEADERS}
)

target_compile_features(adb_shared_components PRIVATE cxx_std_17)
set_target_properties(adb_shared_components PROPERTIES 
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(adb_shared_components PRIVATE
    Qt6::Core
    Qt6::WebSockets
)

qt_add_executable(adb_sequence_d
    main_d.cpp
    remoteserver.cpp
    remoteserver.h
)

target_compile_features(adb_sequence_d PRIVATE cxx_std_17)

target_link_libraries(adb_sequence_d PUBLIC 
    adb_shared_components
    Qt6::Core
    Qt6::Gui
    Qt6::WebSockets
)

set_target_properties(adb_sequence_d PROPERTIES 
    OUTPUT_NAME "adb_sequence_d"
    POSITION_INDEPENDENT_CODE ON 
)

qt_add_executable(adb_sequence
    main.cpp
    mainwindow.cpp
    mainwindow.h
    settingsdialog.cpp
    settingsdialog.h
    SwipeBuilderWidget.cpp
    SwipeBuilderWidget.h
    SwipeCanvas.cpp
    SwipeCanvas.h
    SwipeModel.cpp
    SwipeModel.h
    KeyboardWidget.cpp
    KeyboardWidget.h
    RecordAction.cpp
    RecordAction.h
    ActionEditDialog.cpp
    ActionEditDialog.h
)

target_compile_features(adb_sequence PRIVATE cxx_std_17)

target_link_libraries(adb_sequence PUBLIC 
    adb_shared_components
    Qt6::Core 
    Qt6::Widgets
    Qt6::Gui
    Qt6::WebSockets
)

set_target_properties(adb_sequence PROPERTIES 
    OUTPUT_NAME "adb_sequence"
    POSITION_INDEPENDENT_CODE ON
)
