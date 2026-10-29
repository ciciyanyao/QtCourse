#ifndef CALCULATORWINDOW_H
#define CALCULATORWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class CalculatorWindow;
}
QT_END_NAMESPACE

/**
 * 计算器主窗口。
 * 第 1 阶段：先用 Qt Designer 把界面布局搭好，按键能把数字拼到显示区。
 */
class CalculatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CalculatorWindow(QWidget *parent = nullptr);
    ~CalculatorWindow() override;

private slots:
    void onButtonClicked();

private:
    void handleKey(const QString &key);
    void refreshDisplay();

    Ui::CalculatorWindow *ui;
    QString m_display;   // 显示区当前文本
};

#endif // CALCULATORWINDOW_H
