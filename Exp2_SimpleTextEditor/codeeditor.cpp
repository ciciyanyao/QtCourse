#include "codeeditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QFontMetrics>
#include <QScrollBar>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QList>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    if (!m_showLineNumbers)
        return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    // 左侧 8px 留白 + 数字宽度 + 右侧 10px 分隔留白
    return 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 14;
}

void CodeEditor::setLineNumberVisible(bool visible)
{
    if (m_showLineNumbers == visible)
        return;
    m_showLineNumbers = visible;
    m_lineNumberArea->setVisible(visible);
    updateLineNumberAreaWidth(0);
    update();
}

void CodeEditor::setCurrentLineHighlightColor(const QColor &color)
{
    m_currentLineColor = color;
    highlightCurrentLine();
}

void CodeEditor::refreshLineNumberAreaWidth()
{
    updateLineNumberAreaWidth(0);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
    m_lineNumberArea->update();
}

void CodeEditor::setCurrentLineHighlight(bool on)
{
    m_highlightCurrentLine = on;
    if (!on)
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
    else
        highlightCurrentLine();
}

void CodeEditor::setLineNumberStyle(const QColor &background,
                                   const QColor &numberColor,
                                   const QColor &currentColor)
{
    m_areaBackground = background;
    m_numberColor = numberColor;
    m_currentColor = currentColor;
    m_lineNumberArea->update();
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    if (!m_highlightCurrentLine || isReadOnly())
        return;

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(m_currentLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    QList<QTextEdit::ExtraSelection> list;
    list.append(selection);
    setExtraSelections(list);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), m_areaBackground);

    // 行号区与正文之间的分隔线（随明暗主题自适应）
    const QColor border = m_areaBackground.lightness() < 128
            ? m_areaBackground.lighter(145)
            : QColor("#dde5f0");
    painter.setPen(border);
    painter.drawLine(event->rect().right(), event->rect().top(),
                     event->rect().right(), event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int currentBlock = textCursor().blockNumber();

    QFont normalFont = font();
    QFont boldFont = normalFont;
    boldFont.setBold(true);

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            if (blockNumber == currentBlock) {
                painter.setFont(boldFont);
                painter.setPen(m_currentColor);
            } else {
                painter.setFont(normalFont);
                painter.setPen(m_numberColor);
            }
            painter.drawText(0, top, m_lineNumberArea->width() - 12,
                             fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor), m_editor(editor)
{
    setObjectName(QStringLiteral("lineNumberArea"));
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_editor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}
