#include "finddialog.h"
#include "ui_finddialog.h"

#include <QShortcut>
#include <QKeySequence>

FindDialog::FindDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::FindDialog)
{
    ui->setupUi(this);
    // 非模态：保持主窗口可编辑，便于「连续查找」。
    // 不设置 WindowStaysOnTopHint，否则「未找到」等 QMessageBox 会被压在对话框下面。
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &FindDialog::emitFindNext);
    connect(ui->btnCancel, &QPushButton::clicked, this, &FindDialog::close);
    // 回车由 QDialog 的 default button（btnFindNext）统一处理，
    // 不再额外连接 returnPressed，避免一次回车触发两次查找。

    // F3：在对话框上继续查找下一个
    auto *shortcut = new QShortcut(QKeySequence(Qt::Key_F3), this);
    connect(shortcut, &QShortcut::activated, this, &FindDialog::emitFindNext);

    ui->edtFind->setFocus();
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::setSearchText(const QString &text)
{
    ui->edtFind->setText(text);
    ui->edtFind->selectAll();
    ui->edtFind->setFocus();
}

void FindDialog::emitFindNext()
{
    const QString needle = ui->edtFind->text();
    emit findNextRequested(needle,
                           ui->chkCase->isChecked(),
                           ui->rbUp->isChecked());
}
