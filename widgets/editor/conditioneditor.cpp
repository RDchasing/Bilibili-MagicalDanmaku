#include <QDebug>
#include <QAbstractItemView>
#include <QStringListModel>
#include "conditioneditor.h"

QStringList ConditionEditor::allCompletes;
int ConditionEditor::completerWidth = 200;

ConditionEditor::ConditionEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    // 设置高亮
    new ConditionHighlighter(document());

    // 设置自动补全
    completer = new QCompleter(allCompletes, this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    connect(completer, SIGNAL(activated(QString)), this, SLOT(onCompleterActivated(QString)));

    static bool first = true;
    if (!first)
    {
        first = true;
        QFontMetrics fm(this->font());
        completerWidth = fm.horizontalAdvance(">QwertyuiopAsdfghjklZxcvbnm");
    }
}

void ConditionEditor::updateCompleterModel()
{
    completer->setModel(new QStringListModel(allCompletes));
}

void ConditionEditor::keyPressEvent(QKeyEvent *e)
{
    auto mod = e->modifiers();
    if (mod != Qt::NoModifier && mod != Qt::ShiftModifier)
    {
        return QPlainTextEdit::keyPressEvent(e);
    }
    auto key = e->key();

    // 查找
    if (completer->popup() && completer->popup()->isVisible())
    {
        if (key == Qt::Key_Escape || key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Tab)
        {
            e->ignore();
            return ;
        }
    }

    QPlainTextEdit::keyPressEvent(e);

    // 回车键
    QString left = toPlainText().left(textCursor().position());
    if (key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        if (left.isEmpty())
            return ;
        left = left.left(left.length() - 1);
        // 判断左边是否有表示软换行的反斜杠
        if (!left.contains(QRegularExpression("\\\\\\s*(//.*)?$")))
            return ;

        // 软换行，回车跟随上一行的缩进，或者起码有一个Tab缩进
        QString indent = QRegularExpression("\\s*").match(left.mid(left.lastIndexOf("\n")+1)).captured();
        textCursor().insertText(indent.isEmpty() ? "\t" : indent);
    }

    // 其他键
    else if ((key >= Qt::Key_A &&key <= Qt::Key_Z) || key == Qt::Key_Minus) // 变量
    {
        // 获取当前单词
        QRegularExpressionMatch match;
        if (left.indexOf(QRegularExpression("((%>|%|>)[\\w_4e00-\u9fa5]+)$"), 0, &match) == -1)
            return ;
        QString word = match.captured(1);
        // qInfo() << "当前单词：" << word;

        // 开始提示
        showCompleter(word);
    }
    else if (key == Qt::Key_Percent) // 百分号%
    {
        showCompleter("%");
    }
    else if (key == Qt::Key_Greater)
    {
        showCompleter(">");
    }
    else
    {
        if (completer->popup() && completer->popup()->isVisible())
        {
            completer->popup()->hide();
        }
    }
}

void ConditionEditor::inputMethodEvent(QInputMethodEvent *e)
{
    QPlainTextEdit::inputMethodEvent(e);
    // 获取当前单词
    QString left = toPlainText().left(textCursor().position());
    QRegularExpressionMatch match;
    if (left.indexOf(QRegularExpression("((%>|%|>)[\\w_4e00-\u9fa5]+)$"), 0, &match) == -1)
        return ;
    QString word = match.captured(1);
    // qInfo() << "当前单词：" << word;

    // 开始提示
    showCompleter(word);
}

void ConditionEditor::showCompleter(QString prefix)
{
    completer->setCompletionPrefix(prefix);
    QRect cr = cursorRect();
    completer->complete(QRect(cr.left(), cr.bottom(), completerWidth, completer->popup()->sizeHint().height()));
    completer->popup()->move(mapToGlobal(cr.bottomLeft()));
}

void ConditionEditor::onCompleterActivated(const QString &completion)
{
    QString completionPrefix = completer->completionPrefix(),
            shouldInertText = completion;
    QTextCursor cursor = this->textCursor();
    if (!completion.contains(completionPrefix))
    {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, completionPrefix.size());
        cursor.clearSelection();
    }
    else // 补全相应的字符
    {
        shouldInertText = shouldInertText.replace(
                    shouldInertText.indexOf(completionPrefix),
                    completionPrefix.size(), "");
    }
    cursor.insertText(shouldInertText);

    // 补全右括号
    QString suffixStr = "";
    if (completion.endsWith("("))
        suffixStr = ")";
    if (completion.startsWith("%") && !completion.endsWith("%"))
        suffixStr += "%";

    if (!suffixStr.isEmpty())
    {
        cursor.insertText(suffixStr);
        cursor.setPosition(cursor.position() - suffixStr.length());
        setTextCursor(cursor);
    }
}

ConditionHighlighter::ConditionHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
}

void ConditionHighlighter::highlightBlock(const QString &text)
{
    static auto getTCF = [](QColor c) {
        QTextCharFormat f;
        f.setForeground(c);
        return f;
    };
    static QList<QSSRule> qss_rules = {
        // [condition]
        QSSRule{QRegularExpression("^(\\[.*?\\])"), getTCF(QColor(128, 34, 172))},
        // 执行函数 >func(args)
        QSSRule{QRegularExpression("(^|[\\]\\)\\*])\\s*>\\s*\\w+\\s*\\(.*?\\)\\s*($|\\\\n|\\\\|//.*)"), getTCF(QColor(136, 80, 80))},
        // 变量 %val%
        QSSRule{QRegularExpression("%\\S+?%"), getTCF(QColor(204, 85, 0))},
        // 取值 %{}%
        QSSRule{QRegularExpression("%\\{\\S+?\\}%"), getTCF(QColor(153, 107, 31))},
        // 计算 %[]%
        QSSRule{QRegularExpression("%\\[\\S+?\\]%"), getTCF(QColor(82, 165, 190))},
        // 名字类变量 %xxx_name%
        QSSRule{QRegularExpression("%[^%\\s]*?name[^%\\s]*?%"), getTCF(QColor(237, 51, 0))},
        // 数字 123
        QSSRule{QRegularExpression("\\d+?"), getTCF(QColor(9, 54, 184))},
        // 字符串 'str'  "str
        QSSRule{QRegularExpression("('.*?'|\".*?\")"), getTCF(QColor(80, 200, 120))},
        // 优先级 []**
        QSSRule{QRegularExpression("(?<=\\])\\s*(\\*)+"), getTCF(QColor(82, 165, 190))},
        // 标签 <h1>
        QSSRule{QRegularExpression("(<[^\\],]+?>|\\\\n)"), getTCF(QColor(216, 167, 9))},
        // 冷却通道 (cd5:10)
        QSSRule{QRegularExpression("\\(cd\\d{1,2}:\\d+\\)"), getTCF(QColor(0, 128, 0))},
        // 注释
        QSSRule{QRegularExpression("(?<!:)//.*?(?=\\n|$|\\\\n)"), getTCF(QColor(119, 136, 153))},
        // 开头注释，标记为标题
        QSSRule{QRegularExpression("^\\s*///.*?(?=\\n|$|\\\\n)"), getTCF(QColor(43, 85, 213))},
        // 软换行符
        QSSRule{QRegularExpression("\\s*\\\\\\s*\\n\\s*"), getTCF(QColor(119, 136, 153))},
    };

    foreach (auto rule, qss_rules)
    {
        auto iterator = rule.pattern.globalMatch(text);
        while (iterator.hasNext()) {
            auto match = iterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
