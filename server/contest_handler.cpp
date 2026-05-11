#include "contest_handler.h"
#include "auth_middleware.h"
#include "database.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QtConcurrent>

ContestHandler::ContestHandler() {}

void ContestHandler::registerRoutes(QHttpServer &server) {
  server.route(
      "/api/login", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString login = in["login"].toString();
        QString password = in["password"].toString();
        qInfo() << "ContestHandler - Attempting login for user:" << login;

        QJsonObject user = Database::authenticate(login, password);
        if (!user.isEmpty() && !user.contains("error")) {
          qInfo() << "ContestHandler - Login successful. Role:"
                  << user["role"].toString();
          QJsonObject res;
          res["token"] = AuthMiddleware::generateJwt(user["id"].toInt(),
                                                     user["role"].toString());
          res["role"] = user["role"].toString();
          return QHttpServerResponse(res);
        }
        if (user.contains("error") && user["error"] == "banned") {
          qWarning() << "ContestHandler - Login failed: Account Banned:"
                     << login;
          return QHttpServerResponse(
              QJsonObject{{"error", "Account Banned"}},
              QHttpServerResponder::StatusCode::Forbidden);
        }
        qWarning() << "ContestHandler - Login failed. Invalid credentials:"
                   << login;
        return QHttpServerResponse(
            QHttpServerResponder::StatusCode::Unauthorized);
      });

  server.route(
      "/api/login/email", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString email = in["email"].toString();
        QString password = in["password"].toString();
        qInfo() << "ContestHandler - Attempting email login for:" << email;

        QJsonObject user = Database::authenticateByEmail(email, password);
        if (!user.isEmpty() && !user.contains("error")) {
          QJsonObject res;
          res["token"] = AuthMiddleware::generateJwt(user["id"].toInt(),
                                                     user["role"].toString());
          res["role"] = user["role"].toString();
          return QHttpServerResponse(res);
        }
        if (user.contains("error") && user["error"] == "banned") {
          return QHttpServerResponse(
              QJsonObject{{"error", "Account Banned"}},
              QHttpServerResponder::StatusCode::Forbidden);
        }
        return QHttpServerResponse(
            QHttpServerResponder::StatusCode::Unauthorized);
      });

  server.route("/api/register/email", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 bool ok = Database::registerByEmail(
                     in["email"].toString(), in["password"].toString(),
                     in["username"].toString(), in["name"].toString());
                 if (ok)
                   return QHttpServerResponse(QJsonObject{{"status", "ok"}});
                 return QHttpServerResponse(
                     QHttpServerResponder::StatusCode::BadRequest);
               });

  server.route("/api/oauth_login", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 // Here client sends the access token they got from explicit or
                 // implicit flow
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 QString email = in["email"].toString();
                 QString googleId = in["google_id"].toString();
                 QString name = in["name"].toString();

                 QJsonObject user =
                     Database::authenticateOAuth(email, googleId, name);
                 if (!user.isEmpty() && !user.contains("error")) {
                   QJsonObject res;
                   res["token"] = AuthMiddleware::generateJwt(
                       user["id"].toInt(), user["role"].toString());
                   res["role"] = user["role"].toString();
                   return QHttpServerResponse(res);
                 }
                 if (user.contains("error") && user["error"] == "banned") {
                   return QHttpServerResponse(
                       QJsonObject{{"error", "Account Banned"}},
                       QHttpServerResponder::StatusCode::Forbidden);
                 }
                 return QHttpServerResponse(
                     QHttpServerResponder::StatusCode::Unauthorized);
               });

  server.route("/api/oauth_callback_client", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &) {
                 QString html = R"HTML(
            <!DOCTYPE html>
            <html>
            <head><title>Google Login Success</title></head>
            <body>
            <script>
                // Extract access token from URL fragment
                var hash = window.location.hash.substring(1);
                var params = new URLSearchParams(hash);
                var token = params.get('access_token');
                
                if (token) {
                    document.body.innerHTML = '<h2>Получаем данные из Google...</h2>';
                    
                    fetch('https://www.googleapis.com/oauth2/v3/userinfo', {
                        headers: { 'Authorization': 'Bearer ' + token }
                    })
                    .then(res => res.json())
                    .then(data => {
                        // POST to backend oauth_login
                        return fetch('/api/oauth_login', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({
                                email: data.email,
                                google_id: data.sub,
                                name: data.name
                            })
                        });
                    })
                    .then(res => res.json())
                    .then(data => {
                        if (data.token) {
                            document.body.innerHTML = '<h2>Успех! Теперь введите этот токен в приложении:</h2><textarea cols="50" rows="5">' + data.token + '-' + data.role + '</textarea><br><p>Вы можете скопировать токен и закрыть это окно.</p>';
                        } else {
                            document.body.innerHTML = '<h2>Ошибка авторизации</h2>';
                        }
                    }).catch(e => {
                        document.body.innerHTML = '<h2>Ошибка: ' + e + '</h2>';
                    });
                } else {
                    document.body.innerHTML = '<h2>Ошибка: Токен не найден</h2>';
                }
            </script>
            </body>
            </html>
        )HTML";
                 return QHttpServerResponse("text/html", html.toUtf8());
               });

  server.route("/api/contests", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &) {
                 qDebug() << "API: GET /api/contests requested";
                 return QHttpServerResponse(Database::getContests());
               });

  server.route("/api/tasks", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int cid = req.query().queryItemValue("contest_id").toInt();
                 qDebug() << "API: GET /api/tasks requested for contest:"
                          << cid;
                 return QHttpServerResponse(Database::getTasks(cid));
               });

  server.route("/api/results", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int cid = req.query().queryItemValue("contest_id").toInt();
                 qDebug() << "API: GET /api/results requested for contest:"
                          << cid;
                 return QHttpServerResponse(Database::getResults(cid));
               });

  server.route(
      "/api/submit", QHttpServerRequest::Method::Post,
      [this](const QHttpServerRequest &req) {
        int userId;
        QString role;
        qInfo() << "ContestHandler - POST /api/submit requested";
        if (!AuthMiddleware::getAuthInfo(req, userId, role)) {
          qWarning() << "ContestHandler - POST /api/submit - Unauthorized "
                        "access attempt";
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        }

        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        int taskId = in["task_id"].toInt();
        QString answer = in["answer"].toString();
        qInfo() << "ContestHandler - POST /api/submit - User:" << userId
                << "Task:" << taskId;

        QFuture<int> f = QtConcurrent::run([=]() {
          return Database::savePendingSubmission(taskId, userId, answer);
        });
        int subId = f.result();
        qInfo() << "ContestHandler - POST /api/submit - Saved pending "
                   "submission ID:"
                << subId;

        QJsonObject taskInfo = Database::getTaskDetails(taskId);
        QString type = taskInfo["task_type"].toString();

        if (type == "answer_only") {
          QString correctAnswer =
              taskInfo["correct_answer"].toString().trimmed();
          QString userAnswer = answer.trimmed();
          int score =
              (correctAnswer.compare(userAnswer, Qt::CaseInsensitive) == 0)
                  ? taskInfo["max_score"].toInt()
                  : 0;
          Database::updateSubmissionResult(
              subId, score, "Автоматическая проверка ответа", "");
        } else {
          QString desc = taskInfo["description"].toString();
          QString aiComment = taskInfo["ai_comment"].toString();
          bool sendEditorial = taskInfo["send_editorial_to_ai"].toBool();
          if (sendEditorial) {
            desc +=
                "\n\nАВТОРСКОЕ РЕШЕНИЕ:\n" + taskInfo["editorial"].toString();
          }

          // Асинхронно отправляем в ИИ
          qInfo() << "ContestHandler - POST /api/submit - Sending to LLM for "
                     "evaluation";
          m_llm.evaluate(
              subId, desc, aiComment, answer, [](int id, QJsonObject aiRes) {
                qInfo() << "ContestHandler - LLM evaluation received for "
                           "submission ID:"
                        << id << "Score:" << aiRes["score"].toInt();
                float prob = aiRes.contains("probability")
                                 ? aiRes["probability"].toDouble()
                                 : 0.0;
                QThreadPool::globalInstance()->start([=]() {
                  Database::updateSubmissionResult(
                      id, aiRes["score"].toInt(), aiRes["feedback"].toString(),
                      aiRes["thinking"].toString(), prob);
                });
              });
        }

        QJsonObject res;
        res["status"] = "accepted";
        res["submission_id"] = subId;
        return QHttpServerResponse(res);
      });

  // Админские эндпоинты
  server.route(
      "/api/admin/contest", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int userId;
        QString role;
        qInfo() << "ContestHandler - POST /api/admin/contest requested";
        if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
            (role != "admin" && role != "superadmin")) {
          qWarning() << "ContestHandler - POST /api/admin/contest - Forbidden "
                        "access attempt";
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::createContest(
            userId, in["title"].toString(), in["description"].toString(),
            in["start"].toString(), (float)in["duration_hours"].toDouble());
        qInfo() << "ContestHandler - POST /api/admin/contest - Create result:"
                << ok;
        return QHttpServerResponse(QJsonObject{{"status", "ok"}});
      });

  server.route(
      "/api/admin/task", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int u;
        QString r;
        qInfo() << "ContestHandler - POST /api/admin/task requested";
        if (!AuthMiddleware::getAuthInfo(req, u, r) ||
            (r != "admin" && r != "superadmin")) {
          qWarning() << "ContestHandler - POST /api/admin/task - Forbidden "
                        "access attempt";
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::createTask(
            in["contest_id"].toInt(), in["task_type"].toString(),
            in["title"].toString(), in["description"].toString(),
            in["max_score"].toInt(), in["max_submissions"].toInt(),
            in["correct_answer"].toString(), in["editorial"].toString(),
            in["ai_comment"].toString(), in["send_editorial_to_ai"].toBool());
        qInfo() << "ContestHandler - POST /api/admin/task - Create result:"
                << ok;
        return QHttpServerResponse(QJsonObject{{"status", "ok"}});
      });

  server.route(
      "/api/submissions", QHttpServerRequest::Method::Get,
      [](const QHttpServerRequest &req) {
        int u;
        QString r;
        if (!AuthMiddleware::getAuthInfo(req, u, r)) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        }
        int taskId = req.query().queryItemValue("task_id").toInt();

        QSqlDatabase db =
            QSqlDatabase::database(Database::getThreadLocalConnection());
        QSqlQuery q(db);
        if (r == "superadmin" || r == "admin" || r == "moderator") {
          // Admins see all submissions and thinking
          q.prepare("SELECT s.id, s.score, s.feedback, s.status, "
                    "s.answer_text, u.username, s.thinking, s.ai_probability "
                    "FROM submissions s JOIN users u ON s.user_id = u.id WHERE "
                    "s.task_id = :t ORDER BY s.submitted_at DESC");
          q.bindValue(":t", taskId);
        } else {
          // Students see only their own and no thinking
          q.prepare("SELECT s.id, s.score, s.feedback, s.status, "
                    "s.answer_text, u.username FROM submissions s JOIN users u "
                    "ON s.user_id = u.id WHERE s.task_id = :t AND s.user_id = "
                    ":u ORDER BY s.submitted_at DESC");
          q.bindValue(":t", taskId);
          q.bindValue(":u", u);
        }

        QJsonArray arr;
        if (q.exec()) {
          while (q.next()) {
            QJsonObject o;
            o["id"] = q.value(0).toInt();
            o["score"] = q.value(1).toInt();
            o["feedback"] = q.value(2).toString();
            o["status"] = q.value(3).toString();
            o["answer"] = q.value(4).toString();
            o["username"] = q.value(5).toString();
            if (r == "superadmin" || r == "admin" || r == "moderator") {
              o["thinking"] = q.value(6).toString();
              o["ai_probability"] = q.value(7).toDouble();
            }
            arr.append(o);
          }
        } else {
          qDebug() << "DB ERROR /api/submissions:" << q.lastError().text();
        }
        return QHttpServerResponse(arr);
      });

  server.route(
      "/api/admin/promote", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int u;
        QString r;
        if (!AuthMiddleware::getAuthInfo(req, u, r) || r != "superadmin") {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString targetUser = in["username"].toString();
        QString newRole = in["role"].toString(); // "admin" or "moderator"

        QSqlDatabase db =
            QSqlDatabase::database(Database::getThreadLocalConnection());
        QSqlQuery q(db);
        q.prepare(
            "UPDATE users SET role = :r WHERE username = :u OR email = :u");
        q.bindValue(":r", newRole);
        q.bindValue(":u", targetUser);
        if (q.exec() && q.numRowsAffected() > 0) {
          return QHttpServerResponse(QJsonObject{{"status", "ok"}});
        }
        return QHttpServerResponse(
            QHttpServerResponder::StatusCode::BadRequest);
      });

  server.route("/api/admin/ban", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 int u;
                 QString r;
                 if (!AuthMiddleware::getAuthInfo(req, u, r) ||
                     (r != "superadmin" && r != "moderator")) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Forbidden);
                 }
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 QString targetUser = in["username"].toString();
                 bool banStatus =
                     in["ban"].toBool(true); // true to ban, false to unban

                 QSqlDatabase db = QSqlDatabase::database(
                     Database::getThreadLocalConnection());
                 QSqlQuery q(db);
                 q.prepare("UPDATE users SET is_banned = :b WHERE username = "
                           ":u OR email = :u");
                 q.bindValue(":b", banStatus);
                 q.bindValue(":u", targetUser);
                 if (q.exec() && q.numRowsAffected() > 0) {
                   return QHttpServerResponse(QJsonObject{{"status", "ok"}});
                 }
                 return QHttpServerResponse(
                     QHttpServerResponder::StatusCode::BadRequest);
               });

  server.route("/api/user/name", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 int u;
                 QString r;
                 if (!AuthMiddleware::getAuthInfo(req, u, r)) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 }
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 QString newName = in["name"].toString();

                 QSqlDatabase db = QSqlDatabase::database(
                     Database::getThreadLocalConnection());
                 QSqlQuery q(db);
                 q.prepare("UPDATE users SET name = :n WHERE id = :id");
                 q.bindValue(":n", newName);
                 q.bindValue(":id", u);
                 if (q.exec()) {
                   return QHttpServerResponse(QJsonObject{{"status", "ok"}});
                 }
                 return QHttpServerResponse(
                     QHttpServerResponder::StatusCode::BadRequest);
               });

  server.route(
      "/api/user", QHttpServerRequest::Method::Get,
      [](const QHttpServerRequest &req) {
        QString username = req.query().queryItemValue("username");
        QSqlDatabase db =
            QSqlDatabase::database(Database::getThreadLocalConnection());
        QSqlQuery q(db);
        q.prepare("SELECT id, username, name, rating, reputation, role FROM "
                  "users WHERE username = :u OR email = :u");
        q.bindValue(":u", username);
        if (q.exec() && q.next()) {
          QJsonObject o;
          o["id"] = q.value(0).toInt();
          o["username"] = q.value(1).toString();
          o["name"] = q.value(2).toString();
          o["rating"] = q.value(3).toInt();
          o["reputation"] = q.value(4).toInt();
          o["role"] = q.value(5).toString();
          return QHttpServerResponse(o);
        }
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
      });

  server.route(
      "/api/blogs", QHttpServerRequest::Method::Get,
      [](const QHttpServerRequest &req) {
        QSqlDatabase db =
            QSqlDatabase::database(Database::getThreadLocalConnection());
        QSqlQuery q("SELECT b.id, b.title, b.content, b.created_at, u.username "
                    "FROM blogs b JOIN users u ON b.user_id = u.id ORDER BY "
                    "b.created_at DESC LIMIT 50",
                    db);
        QJsonArray arr;
        while (q.next()) {
          QJsonObject o;
          o["id"] = q.value(0).toInt();
          o["title"] = q.value(1).toString();
          o["content"] = q.value(2).toString();
          o["created_at"] = q.value(3).toString();
          o["author"] = q.value(4).toString();
          arr.append(o);
        }
        return QHttpServerResponse(arr);
      });

  server.route(
      "/api/compile_typst", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        qDebug() << "API: POST /api/compile_typst requested";
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString typstCode = in["code"].toString();

        QTemporaryFile tempIn;
        if (!tempIn.open()) {
          qCritical() << "Failed to open temporary input file for Typst";
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::InternalServerError);
        }
        tempIn.write(typstCode.toUtf8());
        tempIn.flush();

        QTemporaryFile tempOut;
        if (!tempOut.open()) {
          qCritical() << "Failed to open temporary output file for Typst";
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::InternalServerError);
        }
        QString outPath = tempOut.fileName() + ".pdf";
        tempOut.close();

        QProcess process;
        process.start("typst", QStringList() << "compile" << tempIn.fileName()
                                             << outPath);
        process.waitForFinished();

        if (process.exitCode() != 0) {
          qWarning() << "Typst compilation failed:"
                     << process.readAllStandardError();
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::BadRequest);
        }

        QFile outFile(outPath);
        if (!outFile.open(QIODevice::ReadOnly)) {
          qCritical() << "Failed to read typst output file";
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::InternalServerError);
        }

        QByteArray pdfData = outFile.readAll();
        outFile.close();
        QFile::remove(outPath);
        return QHttpServerResponse("application/pdf", pdfData);
      });
}
