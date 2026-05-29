#include "welcome_widget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *outer = new QVBoxLayout(this);
  outer->setContentsMargins(18, 18, 18, 18);
  outer->addStretch();

  QFrame *card = new QFrame(this);
  card->setObjectName("welcomeHero");
  QVBoxLayout *layout = new QVBoxLayout(card);
  layout->setContentsMargins(34, 34, 34, 34);
  layout->setSpacing(14);

  QLabel *icon = new QLabel(QStringLiteral("∑∞"), card);
  icon->setObjectName("brandIcon");
  icon->setAlignment(Qt::AlignCenter);

  QLabel *title = new QLabel(QStringLiteral("Добро пожаловать в MathForces"), card);
  title->setObjectName("welcomeTitle");
  title->setAlignment(Qt::AlignCenter);
  title->setWordWrap(true);

  QLabel *text = new QLabel(
      QStringLiteral("Выберите контест во вкладке «Контесты», чтобы открыть задачи. "
                     "Пока контест не выбран, здесь будет стартовая страница, "
                     "подсказки и быстрый доступ к архиву."),
      card);
  text->setObjectName("mutedLabel");
  text->setAlignment(Qt::AlignCenter);
  text->setWordWrap(true);

  QLabel *steps = new QLabel(
      QStringLiteral("1. Откройте контест\n2. Выберите задачу\n3. Пишите решение с Typst/LaTeX предпросмотром\n4. Отправляйте и следите за вердиктами"),
      card);
  steps->setObjectName("infoCard");
  steps->setWordWrap(true);

  QHBoxLayout *iconRow = new QHBoxLayout();
  iconRow->addStretch();
  iconRow->addWidget(icon);
  iconRow->addStretch();

  layout->addLayout(iconRow);
  layout->addWidget(title);
  layout->addWidget(text);
  layout->addSpacing(10);
  layout->addWidget(steps);

  outer->addWidget(card, 0, Qt::AlignCenter);
  outer->addStretch();
}
