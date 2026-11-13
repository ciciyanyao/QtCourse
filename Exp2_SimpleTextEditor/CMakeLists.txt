cmake_minimum_required(VERSION 3.16)

project(Exp2_SimpleTextEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 同时兼容实验指导书要求的 Qt 5.12.11 与本机 Qt 6.10.3
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    codeeditor.h
    codeeditor.cpp
    finddialog.h
    finddialog.cpp
    finddialog.ui
    replacedialog.h
    replacedialog.cpp
    replacedialog.ui
    aboutdialog.h
    aboutdialog.cpp
    aboutdialog.ui
    resources.qrc
)

add_executable(Exp2_SimpleTextEditor WIN32 ${PROJECT_SOURCES})
# uic 生成的 ui_mainwindow.h 会 #include "codeeditor.h"（promote 的自定义控件）
target_include_directories(Exp2_SimpleTextEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Exp2_SimpleTextEditor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
set_target_properties(Exp2_SimpleTextEditor PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "Exp2_SimpleTextEditor"
)
