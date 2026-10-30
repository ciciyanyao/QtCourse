#include "qwmainwind.h"
#include <QApplication>
#include <QStyleFactory>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 强制使用 Fusion 浅色（白色）风格，不跟随系统深色模式
    a.setStyle(QStyleFactory::create("Fusion"));

    // 设置浅色调色板，确保界面是白色背景
    QPalette palette = a.palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::WindowText, Qt::black);
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Text, Qt::black);
    palette.setColor(QPalette::Button, Qt::white);
    palette.setColor(QPalette::ButtonText, Qt::black);
    palette.setColor(QPalette::Highlight, QColor(0x30, 0x80, 0xCC));
    a.setPalette(palette);

    QWMainWind w;
    w.show();

    return a.exec();
}
