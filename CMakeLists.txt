cmake_minimum_required(VERSION 3.16)

project(Exp1_KeyboardCalculator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)      # 自动处理 .ui 文件，生成 ui_calculatorwindow.h
set(CMAKE_AUTOMOC ON)      # 自动处理 Q_OBJECT，生成 moc 文件
set(CMAKE_AUTORCC ON)      # 自动编译 .qrc 资源（样式表）

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(Exp1_KeyboardCalculator
    main.cpp
    calculatorwindow.cpp
    calculatorwindow.h
    calculatorwindow.ui
    resources.qrc
)

target_link_libraries(Exp1_KeyboardCalculator PRIVATE Qt6::Widgets)

# Windows 下不弹出控制台窗口
set_target_properties(Exp1_KeyboardCalculator PROPERTIES
    WIN32_EXECUTABLE TRUE
)
