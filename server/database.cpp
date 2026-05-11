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
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery query(db);
    query.prepare("SELECT id, role FROM users WHERE username = :u AND password_hash = :p");
    query.bindValue(":u", username); query.bindValue(":p", password);
    QJsonObject res;
    if (query.exec()) {
        if (query.next()) {
            res["id"] = query.value(0).toInt();
            res["role"] = query.value(1).toString();
        } else {
            qDebug() << "SQL: No matching user found for" << username;
        }
    } else {
        qDebug() << "SQL auth error:" << query.lastError().text();
    }
    return res;
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
    if (query.lastError().isValid()) qDebug() << "DB Error in getContests:" << query.lastError().text();
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
    if (q.lastError().isValid()) qDebug() << "DB Error in getTasks:" << q.lastError().text();
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
    } else {
        qDebug() << "DB Error in savePendingSubmission:" << q.lastError().text();
    }
    return resId;
}

void Database::updateSubmissionResult(int submissionId, int score, const QString& feedback, const QString& thinking) {
    qDebug() << "DB: Updating submission result for ID:" << submissionId << "score:" << score;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("UPDATE submissions SET score=:s, feedback=:f, thinking=:th, status='graded' WHERE id=:id");
    q.bindValue(":s", score); q.bindValue(":f", feedback); q.bindValue(":th", thinking); q.bindValue(":id", submissionId);
    if (!q.exec()) qDebug() << "DB Error in updateSubmissionResult:" << q.lastError().text();
}

QPair<QString, QString> Database::getTaskDescriptionAndComment(int taskId) {
    qDebug() << "DB: Fetching task description and AI comment for task:" << taskId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); q.prepare("SELECT description, ai_comment FROM tasks WHERE id = :id");
    q.bindValue(":id", taskId);
    QPair<QString, QString> res("", "");
    if (q.exec() && q.next()) {
        res.first = q.value(0).toString();
        res.second = q.value(1).toString();
    }
    else if (q.lastError().isValid()) qDebug() << "DB Error in getTaskDescriptionAndComment:" << q.lastError().text();
    return res;
}

bool Database::createContest(const QString& title, const QString& description, const QString& start, const QString& end) {
    qDebug() << "DB: Creating contest:" << title;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); q.prepare("INSERT INTO contests (title, description, start_time, end_time) VALUES (:t, :d, :s, :e)");
    q.bindValue(":t", title); q.bindValue(":d", description); q.bindValue(":s", start); q.bindValue(":e", end);
    bool ok = q.exec();
    if (!ok) qDebug() << "DB Error in createContest:" << q.lastError().text();
    return ok;
}

bool Database::createTask(int contestId, const QString& title, const QString& description, int maxScore, const QString& aiComment) {
    qDebug() << "DB: Creating task:" << title << "for contest:" << contestId << "with AI comment:" << aiComment;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); q.prepare("INSERT INTO tasks (contest_id, title, description, max_score, ai_comment) VALUES (:c, :t, :d, :m, :a)");
    q.bindValue(":c", contestId); q.bindValue(":t", title); q.bindValue(":d", description); q.bindValue(":m", maxScore); q.bindValue(":a", aiComment);
    bool ok = q.exec();
    if (!ok) qDebug() << "DB Error in createTask:" << q.lastError().text();
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
    if (q.lastError().isValid()) qDebug() << "DB Error in getResults:" << q.lastError().text();
    return arr;
}
