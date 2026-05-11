#include "database.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>

namespace {
    QString g_host, g_dbName, g_user, g_pass;
    int g_port;
}

bool Database::init(const QString& dbName, const QString& user, const QString& pass, const QString& host, int port) {
    g_dbName = dbName; g_user = user; g_pass = pass; g_host = host; g_port = port;
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "init_check");
    db.setHostName(host); db.setDatabaseName(dbName);
    db.setUserName(user); db.setPassword(pass); db.setPort(port);
    return db.open();
}

QString Database::getThreadLocalConnection() {
    QString connName = QString("db_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    if (!QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connName);
        db.setHostName(g_host); db.setDatabaseName(g_dbName);
        db.setUserName(g_user); db.setPassword(g_pass); db.setPort(g_port);
        if (!db.open()) {
            qDebug() << "Failed to open database connection for thread" << connName << ":" << db.lastError().text();
        }
    }
    return connName;
}

QJsonObject Database::authenticate(const QString& username, const QString& password) {
    qInfo() << "Database::authenticate - Attempting login for username:" << username;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery query(db);
    query.prepare("SELECT id, role, is_banned, name FROM users WHERE username = :u AND password_hash = :p");
    query.bindValue(":u", username); query.bindValue(":p", password);
    QJsonObject res;
    if (query.exec()) {
        if (query.next()) {
            if (query.value(2).toBool()) {
                qWarning() << "Database::authenticate - User is banned:" << username;
                res["error"] = "banned";
            } else {
                qInfo() << "Database::authenticate - User auth successful:" << username;
                res["id"] = query.value(0).toInt();
                res["role"] = query.value(1).toString();
                res["name"] = query.value(3).toString();
            }
        } else {
            qWarning() << "Database::authenticate - No matching user/password found for:" << username;
        }
    } else {
        qCritical() << "Database::authenticate - SQL Error:" << query.lastError().text();
    }
    return res;
}

QJsonObject Database::authenticateByEmail(const QString& email, const QString& password) {
    qInfo() << "Database::authenticateByEmail - Attempting login for email:" << email;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery query(db);
    query.prepare("SELECT id, role, is_banned, name FROM users WHERE email = :e AND password_hash = :p");
    query.bindValue(":e", email); query.bindValue(":p", password);
    QJsonObject res;
    if (query.exec()) {
        if (query.next()) {
             if (query.value(2).toBool()) {
                qWarning() << "Database::authenticateByEmail - User is banned:" << email;
                res["error"] = "banned";
            } else {
                qInfo() << "Database::authenticateByEmail - Success for:" << email;
                res["id"] = query.value(0).toInt();
                res["role"] = query.value(1).toString();
                res["name"] = query.value(3).toString();
            }
        } else {
             qWarning() << "Database::authenticateByEmail - Incorrect credentials for:" << email;
        }
    } else {
         qCritical() << "Database::authenticateByEmail - SQL Error:" << query.lastError().text();
    }
    return res;
}

QJsonObject Database::authenticateOAuth(const QString& email, const QString& googleId, const QString& name) {
    qInfo() << "Database::authenticateOAuth - Google OAuth login attempt for email:" << email;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery query(db);
    query.prepare("SELECT id, role, is_banned, name FROM users WHERE email = :e OR google_id = :g");
    query.bindValue(":e", email); query.bindValue(":g", googleId);
    QJsonObject res;
    if (query.exec() && query.next()) {
         if (query.value(2).toBool()) {
             qWarning() << "Database::authenticateOAuth - User is banned:" << email;
             res["error"] = "banned";
             return res;
         }
         qInfo() << "Database::authenticateOAuth - Existing user authenticated:" << email;
         res["id"] = query.value(0).toInt();
         res["role"] = query.value(1).toString();
         res["name"] = query.value(3).toString();
         
         int userId = query.value(0).toInt();
         QSqlQuery update(db);
         update.prepare("UPDATE users SET google_id = :g WHERE id = :id AND google_id IS NULL");
         update.bindValue(":g", googleId);
         update.bindValue(":id", userId);
         if (!update.exec()) {
             qCritical() << "Database::authenticateOAuth - Failed to update google_id:" << update.lastError().text();
         } else {
             qDebug() << "Database::authenticateOAuth - Successfully linked google_id if needed.";
         }
    } else {
         qInfo() << "Database::authenticateOAuth - Creating new user for email:" << email;
         QSqlQuery insert(db);
         insert.prepare("INSERT INTO users (email, google_id, name, role) VALUES (:e, :g, :n, 'student') RETURNING id, role, name");
         insert.bindValue(":e", email); insert.bindValue(":g", googleId); insert.bindValue(":n", name);
         if (insert.exec() && insert.next()) {
             qInfo() << "Database::authenticateOAuth - New user created via OAuth:" << email;
             res["id"] = insert.value(0).toInt();
             res["role"] = insert.value(1).toString();
             res["name"] = insert.value(2).toString();
         } else {
             qCritical() << "Database::authenticateOAuth - SQL Error on Insert:" << insert.lastError().text();
         }
    }
    return res;
}

bool Database::registerByEmail(const QString& email, const QString& password, const QString& username, const QString& name) {
    qInfo() << "Database::registerByEmail - Registration attempt for email:" << email << "username:" << username;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    q.prepare("INSERT INTO users (email, password_hash, username, name, role) VALUES (:e, :p, :u, :n, 'student')");
    q.bindValue(":e", email); q.bindValue(":p", password); q.bindValue(":u", username); q.bindValue(":n", name);
    bool ok = q.exec();
    if (!ok) {
        qCritical() << "Database::registerByEmail - SQL Error:" << q.lastError().text();
    } else {
        qInfo() << "Database::registerByEmail - Successful registration for email:" << email;
    }
    return ok;
}

QJsonArray Database::getContests() {
    qDebug() << "DB: Fetching contests";
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery query("SELECT id, title, description, start_time, end_time FROM contests", db);
    QJsonArray arr;
    while (query.next()) {
        QJsonObject o; o["id"] = query.value(0).toInt(); o["title"] = query.value(1).toString();
        o["description"] = query.value(2).toString(); o["start_time"] = query.value(3).toString();
        o["end_time"] = query.value(4).toString(); arr.append(o);
    }
    if (query.lastError().isValid()) qCritical() << "Database::getContests - SQL Error:" << query.lastError().text();
    return arr;
}

QJsonArray Database::getTasks(int contestId) {
    qDebug() << "DB: Fetching tasks for contest:" << contestId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); q.prepare("SELECT id, title, description, max_score FROM tasks WHERE contest_id = :id");
    q.bindValue(":id", contestId); q.exec();
    QJsonArray arr;
    while (q.next()) {
        QJsonObject o; o["id"] = q.value(0).toInt(); o["title"] = q.value(1).toString();
        o["description"] = q.value(2).toString(); o["max_score"] = q.value(3).toInt(); arr.append(o);
    }
    if (q.lastError().isValid()) qCritical() << "Database::getTasks - SQL Error:" << q.lastError().text();
    return arr;
}

int Database::savePendingSubmission(int taskId, int userId, const QString& answer) {
    qDebug() << "DB: Saving pending submission for user:" << userId << "task:" << taskId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO submissions (task_id, user_id, answer_text) VALUES (:t, :u, :a) RETURNING id");
    q.bindValue(":t", taskId); q.bindValue(":u", userId); q.bindValue(":a", answer);
    int resId = -1;
    if (q.exec() && q.next()) {
        resId = q.value(0).toInt();
        qInfo() << "Database::savePendingSubmission - Successfully saved with ID:" << resId;
    } else {
        qCritical() << "Database::savePendingSubmission - SQL Error:" << q.lastError().text();
    }
    return resId;
}

void Database::updateSubmissionResult(int submissionId, int score, const QString& feedback, const QString& thinking, float probability) {
    qInfo() << "Database::updateSubmissionResult - Updating ID:" << submissionId << "score:" << score << "prob:" << probability;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("UPDATE submissions SET score=:s, feedback=:f, thinking=:th, ai_probability=:p, status='graded' WHERE id=:id");
    q.bindValue(":s", score); q.bindValue(":f", feedback); q.bindValue(":th", thinking); q.bindValue(":p", probability); q.bindValue(":id", submissionId);
    if (!q.exec()) qCritical() << "Database::updateSubmissionResult - SQL Error:" << q.lastError().text();
}

QJsonObject Database::getTaskDetails(int taskId) {
    qDebug() << "DB: Fetching task details for task:" << taskId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    q.prepare("SELECT description, ai_comment, task_type, correct_answer, editorial, send_editorial_to_ai, max_score FROM tasks WHERE id = :id");
    q.bindValue(":id", taskId);
    QJsonObject res;
    if (q.exec() && q.next()) {
        res["description"] = q.value(0).toString();
        res["ai_comment"] = q.value(1).toString();
        res["task_type"] = q.value(2).toString();
        res["correct_answer"] = q.value(3).toString();
        res["editorial"] = q.value(4).toString();
        res["send_editorial_to_ai"] = q.value(5).toBool();
        res["max_score"] = q.value(6).toInt();
        qInfo() << "Database::getTaskDetails - Success for task:" << taskId << "type:" << res["task_type"].toString();
    }
    else if (q.lastError().isValid()) qCritical() << "Database::getTaskDetails - SQL Error:" << q.lastError().text();
    return res;
}

bool Database::createContest(int authorId, const QString& title, const QString& description, const QString& start, float durationHours) {
    qInfo() << "Database::createContest - Action by user:" << authorId << "title:" << title;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    // Calculate end_time by adding duration_hours to start_time
    q.prepare("INSERT INTO contests (author_id, title, description, start_time, duration_hours, end_time) VALUES (:a, :t, :d, :s, :dur, :s::timestamp + interval '1 hour' * :dur)");
    q.bindValue(":a", authorId); q.bindValue(":t", title); q.bindValue(":d", description); q.bindValue(":s", start); q.bindValue(":dur", durationHours);
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::createContest - SQL Error:" << q.lastError().text();
    else qInfo() << "Database::createContest - Success";
    return ok;
}

bool Database::createTask(int contestId, const QString& type, const QString& title, const QString& description, int maxScore, int maxSubmissions, const QString& correctAnswer, const QString& editorial, const QString& aiComment, bool sendEditorial) {
    qInfo() << "Database::createTask - Creating task:" << title << "for contest:" << contestId << "type:" << type;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    q.prepare("INSERT INTO tasks (contest_id, task_type, title, description, max_score, max_submissions, correct_answer, editorial, ai_comment, send_editorial_to_ai) VALUES (:c, :tt, :t, :d, :m, :ms, :ca, :ed, :ac, :seda)");
    q.bindValue(":c", contestId); q.bindValue(":tt", type); q.bindValue(":t", title); q.bindValue(":d", description); 
    q.bindValue(":m", maxScore); q.bindValue(":ms", maxSubmissions); q.bindValue(":ca", correctAnswer); 
    q.bindValue(":ed", editorial); q.bindValue(":ac", aiComment); q.bindValue(":seda", sendEditorial);
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::createTask - SQL Error:" << q.lastError().text();
    else qInfo() << "Database::createTask - Success";
    return ok;
}

QJsonArray Database::getResults(int contestId) {
    qDebug() << "DB: Fetching results for contest:" << contestId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT u.username, SUM(s.score) FROM submissions s JOIN users u ON s.user_id = u.id "
              "JOIN tasks t ON s.task_id = t.id WHERE t.contest_id = :cid GROUP BY u.username ORDER BY SUM(s.score) DESC");
    q.bindValue(":cid", contestId); q.exec();
    QJsonArray arr;
    int place = 1;
    while (q.next()) {
        QJsonObject o; o["place"] = place++; o["username"] = q.value(0).toString(); o["total_score"] = q.value(1).toInt();
        arr.append(o);
    }
    if (q.lastError().isValid()) qCritical() << "Database::getResults - SQL Error:" << q.lastError().text();
    return arr;
}
