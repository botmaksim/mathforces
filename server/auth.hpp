#pragma once
#include <QString>
#include <QMessageAuthenticationCode>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QStringList>

inline QString Sha3_256(const QString& data) {
    QByteArray hash = QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha3_256);
    return QString(hash.toHex());
}

inline QString CreateJwt(int user_id, const QString& role) {
    QJsonObject header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    QJsonObject payload;
    payload["userId"] = user_id;
    payload["role"] = role;
    payload["exp"] = QDateTime::currentSecsSinceEpoch() + 86400 * 7;

    QString header_b64 = QJsonDocument(header).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QString payload_b64 = QJsonDocument(payload).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QString unsigned_token = header_b64 + "." + payload_b64;

    QString secret = qEnvironmentVariable("JWT_SECRET", "super_secret_jwt_key_that_needs_to_be_secure");
    QByteArray sig = QMessageAuthenticationCode::hash(unsigned_token.toUtf8(), secret.toUtf8(), QCryptographicHash::Sha256);
    return unsigned_token + "." + sig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

inline bool VerifyJwt(const QString& token, int& out_user, QString& out_role) {
    QStringList parts = token.split('.');
    if (parts.size() != 3) return false;

    QString unsigned_part = parts[0] + "." + parts[1];
    QString signature_part = parts[2];

    QString secret = qEnvironmentVariable("JWT_SECRET", "super_secret_jwt_key_that_needs_to_be_secure");
    QByteArray expected_sig = QMessageAuthenticationCode::hash(unsigned_part.toUtf8(), secret.toUtf8(), QCryptographicHash::Sha256);
    QString expected_sig_b64 = expected_sig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    if (signature_part != expected_sig_b64) return false;

    QByteArray payload_json = QByteArray::fromBase64(parts[1].toUtf8(), QByteArray::Base64UrlEncoding);
    QJsonObject val = QJsonDocument::fromJson(payload_json).object();
    out_user = val["userId"].toInt();
    out_role = val["role"].toString();
    return true;
}

