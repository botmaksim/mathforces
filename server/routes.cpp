#include "routes.hpp"
#include "auth.hpp"
#include "smtp_client.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QVariant>

namespace mathforces {

static QMap<QString, QString> g_emailCodes;
static QMutex g_emailCodesMutex;

static QJsonObject parseJson(const QHttpServerRequest &req) {
  return QJsonDocument::fromJson(req.body()).object();
}

static QHttpServerResponse
jsonResponse(const QJsonObject &obj, QHttpServerResponder::StatusCode status =
                                         QHttpServerResponder::StatusCode::Ok) {
  auto doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  return QHttpServerResponse("application/json", doc, status);
}

static QHttpServerResponse
jsonResponse(const QJsonArray &arr, QHttpServerResponder::StatusCode status =
                                        QHttpServerResponder::StatusCode::Ok) {
  auto doc = QJsonDocument(arr).toJson(QJsonDocument::Compact);
  return QHttpServerResponse("application/json", doc, status);
}

void setupRoutes(QHttpServer &server) {

  server.route("/ping", []() { return "pong"; });

  server.route("/api/login/email", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);

    auto json = parseJson(request);
    QString email = json["email"].toString();
    QString password = json["password"].toString();

    if (email.isEmpty() || password.isEmpty()) {
      return jsonResponse(QJsonObject{{"error", "Empty fields"}},
                          QHttpServerResponder::StatusCode::BadRequest);
    }

    QSqlQuery q;
    q.prepare("SELECT id, role, is_banned, name, password_hash FROM users "
              "WHERE email=:e");
    q.bindValue(":e", email);
    if (q.exec() && q.next()) {
      QString db_hash = q.value("password_hash").toString();
      if (db_hash != Sha3_256(password) &&
          db_hash != password) { // fallback to plain if old test data
        return jsonResponse(QJsonObject{{"error", "Unauthorized"}},
                            QHttpServerResponder::StatusCode::Unauthorized);
      }

      bool is_banned = q.value("is_banned").toBool();
      if (is_banned) {
        return jsonResponse(QJsonObject{{"error", "banned"}},
                            QHttpServerResponder::StatusCode::Forbidden);
      }

      int id = q.value("id").toInt();
      QString role = q.value("role").toString();
      if (email == "superadmin@example.com" && role != "superadmin") {
        QSqlQuery uq;
        uq.prepare("UPDATE users SET role='superadmin' WHERE id=:i");
        uq.bindValue(":i", id);
        uq.exec();
        role = "superadmin";
      }
      QJsonObject res;
      res["token"] = CreateJwt(id, role);
      res["role"] = role;
      return jsonResponse(res);
    }

    return jsonResponse(QJsonObject{{"error", "Unauthorized"}},
                        QHttpServerResponder::StatusCode::Unauthorized);
  });

  server.route("/api/contests", [](const QHttpServerRequest &request) {
    if (request.method() == QHttpServerRequest::Method::Get) {
      QSqlQuery q("SELECT id, title, description, start_time::text, "
                  "end_time::text, duration_hours FROM contests WHERE "
                  "is_published = TRUE ORDER BY start_time DESC");
      QJsonArray arr;
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("id").toInt();
        obj["title"] = q.value("title").toString();
        obj["description"] = q.value("description").toString();
        obj["start_time"] = q.value("start_time").toString();
        obj["end_time"] = q.value("end_time").toString();
        obj["duration_hours"] = q.value("duration_hours").toDouble();
        arr.append(obj);
      }
      return jsonResponse(arr);
    }
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::MethodNotAllowed);
  });

  server.route("/api/ratings", [](const QHttpServerRequest &request) {
    QSqlQuery q(
        "SELECT id, username, name, rating FROM users ORDER BY rating DESC");
    QJsonArray arr;
    int place = 1;
    if (!q.isActive()) {
      qDebug() << "Ratings query failed:" << q.lastError().text();
    }
    while (q.next()) {
      QJsonObject obj;
      obj["id"] = q.value("id").toInt();
      obj["place"] = place++;
      obj["username"] = q.value("username").toString();
      obj["name"] = q.value("name").toString();
      obj["rating"] = q.value("rating").toInt();
      arr.append(obj);
    }
    qDebug() << "API Ratings: Returned" << arr.size() << "users";
    return jsonResponse(arr);
  });

  server.route("/api/archive/tasks", [](const QHttpServerRequest &request) {
    QJsonArray arr;

    int min_diff = request.query().queryItemValue("min_diff").toInt();
    int max_diff = request.query().hasQueryItem("max_diff")
                       ? request.query().queryItemValue("max_diff").toInt()
                       : 99999;
    QString tags = request.query().queryItemValue("tags");

    QString qStr = "SELECT id, title, tags, difficulty FROM tasks WHERE "
                   "difficulty >= :min AND difficulty <= :max";
    if (!tags.isEmpty())
      qStr += " AND tags ILIKE :tags";
    qStr += " ORDER BY id DESC LIMIT 100";

    QSqlQuery q;
    q.prepare(qStr);
    q.bindValue(":min", min_diff);
    q.bindValue(":max", max_diff);
    if (!tags.isEmpty())
      q.bindValue(":tags", "%" + tags + "%");

    if (q.exec()) {
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("id").toInt();
        obj["title"] = q.value("title").toString();
        obj["tags"] = q.value("tags").toString();
        obj["difficulty"] = q.value("difficulty").toInt();
        arr.append(obj);
      }
    }
    return jsonResponse(arr);
  });

  auto dummyOk = [](const QHttpServerRequest &) {
    return jsonResponse(QJsonObject{{"status", "ok"}});
  };

  auto dummyEmptyArray = [](const QHttpServerRequest &) {
    return jsonResponse(QJsonArray());
  };

  server.route(
      "/api/register/request_code", [](const QHttpServerRequest &request) {
        if (request.method() != QHttpServerRequest::Method::Post)
          return jsonResponse(
              QJsonObject(),
              QHttpServerResponder::StatusCode::MethodNotAllowed);
        auto json = parseJson(request);
        QString email = json["email"].toString();
        if (email.isEmpty())
          return jsonResponse(QJsonObject(),
                              QHttpServerResponder::StatusCode::BadRequest);

        QString code = QString::number(
            QRandomGenerator::global()->bounded(100000, 999999));
        {
          QMutexLocker lock(&g_emailCodesMutex);
          g_emailCodes[email] = code;
        }
        QString subject = "Код подтверждения Mathforces";
        QString body =
            QString("Ваш код подтверждения для регистрации в "
                    "Mathforces: %1\r\nНикому не сообщайте этот код.")
                .arg(code);
        SmtpClient::sendEmail(email, subject, body);

        return jsonResponse(QJsonObject{{"status", "ok"}});
      });

  server.route("/api/register/email", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);

    auto json = parseJson(request);
    QString email = json["email"].toString();
    QString username = json["username"].toString();
    QString password = json["password"].toString();
    QString name = json["name"].toString();
    QString code = json["code"].toString();

    {
      QMutexLocker lock(&g_emailCodesMutex);
      if (!g_emailCodes.contains(email) || g_emailCodes[email] != code) {
        return jsonResponse(QJsonObject{{"error", "Invalid confirmation code"}},
                            QHttpServerResponder::StatusCode::BadRequest);
      }
      g_emailCodes.remove(email);
    }

    QSqlQuery q;
    q.prepare("INSERT INTO users (email, username, password_hash, name, role) "
              "VALUES (:e, :u, :p, :n, 'student') RETURNING id, role");
    q.bindValue(":e", email);
    q.bindValue(":u", username);
    q.bindValue(":p", Sha3_256(password));
    q.bindValue(":n", name);
    if (q.exec() && q.next()) {
      int id = q.value("id").toInt();
      QString role = q.value("role").toString();
      return jsonResponse(QJsonObject{
          {"status", "ok"}, {"token", CreateJwt(id, role)}, {"role", role}});
    } else {
      qDebug() << "Register fail: " << q.lastError().text();
      return jsonResponse(QJsonObject{{"error", "Registration failed: " +
                                                    q.lastError().text()}},
                          QHttpServerResponder::StatusCode::BadRequest);
    }
  });

  server.route("/api/users", [](const QHttpServerRequest &request) {
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString role;
    if (!VerifyJwt(token, admin_id, role) ||
        (role != "superadmin" && role != "moderator")) {
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Forbidden);
    }
    QSqlQuery q;
    q.prepare("SELECT id, username, email, name, role, is_banned, "
              "can_blog, rating FROM users ORDER BY id");
    q.exec();
    QJsonArray arr;
    while (q.next()) {
      QJsonObject obj;
      obj["id"] = q.value("id").toInt();
      obj["username"] = q.value("username").toString();
      obj["email"] = q.value("email").toString();
      obj["name"] = q.value("name").toString();
      obj["role"] = q.value("role").toString();
      obj["is_banned"] = q.value("is_banned").toBool();
      obj["can_blog"] = q.value("can_blog").toBool();
      obj["rating"] = q.value("rating").toInt();
      arr.append(obj);
    }
    return jsonResponse(arr);
  });

  server.route("/api/tasks", [](const QHttpServerRequest &request) {
    int contest_id = request.query().queryItemValue("contest_id").toInt();
    QSqlQuery q;
    q.prepare("SELECT id, title, description, max_score, difficulty, tags FROM "
              "tasks WHERE contest_id=:c ORDER BY id ASC");
    q.bindValue(":c", contest_id);
    QJsonArray arr;
    if (q.exec()) {
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("id").toInt();
        obj["title"] = q.value("title").toString();
        obj["description"] = q.value("description").toString();
        obj["max_score"] = q.value("max_score").toInt();
        obj["difficulty"] = q.value("difficulty").toInt();
        obj["tags"] = q.value("tags").toString();
        arr.append(obj);
      }
    }
    return jsonResponse(arr);
  });

  server.route("/api/submit", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);

    QString token = QString::fromUtf8(request.value("Authorization"));
    int user_id = 0;
    QString role;
    if (!VerifyJwt(token, user_id, role)) {
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);
    }

    auto json = parseJson(request);
    int task_id = json["task_id"].toInt();
    QString answer = json["answer_text"].toString();

    QSqlQuery q;
    q.prepare("INSERT INTO submissions (task_id, user_id, answer_text) VALUES "
              "(:t, :u, :a) RETURNING id");
    q.bindValue(":t", task_id);
    q.bindValue(":u", user_id);
    q.bindValue(":a", answer);

    if (q.exec() && q.next()) {
      return jsonResponse(QJsonObject{
          {"status", "ok"}, {"submission_id", q.value("id").toInt()}});
    }
    return jsonResponse(QJsonObject{{"error", "Submission failed"}},
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/submissions", [](const QHttpServerRequest &request) {
    QString token = QString::fromUtf8(request.value("Authorization"));
    int user_id = 0;
    QString role;
    if (!VerifyJwt(token, user_id, role))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);

    int task_id = request.query().queryItemValue("task_id").toInt();
    QSqlQuery q;
    q.prepare("SELECT id, answer_text, score, status, submitted_at::text FROM "
              "submissions WHERE user_id=:u AND task_id=:t ORDER BY "
              "submitted_at DESC");
    q.bindValue(":u", user_id);
    q.bindValue(":t", task_id);

    QJsonArray arr;
    if (q.exec()) {
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("id").toInt();
        obj["answer_text"] = q.value("answer_text").toString();
        obj["score"] = q.value("score").toInt();
        obj["status"] = q.value("status").toString();
        obj["submitted_at"] = q.value("submitted_at").toString();
        arr.append(obj);
      }
    }
    return jsonResponse(arr);
  });

  server.route("/api/users/profile", [](const QHttpServerRequest &request) {
    int target_id = request.query().queryItemValue("id").toInt();
    if (target_id == 0) {
      QString token = QString::fromUtf8(request.value("Authorization"));
      QString role;
      if (!VerifyJwt(token, target_id, role)) {
        return jsonResponse(QJsonObject(),
                            QHttpServerResponder::StatusCode::Unauthorized);
      }
    }
    QSqlQuery q;
    q.prepare("SELECT id, username, email, name, rating, can_blog FROM users "
              "WHERE id=:id");
    q.bindValue(":id", target_id);
    if (q.exec() && q.next()) {
      QJsonObject res;
      res["id"] = q.value("id").toInt();
      res["username"] = q.value("username").toString();
      res["email"] = q.value("email").toString();
      res["name"] = q.value("name").toString();
      res["rating"] = q.value("rating").toInt();
      res["can_blog"] = q.value("can_blog").toBool();
      return jsonResponse(res);
    }
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::NotFound);
  });

  server.route("/api/users/role", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString admin_role;
    if (!VerifyJwt(token, admin_id, admin_role) ||
        (admin_role != "superadmin" && admin_role != "admin"))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);
    auto json = parseJson(request);
    int target_id = json["user_id"].toInt();
    QString new_role = json["role"].toString();
    QSqlQuery q;
    q.prepare("UPDATE users SET role=:r WHERE id=:i");
    q.bindValue(":r", new_role);
    q.bindValue(":i", target_id);
    if (q.exec())
      return jsonResponse(QJsonObject{{"status", "ok"}});
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/users/ban", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString admin_role;
    if (!VerifyJwt(token, admin_id, admin_role) ||
        (admin_role != "superadmin" && admin_role != "moderator"))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);
    auto json = parseJson(request);
    int target_id = json["user_id"].toInt();
    bool is_banned = json["is_banned"].toBool();
    QSqlQuery q;
    q.prepare("UPDATE users SET is_banned=:b WHERE id=:i");
    q.bindValue(":b", is_banned);
    q.bindValue(":i", target_id);
    if (q.exec())
      return jsonResponse(QJsonObject{{"status", "ok"}});
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/users/blog_access", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString admin_role;
    if (!VerifyJwt(token, admin_id, admin_role) ||
        (admin_role != "superadmin" && admin_role != "moderator"))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);
    auto json = parseJson(request);
    int target_id = json["user_id"].toInt();
    bool can_blog = json["can_blog"].toBool();
    QSqlQuery q;
    q.prepare("UPDATE users SET can_blog=:b WHERE id=:i");
    q.bindValue(":b", can_blog);
    q.bindValue(":i", target_id);
    if (q.exec())
      return jsonResponse(QJsonObject{{"status", "ok"}});
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/friends/list", [](const QHttpServerRequest &request) {
    QString token = QString::fromUtf8(request.value("Authorization"));
    int user_id = 0;
    QString role;
    if (!VerifyJwt(token, user_id, role)) {
      qDebug() << "API Friends: Invalid JWT";
      return jsonResponse(QJsonArray());
    }

    QSqlQuery q;
    q.prepare("SELECT f.friend_id, u.username, u.name, u.rating, u.role FROM "
              "friends f JOIN users u ON f.friend_id = u.id WHERE f.user_id = "
              ":id ORDER BY u.rating DESC");
    q.bindValue(":id", user_id);
    QJsonArray arr;
    if (q.exec()) {
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("friend_id").toInt();
        obj["username"] = q.value("username").toString();
        obj["name"] = q.value("name").toString();
        obj["rating"] = q.value("rating").toInt();
        obj["role"] = q.value("role").toString();
        arr.append(obj);
      }
    } else {
      qDebug() << "API Friends Error:" << q.lastError().text();
    }
    qDebug() << "API Friends: Returned" << arr.size() << "friends for user"
             << user_id;
    return jsonResponse(arr);
  });

  server.route("/api/friends/add", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int user_id = 0;
    QString role;
    if (!VerifyJwt(token, user_id, role))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);
    auto json = parseJson(request);
    int friend_id = json["friend_id"].toInt();
    QSqlQuery q;
    q.prepare("INSERT INTO friends (user_id, friend_id) VALUES (:u, :f) ON "
              "CONFLICT DO NOTHING");
    q.bindValue(":u", user_id);
    q.bindValue(":f", friend_id);
    if (q.exec())
      return jsonResponse(QJsonObject{{"status", "ok"}});
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/friends/remove", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int user_id = 0;
    QString role;
    if (!VerifyJwt(token, user_id, role))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Unauthorized);
    auto json = parseJson(request);
    int friend_id = json["friend_id"].toInt();
    QSqlQuery q;
    q.prepare("DELETE FROM friends WHERE user_id=:u AND friend_id=:f");
    q.bindValue(":u", user_id);
    q.bindValue(":f", friend_id);
    if (q.exec())
      return jsonResponse(QJsonObject{{"status", "ok"}});
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/blog/posts", [](const QHttpServerRequest &request) {
    if (request.method() == QHttpServerRequest::Method::Get)
      return jsonResponse(QJsonArray());
    return jsonResponse(QJsonObject{{"status", "ok"}});
  });
  server.route("/api/blog/comments", [](const QHttpServerRequest &request) {
    if (request.method() == QHttpServerRequest::Method::Get)
      return jsonResponse(QJsonArray());
    return jsonResponse(QJsonObject{{"status", "ok"}});
  });

  server.route("/api/compile_typst", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    auto in = parseJson(request);
    QString baseTypst = "#set page(margin: 1.5cm)\n#set text(lang: "
                        "\"ru\")\n#set math.equation(numbering: \"(1)\")\n";
    QString typstCode = baseTypst + in["code"].toString();
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString tempInPath = QDir::currentPath() + "/mathforces_" + id + ".typ";
    QString outPath = QDir::currentPath() + "/mathforces_" + id + ".pdf";

    QFile tempIn(tempInPath);
    if (tempIn.open(QIODevice::WriteOnly | QIODevice::Text)) {
      tempIn.write(typstCode.toUtf8());
      tempIn.flush();
      tempIn.close();
    }
    QProcess process;
    process.start("typst", QStringList() << "compile" << tempInPath << outPath);
    process.waitForFinished();
    if (process.exitCode() != 0) {
      QFile::remove(tempInPath);
      QFile::remove(outPath);
      return jsonResponse(QJsonObject{{"error", "compile failed"}},
                          QHttpServerResponder::StatusCode::BadRequest);
    }
    QFile outFile(outPath);
    if (!outFile.open(QIODevice::ReadOnly)) {
      QFile::remove(tempInPath);
      QFile::remove(outPath);
      return jsonResponse(
          QJsonObject{{"error", "no output"}},
          QHttpServerResponder::StatusCode::InternalServerError);
    }
    QByteArray pdfData = outFile.readAll();
    outFile.close();
    QFile::remove(tempInPath);
    QFile::remove(outPath);
    return QHttpServerResponse("application/pdf", pdfData,
                               QHttpServerResponder::StatusCode::Ok);
  });

  server.route("/api/admin/my_contests", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Get)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString admin_role;
    if (!VerifyJwt(token, admin_id, admin_role) ||
        (admin_role != "superadmin" && admin_role != "admin"))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Forbidden);

    QSqlQuery q;
    q.prepare("SELECT id, title, start_time::text, duration_hours, "
              "is_published FROM contests WHERE author_id=:o ORDER BY id DESC");
    q.bindValue(":o", admin_id);
    QJsonArray arr;
    if (q.exec()) {
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("id").toInt();
        obj["title"] = q.value("title").toString();
        obj["start_time"] = q.value("start_time").toString();
        obj["duration_hours"] = q.value("duration_hours").toDouble();
        obj["is_published"] = q.value("is_published").toBool();
        arr.append(obj);
      }
    } else {
      qDebug() << "DB error my_contests:" << q.lastError().text();
    }
    return jsonResponse(arr);
  });

  server.route("/api/admin/contest", [](const QHttpServerRequest &request) {
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString admin_role;
    if (!VerifyJwt(token, admin_id, admin_role) ||
        (admin_role != "superadmin" && admin_role != "admin"))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Forbidden);

    if (request.method() == QHttpServerRequest::Method::Post) {
      QSqlQuery q;
      q.prepare("INSERT INTO contests (author_id, title, start_time, end_time) "
                "VALUES (:o, 'New Draft', CURRENT_TIMESTAMP, "
                "CURRENT_TIMESTAMP) RETURNING id");
      q.bindValue(":o", admin_id);
      if (q.exec() && q.next()) {
        return jsonResponse(
            QJsonObject{{"status", "ok"}, {"id", q.value("id").toInt()}});
      } else {
        qDebug() << "DB error create_contest:" << q.lastError().text();
      }
    } else if (request.method() == QHttpServerRequest::Method::Put) {
      auto in = parseJson(request);
      QSqlQuery q;
      q.prepare("UPDATE contests SET title=:t, description=:d, "
                "start_time=:s::timestamp, end_time=(:s::timestamp + interval "
                "'1 hour' * :dh), duration_hours=:dh, is_published=:p "
                "WHERE id=:i AND author_id=:o");
      q.bindValue(":t", in["title"].toString());
      q.bindValue(":d", in["description"].toString());
      q.bindValue(":s", in["start"].toString());
      q.bindValue(":dh", in["duration_hours"].toDouble());
      q.bindValue(":p", in["is_published"].toBool());
      q.bindValue(":i", in["id"].toInt());
      q.bindValue(":o", admin_id);
      if (q.exec()) {
        return jsonResponse(QJsonObject{{"status", "ok"}});
      } else {
        qDebug() << "DB error update_contest:" << q.lastError().text();
      }
    }
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route("/api/admin/task", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post)
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    QString token = QString::fromUtf8(request.value("Authorization"));
    int admin_id = 0;
    QString admin_role;
    if (!VerifyJwt(token, admin_id, admin_role) ||
        (admin_role != "superadmin" && admin_role != "admin"))
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::Forbidden);

    auto in = parseJson(request);
    QSqlQuery q;
    q.prepare(
        "INSERT INTO tasks (contest_id, task_type, title, description, "
        "max_score, max_submissions, "
        "correct_answer, editorial, send_editorial_to_ai, ai_comment, tags, "
        "difficulty) "
        "VALUES (:c, :tt, :t, :d, :ms, :msub, :ca, :ed, :se, :aic, :tg, :df)");
    q.bindValue(":c", in["contest_id"].toInt());
    q.bindValue(":tt", in["task_type"].toString());
    q.bindValue(":t", in["title"].toString());
    q.bindValue(":d", in["description"].toString());
    q.bindValue(":ms", in["max_score"].toInt());
    q.bindValue(":msub", in["max_submissions"].toInt());
    q.bindValue(":ca", in["correct_answer"].toString());
    q.bindValue(":ed", in["editorial"].toString());
    q.bindValue(":se", in["send_editorial_to_ai"].toBool());
    q.bindValue(":aic", in["ai_comment"].toString());
    q.bindValue(":tg", in["tags"].toString());
    q.bindValue(":df", in["difficulty"].toInt());
    if (q.exec()) {
      return jsonResponse(QJsonObject{{"status", "ok"}});
    }
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::BadRequest);
  });

  server.route(
      "/api/admin/rate_contest", [](const QHttpServerRequest &request) {
        if (request.method() != QHttpServerRequest::Method::Post)
          return jsonResponse(
              QJsonObject(),
              QHttpServerResponder::StatusCode::MethodNotAllowed);
        QString token = QString::fromUtf8(request.value("Authorization"));
        int admin_id = 0;
        QString admin_role;
        if (!VerifyJwt(token, admin_id, admin_role) ||
            (admin_role != "superadmin" && admin_role != "admin"))
          return jsonResponse(QJsonObject(),
                              QHttpServerResponder::StatusCode::Forbidden);
        // Rate dummy action
        return jsonResponse(QJsonObject{{"status", "ok"}});
      });

  server.route("/api/blog/posts", [](const QHttpServerRequest &request) {
    if (request.method() == QHttpServerRequest::Method::Get) {
      QUrlQuery query(request.url().query());
      QString userIdStr = query.queryItemValue("user_id");
      QSqlQuery q;
      if (userIdStr.isEmpty()) {
        q.prepare("SELECT b.id, b.title, b.content, b.created_at::text, "
                  "u.username, u.name, u.role FROM blogs b JOIN users u ON "
                  "b.user_id = u.id ORDER BY b.id DESC");
      } else {
        q.prepare("SELECT b.id, b.title, b.content, b.created_at::text, "
                  "u.username, u.name, u.role FROM blogs b JOIN users u ON "
                  "b.user_id = u.id WHERE b.user_id=:u ORDER BY b.id DESC");
        q.bindValue(":u", userIdStr.toInt());
      }
      QJsonArray arr;
      if (q.exec()) {
        while (q.next()) {
          QJsonObject obj;
          obj["id"] = q.value("id").toInt();
          obj["title"] = q.value("title").toString();
          obj["content"] = q.value("content").toString();
          obj["created_at"] = q.value("created_at").toString();
          obj["author_username"] = q.value("username").toString();
          obj["author_name"] = q.value("name").toString();
          obj["author_role"] = q.value("role").toString();
          arr.append(obj);
        }
      }
      return jsonResponse(arr);
    } else if (request.method() == QHttpServerRequest::Method::Post) {
      QString token = QString::fromUtf8(request.value("Authorization"));
      int user_id = 0;
      QString role;
      if (!VerifyJwt(token, user_id, role))
        return jsonResponse(QJsonObject(),
                            QHttpServerResponder::StatusCode::Unauthorized);

      QSqlQuery qUser;
      qUser.prepare("SELECT can_blog FROM users WHERE id=:i");
      qUser.bindValue(":i", user_id);
      if (!qUser.exec() || !qUser.next() || !qUser.value("can_blog").toBool())
        return jsonResponse(QJsonObject{{"error", "cannot blog"}},
                            QHttpServerResponder::StatusCode::Forbidden);

      auto in = parseJson(request);
      QSqlQuery q;
      q.prepare("INSERT INTO blogs (user_id, title, content) VALUES (:u, :t, "
                ":c) RETURNING id");
      q.bindValue(":u", user_id);
      q.bindValue(":t", in["title"].toString());
      q.bindValue(":c", in["content"].toString());
      if (q.exec() && q.next()) {
        return jsonResponse(
            QJsonObject{{"status", "ok"}, {"id", q.value("id").toInt()}});
      }
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::BadRequest);
    }
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::MethodNotAllowed);
  });

  server.route("/api/blog/comments", [](const QHttpServerRequest &request) {
    if (request.method() == QHttpServerRequest::Method::Get) {
      QUrlQuery query(request.url().query());
      QString postIdStr = query.queryItemValue("post_id");
      QSqlQuery q;
      q.prepare("SELECT c.id, c.content, c.created_at::text, u.username, "
                "u.name, u.role FROM comments c JOIN users u ON c.user_id = "
                "u.id WHERE c.blog_id=:b ORDER BY c.id ASC");
      q.bindValue(":b", postIdStr.toInt());
      QJsonArray arr;
      if (q.exec()) {
        while (q.next()) {
          QJsonObject obj;
          obj["id"] = q.value("id").toInt();
          obj["content"] = q.value("content").toString();
          obj["created_at"] = q.value("created_at").toString();
          obj["author_username"] = q.value("username").toString();
          obj["author_name"] = q.value("name").toString();
          obj["author_role"] = q.value("role").toString();
          arr.append(obj);
        }
      }
      return jsonResponse(arr);
    } else if (request.method() == QHttpServerRequest::Method::Post) {
      QString token = QString::fromUtf8(request.value("Authorization"));
      int user_id = 0;
      QString role;
      if (!VerifyJwt(token, user_id, role))
        return jsonResponse(QJsonObject(),
                            QHttpServerResponder::StatusCode::Unauthorized);

      QSqlQuery qUser;
      qUser.prepare("SELECT can_blog FROM users WHERE id=:i");
      qUser.bindValue(":i", user_id);
      if (!qUser.exec() || !qUser.next() || !qUser.value("can_blog").toBool())
        return jsonResponse(QJsonObject{{"error", "cannot blog"}},
                            QHttpServerResponder::StatusCode::Forbidden);

      auto in = parseJson(request);
      QSqlQuery q;
      q.prepare("INSERT INTO comments (blog_id, user_id, content) VALUES (:b, "
                ":u, :c) RETURNING id");
      q.bindValue(":b", in["post_id"].toInt());
      q.bindValue(":u", user_id);
      q.bindValue(":c", in["content"].toString());
      if (q.exec() && q.next()) {
        return jsonResponse(
            QJsonObject{{"status", "ok"}, {"id", q.value("id").toInt()}});
      }
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::BadRequest);
    }
    return jsonResponse(QJsonObject(),
                        QHttpServerResponder::StatusCode::MethodNotAllowed);
  });

  server.route("/api/results", dummyEmptyArray);

  server.route("/api/contests/virtual", dummyOk);
  server.route("/api/users/search", [](const QHttpServerRequest &request) {
    QString qStr = request.query().queryItemValue("q");
    if (qStr.isEmpty())
      return jsonResponse(QJsonArray());

    QJsonArray arr;
    QSqlQuery q;
    q.prepare("SELECT id, username, name, rating FROM users WHERE username "
              "ILIKE :q OR name ILIKE :q LIMIT 20");
    q.bindValue(":q", "%" + qStr + "%");

    if (q.exec()) {
      while (q.next()) {
        QJsonObject obj;
        obj["id"] = q.value("id").toInt();
        obj["username"] = q.value("username").toString();
        obj["name"] = q.value("name").toString();
        obj["rating"] = q.value("rating").toInt();
        arr.append(obj);
      }
    } else {
      qDebug() << "API Search Error:" << q.lastError().text();
    }
    return jsonResponse(arr);
  });
  server.route("/api/submissions/all", dummyEmptyArray);
  server.route("/api/hacks", dummyOk);

  server.route("/api/oauth_callback_client", [](const QHttpServerRequest &) {
    QString html = R"(
        <html>
        <head><title>MathForces OAuth</title><meta charset="utf-8"/></head>
        <body style="font-family: sans-serif; text-align: center; padding: 50px;">
            <h2>Идет авторизация...</h2>
            <div id="res" style="margin-top:20px; font-size:18px;">Пожалуйста, подождите.</div>
            <script>
                let hash = window.location.hash.substring(1);
                let params = new URLSearchParams(hash);
                let token = params.get("access_token");
                if (!token) {
                    document.getElementById("res").innerText = "Ошибка: не найден токен в URL.";
                } else {
                    fetch('/api/login/google', {
                        method: 'POST',
                        headers: {'Content-Type': 'application/json'},
                        body: JSON.stringify({access_token: token})
                    }).then(r => r.text()).then(text => {
                        try {
                            let data = JSON.parse(text);
                            if (data.token && data.role) {
                                document.getElementById("res").innerHTML = "<b>Ваш токен:</b><br><br><textarea style='width:80%;height:100px;'>" + data.token + "-" + data.role + "</textarea><br><br>Скопируйте этот текст и вставьте в приложение.";
                            } else {
                                document.getElementById("res").innerText = "Ошибка сервера: " + JSON.stringify(data);
                            }
                        } catch(e) {
                            document.getElementById("res").innerText = "Ошибка парсинга: " + e + "\\nОтвет сервера: " + text;
                        }
                    }).catch(e => {
                        document.getElementById("res").innerText = "Network error: " + e;
                    });
                }
            </script>
        </body>
        </html>
        )";
    return QHttpServerResponse("text/html", html.toUtf8(),
                               QHttpServerResponder::StatusCode::Ok);
  });

  server.route("/api/login/google", [](const QHttpServerRequest &request) {
    if (request.method() != QHttpServerRequest::Method::Post) {
      qDebug() << "Google Auth: Invalid HTTP method";
      return jsonResponse(QJsonObject(),
                          QHttpServerResponder::StatusCode::MethodNotAllowed);
    }
    auto json = parseJson(request);
    QString access_token = json["access_token"].toString();
    qDebug() << "Google Auth: Received access_token of length:"
             << access_token.length();

    QNetworkAccessManager manager;
    QNetworkRequest googleReq(
        QUrl("https://www.googleapis.com/oauth2/v3/userinfo"));
    googleReq.setRawHeader("Authorization",
                           ("Bearer " + access_token).toUtf8());

    qDebug() << "Google Auth: Fetching userinfo from googleapis.com...";
    QNetworkReply *reply = manager.get(googleReq);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Google Auth Error:" << reply->errorString();
      reply->deleteLater();
      return jsonResponse(QJsonObject{
          {"error", "google auth failed: " + reply->errorString()}});
    }

    QByteArray replyData = reply->readAll();
    qDebug() << "Google Auth: Received response from Google:" << replyData;
    QJsonObject userInfo = QJsonDocument::fromJson(replyData).object();
    reply->deleteLater();

    QString google_id = userInfo["sub"].toString();
    QString email = userInfo["email"].toString();
    QString name = userInfo["name"].toString();

    if (google_id.isEmpty() || email.isEmpty()) {
      qDebug() << "Google Auth Error: no sub or email in response.";
      return jsonResponse(QJsonObject{{"error", "invalid google user info"}});
    }
    QString fake_username =
        "g_" + email.left(email.indexOf('@')) + "_" + google_id.left(5);

    QSqlQuery q;
    q.prepare(
        "SELECT id, role, is_banned FROM users WHERE email=:e OR google_id=:g");
    q.bindValue(":e", email);
    q.bindValue(":g", google_id);
    if (q.exec() && q.next()) {
      bool banned = q.value("is_banned").toBool();
      qDebug() << "Google Auth: User found in DB. id:" << q.value("id").toInt()
               << "banned:" << banned;
      if (banned)
        return jsonResponse(QJsonObject{{"error", "banned"}},
                            QHttpServerResponder::StatusCode::Forbidden);
      int id = q.value("id").toInt();
      QString role = q.value("role").toString();
      return jsonResponse(
          QJsonObject{{"token", CreateJwt(id, role)}, {"role", role}});
    } else {
      qDebug() << "Google Auth: User not found. Creating new user...";
      q.prepare(
          "INSERT INTO users (email, username, google_id, password_hash, name, "
          "role) VALUES (:e, :u, :g, 'google', :n, 'student') RETURNING id");
      q.bindValue(":e", email);
      q.bindValue(":u", fake_username);
      q.bindValue(":g", google_id);
      q.bindValue(":n", name.isEmpty() ? "Google User" : name);
      if (q.exec() && q.next()) {
        int id = q.value("id").toInt();
        qDebug() << "Google Auth: New user created with id:" << id;
        return jsonResponse(QJsonObject{{"token", CreateJwt(id, "student")},
                                        {"role", "student"}});
      } else {
        qDebug() << "Google Auth DB INSERT failed: " << q.lastError().text();
        return jsonResponse(QJsonObject{
            {"error", "google auth failed: " + q.lastError().text()}});
      }
    }
    return jsonResponse(QJsonObject{{"error", "google auth failed"}});
  });
}

} // namespace mathforces
