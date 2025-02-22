#ifndef CONDITIONEDITOR_H
#define CONDITIONEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QCompleter>

class ConditionEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    ConditionEditor(QWidget* parent = nullptr);

    void updateCompleterModel();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void inputMethodEvent(QInputMethodEvent *e) override;

private slots:
    void showCompleter(QString prefix);
    void onCompleterActivated(const QString &completion);

public:
    static QStringList allCompletes; // 所有默认填充的
    static int completerWidth;

private:
    QCompleter* completer;
    QString currentPrefix;
};

class ConditionHighlighter : public QSyntaxHighlighter
{
    struct QSSRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

public:
    ConditionHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;
};


#endif // CONDITIONEDITOR_H
