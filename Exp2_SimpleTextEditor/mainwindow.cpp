#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"
#include "aboutdialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QColorDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>

namespace {

const qint64 kMaxFileBytes = 8LL * 1024 * 1024;   // 8 MB，避免误打开超大文件卡死界面

/* 严格校验 UTF-8：拒绝截断序列、过长编码、代理区码点。
   不依赖 Qt5/Qt6 各自不同的编解码 API，两个版本都能编译。*/
bool isValidUtf8(const QByteArray &data)
{
    const unsigned char *s = reinterpret_cast<const unsigned char *>(data.constData());
    const int n = data.size();
    int i = 0;
    while (i < n) {
        const unsigned char c = s[i];
        if (c < 0x80) { ++i; continue; }

        int need = 0;
        unsigned int cp = 0;
        unsigned int minCp = 0;
        if ((c & 0xE0) == 0xC0)      { need = 1; cp = c & 0x1F; minCp = 0x80; }
        else if ((c & 0xF0) == 0xE0) { need = 2; cp = c & 0x0F; minCp = 0x800; }
        else if ((c & 0xF8) == 0xF0) { need = 3; cp = c & 0x07; minCp = 0x10000; }
        else return false;

        if (i + need >= n) return false;
        for (int k = 1; k <= need; ++k) {
            const unsigned char cc = s[i + k];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (cp < minCp || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
            return false;
        i += need + 1;
    }
    return true;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    // ---------- 状态栏：长度 / 行数 / 行列 / 文件状态 / 作者 ----------
    auto addLabel = [this](const QString &name, const QString &text) -> QLabel * {
        QLabel *label = new QLabel(text, this);
        label->setObjectName(name);
        ui->statusbar->addPermanentWidget(label);
        return label;
    };
    m_lblLength    = addLabel("lblLength", "length: 0");
    m_lblLines     = addLabel("lblLines", "lines: 1");
    m_lblLn        = addLabel("lblLn", "Ln: 1");
    m_lblCol       = addLabel("lblCol", "Col: 1");
    m_lblFileState = addLabel("lblFileState", "未命名");
    m_lblAuthor    = addLabel("lblAuthor", "周勤希");

    // ---------- 菜单 / 工具栏 Action ----------
    connect(ui->actNew,      &QAction::triggered, this, &MainWindow::onNew);
    connect(ui->actOpen,     &QAction::triggered, this, &MainWindow::onOpen);
    connect(ui->actSave,     &QAction::triggered, this, &MainWindow::onSave);
    connect(ui->actSaveAs,   &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(ui->actExit,     &QAction::triggered, this, &MainWindow::onExit);

    connect(ui->actUndo,      &QAction::triggered, editor(), &QPlainTextEdit::undo);
    connect(ui->actRedo,      &QAction::triggered, editor(), &QPlainTextEdit::redo);
    connect(ui->actCut,       &QAction::triggered, editor(), &QPlainTextEdit::cut);
    connect(ui->actCopy,      &QAction::triggered, editor(), &QPlainTextEdit::copy);
    connect(ui->actPaste,     &QAction::triggered, editor(), &QPlainTextEdit::paste);
    connect(ui->actSelectAll, &QAction::triggered, editor(), &QPlainTextEdit::selectAll);

    connect(ui->actFind,    &QAction::triggered, this, &MainWindow::onFind);
    connect(ui->actReplace, &QAction::triggered, this, &MainWindow::onReplace);
    connect(ui->actAbout,   &QAction::triggered, this, &MainWindow::onAbout);

    connect(ui->actWrap,       &QAction::toggled, this, &MainWindow::onWrapToggled);
    connect(ui->actLineNum,    &QAction::toggled, this, &MainWindow::onLineNumberToggled);
    connect(ui->actFont,       &QAction::triggered, this, &MainWindow::onFont);
    connect(ui->actFontColor,  &QAction::triggered, this, &MainWindow::onFontColor);
    connect(ui->actTextBg,     &QAction::triggered, this, &MainWindow::onTextBackground);
    connect(ui->actEditorBg,   &QAction::triggered, this, &MainWindow::onEditorBackground);
    connect(ui->actAutoSave,   &QAction::toggled, this, &MainWindow::onAutoToggled);
    connect(ui->actToolbar,    &QAction::toggled, this, &MainWindow::onToolBarToggled);
    connect(ui->actStatusBar,  &QAction::toggled, this, &MainWindow::onStatusBarToggled);

    // ---------- 编辑器信号：让 Action 的使能状态与编辑状态一致 ----------
    connect(editor(), &QPlainTextEdit::textChanged,
            this, &MainWindow::onTextChanged);
    connect(editor(), &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateStatus);
    connect(editor(), &QPlainTextEdit::undoAvailable,
            ui->actUndo, &QAction::setEnabled);
    connect(editor(), &QPlainTextEdit::redoAvailable,
            ui->actRedo, &QAction::setEnabled);
    connect(editor(), &QPlainTextEdit::copyAvailable,
            ui->actCopy, &QAction::setEnabled);
    connect(editor(), &QPlainTextEdit::copyAvailable,
            ui->actCut, &QAction::setEnabled);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updatePasteAction);

    // ---------- 定时自动保存（选做） ----------
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(30 * 1000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::onAutoSaveTimeout);

    // 初始状态与 .ui 中设置的属性保持一致
    editor()->setLineNumberVisible(ui->actLineNum->isChecked());
    editor()->setLineWrapMode(ui->actWrap->isChecked()
                              ? QPlainTextEdit::WidgetWidth
                              : QPlainTextEdit::NoWrap);
    updateTitle();
    updateStatus();
    updateActionStates();
    ui->statusbar->showMessage("就绪  |  Ctrl+N 新建，Ctrl+O 打开，Ctrl+F 查找，Ctrl+H 替换", 6000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

CodeEditor *MainWindow::editor() const
{
    return ui->plainTextEdit;   // .ui 中已 promote 为 CodeEditor
}

QString MainWindow::displayName() const
{
    return m_currentFile.isEmpty()
            ? QStringLiteral("新建文本文件")
            : QFileInfo(m_currentFile).fileName();
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path;
    if (!path.isEmpty())
        m_lastDir = QFileInfo(path).absolutePath();
    updateTitle();
}

void MainWindow::setModified(bool modified)
{
    m_modified = modified;
    updateTitle();
    ui->actSave->setEnabled(m_modified);
    if (m_lblFileState)
        m_lblFileState->setText(m_modified ? QStringLiteral("已修改")
                                           : (m_currentFile.isEmpty()
                                              ? QStringLiteral("未命名")
                                              : QStringLiteral("已保存")));
}

void MainWindow::updateTitle()
{
    setWindowTitle(QString("%1%2 - 简单文本编辑器")
                   .arg(m_modified ? QStringLiteral("* ") : QString())
                   .arg(displayName()));
}

void MainWindow::updateStatus()
{
    if (!m_lblLength) return;
    const QTextCursor c = editor()->textCursor();
    m_lblLength->setText(QString("length: %1").arg(editor()->toPlainText().length()));
    m_lblLines->setText(QString("lines: %1").arg(editor()->blockCount()));
    m_lblLn->setText(QString("Ln: %1").arg(c.blockNumber() + 1));
    m_lblCol->setText(QString("Col: %1").arg(c.positionInBlock() + 1));
}

void MainWindow::updateActionStates()
{
    ui->actUndo->setEnabled(editor()->document()->isUndoAvailable());
    ui->actRedo->setEnabled(editor()->document()->isRedoAvailable());
    const bool selected = editor()->textCursor().hasSelection();
    ui->actCut->setEnabled(selected);
    ui->actCopy->setEnabled(selected);
    updatePasteAction();
    ui->actSelectAll->setEnabled(!editor()->document()->isEmpty());
    ui->actSave->setEnabled(m_modified);
}

void MainWindow::updatePasteAction()
{
    const QMimeData *mime = QApplication::clipboard()->mimeData();
    ui->actPaste->setEnabled(mime && mime->hasText() && !editor()->isReadOnly());
}

void MainWindow::onTextChanged()
{
    if (m_loading) return;      // 打开文件时不要把「载入」误判成用户修改
    setModified(true);
    updateStatus();
    updateActionStates();
}

/* ------------------------------------------------------------------
   文件操作
   ------------------------------------------------------------------ */

bool MainWindow::maybeSave()
{
    if (!m_modified) return true;

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("未保存的修改"));
    box.setIcon(QMessageBox::Question);
    box.setText(QString("文件「%1」已修改，是否保存？").arg(displayName()));
    box.setInformativeText(QStringLiteral("选择「丢弃」将丢失本次修改；选择「取消」可返回继续编辑。"));
    box.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Save);
    box.button(QMessageBox::Save)->setText(QStringLiteral("保存"));
    box.button(QMessageBox::Discard)->setText(QStringLiteral("丢弃"));
    box.button(QMessageBox::Cancel)->setText(QStringLiteral("取消"));

    const int ret = box.exec();
    if (ret == QMessageBox::Save)    return onSave();
    if (ret == QMessageBox::Discard) return true;
    return false;                     // Cancel：不关闭、不新建、不打开
}

void MainWindow::onNew()
{
    if (!maybeSave()) return;
    m_loading = true;
    editor()->clear();                // clear() 同时清空 undo/redo 栈
    m_loading = false;
    setCurrentFile(QString());
    setModified(false);
    updateStatus();
    updateActionStates();
    ui->statusbar->showMessage(QStringLiteral("已新建空白文件"), 3000);
}

void MainWindow::onOpen()
{
    if (!maybeSave()) return;
    const QString dir = m_lastDir.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
            : m_lastDir;
    const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("打开文件"), dir,
                QStringLiteral("文本文件 (*.txt *.md *.log *.csv *.cpp *.h);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        ui->statusbar->showMessage(QStringLiteral("已取消打开"), 2500);
        return;
    }
    openFilePath(path);
}

void MainWindow::openFilePath(const QString &path)
{
    loadFile(path);     // 路径可能不存在 / 是二进制文件 / 权限不足，统一在这里处理
}

bool MainWindow::loadFile(const QString &path)
{
    if (path.trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("打开文件"), QStringLiteral("文件路径为空。"));
        return false;
    }

    const QFileInfo info(path);
    if (!info.exists()) {
        QMessageBox::warning(this, QStringLiteral("打开文件"),
                             QString("文件不存在：\n%1\n请检查路径后重试。").arg(path));
        return false;
    }
    if (info.isDir() || !info.isFile()) {
        QMessageBox::warning(this, QStringLiteral("打开文件"),
                             QString("这不是一个普通文件：\n%1").arg(path));
        return false;
    }
    if (info.size() > kMaxFileBytes) {
        QMessageBox::warning(this, QStringLiteral("打开文件"),
                             QString("文件超过 %1 MB，已拒绝打开以避免界面失去响应。")
                             .arg(kMaxFileBytes / 1024 / 1024));
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("打开文件"),
                             QString("无法读取文件：\n%1\n%2").arg(path, file.errorString()));
        return false;
    }
    QByteArray bytes = file.readAll();
    file.close();

    if (bytes.startsWith("\xEF\xBB\xBF"))    // 去掉 UTF-8 BOM
        bytes.remove(0, 3);

    if (bytes.contains('\0')) {
        QMessageBox::warning(this, QStringLiteral("打开文件"),
                             QStringLiteral("检测到 NUL 字节，该文件疑似二进制文件。\n"
                                            "已拒绝打开，避免把乱码写入编辑区。"));
        return false;
    }

    QString text;
    if (!isValidUtf8(bytes)) {
        const int ret = QMessageBox::question(
                    this, QStringLiteral("打开文件"),
                    QStringLiteral("该文件不是有效的 UTF-8 文本（可能是 GBK 编码或已损坏）。\n"
                                   "是否仍按本地编码尝试打开？"),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            ui->statusbar->showMessage(QStringLiteral("已取消打开非 UTF-8 文件"), 3000);
            return false;
        }
        text = QString::fromLocal8Bit(bytes);
    } else {
        text = QString::fromUtf8(bytes);
    }

    m_loading = true;
    editor()->setPlainText(text);
    QTextCursor c = editor()->textCursor();
    c.movePosition(QTextCursor::Start);
    editor()->setTextCursor(c);
    m_loading = false;

    setCurrentFile(path);
    setModified(false);
    updateStatus();
    updateActionStates();
    ui->statusbar->showMessage(QString("已打开：%1").arg(path), 4000);
    return true;
}

bool MainWindow::writeFile(const QString &path)
{
    if (path.isEmpty()) return false;

    QSaveFile file(path);      // 先写临时文件再原子替换，写失败不会破坏原文件
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, QStringLiteral("保存文件"),
                             QString("无法写入文件：\n%1\n%2").arg(path, file.errorString()));
        return false;
    }
    const QByteArray data = editor()->toPlainText().toUtf8();
    if (file.write(data) != data.size()) {
        file.cancelWriting();
        QMessageBox::warning(this, QStringLiteral("保存文件"),
                             QStringLiteral("写入过程发生错误，已取消本次保存，原文件保持不变。"));
        return false;
    }
    if (!file.commit()) {
        QMessageBox::warning(this, QStringLiteral("保存文件"),
                             QStringLiteral("保存失败（目标目录可能只读或文件被占用）。"));
        return false;
    }
    return true;
}

bool MainWindow::onSave()
{
    if (m_currentFile.isEmpty())
        return onSaveAs();                 // 新建文件：保存 == 另存为
    if (!writeFile(m_currentFile)) return false;
    setModified(false);
    ui->statusbar->showMessage(QString("已保存：%1").arg(m_currentFile), 3000);
    return true;
}

bool MainWindow::onSaveAs()
{
    const QString dir = m_currentFile.isEmpty()
            ? (m_lastDir.isEmpty()
               ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
               : m_lastDir)
            : m_currentFile;
    const QString path = QFileDialog::getSaveFileName(
                this, QStringLiteral("另存为"), dir,
                QStringLiteral("文本文件 (*.txt);;Markdown (*.md);;所有文件 (*.*)"));
    if (path.isEmpty()) {
        ui->statusbar->showMessage(QStringLiteral("已取消另存为"), 2500);
        return false;
    }
    if (!writeFile(path)) return false;
    setCurrentFile(path);
    setModified(false);
    ui->statusbar->showMessage(QString("已另存为：%1").arg(path), 3000);
    return true;
}

void MainWindow::onExit()
{
    close();            // 走 closeEvent，统一处理「未保存」提示
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

/* ------------------------------------------------------------------
   查找 / 替换
   ------------------------------------------------------------------ */

QTextDocument::FindFlags MainWindow::makeFindFlags(bool caseSensitive,
                                                  bool backward) const
{
    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (backward)      flags |= QTextDocument::FindBackward;
    return flags;
}

bool MainWindow::findFrom(const QString &needle, bool caseSensitive,
                          bool backward, bool allowWrap)
{
    if (needle.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("查找结果"),
                                 QStringLiteral("查找内容不能为空，请先输入要查找的文字。"));
        return false;
    }

    QTextDocument *doc = editor()->document();
    QTextCursor start = editor()->textCursor();
    // 连续查找：从「当前选区的末端/首端」继续，避免反复命中同一个词
    if (start.hasSelection())
        start.setPosition(backward ? start.selectionStart() : start.selectionEnd());

    const QTextDocument::FindFlags flags = makeFindFlags(caseSensitive, backward);
    QTextCursor hit = doc->find(needle, start, flags);

    if (hit.isNull() && allowWrap) {
        QTextCursor edge(doc);
        if (backward) edge.movePosition(QTextCursor::End);
        hit = doc->find(needle, edge, flags);
        if (!hit.isNull())
            ui->statusbar->showMessage(
                        backward ? QStringLiteral("已到文档开头，从末尾继续查找")
                                 : QStringLiteral("已到文档末尾，从头继续查找"), 3000);
    }

    if (hit.isNull()) {
        QMessageBox::information(this, QStringLiteral("查找结果"),
                                 QString("未找到 \"%1\"").arg(needle));
        return false;
    }

    editor()->setTextCursor(hit);
    editor()->centerCursor();
    return true;
}

void MainWindow::onFindNext(const QString &text, bool caseSensitive, bool backward)
{
    findFrom(text, caseSensitive, backward, true);
}

void MainWindow::onReplaceOne(const QString &find, const QString &replace,
                              bool caseSensitive, bool backward)
{
    if (find.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("替换结果"),
                                 QStringLiteral("查找内容不能为空。"));
        return;
    }

    QTextCursor c = editor()->textCursor();
    const QString selected = c.selectedText();
    const bool matched = c.hasSelection() &&
            (caseSensitive ? selected == find
                           : selected.compare(find, Qt::CaseInsensitive) == 0);
    if (matched) {
        c.beginEditBlock();
        c.insertText(replace);           // 只替换当前选中项，作为一次可撤销操作
        c.endEditBlock();
        editor()->setTextCursor(c);
    }
    // 替换后（或当前没有匹配选区时）继续定位到下一处
    findFrom(find, caseSensitive, backward, true);
}

void MainWindow::onReplaceAll(const QString &find, const QString &replace,
                              bool caseSensitive)
{
    if (find.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("替换结果"),
                                 QStringLiteral("查找内容不能为空。"));
        return;
    }

    QTextDocument *doc = editor()->document();
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Start);
    cursor.beginEditBlock();             // 整批替换只占一步撤销

    const QTextDocument::FindFlags flags = makeFindFlags(caseSensitive, false);
    int count = 0;
    for (;;) {
        QTextCursor hit = doc->find(find, cursor, flags);
        if (hit.isNull()) break;
        hit.insertText(replace);
        cursor = hit;                    // 从替换后的位置继续，避免重复命中
        ++count;
        if (count > 100000) break;       // 防御性上限
    }
    cursor.endEditBlock();

    if (count == 0)
        QMessageBox::information(this, QStringLiteral("替换结果"),
                                 QString("未找到 \"%1\"，文档未改动。").arg(find));
    else
        QMessageBox::information(this, QStringLiteral("替换结果"),
                                 QString("已完成全部替换，共 %1 处。").arg(count));
}

void MainWindow::onFind()
{
    if (!m_findDlg) {
        m_findDlg = new FindDialog(this);
        connect(m_findDlg, &FindDialog::findNextRequested,
                this, &MainWindow::onFindNext);
    }
    QTextCursor c = editor()->textCursor();
    if (c.hasSelection()) m_findDlg->setSearchText(c.selectedText());
    m_findDlg->show();
    m_findDlg->raise();
    m_findDlg->activateWindow();
}

void MainWindow::onReplace()
{
    if (!m_replaceDlg) {
        m_replaceDlg = new ReplaceDialog(this);
        connect(m_replaceDlg, &ReplaceDialog::findNextRequested,
                this, &MainWindow::onFindNext);
        connect(m_replaceDlg, &ReplaceDialog::replaceRequested,
                this, &MainWindow::onReplaceOne);
        connect(m_replaceDlg, &ReplaceDialog::replaceAllRequested,
                this, &MainWindow::onReplaceAll);
    }
    QTextCursor c = editor()->textCursor();
    if (c.hasSelection()) m_replaceDlg->setSearchText(c.selectedText());
    m_replaceDlg->show();
    m_replaceDlg->raise();
    m_replaceDlg->activateWindow();
}

void MainWindow::onAbout()
{
    if (!m_aboutDlg) m_aboutDlg = new AboutDialog(this);
    m_aboutDlg->exec();
}

/* ------------------------------------------------------------------
   格式 / 视图
   ------------------------------------------------------------------ */

void MainWindow::onWrapToggled(bool checked)
{
    editor()->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth
                                      : QPlainTextEdit::NoWrap);
    ui->statusbar->showMessage(checked ? QStringLiteral("自动换行：开")
                                       : QStringLiteral("自动换行：关"), 2500);
}

void MainWindow::onLineNumberToggled(bool checked)
{
    editor()->setLineNumberVisible(checked);
    ui->statusbar->showMessage(checked ? QStringLiteral("行号：显示")
                                       : QStringLiteral("行号：隐藏"), 2500);
}

void MainWindow::onFont()
{
    bool ok = false;
    const QFont f = QFontDialog::getFont(&ok, editor()->font(), this,
                                         QStringLiteral("选择字体与字号"));
    if (!ok) return;
    editor()->setFont(f);
    editor()->refreshLineNumberAreaWidth();   // 字号变大后行号区宽度同步重算
}

void MainWindow::applyCharFormat(const QColor &color, bool foreground)
{
    QTextCursor c = editor()->textCursor();
    if (!c.hasSelection())
        c.select(QTextCursor::Document);
    QTextCharFormat fmt;
    if (foreground) fmt.setForeground(color);
    else            fmt.setBackground(color);
    c.mergeCharFormat(fmt);
    ui->statusbar->showMessage(
                QStringLiteral("已应用字符格式（纯文本保存时不保留颜色）"), 4000);
}

void MainWindow::onFontColor()
{
    const QColor color = QColorDialog::getColor(
                editor()->palette().color(QPalette::Text), this,
                QStringLiteral("选择字体颜色"));
    if (color.isValid()) applyCharFormat(color, true);
}

void MainWindow::onTextBackground()
{
    const QColor color = QColorDialog::getColor(QColor("#fff3bf"), this,
                                                QStringLiteral("选择字体背景色"));
    if (color.isValid()) applyCharFormat(color, false);
}

void MainWindow::onEditorBackground()
{
    const QColor color = QColorDialog::getColor(QColor("#ffffff"), this,
                                                QStringLiteral("选择编辑器背景色"));
    if (!color.isValid()) return;

    const bool dark = color.lightness() < 128;
    const QColor fg   = dark ? QColor("#e6edf7") : QColor("#1f2937");
    const QColor area = dark ? QColor("#2c3a4f") : QColor("#f2f6fc");
    const QColor num  = dark ? QColor("#8fa3bf") : QColor("#94a3b8");

    editor()->setStyleSheet(QString(
        "#plainTextEdit { background-color:%1; color:%2; border:1px solid %3;"
        " border-radius:8px; padding:6px 8px;"
        " selection-background-color:%4; selection-color:%5; }")
        .arg(color.name(), fg.name(),
             dark ? QString("#44566f") : QString("#dfe6f0"),
             dark ? QString("#3d5c85") : QString("#b9d3ff"),
             dark ? QString("#f1f5f9") : QString("#102a56")));
    editor()->setLineNumberStyle(area, num,
                                 dark ? QColor("#93c5fd") : QColor("#2563eb"));
    editor()->setCurrentLineHighlightColor(
                dark ? QColor("#24384f") : QColor("#eaf1ff"));
    editor()->setCurrentLineHighlight(true);
    ui->statusbar->showMessage(QStringLiteral("编辑器背景色已更新"), 3000);
}

void MainWindow::onAutoToggled(bool checked)
{
    if (checked) {
        m_autoSaveTimer->start();
        ui->statusbar->showMessage(QStringLiteral("定时自动保存：开（每 30 秒）"), 3500);
    } else {
        m_autoSaveTimer->stop();
        ui->statusbar->showMessage(QStringLiteral("定时自动保存：关"), 2500);
    }
}

void MainWindow::onAutoSaveTimeout()
{
    if (!m_modified) return;
    if (m_currentFile.isEmpty()) {
        ui->statusbar->showMessage(
                    QStringLiteral("自动保存跳过：文件尚未命名，请先执行「另存为」"), 4000);
        return;
    }
    if (writeFile(m_currentFile)) {
        setModified(false);
        ui->statusbar->showMessage(QStringLiteral("自动保存完成"), 3000);
    }
}

void MainWindow::onToolBarToggled(bool checked)
{
    ui->mainToolBar->setVisible(checked);
}

void MainWindow::onStatusBarToggled(bool checked)
{
    ui->statusbar->setVisible(checked);
}
