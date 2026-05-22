#include <QCoreApplication>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QString>
#include "routes.hpp"

void loadEnv() {
    QFile file("../config.env");
    if (!file.exists()) file.setFileName("config.env");
    if (!file.exists()) file.setFileName("../../config.env");
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith("#")) continue;
            int idx = line.indexOf('=');
            if (idx != -1) {
                QString key = line.left(idx).trimmed();
                QString val = line.mid(idx + 1).trimmed();
                qputenv(key.toUtf8(), val.toUtf8());
            }
        }
    }
}

bool initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(qEnvironmentVariable("DB_HOST", "127.0.0.1"));
    db.setPort(qEnvironmentVariable("DB_PORT", "5432").toInt());
    db.setDatabaseName(qEnvironmentVariable("DB_NAME", "mathforces_db"));
    db.setUserName(qEnvironmentVariable("DB_USER", "mathforces"));
    db.setPassword(qEnvironmentVariable("DB_PASSWORD", "mathforces_pass"));
    
    if (!db.open()) {
        qWarning() << "Failed to connect to database:" << db.lastError().text();
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    
    loadEnv();
    
    if (!initDatabase()) {
        return 1;
    }

    QHttpServer server;
    mathforces::setupRoutes(server);
    
    int port = qEnvironmentVariable("SERVER_PORT", "3000").toInt();
    
    QTcpServer* tcpServer = new QTcpServer(&app);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to bind to port" << port;
        return 1;
    }
    
    server.bind(tcpServer);
    qDebug() << "Mathforces Qt6 Server listening on port" << port;

    return app.exec();
}
