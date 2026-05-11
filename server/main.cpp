#include <QCoreApplication>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include "database.h"
#include "contest_handler.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Подключение к БД
    if (!Database::init("mathforces_db", "mathforces", "mathforces_pass")) {
        qCritical() << "Не удалось подключиться к базе данных.";
        return -1;
    }

    QHttpServer server;
    ContestHandler handler;
    handler.registerRoutes(server);

    const int port = 8080;
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
