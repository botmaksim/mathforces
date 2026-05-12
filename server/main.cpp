#include <QCoreApplication>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include <QFile>
#include <QTextStream>
#include "database.h"
#include "contest_handler.h"

void loadEnv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open" << filePath;
        return; 
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;
        int idx = line.indexOf('=');
        if (idx != -1) {
            QString key = line.left(idx).trimmed();
            QString val = line.mid(idx + 1).trimmed();
            if (val.startsWith("\"") && val.endsWith("\"")) {
                val = val.mid(1, val.length() - 2);
            }
            qputenv(key.toUtf8(), val.toUtf8());
        }
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    loadEnv("config.env");

    QString dbHost = qEnvironmentVariable("DB_HOST", "127.0.0.1");
    QString dbName = qEnvironmentVariable("DB_NAME", "mathforces_db");
    QString dbUser = qEnvironmentVariable("DB_USER", "mathforces");
    QString dbPass = qEnvironmentVariable("DB_PASS", "mathforces_pass");
    int dbPort = qEnvironmentVariable("DB_PORT", "5432").toInt();

    // Подключение к БД
    if (!Database::init(dbName, dbUser, dbPass, dbHost, dbPort)) {
        qCritical() << "Не удалось подключиться к базе данных.";
        return -1;
    }

    // Создаем дефолтных админов (если они не созданы), чтобы можно было залогиниться.
    Database::createInitialUsers();

    QHttpServer server;
    ContestHandler handler;
    handler.registerRoutes(server);

    const int port = qEnvironmentVariable("SERVER_PORT", "8080").toInt();
    QTcpServer* tcpServer = new QTcpServer(&app);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "Не удалось запустить сервер на порту" << port;
        return -1;
    }
    
    server.bind(tcpServer);
    qInfo() << "==========================================";
    qInfo() << "Mathforces Server listening on port" << port;
    qInfo() << "==========================================";

    int ret = app.exec();
    qInfo() << "Mathforces Server shutting down, exit code:" << ret;
    return ret;
}
