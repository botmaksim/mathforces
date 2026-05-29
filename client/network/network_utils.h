#pragma once

#include <QString>
#include <QWidget>

class QNetworkReply;

namespace NetworkUtils {

QString friendlyError(QNetworkReply *reply);
void showError(QWidget *parent, const QString &title, QNetworkReply *reply);

} // namespace NetworkUtils
