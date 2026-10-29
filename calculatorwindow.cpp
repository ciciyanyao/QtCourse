#include "calculatorwindow.h"
#include "ui_calculatorwindow.h"

#include <QPushButton>

namespace {

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
{
    ui->setupUi(this);

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
    if (m_isNewInput) {
        m_display = digit;
        m_isNewInput = false;
    } else if (m_display == QLatin1String("0")) {
        m_display = digit; // 避免出现 05 这样的前导 0
    } else {
        m_display.append(digit);
    }
}

void CalculatorWindow::inputDot()
{
    if (m_isNewInput) {
        m_display = QStringLiteral("0.");
        m_isNewInput = false;
    } else {
        m_display.append(QLatin1Char('.'));
    }
}

void CalculatorWindow::inputOperator(const QString &op)
{
    if (m_pendingOp.isEmpty()) {
        m_operand = m_display;
    } else {
        // 已经有运算符：先把前面的算式算出来，实现连续运算
        const double result = applyOperator(m_pendingOp, m_operand.toDouble(), currentValue());
        m_display = formatNumber(result);
        m_operand = m_display;
    }

    m_pendingOp = op;
    m_isNewInput = true;
    updateExpression(m_operand + QLatin1Char(' ') + operatorSymbol(op));
}

void CalculatorWindow::evaluate()
{
    if (m_pendingOp.isEmpty()) {
        return;
    }

    const QString lhsText = m_operand;
    const QString rhsText = m_display;
    const double result = applyOperator(m_pendingOp, m_operand.toDouble(), currentValue());

    updateExpression(lhsText + QLatin1Char(' ') + operatorSymbol(m_pendingOp)
                     + QLatin1Char(' ') + rhsText + QStringLiteral(" ="));
    m_display = formatNumber(result);
    m_operand = m_display;
    m_pendingOp.clear();
    m_isNewInput = true;
}

void CalculatorWindow::backspace()
{
    if (m_isNewInput) {
        return; // 刚按完运算符，显示区还不是用户输入的数
    }
    m_display.chop(1);
    if (m_display.isEmpty()) {
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
    updateExpression(QString());
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
