#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

#include <QObject>
#include <QString>

class SmtpClient : public QObject {
    Q_OBJECT
public:
    explicit SmtpClient(QObject *parent = nullptr);
    static void sendEmail(const QString& toEmail, const QString& subject, const QString& body);
};

#endif // SMTP_CLIENT_H
