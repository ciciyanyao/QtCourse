#include "calculatorwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 从 qrc 资源里读取样式表，方便后期统一调整界面风格
    QFile styleFile(QStringLiteral(":/style.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    CalculatorWindow window;
    window.show();

    return app.exec();
}
