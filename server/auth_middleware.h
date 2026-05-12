#pragma once
#include <QByteArray>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QMessageAuthenticationCode>
#include <QDateTime>
#include <qcoreapplication.h>

namespace AuthMiddleware {
inline QString base64UrlEncode(const QByteArray &data) {
    QString res = data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return res;
}

inline QByteArray base64UrlDecode(const QString &str) {
    return QByteArray::fromBase64(str.toUtf8(), QByteArray::Base64UrlEncoding);
}

inline QString generateJwt(int userId, const QString &role) {
    QJsonObject header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    QJsonObject payload;
    payload["userId"] = userId;
    payload["role"] = role;
    payload["exp"] = QDateTime::currentSecsSinceEpoch() + 86400 * 7; // 7 days

    QString headerB64 = base64UrlEncode(QJsonDocument(header).toJson(QJsonDocument::Compact));
    QString payloadB64 = base64UrlEncode(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QString unsignedToken = headerB64 + "." + payloadB64;

    QString secret = qEnvironmentVariable("JWT_SECRET", "default_secret_key");
    QByteArray sig = QMessageAuthenticationCode::hash(unsignedToken.toUtf8(), secret.toUtf8(), QCryptographicHash::Sha256);
    
    return unsignedToken + "." + base64UrlEncode(sig);
}

inline bool isValid(QString token, int &outUserId, QString &outRole) {
    if (token.startsWith("Bearer "))
        token = token.mid(7);
        
    QStringList parts = token.split('.');
    if (parts.size() != 3)
        return false;
        
    QString unsignedToken = parts[0] + "." + parts[1];
    QString secret = qEnvironmentVariable("JWT_SECRET", "default_secret_key");
    QByteArray expectedSig = QMessageAuthenticationCode::hash(unsignedToken.toUtf8(), secret.toUtf8(), QCryptographicHash::Sha256);
    if (base64UrlEncode(expectedSig) != parts[2])
        return false;
        
    QJsonDocument payloadDoc = QJsonDocument::fromJson(base64UrlDecode(parts[1]));
    if (!payloadDoc.isObject())
        return false;
        
    QJsonObject payload = payloadDoc.object();
    qint64 exp = payload["exp"].toVariant().toLongLong();
    if (exp > 0 && QDateTime::currentSecsSinceEpoch() > exp)
        return false; // Expired
        
    outUserId = payload["userId"].toInt();
    outRole = payload["role"].toString();
    return true;
}

inline bool getAuthInfo(const QHttpServerRequest &request, int &outUserId,
                        QString &outRole) {
  QString token;
  auto val = request.value("Authorization");
  if (!val.isEmpty())
    token = QString::fromUtf8(val);
  else {
    val = request.value("authorization");
    if (!val.isEmpty())
      token = QString::fromUtf8(val);
  }
  return isValid(token, outUserId, outRole);
}
} // namespace AuthMiddleware
