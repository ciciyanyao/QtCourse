#ifndef CALCULATORWINDOW_H
#define CALCULATORWINDOW_H

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class CalculatorWindow;
}
QT_END_NAMESPACE

/**
 * 计算器主窗口。
 *
 * 设计要点：
 *  1. 鼠标与键盘不各写一套逻辑：所有入口最终都转成同一个按键字符串 key，
 *     交给 handleKey() 处理（clear / back / +-/ * / digit / dot / = ...）。
 *  2. 交互状态只用三个成员描述：
 *       m_operand    —— 已锁定的第一个操作数
 *       m_pendingOp  —— 待执行的运算符（空表示没有待运算）
 *       m_isNewInput —— 下一次输入数字是否要覆盖显示区（按完运算符或等号后为 true）
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

    void inputDigit(const QString &digit);
    void inputDot();
    void inputOperator(const QString &op);
    void evaluate();
    void backspace();
    void clearAll();

    double currentValue() const;
    static double applyOperator(const QString &op, double lhs, double rhs);

    void refreshDisplay();
    void updateExpression(const QString &text);

    Ui::CalculatorWindow *ui;

    QString m_display;      // 显示区当前文本
    QString m_operand;      // 第一个操作数（字符串形式，便于原样显示）
    QString m_pendingOp;    // 待执行运算符
    bool m_isNewInput;      // 下一次数字输入是否重新开始
};

#endif // CALCULATORWINDOW_H
