#ifndef CALCULATORWINDOW_H
#define CALCULATORWINDOW_H

#include <QHash>
#include <QString>
#include <QWidget>

class QKeyEvent;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class CalculatorWindow;
}
QT_END_NAMESPACE

/**
 * 计算器主窗口。
 *
 * 设计要点：
 *  1. 鼠标与键盘不各写一套逻辑：每个按钮都用动态属性 calcKey 标明"我是哪个键"，
 *     键盘事件先翻译成同样的 calcKey，再用 animateClick() 去点对应按钮，
 *     于是两条路径最终都汇到 clicked -> onButtonClicked -> handleKey 这一处。
 *     附带好处：键盘操作时按钮同样有按下的动画反馈。
 *  2. 交互状态用四个成员描述：
 *       m_operand    —— 已锁定的第一个操作数
 *       m_pendingOp  —— 待执行的运算符（空表示没有待运算）
 *       m_isNewInput —— 下一次输入数字是否要覆盖显示区（按完运算符或等号后为 true）
 *       m_errorState —— 是否处于错误态（如除数为零），错误态下只接受"清除"
 */
class CalculatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CalculatorWindow(QWidget *parent = nullptr);
    ~CalculatorWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onButtonClicked();

private:
    void handleKey(const QString &key);
    static QString keyToCalcKey(QKeyEvent *event);

    void inputDigit(const QString &digit);
    void inputDot();
    void inputOperator(const QString &op);
    void evaluate();
    void backspace();
    void negate();
    void percent();
    void applyUnary(const QString &op);
    void clearEntry();
    void clearAll();

    bool compute(double lhs, double rhs, const QString &op, double *result);
    void enterErrorState(const QString &message = QStringLiteral("除数不能为零"));
    double currentValue() const;
    static double applyOperator(const QString &op, double lhs, double rhs);

    void refreshDisplay();
    void updateExpression(const QString &text);

    Ui::CalculatorWindow *ui;
    QHash<QString, QPushButton *> m_keyButtons;  // calcKey -> 对应按钮，供键盘事件回查

    QString m_display;      // 显示区当前文本
    QString m_operand;      // 第一个操作数（字符串形式，便于原样显示）
    QString m_pendingOp;    // 待执行运算符
    bool m_isNewInput;      // 下一次数字输入是否重新开始
    bool m_errorState;      // 是否处于错误态
};

#endif // CALCULATORWINDOW_H
