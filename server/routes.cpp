#include "routes.hpp"
#include "auth.hpp"
#include <QHttpServerResponse>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QStringList>

namespace mathforces {

static QJsonObject parseJson(const QHttpServerRequest& req) {
    return QJsonDocument::fromJson(req.body()).object();
}

static QHttpServerResponse jsonResponse(const QJsonObject& obj, QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok) {
    auto doc = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return QHttpServerResponse(doc, "application/json", status);
}

static QHttpServerResponse jsonResponse(const QJsonArray& arr, QHttpServerResponder::StatusCode status = QHttpServerResponder::StatusCode::Ok) {
    auto doc = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    return QHttpServerResponse(doc, "application/json", status);
}

void setupRoutes(QHttpServer& server) {

    server.route("/ping", []() {
        return "pong";
    });

    server.route("/api/login/email", [](const QHttpServerRequest& request) {
        if (request.method() != QHttpServerRequest::Method::Post)
            return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::MethodNotAllowed);
            
        auto json = parseJson(request);
        QString email = json["email"].toString();
        QString password = json["password"].toString();

        if (email.isEmpty() || password.isEmpty()) {
            return jsonResponse(QJsonObject{{"error", "Empty fields"}}, QHttpServerResponder::StatusCode::BadRequest);
        }

        QSqlQuery q;
        q.prepare("SELECT id, role, is_banned, name, password_hash FROM users WHERE email=:e");
        q.bindValue(":e", email);
        if (q.exec() && q.next()) {
            QString db_hash = q.value("password_hash").toString();
            if (db_hash != Sha3_256(password) && db_hash != password) { // fallback to plain if old test data
                return jsonResponse(QJsonObject{{"error", "Unauthorized"}}, QHttpServerResponder::StatusCode::Unauthorized);
            }

            bool is_banned = q.value("is_banned").toBool();
            if (is_banned) {
                return jsonResponse(QJsonObject{{"error", "banned"}}, QHttpServerResponder::StatusCode::Forbidden);
            }
            
            int id = q.value("id").toInt();
            QString role = q.value("role").toString();
            QJsonObject res;
            res["token"] = CreateJwt(id, role);
            res["role"] = role;
            return jsonResponse(res);
        }

        return jsonResponse(QJsonObject{{"error", "Unauthorized"}}, QHttpServerResponder::StatusCode::Unauthorized);
    });

    server.route("/api/contests", [](const QHttpServerRequest& request) {
        if (request.method() == QHttpServerRequest::Method::Get) {
            QSqlQuery q("SELECT id, title, description, start_time::text, end_time::text, duration_hours FROM contests WHERE is_published = TRUE ORDER BY start_time DESC");
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
        return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::MethodNotAllowed);
    });

    server.route("/api/ratings", [](const QHttpServerRequest& request) {
        QSqlQuery q("SELECT id, username, name, rating FROM users ORDER BY rating DESC");
        QJsonArray arr;
        int place = 1;
        while (q.next()) {
            QJsonObject obj;
            obj["id"] = q.value("id").toInt();
            obj["place"] = place++;
            obj["username"] = q.value("username").toString();
            obj["name"] = q.value("name").toString();
            obj["rating"] = q.value("rating").toInt();
            arr.append(obj);
        }
        return jsonResponse(arr);
    });
    
    server.route("/api/archive/tasks", [](const QHttpServerRequest& request) {
        QJsonArray arr;
        
        int min_diff = request.query().queryItemValue("min_diff").toInt();
        int max_diff = request.query().hasQueryItem("max_diff") ? request.query().queryItemValue("max_diff").toInt() : 99999;
        QString tags = request.query().queryItemValue("tags");

        QString qStr = "SELECT id, title, tags, difficulty FROM tasks WHERE difficulty >= :min AND difficulty <= :max";
        if (!tags.isEmpty()) qStr += " AND tags ILIKE :tags";
        qStr += " ORDER BY id DESC LIMIT 100";

        QSqlQuery q;
        q.prepare(qStr);
        q.bindValue(":min", min_diff);
        q.bindValue(":max", max_diff);
        if (!tags.isEmpty()) q.bindValue(":tags", "%" + tags + "%");
        
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

    auto dummyOk = [](const QHttpServerRequest&) {
        return jsonResponse(QJsonObject{{"status", "ok"}});
    };
    
    auto dummyEmptyArray = [](const QHttpServerRequest&) {
        return jsonResponse(QJsonArray());
    };

    server.route("/api/register/request_code", dummyOk);
    server.route("/api/register/email", [](const QHttpServerRequest& request) {
        if (request.method() != QHttpServerRequest::Method::Post)
            return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::MethodNotAllowed);
            
        auto json = parseJson(request);
        QString email = json["email"].toString();
        QString username = json["username"].toString();
        QString password = json["password"].toString();
        QString name = json["name"].toString();
        
        QSqlQuery q;
        q.prepare("INSERT INTO users (email, username, password_hash, name) VALUES (:e, :u, :p, :n) RETURNING id");
        q.bindValue(":e", email);
        q.bindValue(":u", username);
        q.bindValue(":p", Sha3_256(password));
        q.bindValue(":n", name);
        if (q.exec() && q.next()) {
            return jsonResponse(QJsonObject{{"status", "ok"}, {"id", q.value("id").toInt()}});
        }
        return jsonResponse(QJsonObject{{"error", "Registration failed"}}, QHttpServerResponder::StatusCode::BadRequest);
    });

    server.route("/api/users", [](const QHttpServerRequest& request) {
        QSqlQuery q("SELECT id, username, name, rating FROM users ORDER BY rating DESC");
        QJsonArray arr;
        while (q.next()) {
            QJsonObject obj;
            obj["id"] = q.value("id").toInt();
            obj["username"] = q.value("username").toString();
            obj["name"] = q.value("name").toString();
            obj["rating"] = q.value("rating").toInt();
            arr.append(obj);
        }
        return jsonResponse(arr);
    });

    server.route("/api/tasks", [](const QHttpServerRequest& request) {
        int contest_id = request.query().queryItemValue("contest_id").toInt();
        QSqlQuery q;
        q.prepare("SELECT id, title, description, max_score, difficulty, tags FROM tasks WHERE contest_id=:c ORDER BY id ASC");
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

    server.route("/api/submit", [](const QHttpServerRequest& request) {
        if (request.method() != QHttpServerRequest::Method::Post)
            return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::MethodNotAllowed);
            
        QString token = QString::fromUtf8(request.value("Authorization"));
        int user_id = 0; QString role;
        if (!VerifyJwt(token, user_id, role)) {
            return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::Unauthorized);
        }

        auto json = parseJson(request);
        int task_id = json["task_id"].toInt();
        QString answer = json["answer_text"].toString();

        QSqlQuery q;
        q.prepare("INSERT INTO submissions (task_id, user_id, answer_text) VALUES (:t, :u, :a) RETURNING id");
        q.bindValue(":t", task_id);
        q.bindValue(":u", user_id);
        q.bindValue(":a", answer);
        
        if (q.exec() && q.next()) {
            return jsonResponse(QJsonObject{{"status", "ok"}, {"submission_id", q.value("id").toInt()}});
        }
        return jsonResponse(QJsonObject{{"error", "Submission failed"}}, QHttpServerResponder::StatusCode::BadRequest);
    });

    server.route("/api/submissions", [](const QHttpServerRequest& request) {
        QString token = QString::fromUtf8(request.value("Authorization"));
        int user_id = 0; QString role;
        if (!VerifyJwt(token, user_id, role)) return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::Unauthorized);
        
        int task_id = request.query().queryItemValue("task_id").toInt();
        QSqlQuery q;
        q.prepare("SELECT id, answer_text, score, status, submitted_at::text FROM submissions WHERE user_id=:u AND task_id=:t ORDER BY submitted_at DESC");
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

    server.route("/api/users/profile", [](const QHttpServerRequest& request) {
        int target_id = request.query().queryItemValue("id").toInt();
        if (target_id == 0) {
            QString token = QString::fromUtf8(request.value("Authorization"));
            QString role;
            if (!VerifyJwt(token, target_id, role)) {
                return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::Unauthorized);
            }
        }
        QSqlQuery q;
        q.prepare("SELECT id, username, email, name, rating, can_blog FROM users WHERE id=:id");
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
        return jsonResponse(QJsonObject(), QHttpServerResponder::StatusCode::NotFound);
    });

    server.route("/api/users/role", dummyOk);
    server.route("/api/users/ban", dummyOk);
    server.route("/api/users/blog_access", dummyOk);
    
    server.route("/api/friends/list", dummyEmptyArray);
    server.route("/api/friends/add", dummyOk);
    server.route("/api/friends/remove", dummyOk);

    server.route("/api/blog/posts", [](const QHttpServerRequest& request) {
        if (request.method() == QHttpServerRequest::Method::Get) return jsonResponse(QJsonArray());
        return jsonResponse(QJsonObject{{"status", "ok"}});
    });
    server.route("/api/blog/comments", [](const QHttpServerRequest& request) {
        if (request.method() == QHttpServerRequest::Method::Get) return jsonResponse(QJsonArray());
        return jsonResponse(QJsonObject{{"status", "ok"}});
    });

    server.route("/api/compile_typst", dummyOk);
    
    server.route("/api/admin/my_contests", dummyEmptyArray);
    server.route("/api/admin/contest", dummyOk);
    server.route("/api/admin/task", dummyOk);
    server.route("/api/admin/rate_contest", dummyOk);
    
    server.route("/api/results", dummyEmptyArray);
    
    server.route("/api/contests/virtual", dummyOk);
    server.route("/api/users/search", dummyEmptyArray);
    server.route("/api/submissions/all", dummyEmptyArray);
    server.route("/api/hacks", dummyOk);
    
    server.route("/api/oauth_callback_client", dummyOk);
}

} // namespace mathforces
