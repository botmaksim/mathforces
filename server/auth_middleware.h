#pragma once
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QString>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

// Учебный middleware для работы с токенами
namespace AuthMiddleware {
    inline QString generateJwt(int userId, const QString& role) {
        // Заглушка вместо реального RSA/HMAC JWT: token = "МЕДЖИК_СЛОВО.userId.role"
        return QString("jwt_magic_token.%1.%2").arg(userId).arg(role);
    }

    inline bool isValid(const QString& token, int& outUserId, QString& outRole) {
        if (!token.startsWith("Bearer jwt_magic_token.")) return false;
        QString payload = token.section('.', 1);
        QStringList parts = payload.split('.');
        if (parts.size() != 2) return false;
        outUserId = parts[0].toInt();
        outRole = parts[1];
        return true;
    }

    inline bool getAuthInfo(const QHttpServerRequest& request, int& outUserId, QString& outRole) {
        QString token;
        auto val = request.value("Authorization");
        if (!val.isEmpty()) token = QString::fromUtf8(val);
        else {
             val = request.value("authorization");
             if (!val.isEmpty()) token = QString::fromUtf8(val);
        }
        return isValid(token, outUserId, outRole);
    }
}
