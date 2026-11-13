# Qt 5.12.11 (qmake) 兼容工程文件；Qt 6.10.3 请改用 CMakeLists.txt
QT       += widgets
CONFIG   += c++11
TARGET    = Exp2_SimpleTextEditor
TEMPLATE  = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp \
    finddialog.cpp \
    replacedialog.cpp \
    aboutdialog.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h \
    finddialog.h \
    replacedialog.h \
    aboutdialog.h

FORMS += \
    mainwindow.ui \
    finddialog.ui \
    replacedialog.ui \
    aboutdialog.ui

RESOURCES += resources.qrc

# 自定义提升控件需要能被 uic 找到
INCLUDEPATH += $$PWD
