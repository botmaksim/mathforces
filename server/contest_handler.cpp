#include "contest_handler.h"
#include "database.h"
#include "auth_middleware.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>
#include <QThreadPool>

ContestHandler::ContestHandler() {}

void ContestHandler::registerRoutes(QHttpServer& server) {
    server.route("/api/login", QHttpServerRequest::Method::Post, [](const QHttpServerRequest& req) {
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString login = in["login"].toString();
        QString password = in["password"].toString();
        qDebug() << "Attempting login for user:" << login << "with password:" << password;
        
        QJsonObject user = Database::authenticate(login, password);
        if (!user.isEmpty()) {
            qDebug() << "Login successful. Role:" << user["role"].toString();
            QJsonObject res; res["token"] = AuthMiddleware::generateJwt(user["id"].toInt(), user["role"].toString());
            res["role"] = user["role"].toString();
            return QHttpServerResponse(res);
        }
        qDebug() << "Login failed. Invalid credentials.";
        return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
    });

    server.route("/api/contests", QHttpServerRequest::Method::Get, [](const QHttpServerRequest&) {
        qDebug() << "API: GET /api/contests requested";
        return QHttpServerResponse(Database::getContests());
    });

    server.route("/api/tasks", QHttpServerRequest::Method::Get, [](const QHttpServerRequest& req) {
        int cid = req.query().queryItemValue("contest_id").toInt();
        qDebug() << "API: GET /api/tasks requested for contest:" << cid;
        return QHttpServerResponse(Database::getTasks(cid));
    });
    
    server.route("/api/results", QHttpServerRequest::Method::Get, [](const QHttpServerRequest& req) {
        int cid = req.query().queryItemValue("contest_id").toInt();
        qDebug() << "API: GET /api/results requested for contest:" << cid;
        return QHttpServerResponse(Database::getResults(cid));
    });

    server.route("/api/submit", QHttpServerRequest::Method::Post, [this](const QHttpServerRequest& req) {
        int userId; QString role;
        qDebug() << "API: POST /api/submit requested";
        if (!AuthMiddleware::getAuthInfo(req, userId, role)) {
            qDebug() << "API: POST /api/submit - Unauthorized access attempt";
            return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
        }
        
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        int taskId = in["task_id"].toInt();
        QString answer = in["answer"].toString();
        qDebug() << "API: POST /api/submit - User:" << userId << "Task:" << taskId;

        QFuture<int> f = QtConcurrent::run([=]() { return Database::savePendingSubmission(taskId, userId, answer); });
        int subId = f.result();
        qDebug() << "API: POST /api/submit - Saved pending submission ID:" << subId;
        auto taskInfo = Database::getTaskDescriptionAndComment(taskId);
        QString desc = taskInfo.first;
        QString aiComment = taskInfo.second;

        // Асинхронно отправляем в ИИ
        qDebug() << "API: POST /api/submit - Sending to LLM for evaluation";
        m_llm.evaluate(subId, desc, aiComment, answer, [](int id, QJsonObject aiRes) {
            qDebug() << "API: LLM evaluation received for submission ID:" << id << "Score:" << aiRes["score"].toInt();
            QThreadPool::globalInstance()->start([=]() { Database::updateSubmissionResult(id, aiRes["score"].toInt(), aiRes["feedback"].toString(), aiRes["thinking"].toString()); });
        });

        QJsonObject res; res["status"] = "accepted"; res["submission_id"] = subId;
        return QHttpServerResponse(res);
    });

    // Админские эндпоинты
    server.route("/api/admin/contest", QHttpServerRequest::Method::Post, [](const QHttpServerRequest& req) {
        int userId; QString role;
        qDebug() << "API: POST /api/admin/contest requested";
        if (!AuthMiddleware::getAuthInfo(req, userId, role) || role != "admin") {
            qDebug() << "API: POST /api/admin/contest - Forbidden access attempt";
            return QHttpServerResponse(QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::createContest(in["title"].toString(), in["description"].toString(), in["start"].toString(), in["end"].toString());
        qDebug() << "API: POST /api/admin/contest - Create result:" << ok;
        return QHttpServerResponse(QJsonObject{{"status", "ok"}});
    });

    server.route("/api/admin/task", QHttpServerRequest::Method::Post, [](const QHttpServerRequest& req) {
        int u; QString r;
        qDebug() << "API: POST /api/admin/task requested";
        if (!AuthMiddleware::getAuthInfo(req, u, r) || r != "admin") {
            qDebug() << "API: POST /api/admin/task - Forbidden access attempt";
            return QHttpServerResponse(QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString aiComment = in.contains("ai_comment") ? in["ai_comment"].toString() : "";
        bool ok = Database::createTask(in["contest_id"].toInt(), in["title"].toString(), in["description"].toString(), in["max_score"].toInt(), aiComment);
        qDebug() << "API: POST /api/admin/task - Create result:" << ok;
        return QHttpServerResponse(QJsonObject{{"status", "ok"}});
    });
}
