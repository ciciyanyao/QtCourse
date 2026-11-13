#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ReplaceDialog; }
QT_END_NAMESPACE

class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(QWidget *parent = nullptr);
    ~ReplaceDialog() override;

    void setSearchText(const QString &text);

signals:
    void findNextRequested(const QString &text, bool caseSensitive, bool backward);
    void replaceRequested(const QString &find, const QString &replace,
                          bool caseSensitive, bool backward);
    void replaceAllRequested(const QString &find, const QString &replace,
                             bool caseSensitive);

private slots:
    void emitFindNext();
    void emitReplace();
    void emitReplaceAll();

private:
    Ui::ReplaceDialog *ui;
};

#endif // REPLACEDIALOG_H
