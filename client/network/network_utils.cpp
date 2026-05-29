#include "network_utils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>

namespace NetworkUtils {

QString friendlyError(QNetworkReply *reply) {
  QByteArray body = reply->readAll();
  QString serverMessage;
  const QJsonDocument json = QJsonDocument::fromJson(body);
  if (json.isObject()) {
    const QJsonObject obj = json.object();
    serverMessage = obj.value("error").toString();
    if (serverMessage.isEmpty())
      serverMessage = obj.value("message").toString();
  }
  if (serverMessage.isEmpty())
    serverMessage = QString::fromUtf8(body).trimmed();

  QString reason;
  switch (reply->error()) {
  case QNetworkReply::ConnectionRefusedError:
    reason = QStringLiteral("сервер недоступен или не запущен");
    break;
  case QNetworkReply::RemoteHostClosedError:
    reason = QStringLiteral("сервер закрыл соединение");
    break;
  case QNetworkReply::HostNotFoundError:
    reason = QStringLiteral("адрес сервера не найден");
    break;
  case QNetworkReply::TimeoutError:
    reason = QStringLiteral("превышено время ожидания ответа");
    break;
  case QNetworkReply::OperationCanceledError:
    reason = QStringLiteral("запрос был отменён");
    break;
  case QNetworkReply::SslHandshakeFailedError:
    reason = QStringLiteral("ошибка TLS/SSL соединения");
    break;
  case QNetworkReply::TemporaryNetworkFailureError:
    reason = QStringLiteral("временная ошибка сети");
    break;
  case QNetworkReply::AuthenticationRequiredError:
    reason = QStringLiteral("нужна авторизация");
    break;
  case QNetworkReply::ContentAccessDenied:
    reason = QStringLiteral("доступ запрещён");
    break;
  case QNetworkReply::ContentNotFoundError:
    reason = QStringLiteral("ресурс не найден");
    break;
  case QNetworkReply::NoError:
    break;
  default:
    reason = reply->errorString();
    break;
  }

  QString result = reason.isEmpty() ? reply->errorString() : reason;
  if (!serverMessage.isEmpty())
    result += "\n\nОтвет сервера: " + serverMessage;
  return result;
}

void showError(QWidget *parent, const QString &title, QNetworkReply *reply) {
  QMessageBox::warning(parent, title, friendlyError(reply));
}

} // namespace NetworkUtils
