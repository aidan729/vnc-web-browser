cmake_minimum_required(VERSION 3.16)
project(VNC_Client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Core Network)

set(SOURCES
    main.cpp
    mainwindow.cpp
    vncclient.cpp
    vncviewerwidget.cpp
)

set(HEADERS
    mainwindow.h
    vncclient.h
    vncviewerwidget.h
)

add_executable(VNCClient
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(VNCClient PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Core
    Qt6::Network
    opengl32
)
