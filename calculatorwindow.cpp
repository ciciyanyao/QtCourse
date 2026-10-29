#include "calculatorwindow.h"
#include "ui_calculatorwindow.h"

#include <QPushButton>

CalculatorWindow::CalculatorWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalculatorWindow)
    , m_display(QStringLiteral("0"))
{
    ui->setupUi(this);

    // 所有按钮统一连到同一个槽：按键靠 calcKey 属性区分，避免为 20 个按钮写 20 个槽函数
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
        m_display = QStringLiteral("0");
    } else if (key.size() == 1 && key.at(0).isDigit()) {
        // 首字符为 0 时用新输入的数字替换，避免出现 05 这样的写法
        if (m_display == QLatin1String("0")) {
            m_display = key;
        } else {
            m_display.append(key);
        }
    }
    refreshDisplay();
}

void CalculatorWindow::refreshDisplay()
{
    ui->lblDisplay->setText(m_display);
}
