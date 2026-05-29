#pragma once

#include <QString>
#include <QWidget>

namespace Toast {
void show(QWidget *parent, const QString &message, int timeoutMs = 2600);
}
