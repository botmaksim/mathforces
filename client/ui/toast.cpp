#include "toast.h"

#include <QLabel>
#include <QTimer>

namespace Toast {

void show(QWidget *parent, const QString &message, int timeoutMs) {
  QWidget *anchor = parent ? parent->window() : nullptr;
  if (!anchor)
    return;

  QLabel *toast = new QLabel(message, anchor);
  toast->setObjectName("toast");
  toast->setAttribute(Qt::WA_DeleteOnClose);
  toast->setWordWrap(true);
  toast->setAlignment(Qt::AlignCenter);
  toast->setMinimumWidth(260);
  toast->setMaximumWidth(520);
  toast->adjustSize();

  const int margin = 26;
  const int x = qMax(margin, anchor->width() - toast->width() - margin);
  const int y = qMax(margin, anchor->height() - toast->height() - margin);
  toast->move(x, y);
  toast->raise();
  toast->show();

  QTimer::singleShot(timeoutMs, toast, &QLabel::close);
}

} // namespace Toast
