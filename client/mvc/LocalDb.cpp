#include "LocalDb.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QVariantMap>

void LocalDb::init() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "local_cache");
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    db.setDatabaseName(path + "/cache.sqlite");
    if (!db.open()) {
        qWarning() << "Failed to open local cache:" << db.lastError().text();
        return;
    }
    QSqlQuery q(db);
    q.exec("CREATE TABLE IF NOT EXISTS cached_contests (id INTEGER PRIMARY KEY, title TEXT, start_time TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS cached_ratings (id INTEGER PRIMARY KEY, place INTEGER, username TEXT, name TEXT, rating INTEGER)");
    q.exec("CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT)");
    q.exec("CREATE TABLE IF NOT EXISTS task_drafts (task_id INTEGER PRIMARY KEY, answer TEXT)");
}

void LocalDb::cacheContests(const QVariantList& contests) {
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q(db);
    q.exec("DELETE FROM cached_contests"); // Отчистить старый кэш
    q.prepare("INSERT INTO cached_contests (id, title, start_time) VALUES (:i, :t, :s)");
    
    for(const QVariant& v : contests) {
        QVariantMap map = v.toMap();
        q.bindValue(":i", map["id"].toInt());
        q.bindValue(":t", map["title"].toString());
        q.bindValue(":s", map["start_time"].toString());
        q.exec();
    }
}

QVariantList LocalDb::getCachedContests() {
    QVariantList list;
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q("SELECT id, title, start_time FROM cached_contests", db);
    while (q.next()) {
        QVariantMap map;
        map["id"] = q.value(0).toInt();
        map["title"] = q.value(1).toString();
        map["start_time"] = q.value(2).toString();
        list.append(map);
    }
    return list;
}

void LocalDb::cacheRatings(const QVariantList& ratings) {
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q(db);
    q.exec("DELETE FROM cached_ratings");
    q.prepare("INSERT INTO cached_ratings (id, place, username, name, rating) VALUES (:id, :place, :un, :n, :r)");
    
    for(const QVariant& v : ratings) {
        QVariantMap map = v.toMap();
        q.bindValue(":id", map["id"].toInt());
        q.bindValue(":place", map["place"].toInt());
        q.bindValue(":un", map["username"].toString());
        q.bindValue(":n", map["name"].toString());
        q.bindValue(":r", map["rating"].toInt());
        q.exec();
    }
}

QVariantList LocalDb::getCachedRatings() {
    QVariantList list;
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q("SELECT id, place, username, name, rating FROM cached_ratings ORDER BY place ASC", db);
    while (q.next()) {
        QVariantMap map;
        map["id"] = q.value(0).toInt();
        map["place"] = q.value(1).toInt();
        map["username"] = q.value(2).toString();
        map["name"] = q.value(3).toString();
        map["rating"] = q.value(4).toInt();
        list.append(map);
    }
    return list;
}

void LocalDb::setSetting(const QString& key, const QString& value) {
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :val)");
    q.bindValue(":key", key);
    q.bindValue(":val", value);
    q.exec();
}

QString LocalDb::getSetting(const QString& key, const QString& defaultValue) {
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q(db);
    q.prepare("SELECT value FROM settings WHERE key=:key");
    q.bindValue(":key", key);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return defaultValue;
}

void LocalDb::saveTaskDraft(int taskId, const QString& answer) {
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO task_drafts (task_id, answer) VALUES (:id, :ans)");
    q.bindValue(":id", taskId);
    q.bindValue(":ans", answer);
    q.exec();
}

QString LocalDb::getTaskDraft(int taskId) {
    QSqlDatabase db = QSqlDatabase::database("local_cache");
    QSqlQuery q(db);
    q.prepare("SELECT answer FROM task_drafts WHERE task_id=:id");
    q.bindValue(":id", taskId);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return "";
}
