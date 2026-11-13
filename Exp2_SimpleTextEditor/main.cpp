#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QStringList>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("简单文本编辑器"));
    QApplication::setOrganizationName(QStringLiteral("DGUT-QtCourse"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));

    // 从 qrc 载入全局 QSS（浅色主题 + 分组工具栏 + 圆角控件）
    QFile qss(QStringLiteral(":/style.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        qss.close();
    }
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    MainWindow w;
    w.show();

    // 支持命令行传入文件路径：方便验证「文件不存在 / 非 UTF-8 / 二进制」等边界场景
    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString path = args.at(1);
        QTimer::singleShot(0, &w, [&w, path]() { w.openFilePath(path); });
    }

    return app.exec();
}
