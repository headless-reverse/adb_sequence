cmake_minimum_required(VERSION 3.16)
project(adb_sequence VERSION 4.0 LANGUAGES CXX) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") # gdb ./adb_sequence -->run -->bt

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    WebSockets 
    OpenGLWidgets
    OpenGL
    Network
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann/json.hpp")
    message(FATAL_ERROR "Missing nlohmann/json.hpp header.")
endif()
include_directories(nlohmann)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)
pkg_check_modules(DRM libdrm) 

include_directories(${FFMPEG_INCLUDE_DIRS})
if(DRM_FOUND)
    include_directories(${DRM_INCLUDE_DIRS})
endif()

set(SHARED_SOURCES
    argsparser.cpp
    commandexecutor.cpp
    sequencerunner.cpp
    adb_client.cpp
    video_client.cpp
    video_worker.cpp
    h264decoder.cpp
    control_protocol.cpp
    control_socket.cpp
    swipecanvas.cpp
    SwipeModel.cpp
    hardware_grabbed.cpp
)

set(SHARED_HEADERS
    argsparser.h
    commandexecutor.h
    sequencerunner.h
    adb_client.h
    video_client.h
    video_worker.h
    h264decoder.h
    control_protocol.h
    control_socket.h
    swipecanvas.h
    SwipeModel.h
    hardware_grabbed.h
)

add_library(adb_shared_components STATIC ${SHARED_SOURCES} ${SHARED_HEADERS})
target_compile_features(adb_shared_components PRIVATE cxx_std_17)
set_target_properties(adb_shared_components PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(adb_shared_components
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::WebSockets
        Qt6::OpenGLWidgets
        Qt6::OpenGL
        ${FFMPEG_LIBRARIES}
)

if(DRM_FOUND)
    target_link_libraries(adb_shared_components PRIVATE ${DRM_LIBRARIES})
endif()

qt_add_executable(adb_sequence_d
    main_d.cpp
    remoteserver.cpp
    remoteserver.h
)

target_link_libraries(adb_sequence_d
    PUBLIC
        adb_shared_components
        Qt6::Core
        Qt6::Network
        Qt6::Gui
        Qt6::WebSockets
        Qt6::OpenGLWidgets
)

qt_add_executable(adb_sequence
    main.cpp
    mainwindow.cpp
    mainwindow.h
    settingsdialog.cpp
    settingsdialog.h
    SwipeBuilderWidget.cpp
    SwipeBuilderWidget.h
    KeyboardWidget.cpp
    KeyboardWidget.h
    RecordAction.cpp
    RecordAction.h
    ActionEditDialog.cpp
    ActionEditDialog.h
    view_hardware_grabbed.cpp
    view_hardware_grabbed.h
)

target_link_libraries(adb_sequence
    PUBLIC
        adb_shared_components
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Network
        Qt6::WebSockets
        Qt6::OpenGLWidgets
)

set_target_properties(adb_sequence_d PROPERTIES OUTPUT_NAME "adb_sequence_d")
set_target_properties(adb_sequence PROPERTIES OUTPUT_NAME "adb_sequence")
