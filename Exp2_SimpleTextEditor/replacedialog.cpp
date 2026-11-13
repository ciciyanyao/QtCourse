#include "replacedialog.h"
#include "ui_replacedialog.h"

#include <QShortcut>
#include <QKeySequence>

ReplaceDialog::ReplaceDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ReplaceDialog)
{
    ui->setupUi(this);
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);

    connect(ui->btnFindNext, &QPushButton::clicked, this, &ReplaceDialog::emitFindNext);
    connect(ui->btnReplace, &QPushButton::clicked, this, &ReplaceDialog::emitReplace);
    connect(ui->btnReplaceAll, &QPushButton::clicked, this, &ReplaceDialog::emitReplaceAll);
    connect(ui->btnCancel, &QPushButton::clicked, this, &ReplaceDialog::close);
    // 回车统一交给 default button（查找下一个），避免重复触发信号

    auto *shortcut = new QShortcut(QKeySequence(Qt::Key_F3), this);
    connect(shortcut, &QShortcut::activated, this, &ReplaceDialog::emitFindNext);

    ui->edtFind->setFocus();
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

void ReplaceDialog::setSearchText(const QString &text)
{
    ui->edtFind->setText(text);
    ui->edtFind->selectAll();
    ui->edtFind->setFocus();
}

void ReplaceDialog::emitFindNext()
{
    emit findNextRequested(ui->edtFind->text(),
                           ui->chkCase->isChecked(),
                           ui->rbUp->isChecked());
}

void ReplaceDialog::emitReplace()
{
    emit replaceRequested(ui->edtFind->text(), ui->edtReplace->text(),
                          ui->chkCase->isChecked(),
                          ui->rbUp->isChecked());
}

void ReplaceDialog::emitReplaceAll()
{
    emit replaceAllRequested(ui->edtFind->text(), ui->edtReplace->text(),
                             ui->chkCase->isChecked());
}
