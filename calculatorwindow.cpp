#include "calculatorwindow.h"
#include "ui_calculatorwindow.h"

#include <QKeyEvent>
#include <QPushButton>

namespace {

// 输入位数上限，避免显示区塞进过长数字
constexpr int kMaxDigits = 16;

// 把运算符转成界面上显示的符号
QString operatorSymbol(const QString &op)
{
    if (op == QLatin1String("*")) {
        return QStringLiteral("×");
    }
    if (op == QLatin1String("/")) {
        return QStringLiteral("÷");
    }
    if (op == QLatin1String("-")) {
        return QStringLiteral("−");
    }
    return op;
}

// double -> 显示文本：整数不带小数点，小数最多保留 10 位并去掉多余的 0
QString formatNumber(double value)
{
    if (qIsNull(value - qRound64(value)) && qAbs(value) < 1e15) {
        return QString::number(static_cast<qint64>(qRound64(value)));
    }
    QString text = QString::number(value, 'f', 10);
    while (text.endsWith(QLatin1Char('0'))) {
        text.chop(1);
    }
    if (text.endsWith(QLatin1Char('.'))) {
        text.chop(1);
    }
    return text;
}

} // namespace

CalculatorWindow::CalculatorWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalculatorWindow)
    , m_display(QStringLiteral("0"))
    , m_isNewInput(true)
    , m_errorState(false)
{
    ui->setupUi(this);

    // 窗口本身获得焦点，按键事件才会送进 keyPressEvent（界面上的按钮都是 NoFocus）
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    // 所有按钮连到同一个槽，用 calcKey 属性区分是哪个键
    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        connect(button, &QPushButton::clicked, this, &CalculatorWindow::onButtonClicked);
    }

    refreshDisplay();
}

CalculatorWindow::~CalculatorWindow()
{
    delete ui;
}

void CalculatorWindow::onButtonClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) {
        return;
    }
    handleKey(button->property("calcKey").toString());
}

void CalculatorWindow::keyPressEvent(QKeyEvent *event)
{
    const QString key = keyToCalcKey(event);
    if (key.isEmpty()) {
        QWidget::keyPressEvent(event); // 不认识的键交回父类，避免吞掉系统快捷键
        return;
    }
    handleKey(key);
    event->accept();
}

/**
 * 把键盘按键翻译成与按钮 calcKey 完全相同的字符串。
 * 鼠标走 handleKey(button->calcKey)，键盘走 keyPressEvent -> keyToCalcKey -> handleKey，
 * 两条路径在这里汇合，因此不存在"鼠标一套逻辑、键盘另一套逻辑"的问题。
 */
QString CalculatorWindow::keyToCalcKey(QKeyEvent *event)
{
    const int key = event->key();
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString(QChar(QLatin1Char('0').unicode() + (key - Qt::Key_0)));
    }

    switch (key) {
    case Qt::Key_Period:
    case Qt::Key_Comma:
        return QStringLiteral("dot");
    case Qt::Key_Plus:
        return QStringLiteral("+");
    case Qt::Key_Minus:
        return QStringLiteral("-");
    case Qt::Key_Asterisk:
    case Qt::Key_X: // 键盘上常用 x 代表乘号
        return QStringLiteral("*");
    case Qt::Key_Slash:
        return QStringLiteral("/");
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:
        return QStringLiteral("=");
    case Qt::Key_Backspace:
        return QStringLiteral("back");
    case Qt::Key_Escape:
    case Qt::Key_Delete:
    case Qt::Key_C:
        return QStringLiteral("clear");
    default:
        return QString();
    }
}

void CalculatorWindow::handleKey(const QString &key)
{
    if (key == QLatin1String("clear")) {
        clearAll();
    } else if (key == QLatin1String("back")) {
        backspace();
    } else if (key == QLatin1String("dot")) {
        inputDot();
    } else if (key == QLatin1String("=")) {
        evaluate();
    } else if (key == QLatin1String("+") || key == QLatin1String("-")
               || key == QLatin1String("*") || key == QLatin1String("/")) {
        inputOperator(key);
    } else if (key.size() == 1 && key.at(0).isDigit()) {
        inputDigit(key);
    }
    refreshDisplay();
}

void CalculatorWindow::inputDigit(const QString &digit)
{
    if (m_errorState) {
        clearAll(); // 出错后按数字，直接开始一次新的输入
    }
    if (m_isNewInput) {
        m_display = digit;
        m_isNewInput = false;
        return;
    }
    if (m_display == QLatin1String("0")) {
        m_display = digit; // 避免出现 05 这样的前导 0
        return;
    }
    if (m_display.size() < kMaxDigits) {
        m_display.append(digit);
    }
}

void CalculatorWindow::inputDot()
{
    if (m_errorState) {
        clearAll();
    }
    if (m_isNewInput) {
        m_display = QStringLiteral("0.");
        m_isNewInput = false;
        return;
    }
    // 同一个操作数只允许有一个小数点：已经存在 '.' 就忽略这次输入。
    // 这是"连续小数点"问题的处理点，输入 1..2 时第二次 '.' 被丢弃，结果为 1.2
    if (m_display.contains(QLatin1Char('.'))) {
        return;
    }
    m_display.append(QLatin1Char('.'));
}

void CalculatorWindow::inputOperator(const QString &op)
{
    if (m_errorState) {
        return; // 错误态只接受"清除"或数字（数字会隐式清零）
    }

    // 连按两个运算符：只替换待执行的运算符，
    // 不能拿显示区里残留的操作数再算一次（否则 5 + × 3 会变成 10 × 3）
    if (m_isNewInput && !m_pendingOp.isEmpty()) {
        m_pendingOp = op;
        updateExpression(m_operand + QLatin1Char(' ') + operatorSymbol(op));
        return;
    }

    if (m_pendingOp.isEmpty()) {
        m_operand = m_display;
    } else {
        // 已有运算符：先把前面的算式算出来，实现连续运算
        double result = 0;
        if (!compute(m_operand.toDouble(), currentValue(), m_pendingOp, &result)) {
            return;
        }
        m_display = formatNumber(result);
        m_operand = m_display;
    }

    m_pendingOp = op;
    m_isNewInput = true;
    updateExpression(m_operand + QLatin1Char(' ') + operatorSymbol(op));
}

void CalculatorWindow::evaluate()
{
    if (m_errorState) {
        return;
    }
    if (m_pendingOp.isEmpty()) {
        return; // 没有待运算的表达式，按 = 不做任何事
    }

    const QString lhsText = m_operand;
    const QString rhsText = m_display;
    const QString op = m_pendingOp;

    double result = 0;
    if (!compute(m_operand.toDouble(), currentValue(), op, &result)) {
        updateExpression(lhsText + QLatin1Char(' ') + operatorSymbol(op)
                         + QLatin1Char(' ') + rhsText + QStringLiteral(" ="));
        return;
    }

    updateExpression(lhsText + QLatin1Char(' ') + operatorSymbol(op)
                     + QLatin1Char(' ') + rhsText + QStringLiteral(" ="));
    m_display = formatNumber(result);
    m_operand = m_display;   // 结果可以继续参与下一次运算
    m_pendingOp.clear();
    m_isNewInput = true;     // 计算结果后直接按数字，会开始一次新的输入
}

void CalculatorWindow::backspace()
{
    if (m_errorState) {
        clearAll();
        return;
    }
    if (m_isNewInput) {
        return; // 刚按完运算符，显示区还不是用户正在输入的数
    }

    m_display.chop(1);
    if (m_display.isEmpty() || m_display == QLatin1String("-")) {
        m_display = QStringLiteral("0");
        m_isNewInput = true;
    }
}

void CalculatorWindow::clearAll()
{
    m_display = QStringLiteral("0");
    m_operand.clear();
    m_pendingOp.clear();
    m_isNewInput = true;
    m_errorState = false;
    updateExpression(QString());
}

bool CalculatorWindow::compute(double lhs, double rhs, const QString &op, double *result)
{
    if (op == QLatin1String("/") && qFuzzyIsNull(rhs)) {
        enterErrorState();
        return false;
    }
    *result = applyOperator(op, lhs, rhs);
    return true;
}

void CalculatorWindow::enterErrorState()
{
    // 除数为零：显示提示并锁住后续运算，直到用户按 C 或直接输入新数字
    m_errorState = true;
    m_display = QStringLiteral("除数不能为零");
    m_operand.clear();
    m_pendingOp.clear();
    m_isNewInput = true;
}

double CalculatorWindow::currentValue() const
{
    return m_display.toDouble();
}

double CalculatorWindow::applyOperator(const QString &op, double lhs, double rhs)
{
    if (op == QLatin1String("+")) {
        return lhs + rhs;
    }
    if (op == QLatin1String("-")) {
        return lhs - rhs;
    }
    if (op == QLatin1String("*")) {
        return lhs * rhs;
    }
    return lhs / rhs;
}

void CalculatorWindow::refreshDisplay()
{
    ui->lblDisplay->setText(m_display);
}

void CalculatorWindow::updateExpression(const QString &text)
{
    ui->lblExpression->setText(text);
}
