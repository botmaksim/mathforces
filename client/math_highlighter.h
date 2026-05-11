#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class MathHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit MathHighlighter(QTextDocument *parent = nullptr);
protected:
    void highlightBlock(const QString &text) override;
private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QList<HighlightingRule> highlightingRules;
    
    QTextCharFormat latexKeywordFormat;
    QTextCharFormat typstKeywordFormat;
    QTextCharFormat mathModeFormat;
    QTextCharFormat commentFormat;
};
