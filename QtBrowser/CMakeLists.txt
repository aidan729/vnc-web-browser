cmake_minimum_required(VERSION 3.16)
project(QtBrowser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8.2 COMPONENTS Widgets WebEngineWidgets OpenGL OpenGLWidgets REQUIRED)

add_executable(QtBrowser
    main.cpp
    mainwindow.h
    mainwindow.cpp
    resources.qrc
    vncserver.h
    vncserver.cpp
)

qt6_add_resources(QtBrowser "resources"
    PREFIX "/"
    FILES
    icons/close-black.png
    icons/delete-red.png
    icons/duplicated.png
    icons/left-arrow.png
    icons/refresh-page-option.png
    icons/right-arrow.png
)

target_link_libraries(QtBrowser PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)
