#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QIcon>
#include <QPixmap>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->lblIcon->setPixmap(QIcon(QStringLiteral(":/icons/app.svg")).pixmap(72, 72));
    connect(ui->btnOK, &QPushButton::clicked, this, &AboutDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
