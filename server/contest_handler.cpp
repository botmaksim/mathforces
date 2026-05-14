#include "contest_handler.h"
#include "auth_middleware.h"
#include "database.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QProcess>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QThreadPool>
#include <QUuid>
#include <QtConcurrent>

#include "smtp_client.h"

static QMap<QString, QString> g_emailCodes;
static QMutex g_emailCodesMutex;

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

  server.route("/api/register/request_code", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 QString email = in["email"].toString();
                 if (email.isEmpty())
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::BadRequest);

                 QString code = QString::number(
                     QRandomGenerator::global()->bounded(100000, 999999));

                 {
                   QMutexLocker lock(&g_emailCodesMutex);
                   g_emailCodes[email] = code;
                 }

                 qInfo() << "Attempting to send email via SmtpClient...";
                 QString subject = "Код подтверждения Mathforces";
                 QString body =
                     QString("Ваш код подтверждения для регистрации в "
                             "Mathforces: %1\r\nНикому не сообщайте этот код.")
                         .arg(code);
                 SmtpClient::sendEmail(email, subject, body);

                 return QHttpServerResponse(QJsonObject{{"status", "ok"}});
               });

  server.route(
      "/api/register/email", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        QString email = in["email"].toString();
        QString code = in["code"].toString();

        {
          QMutexLocker lock(&g_emailCodesMutex);
          if (!g_emailCodes.contains(email) || g_emailCodes[email] != code) {
            return QHttpServerResponse(
                QJsonObject{{"error", "Invalid confirmation code"}},
                QHttpServerResponder::StatusCode::BadRequest);
          }
          g_emailCodes.remove(email); // Use code once
        }

        bool ok = Database::registerByEmail(email, in["password"].toString(),
                                            in["username"].toString(),
                                            in["name"].toString());
        if (ok)
          return QHttpServerResponse(QJsonObject{{"status", "ok"}});
        return QHttpServerResponse(
            QJsonObject{
                {"error", "Database error or username/email already exists"}},
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
                        } else if (data.error === "Account Banned") {
                            document.body.innerHTML = '<h2 style="color:red">Ваш аккаунт заблокирован!</h2>';
                        } else {
                            document.body.innerHTML = '<h2>Ошибка авторизации: ' + (data.error || 'Неизвестная ошибка') + '</h2>';
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
               [](const QHttpServerRequest &req) {
                 qDebug() << "API: GET /api/contests requested";
                 int userId = -1;
                 QString role;
                 AuthMiddleware::getAuthInfo(req, userId, role);
                 return QHttpServerResponse(Database::getContests(
                     userId, role == "admin" || role == "superadmin"));
               });

  server.route("/api/contests/register", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 int u;
                 QString r;
                 if (!AuthMiddleware::getAuthInfo(req, u, r))
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 bool ok = Database::registerContest(
                     in["contest_id"].toInt(), u, in["is_official"].toBool());
                 return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                           : QHttpServerResponse(
                                 QHttpServerResponder::StatusCode::BadRequest);
               });

  server.route("/api/contests/virtual", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 int u;
                 QString r;
                 if (!AuthMiddleware::getAuthInfo(req, u, r))
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 bool ok = Database::registerVirtualParticipation(
                     in["contest_id"].toInt(), u);
                 return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                           : QHttpServerResponse(
                                 QHttpServerResponder::StatusCode::BadRequest);
               });

  server.route("/api/admin/rate_contest", QHttpServerRequest::Method::Post,
               [](const QHttpServerRequest &req) {
                 int u;
                 QString r;
                 if (!AuthMiddleware::getAuthInfo(req, u, r) ||
                     (r != "admin" && r != "superadmin"))
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Forbidden);
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 Database::rateContest(in["contest_id"].toInt());
                 return QHttpServerResponse(QJsonObject{{"status", "ok"}});
               });

  server.route("/api/ratings", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 Database::autoRateContests();
                 return QHttpServerResponse(Database::getGlobalRatings());
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

        QJsonObject ctx = Database::getContestContextForTask(taskId);
        if (ctx.isEmpty()) {
          return QHttpServerResponse(
              QJsonObject{{"error", "Task not found"}},
              QHttpServerResponder::StatusCode::BadRequest);
        }

        QDateTime start =
            QDateTime::fromString(ctx["start_time"].toString(), Qt::ISODate);
        QDateTime end =
            QDateTime::fromString(ctx["end_time"].toString(), Qt::ISODate);
        QDateTime now = QDateTime::currentDateTime();

        qInfo() << "Contest start:" << start << "end:" << end << "now:" << now;

        bool isAdmin = (role == "admin" || role == "superadmin");

        QDateTime virtualStart;
        bool isVirtual = Database::isVirtualParticipating(
            ctx["contest_id"].toInt(), userId, virtualStart);
        if (isVirtual) {
          start = virtualStart;
          end = virtualStart.addSecs(ctx["duration_hours"].toDouble() * 3600);
        }

        if (now < start && !isAdmin) {
          return QHttpServerResponse(
              QJsonObject{{"error", "Contest hasn't started"}},
              QHttpServerResponder::StatusCode::BadRequest);
        }

        bool isUpsolving = (now > end);
        if (!isUpsolving && !isAdmin && !isVirtual &&
            !Database::isRegistered(ctx["contest_id"].toInt(), userId)) {
          return QHttpServerResponse(
              QJsonObject{{"error", "Not registered for contest"}},
              QHttpServerResponder::StatusCode::Forbidden);
        }

        QJsonObject taskInfo = Database::getTaskDetails(taskId);
        int maxSubs = taskInfo["max_submissions"].toInt();
        int currentSubs = Database::getUserSubmissionsCount(taskId, userId);
        if (maxSubs > 0 && currentSubs >= maxSubs) {
          QJsonObject res;
          res["error"] = "Max submissions reached";
          return QHttpServerResponse(
              res, QHttpServerResponder::StatusCode::BadRequest);
        }

        QFuture<int> f = QtConcurrent::run([=]() {
          return Database::savePendingSubmission(taskId, userId, answer,
                                                 isUpsolving);
        });
        int subId = f.result();
        qInfo() << "ContestHandler - POST /api/submit - Saved pending "
                   "submission ID:"
                << subId;

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

  // Управление пользователями
  server.route("/api/users", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int userId;
                 QString role;
                 if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
                     (role != "superadmin" && role != "moderator")) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Forbidden);
                 }
                 return QHttpServerResponse(Database::getUsers(role));
               });

  server.route(
      "/api/users/role", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int userId;
        QString role;
        // Только суперадмин может менять роли (напр., назначать
        // админов/модераторов)
        if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
            role != "superadmin") {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::updateUserRole(in["user_id"].toInt(),
                                           in["role"].toString());
        return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                  : QHttpServerResponse(
                        QHttpServerResponder::StatusCode::InternalServerError);
      });

  server.route(
      "/api/users/ban", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int userId;
        QString role;
        // Суперадмин и модератор могут банить
        if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
            (role != "superadmin" && role != "moderator")) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::toggleUserBan(in["user_id"].toInt(),
                                          in["is_banned"].toBool());
        return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                  : QHttpServerResponse(
                        QHttpServerResponder::StatusCode::InternalServerError);
      });

  server.route(
      "/api/users/blog_access", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int userId;
        QString role;
        if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
            (role != "superadmin" && role != "moderator")) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::toggleUserBlog(in["user_id"].toInt(),
                                           in["can_blog"].toBool());
        return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                  : QHttpServerResponse(
                        QHttpServerResponder::StatusCode::InternalServerError);
      });

  server.route("/api/users/profile", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int reqUserId;
                 QString reqRole;
                 if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 }
                 int targetId = req.query().queryItemValue("id").toInt();
                 if (targetId <= 0)
                   targetId = reqUserId;
                 return QHttpServerResponse(
                     Database::getUserProfile(targetId, reqUserId, reqRole));
               });

  server.route("/api/users/search", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int reqUserId;
                 QString reqRole;
                 if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 }
                 QString q = req.query().queryItemValue("q");
                 return QHttpServerResponse(Database::searchUsers(q));
               });

  server.route(
      "/api/friends/add", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int reqUserId;
        QString reqRole;
        if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::addFriend(reqUserId, in["friend_id"].toInt());
        return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                  : QHttpServerResponse(
                        QHttpServerResponder::StatusCode::InternalServerError);
      });

  server.route(
      "/api/friends/remove", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int reqUserId;
        QString reqRole;
        if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        bool ok = Database::removeFriend(reqUserId, in["friend_id"].toInt());
        return ok ? QHttpServerResponse(QJsonObject{{"status", "ok"}})
                  : QHttpServerResponse(
                        QHttpServerResponder::StatusCode::InternalServerError);
      });

  server.route("/api/friends/list", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int reqUserId;
                 QString reqRole;
                 if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 }
                 return QHttpServerResponse(Database::getFriends(reqUserId));
               });

  server.route("/api/blog/posts", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int reqUserId;
                 QString reqRole;
                 if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 }
                 int targetId = req.query().queryItemValue("user_id").toInt();
                 return QHttpServerResponse(Database::getBlogPosts(targetId));
               });

  server.route(
      "/api/blog/posts", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int reqUserId;
        QString reqRole;
        if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        }
        QJsonObject profile =
            Database::getUserProfile(reqUserId, reqUserId, reqRole);
        if (!profile["can_blog"].toBool()) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Forbidden);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        int postId = Database::addBlogPost(reqUserId, in["content"].toString());
        return postId > 0
                   ? QHttpServerResponse(
                         QJsonObject{{"status", "ok"}, {"id", postId}})
                   : QHttpServerResponse(
                         QHttpServerResponder::StatusCode::InternalServerError);
      });

  server.route("/api/blog/comments", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int reqUserId;
                 QString reqRole;
                 if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 }
                 int postId = req.query().queryItemValue("post_id").toInt();
                 return QHttpServerResponse(Database::getBlogComments(postId));
               });

  server.route(
      "/api/blog/comments", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        int reqUserId;
        QString reqRole;
        if (!AuthMiddleware::getAuthInfo(req, reqUserId, reqRole)) {
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        }
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        int commentId = Database::addBlogComment(
            in["post_id"].toInt(), reqUserId, in["content"].toString());
        return commentId > 0
                   ? QHttpServerResponse(
                         QJsonObject{{"status", "ok"}, {"id", commentId}})
                   : QHttpServerResponse(
                         QHttpServerResponder::StatusCode::InternalServerError);
      });

  // Админские эндпоинты
  server.route("/api/admin/my_contests", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int userId;
                 QString role;
                 if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
                     (role != "admin" && role != "superadmin")) {
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Forbidden);
                 }
                 return QHttpServerResponse(Database::getMyContests(userId));
               });

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
        int id = Database::createContestInitial(userId);
        qInfo() << "ContestHandler - POST /api/admin/contest - Create draft "
                   "result ID:"
                << id;
        return QHttpServerResponse(QJsonObject{{"status", "ok"}, {"id", id}});
      });

  server.route("/api/admin/contest", QHttpServerRequest::Method::Put,
               [](const QHttpServerRequest &req) {
                 int userId;
                 QString role;
                 qInfo() << "ContestHandler - PUT /api/admin/contest requested";
                 if (!AuthMiddleware::getAuthInfo(req, userId, role) ||
                     (role != "admin" && role != "superadmin")) {
                   qWarning() << "ContestHandler - PUT /api/admin/contest - "
                                 "Forbidden access attempt";
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Forbidden);
                 }
                 QJsonObject in = QJsonDocument::fromJson(req.body()).object();
                 bool ok = Database::updateContest(
                     in["id"].toInt(), userId, in["title"].toString(),
                     in["description"].toString(), in["start"].toString(),
                     (float)in["duration_hours"].toDouble(),
                     in["is_published"].toBool());
                 return QHttpServerResponse(
                     QJsonObject{{"status", ok ? "ok" : "error"}});
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
            in["ai_comment"].toString(), in["send_editorial_to_ai"].toBool(),
            in["tags"].toString(), in["difficulty"].toInt());
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

  server.route("/api/submissions/all", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 int u;
                 QString r;
                 if (!AuthMiddleware::getAuthInfo(req, u, r))
                   return QHttpServerResponse(
                       QHttpServerResponder::StatusCode::Unauthorized);
                 int taskId = req.query().queryItemValue("task_id").toInt();
                 return QHttpServerResponse(
                     Database::getAllSubmissions(taskId));
               });

  server.route(
      "/api/hacks", QHttpServerRequest::Method::Post,
      [this](const QHttpServerRequest &req) {
        int u;
        QString r;
        if (!AuthMiddleware::getAuthInfo(req, u, r))
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::Unauthorized);
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();
        int subId = in["submission_id"].toInt();
        QString txt = in["hack_text"].toString();
        int hackId = Database::saveHack(u, subId, txt);
        if (hackId != -1) {
          QJsonObject ctx = Database::getHackContext(hackId);
          m_llm.evaluateHack(
              hackId, ctx["editorial"].toString(),
              ctx["answer_text"].toString(), ctx["hack_text"].toString(),
              [](int hId, bool isSuccess, const QString &expl) {
                QThreadPool::globalInstance()->start([hId, isSuccess, expl]() {
                  Database::updateHackStatus(hId, isSuccess, expl);
                });
              });
          return QHttpServerResponse(QJsonObject{{"status", "ok"}});
        }
        return QHttpServerResponse(
            QHttpServerResponder::StatusCode::BadRequest);
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

  server.route("/api/archive/tasks", QHttpServerRequest::Method::Get,
               [](const QHttpServerRequest &req) {
                 QString tags = req.query().queryItemValue("tags");
                 QString minDiffStr = req.query().queryItemValue("min_diff");
                 QString maxDiffStr = req.query().queryItemValue("max_diff");
                 int minDiff = minDiffStr.isEmpty() ? 0 : minDiffStr.toInt();
                 int maxDiff = maxDiffStr.isEmpty() ? 9999 : maxDiffStr.toInt();
                 return QHttpServerResponse(
                     Database::getArchiveTasks(tags, minDiff, maxDiff));
               });

  server.route(
      "/api/compile_typst", QHttpServerRequest::Method::Post,
      [](const QHttpServerRequest &req) {
        qDebug() << "API: POST /api/compile_typst requested";
        QJsonObject in = QJsonDocument::fromJson(req.body()).object();

        QString baseTypst = "#set page(margin: 1.5cm)\n"
                            "#set text(lang: \"ru\")\n"
                            "#set math.equation(numbering: \"(1)\")\n";

        QString typstCode = baseTypst + in["code"].toString();

        QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString tempInPath = QDir::currentPath() + "/mathforces_" + id + ".typ";
        QString outPath = QDir::currentPath() + "/mathforces_" + id + ".pdf";

        QFile tempIn(tempInPath);
        if (!tempIn.open(QIODevice::WriteOnly | QIODevice::Text)) {
          qCritical() << "Failed to open temporary input file for Typst:"
                      << tempInPath;
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::InternalServerError);
        }
        tempIn.write(typstCode.toUtf8());
        tempIn.flush();
        tempIn.close();

        QProcess process;
        process.start("npx", QStringList() << "-y" << "typst" << "compile"
                                           << tempInPath << outPath);
        process.waitForFinished();

        if (process.exitCode() != 0 ||
            process.exitStatus() != QProcess::NormalExit) {
          QString errlog = process.readAllStandardError();
          qWarning() << "Typst compilation failed:" << errlog;
          QFile::remove(tempInPath);
          QFile::remove(outPath);
          return QHttpServerResponse(
              errlog.toUtf8(), QHttpServerResponder::StatusCode::BadRequest);
        }

        QFile outFile(outPath);
        if (!outFile.open(QIODevice::ReadOnly)) {
          qCritical() << "Failed to read typst output file";
          QFile::remove(tempInPath);
          QFile::remove(outPath);
          return QHttpServerResponse(
              QHttpServerResponder::StatusCode::InternalServerError);
        }

        QByteArray pdfData = outFile.readAll();
        outFile.close();
        QFile::remove(tempInPath);
        QFile::remove(outPath);
        return QHttpServerResponse("application/pdf", pdfData);
      });
}
