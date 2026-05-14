#include "math_highlighter.h"

MathHighlighter::MathHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
  HighlightingRule rule;

  // 1. LaTeX команды (начинаются с \)
  latexKeywordFormat.setForeground(QColor("#569cd6")); // Светло-синий
  latexKeywordFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression(QStringLiteral("\\\\[a-zA-Z]+"));
  rule.format = latexKeywordFormat;
  highlightingRules.append(rule);

  // 2. Typst хештеги и функции (#func)
  typstKeywordFormat.setForeground(QColor("#c586c0")); // Розово-фиолетовый
  typstKeywordFormat.setFontWeight(QFont::Bold);
  rule.pattern = QRegularExpression(QStringLiteral("#[a-zA-Z0-9_]+"));
  rule.format = typstKeywordFormat;
  highlightingRules.append(rule);

  // 3. Математический режим ($...$ или $$...$$)
  mathModeFormat.setForeground(QColor("#b5cea8")); // Светло-зеленый
  // Регулярное выражение для захвата текста внутри $...$
  rule.pattern = QRegularExpression(QStringLiteral("\\$[^\\$]*\\$"));
  rule.format = mathModeFormat;
  highlightingRules.append(rule);

  // 4. Комментарии (LaTeX % ..., Typst // ...)
  commentFormat.setForeground(QColor("#6A9955")); // Тускло-зеленый
  commentFormat.setFontItalic(true);
  // Для LaTeX
  rule.pattern = QRegularExpression(QStringLiteral("%[^\n]*"));
  rule.format = commentFormat;
  highlightingRules.append(rule);
  // Для Typst
  rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
  rule.format = commentFormat;
  highlightingRules.append(rule);
}

void MathHighlighter::highlightBlock(const QString &text) {
  for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
    QRegularExpressionMatchIterator matchIterator =
        rule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }
}
