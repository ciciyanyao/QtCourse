#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTextDocument>

class QLabel;
class QCloseEvent;
class QTimer;
class QClipboard;
class CodeEditor;
class FindDialog;
class ReplaceDialog;
class AboutDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // 供 main.cpp 使用：命令行传入的文件路径（含不存在 / 异常文件的场景）
    void openFilePath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNew();
    void onOpen();
    bool onSave();
    bool onSaveAs();
    void onExit();

    void onFind();
    void onReplace();
    void onAbout();

    void onWrapToggled(bool checked);
    void onLineNumberToggled(bool checked);
    void onFont();
    void onFontColor();
    void onTextBackground();
    void onEditorBackground();
    void onAutoToggled(bool checked);
    void onToolBarToggled(bool checked);
    void onStatusBarToggled(bool checked);

    void onTextChanged();
    void onFindNext(const QString &text, bool caseSensitive, bool backward);
    void onReplaceOne(const QString &find, const QString &replace,
                      bool caseSensitive, bool backward);
    void onReplaceAll(const QString &find, const QString &replace, bool caseSensitive);
    void onAutoSaveTimeout();
    void updatePasteAction();

private:
    CodeEditor *editor() const;
    QString displayName() const;
    void setCurrentFile(const QString &path);
    void setModified(bool modified);
    void updateTitle();
    void updateStatus();
    void updateActionStates();
    bool maybeSave();
    bool loadFile(const QString &path);
    bool writeFile(const QString &path);
    bool findFrom(const QString &needle, bool caseSensitive,
                  bool backward, bool allowWrap);
    QTextDocument::FindFlags makeFindFlags(bool caseSensitive, bool backward) const;
    void applyCharFormat(const QColor &color, bool foreground);

    Ui::MainWindow *ui;

    QString m_currentFile;
    QString m_lastDir;
    bool m_modified = false;
    bool m_loading = false;

    QTimer *m_autoSaveTimer = nullptr;
    FindDialog *m_findDlg = nullptr;
    ReplaceDialog *m_replaceDlg = nullptr;
    AboutDialog *m_aboutDlg = nullptr;

    QLabel *m_lblLength = nullptr;
    QLabel *m_lblLines = nullptr;
    QLabel *m_lblLn = nullptr;
    QLabel *m_lblCol = nullptr;
    QLabel *m_lblFileState = nullptr;
    QLabel *m_lblAuthor = nullptr;
};

#endif // MAINWINDOW_H
