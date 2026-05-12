#include "smtp_client.h"
#include <QSslSocket>
#include <QDebug>
#include <QByteArray>
#include <QThread>

class SmtpWorker : public QObject {
    Q_OBJECT
public:
    QString to;
    QString subject;
    QString body;

    SmtpWorker(const QString& to, const QString& subject, const QString& body)
        : to(to), subject(subject), body(body) {}

    void run() {
        QSslSocket socket;
        socket.connectToHostEncrypted("smtp.gmail.com", 465);
        if (!socket.waitForConnected(5000)) {
            qWarning() << "SMTP: Connection timeout:" << socket.errorString();
            deleteLater();
            return;
        }
        if (!socket.waitForEncrypted(5000)) {
            qWarning() << "SMTP: Encryption timeout:" << socket.errorString();
            deleteLater();
            return;
        }

        auto readResponse = [&]() {
            if (!socket.waitForReadyRead(5000)) {
                qWarning() << "SMTP: ReadyRead timeout:" << socket.errorString();
                return false;
            }
            QByteArray response = socket.readAll();
            qDebug() << "SMTP:" << response;
            // Handle fragmentation if needed, but for simple flow this usually works.
            return true;
        };

        auto sendCommand = [&](const QByteArray& cmd) {
            socket.write(cmd + "\r\n");
            socket.waitForBytesWritten(5000);
            return readResponse();
        };

        readResponse(); // Read initial greeting

        if (!sendCommand("EHLO localhost")) return deleteLater();
        if (!sendCommand("AUTH LOGIN")) return deleteLater();

        QByteArray user = "mathforcesmail@gmail.com";
        QByteArray pass = "jpmrlohckglxcbrc";

        if (!sendCommand(user.toBase64())) return deleteLater();
        if (!sendCommand(pass.toBase64())) return deleteLater();

        if (!sendCommand("MAIL FROM:<" + user + ">")) return deleteLater();
        if (!sendCommand("RCPT TO:<" + to.toUtf8() + ">")) return deleteLater();
        if (!sendCommand("DATA")) return deleteLater();

        QByteArray msg;
        msg.append("From: Mathforces <" + user + ">\r\n");
        msg.append("To: " + to.toUtf8() + "\r\n");
        msg.append("Subject: =?utf-8?B?" + subject.toUtf8().toBase64() + "?=\r\n");
        msg.append("MIME-Version: 1.0\r\n");
        msg.append("Content-Type: text/plain; charset=utf-8\r\n\r\n");
        msg.append(body.toUtf8() + "\r\n");
        msg.append("\r\n.\r\n");

        if (!sendCommand(msg)) return deleteLater();
        sendCommand("QUIT");

        socket.disconnectFromHost();
        qInfo() << "SMTP: Email sent to" << to;
        deleteLater();
    }
};

SmtpClient::SmtpClient(QObject *parent) : QObject(parent) {}

void SmtpClient::sendEmail(const QString& toEmail, const QString& subject, const QString& body) {
    SmtpWorker* worker = new SmtpWorker(toEmail, subject, body);
    QThread* thread = new QThread();
    worker->moveToThread(thread);
    connect(thread, &QThread::started, worker, &SmtpWorker::run);
    connect(worker, &QObject::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

#include "smtp_client.moc"
