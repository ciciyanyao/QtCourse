#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QColor>
#include <QRect>
#include <QSize>

class QPaintEvent;
class QResizeEvent;
class QWidget;
class LineNumberArea;

/*
 * CodeEditor —— 从 QPlainTextEdit 派生的自定义控件
 * 参考资料 3（Qt Code Editor Example）的实现思路：
 * 在 viewport 左侧留出边距，用一个独立的 QWidget 作为行号区，
 * 通过 updateRequest 信号与文本滚动保持同步，再在 paintEvent 中自绘行号。
 * 在 Qt Designer 里对 QPlainTextEdit 执行「提升为…」(promote to) 即可使用。
 */
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setLineNumberVisible(bool visible);
    bool lineNumberVisible() const { return m_showLineNumbers; }

    void setCurrentLineHighlight(bool on);
    bool currentLineHighlight() const { return m_highlightCurrentLine; }
    void refreshLineNumberAreaWidth();

    // 与「编辑器背景色」联动，避免行号区与正文配色割裂
    void setLineNumberStyle(const QColor &background,
                            const QColor &numberColor,
                            const QColor &currentColor);
    void setCurrentLineHighlightColor(const QColor &color);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    LineNumberArea *m_lineNumberArea = nullptr;
    bool m_showLineNumbers = true;
    bool m_highlightCurrentLine = true;

    QColor m_areaBackground  { QColor("#f2f6fc") };
    QColor m_numberColor     { QColor("#94a3b8") };
    QColor m_currentColor    { QColor("#2563eb") };
    QColor m_currentLineColor{ QColor("#eaf1ff") };
};

/* 行号区：只负责把 paintEvent 转交给 CodeEditor */
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *m_editor;
};

#endif // CODEEDITOR_H
