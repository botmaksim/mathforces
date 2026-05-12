#include "database.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QCryptographicHash>

namespace {
    QString g_host, g_dbName, g_user, g_pass;
    int g_port;
}

bool Database::init(const QString& dbName, const QString& user, const QString& pass, const QString& host, int port) {
    g_dbName = dbName; g_user = user; g_pass = pass; g_host = host; g_port = port;
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "init_check");
    db.setHostName(host); db.setDatabaseName(dbName);
    db.setUserName(user); db.setPassword(pass); db.setPort(port);
    if (db.open()) {
        QSqlQuery q(db);
        q.exec("ALTER TABLE contests ADD COLUMN IF NOT EXISTS is_published BOOLEAN DEFAULT FALSE;");
        q.exec("ALTER TABLE contests ADD COLUMN IF NOT EXISTS is_rated BOOLEAN DEFAULT FALSE;");
        q.exec("ALTER TABLE users ADD COLUMN IF NOT EXISTS is_banned BOOLEAN DEFAULT FALSE;");
        q.exec("ALTER TABLE users ADD COLUMN IF NOT EXISTS can_blog BOOLEAN DEFAULT TRUE;");
        q.exec("ALTER TABLE tasks ADD COLUMN IF NOT EXISTS tags TEXT DEFAULT '';");
        q.exec("ALTER TABLE tasks ADD COLUMN IF NOT EXISTS difficulty INTEGER DEFAULT 1000;");
        q.exec("CREATE TABLE IF NOT EXISTS virtual_participations (id SERIAL PRIMARY KEY, user_id INTEGER REFERENCES users(id) ON DELETE CASCADE, contest_id INTEGER REFERENCES contests(id) ON DELETE CASCADE, start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, UNIQUE(contest_id, user_id));");
        q.exec("CREATE TABLE IF NOT EXISTS hacks (id SERIAL PRIMARY KEY, hacker_id INTEGER REFERENCES users(id) ON DELETE CASCADE, submission_id INTEGER REFERENCES submissions(id) ON DELETE CASCADE, hack_text TEXT NOT NULL, status VARCHAR(20) DEFAULT 'pending', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
        q.exec("CREATE TABLE IF NOT EXISTS friends (user_id INTEGER REFERENCES users(id), friend_id INTEGER REFERENCES users(id), UNIQUE(user_id, friend_id));");
        q.exec("CREATE TABLE IF NOT EXISTS blog_posts (id SERIAL PRIMARY KEY, user_id INTEGER REFERENCES users(id), content TEXT, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
        q.exec("CREATE TABLE IF NOT EXISTS blog_comments (id SERIAL PRIMARY KEY, post_id INTEGER REFERENCES blog_posts(id) ON DELETE CASCADE, user_id INTEGER REFERENCES users(id), content TEXT, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
        return true;
    }
    return false;
}

void Database::createInitialUsers() {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    
    auto createIfMissing = [&](const QString& username, const QString& email, const QString& row_pass, const QString& role, const QString& name) {
        QSqlQuery check(db);
        check.prepare("SELECT id FROM users WHERE username = :u");
        check.bindValue(":u", username);
        if (check.exec() && check.next()) {
            // Already exists
            return;
        }
        
        QSqlQuery insert(db);
        insert.prepare("INSERT INTO users (username, email, password_hash, role, name) VALUES (:u, :e, :p, :r, :n)");
        QString hashedPwd = QString(QCryptographicHash::hash(row_pass.toUtf8(), QCryptographicHash::Sha256).toHex());
        insert.bindValue(":u", username);
        insert.bindValue(":e", email);
        insert.bindValue(":p", hashedPwd);
        insert.bindValue(":r", role);
        insert.bindValue(":n", name);
        if (!insert.exec()) {
            qCritical() << "Failed to create initial user" << username << ":" << insert.lastError().text();
        } else {
            qInfo() << "Successfully created initial user" << username;
        }
    };
    
    // В случае если в БД есть "сломанные" тестовые пользователи из init_db.sql (у которых хэш '12345'),
    // мы их удалим, чтобы создать новые с правильным хэшем (суперадмин, админ, студент).
    q.exec("DELETE FROM users WHERE password_hash = '12345'");
    
    createIfMissing("superadmin", "superadmin@example.com", "12345", "superadmin", "Super Admin");
    createIfMissing("admin", "admin@example.com", "12345", "admin", "Admin");
    createIfMissing("student", "student@example.com", "12345", "student", "Student");
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
    QString hashedPwd = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    query.bindValue(":u", username); query.bindValue(":p", hashedPwd);
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
    QString hashedPwd = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    query.bindValue(":e", email); query.bindValue(":p", hashedPwd);
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
    QString hashedPwd = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
    q.bindValue(":e", email); q.bindValue(":p", hashedPwd); q.bindValue(":u", username); q.bindValue(":n", name);
    bool ok = q.exec();
    if (!ok) {
        qCritical() << "Database::registerByEmail - SQL Error:" << q.lastError().text();
    } else {
        qInfo() << "Database::registerByEmail - Successful registration for email:" << email;
    }
    return ok;
}

bool Database::registerContest(int contestId, int userId, bool isOfficial) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO contest_participants (contest_id, user_id, is_official) VALUES (:cid, :uid, :iso) "
              "ON CONFLICT(contest_id, user_id) DO UPDATE SET is_official = :iso");
    q.bindValue(":cid", contestId); q.bindValue(":uid", userId); q.bindValue(":iso", isOfficial);
    return q.exec();
}

bool Database::registerVirtualParticipation(int contestId, int userId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO virtual_participations (contest_id, user_id) VALUES (:c, :u) ON CONFLICT DO NOTHING");
    q.bindValue(":c", contestId); q.bindValue(":u", userId);
    return q.exec();
}

bool Database::isVirtualParticipating(int contestId, int userId, QDateTime& virtualStart) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT start_time FROM virtual_participations WHERE contest_id = :c AND user_id = :u");
    q.bindValue(":c", contestId); q.bindValue(":u", userId);
    if (q.exec() && q.next()) {
        virtualStart = q.value(0).toDateTime();
        return true;
    }
    return false;
}

QJsonObject Database::getContestContextForTask(int taskId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT c.id, c.start_time, c.end_time FROM tasks t JOIN contests c ON t.contest_id = c.id WHERE t.id = :t");
    q.bindValue(":t", taskId);
    QJsonObject res;
    if (q.exec() && q.next()) {
        res["contest_id"] = q.value(0).toInt();
        res["start_time"] = q.value(1).toDateTime().toString(Qt::ISODate);
        res["end_time"] = q.value(2).toDateTime().toString(Qt::ISODate);
    }
    return res;
}

bool Database::isRegistered(int contestId, int userId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT 1 FROM contest_participants WHERE contest_id = :cid AND user_id = :uid");
    q.bindValue(":cid", contestId); q.bindValue(":uid", userId);
    return (q.exec() && q.next());
}

QJsonArray Database::getContests(int userId, bool showAll) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QString sql = "SELECT c.id, c.title, c.description, c.start_time, c.end_time, c.duration_hours, cp.is_official "
                  "FROM contests c LEFT JOIN contest_participants cp ON c.id = cp.contest_id AND cp.user_id = :uid";
    if (!showAll) sql += " WHERE c.is_published = TRUE";
    sql += " ORDER BY c.start_time DESC";
    QSqlQuery query(db);
    query.prepare(sql);
    query.bindValue(":uid", userId);
    query.exec();
    QJsonArray arr;
    while (query.next()) {
        QJsonObject o; o["id"] = query.value(0).toInt(); o["title"] = query.value(1).toString();
        o["description"] = query.value(2).toString(); o["start_time"] = query.value(3).toDateTime().toString(Qt::ISODate);
        o["end_time"] = query.value(4).toDateTime().toString(Qt::ISODate);
        o["duration_hours"] = query.value(5).toDouble();
        if (query.value(6).isNull()) {
            o["registered"] = false;
        } else {
            o["registered"] = true;
            o["is_official"] = query.value(6).toBool();
        }
        arr.append(o);
    }
    if (query.lastError().isValid()) qCritical() << "Database::getContests - SQL Error:" << query.lastError().text();
    return arr;
}

QJsonArray Database::getMyContests(int authorId) {
    QJsonArray arr;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT id, title, description, start_time, duration_hours, is_published FROM contests WHERE author_id = :aid");
    q.bindValue(":aid", authorId);
    q.exec();
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["title"] = q.value(1).toString();
        o["description"] = q.value(2).toString();
        o["start_time"] = q.value(3).toString();
        o["duration_hours"] = q.value(4).toDouble();
        o["is_published"] = q.value(5).toBool();
        arr.append(o);
    }
    return arr;
}

QJsonArray Database::getArchiveTasks(const QString& tags, int minDiff, int maxDiff) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    QString queryStr = "SELECT id, title, tags, difficulty FROM tasks WHERE difficulty >= :minD AND difficulty <= :maxD";
    
    // In a real app we'd split tags and use ILIKE or a proper array. For simplicity, we just use a basic string match if tags are provided.
    if (!tags.isEmpty()) {
        queryStr += " AND tags ILIKE :tags";
    }
    
    q.prepare(queryStr);
    q.bindValue(":minD", minDiff);
    q.bindValue(":maxD", maxDiff);
    if (!tags.isEmpty()) {
        q.bindValue(":tags", "%" + tags + "%");
    }
    
    q.exec();
    QJsonArray arr;
    while(q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["title"] = q.value(1).toString();
        o["tags"] = q.value(2).toString();
        o["difficulty"] = q.value(3).toInt();
        arr.append(o);
    }
    return arr;
}

QJsonArray Database::getTasks(int contestId) {
    qDebug() << "DB: Fetching tasks for contest:" << contestId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    q.prepare("SELECT t.id, t.title, t.description, t.max_score, t.editorial, c.end_time < CURRENT_TIMESTAMP FROM tasks t JOIN contests c ON t.contest_id = c.id WHERE t.contest_id = :id");
    q.bindValue(":id", contestId); q.exec();
    QJsonArray arr;
    while (q.next()) {
        QJsonObject o; 
        o["id"] = q.value(0).toInt(); 
        o["title"] = q.value(1).toString();
        o["description"] = q.value(2).toString(); 
        o["max_score"] = q.value(3).toInt(); 
        if (q.value(5).toBool()) { // if end_time < CURRENT_TIMESTAMP
            o["editorial"] = q.value(4).toString();
        }
        arr.append(o);
    }
    if (q.lastError().isValid()) qCritical() << "Database::getTasks - SQL Error:" << q.lastError().text();
    return arr;
}

int Database::savePendingSubmission(int taskId, int userId, const QString& answer, bool isUpsolving) {
    qDebug() << "DB: Saving pending submission for user:" << userId << "task:" << taskId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO submissions (task_id, user_id, answer_text, is_upsolving) VALUES (:t, :u, :a, :up) RETURNING id");
    q.bindValue(":t", taskId); q.bindValue(":u", userId); q.bindValue(":a", answer); q.bindValue(":up", isUpsolving);
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
    db.transaction();
    QSqlQuery q(db);
    q.prepare("UPDATE submissions SET score=:s, feedback=:f, thinking=:th, ai_probability=:p, status='graded' WHERE id=:id");
    q.bindValue(":s", score); q.bindValue(":f", feedback); q.bindValue(":th", thinking); q.bindValue(":p", probability); q.bindValue(":id", submissionId);
    if (!q.exec()) qCritical() << "Database::updateSubmissionResult - SQL Error:" << q.lastError().text();
    
    QSqlQuery q2(db);
    q2.prepare("SELECT user_id FROM submissions WHERE id=:id");
    q2.bindValue(":id", submissionId);
    if(q2.exec() && q2.next()) {
        int uid = q2.value(0).toInt();
        QSqlQuery q3(db);
        q3.prepare("UPDATE users SET hidden_probability = (hidden_probability * 9.0 + :p) / 10.0 WHERE id = :uid");
        q3.bindValue(":p", probability);
        q3.bindValue(":uid", uid);
        q3.exec();
    }
    db.commit();
}

QJsonObject Database::getTaskDetails(int taskId) {
    qDebug() << "DB: Fetching task details for task:" << taskId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    q.prepare("SELECT description, ai_comment, task_type, correct_answer, editorial, send_editorial_to_ai, max_score, max_submissions FROM tasks WHERE id = :id");
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
        res["max_submissions"] = q.value(7).toInt();
        qInfo() << "Database::getTaskDetails - Success for task:" << taskId << "type:" << res["task_type"].toString();
    }
    else if (q.lastError().isValid()) qCritical() << "Database::getTaskDetails - SQL Error:" << q.lastError().text();
    return res;
}

QJsonArray Database::getAllSubmissions(int taskId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT s.id, u.username, s.score, s.answer_text FROM submissions s JOIN users u ON s.user_id = u.id WHERE s.task_id = :t ORDER BY s.submitted_at DESC");
    q.bindValue(":t", taskId);
    q.exec();
    QJsonArray arr;
    while(q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["username"] = q.value(1).toString();
        o["score"] = q.value(2).toInt();
        o["answer_text"] = q.value(3).toString();
        arr.append(o);
    }
    return arr;
}

int Database::saveHack(int hackerId, int submissionId, const QString& hackText) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO hacks (hacker_id, submission_id, hack_text) VALUES (:u, :s, :t) RETURNING id");
    q.bindValue(":u", hackerId);
    q.bindValue(":s", submissionId);
    q.bindValue(":t", hackText);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return -1;
}

void Database::updateHackStatus(int hackId, bool isSuccessful, const QString& explanation) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("UPDATE hacks SET status = :st WHERE id = :h");
    q.bindValue(":st", isSuccessful ? "successful" : "unsuccessful");
    q.bindValue(":h", hackId);
    q.exec();
    
    // Give hacker +100 rating points or reputation if successful
    if (isSuccessful) {
        QSqlQuery q2(db);
        q2.prepare("UPDATE users SET rating = rating + 100 WHERE id = (SELECT hacker_id FROM hacks WHERE id = :h)");
        q2.bindValue(":h", hackId);
        q2.exec();
    }
}

QJsonObject Database::getHackContext(int hackId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT t.editorial, s.answer_text, h.hack_text FROM hacks h JOIN submissions s ON h.submission_id = s.id JOIN tasks t ON s.task_id = t.id WHERE h.id = :h");
    q.bindValue(":h", hackId);
    QJsonObject res;
    if (q.exec() && q.next()) {
        res["editorial"] = q.value(0).toString();
        res["answer_text"] = q.value(1).toString();
        res["hack_text"] = q.value(2).toString();
    }
    return res;
}

int Database::getUserSubmissionsCount(int taskId, int userId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM submissions WHERE task_id = :t AND user_id = :u");
    q.bindValue(":t", taskId);
    q.bindValue(":u", userId);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool Database::createContest(int authorId, const QString& title, const QString& description, const QString& start, float durationHours, bool isPublished) {
    qInfo() << "Database::createContest - Action by user:" << authorId << "title:" << title;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    
    q.prepare("INSERT INTO contests (author_id, title, description, start_time, duration_hours, end_time, is_published) VALUES (:a, :t, :d, :s, :dur, :s::timestamp + interval '1 hour' * CAST(:dur AS float), :p)");
    q.bindValue(":a", authorId); q.bindValue(":t", title); q.bindValue(":d", description); 
    q.bindValue(":s", start); q.bindValue(":dur", durationHours); q.bindValue(":p", isPublished);
    
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::createContest - SQL Error:" << q.lastError().text();
    else qInfo() << "Database::createContest - Success";
    return ok;
}

bool Database::createTask(int contestId, const QString& type, const QString& title, const QString& description, int maxScore, int maxSubmissions, const QString& correctAnswer, const QString& editorial, const QString& aiComment, bool sendEditorial, const QString& tags, int difficulty) {
    qInfo() << "Database::createTask - Creating task:" << title << "for contest:" << contestId << "type:" << type;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db); 
    q.prepare("INSERT INTO tasks (contest_id, task_type, title, description, max_score, max_submissions, correct_answer, editorial, ai_comment, send_editorial_to_ai, tags, difficulty) VALUES (:c, :tt, :t, :d, :m, :ms, :ca, :ed, :ac, :seda, :tag, :diff)");
    q.bindValue(":c", contestId); q.bindValue(":tt", type); q.bindValue(":t", title); q.bindValue(":d", description); 
    q.bindValue(":m", maxScore); q.bindValue(":ms", maxSubmissions); q.bindValue(":ca", correctAnswer); 
    q.bindValue(":ed", editorial); q.bindValue(":ac", aiComment); q.bindValue(":seda", sendEditorial);
    q.bindValue(":tag", tags); q.bindValue(":diff", difficulty);
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::createTask - SQL Error:" << q.lastError().text();
    else qInfo() << "Database::createTask - Success";
    return ok;
}

QJsonArray Database::getResults(int contestId) {
    qDebug() << "DB: Fetching results for contest:" << contestId;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare(R"(
        WITH all_participants AS (
            SELECT user_id, TRUE as is_official, NULL::timestamp as v_start FROM contest_participants WHERE contest_id = :cid
            UNION
            SELECT user_id, FALSE as is_official, start_time as v_start FROM virtual_participations WHERE contest_id = :cid
        ),
        max_scores AS (
            SELECT s.user_id, s.task_id, MAX(s.score) as best_score
            FROM submissions s
            JOIN tasks t ON s.task_id = t.id
            WHERE t.contest_id = :cid AND s.is_upsolving = FALSE
            GROUP BY s.user_id, s.task_id
        ),
        task_results AS (
            SELECT 
                ms.user_id, 
                ms.best_score,
                MIN(s.submitted_at) as first_best_time
            FROM max_scores ms
            JOIN submissions s ON s.user_id = ms.user_id AND s.task_id = ms.task_id AND s.score = ms.best_score
            GROUP BY ms.user_id, ms.task_id, ms.best_score
        ),
        user_scores AS (
            SELECT 
                tr.user_id, 
                SUM(tr.best_score) as total_score,
                SUM(EXTRACT(EPOCH FROM (tr.first_best_time - COALESCE(ap.v_start, c.start_time)))/60.0) as penalty
            FROM task_results tr
            JOIN all_participants ap ON ap.user_id = tr.user_id
            JOIN contests c ON c.id = :cid
            GROUP BY tr.user_id, ap.v_start, c.start_time
        )
        SELECT u.id, u.username, COALESCE(us.total_score, 0), COALESCE(us.penalty, 0), ap.is_official
        FROM all_participants ap
        JOIN users u ON ap.user_id = u.id
        LEFT JOIN user_scores us ON ap.user_id = us.user_id
        ORDER BY COALESCE(us.total_score, 0) DESC, COALESCE(us.penalty, 0) ASC
    )");
    q.bindValue(":cid", contestId); q.exec();
    QJsonArray arr;
    int place = 1;
    while (q.next()) {
        QJsonObject o;
        o["place"] = place++;
        o["user_id"] = q.value(0).toInt();
        o["username"] = q.value(1).toString();
        o["total_score"] = q.value(2).toInt();
        o["penalty"] = q.value(3).toInt();
        o["is_official"] = q.value(4).toBool();
        arr.append(o);
    }
    if (q.lastError().isValid()) qCritical() << "Database::getResults - SQL Error:" << q.lastError().text();
    return arr;
}

void Database::rateContest(int contestId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery check(db);
    check.prepare("SELECT is_rated FROM contests WHERE id = :cid");
    check.bindValue(":cid", contestId);
    if (!check.exec() || !check.next() || check.value(0).toBool() == true) {
        qWarning() << "Database::rateContest - Contest already rated or not found:" << contestId;
        return; // Already rated or doesn't exist
    }

    QSqlQuery q(db);
    q.prepare(R"(
        WITH max_scores AS (
            SELECT s.user_id, s.task_id, MAX(s.score) as best_score
            FROM submissions s
            JOIN tasks t ON s.task_id = t.id
            WHERE t.contest_id = :cid AND s.is_upsolving = FALSE
            GROUP BY s.user_id, s.task_id
        ),
        task_results AS (
            SELECT 
                ms.user_id, 
                ms.best_score,
                MIN(s.submitted_at) as first_best_time
            FROM max_scores ms
            JOIN submissions s ON s.user_id = ms.user_id AND s.task_id = ms.task_id AND s.score = ms.best_score
            GROUP BY ms.user_id, ms.task_id, ms.best_score
        ),
        user_scores AS (
            SELECT 
                tr.user_id, 
                SUM(tr.best_score) as total_score,
                SUM(EXTRACT(EPOCH FROM (tr.first_best_time - c.start_time))/60.0) as penalty
            FROM task_results tr
            JOIN contests c ON c.id = :cid
            GROUP BY tr.user_id, c.start_time
        )
        SELECT cp.user_id, u.rating, COALESCE(us.total_score, 0), COALESCE(us.penalty, 0)
        FROM contest_participants cp
        JOIN users u ON cp.user_id = u.id
        LEFT JOIN user_scores us ON cp.user_id = us.user_id
        WHERE cp.contest_id = :cid AND cp.is_official = TRUE
        ORDER BY COALESCE(us.total_score, 0) DESC, COALESCE(us.penalty, 0) ASC
    )");
    q.bindValue(":cid", contestId);
    if (!q.exec()) return;
    
    struct Part { int uid; double rating; int score; double penalty; double rank; double newRating; };
    QList<Part> participants;
    while(q.next()){
        Part p; p.uid = q.value(0).toInt(); p.rating = q.value(1).toDouble();
        p.score = q.value(2).toInt(); p.penalty = q.value(3).toDouble();
        participants.append(p);
    }
    int n = participants.size();
    if (n < 2) return;
    
    for (int i=0; i<n; ++i) participants[i].rank = i + 1;
    for (int i=0; i<n; ) {
        int j = i;
        while (j < n && participants[j].score == participants[i].score && qAbs(participants[j].penalty - participants[i].penalty) < 1e-4) j++;
        double avgRank = (i + 1 + j) / 2.0;
        for (int k=i; k<j; ++k) participants[k].rank = avgRank;
        i = j;
    }
    
    for (int i=0; i<n; ++i) {
        double expectedWins = 0;
        double actualWins = 0;
        for (int j=0; j<n; ++j) {
            if (i == j) continue;
            double p_win = 1.0 / (1.0 + pow(10.0, (participants[j].rating - participants[i].rating) / 400.0));
            expectedWins += p_win;
            if (participants[i].rank < participants[j].rank) actualWins += 1.0;
            else if (participants[i].rank == participants[j].rank) actualWins += 0.5;
        }
        double delta = 400.0 * (actualWins - expectedWins) / (n - 1);
        if (delta > 200) delta = 200;
        if (delta < -200) delta = -200;
        participants[i].newRating = participants[i].rating + delta;
    }
    
    db.transaction();
    QSqlQuery up(db);
    for (const auto& p : participants) {
        up.prepare("UPDATE users SET rating = :r WHERE id = :id");
        up.bindValue(":r", qRound(p.newRating));
        up.bindValue(":id", p.uid);
        up.exec();
    }
    QSqlQuery ratedCheck(db);
    ratedCheck.prepare("UPDATE contests SET is_rated = TRUE WHERE id = :cid");
    ratedCheck.bindValue(":cid", contestId);
    ratedCheck.exec();
    db.commit();
}

QJsonArray Database::getGlobalRatings() {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q("SELECT id, username, name, rating FROM users WHERE is_banned = FALSE ORDER BY rating DESC", db);
    QJsonArray arr;
    int place = 1;
    while(q.next()){
        QJsonObject o; o["place"] = place++; o["id"] = q.value(0).toInt(); 
        o["username"] = q.value(1).toString(); o["name"] = q.value(2).toString(); 
        o["rating"] = q.value(3).toInt(); arr.append(o);
    }
    return arr;
}

QJsonArray Database::getUsers(const QString& requestorRole) {
    QJsonArray arr;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT id, username, email, name, role, is_banned, hidden_probability, can_blog FROM users ORDER BY id");
    q.exec();
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["username"] = q.value(1).toString();
        o["email"] = q.value(2).toString();
        o["name"] = q.value(3).toString();
        o["role"] = q.value(4).toString();
        o["is_banned"] = q.value(5).toBool();
        o["can_blog"] = q.value(7).toBool();
        if (requestorRole == "superadmin" || requestorRole == "moderator") {
             o["hidden_probability"] = q.value(6).toDouble();
        }
        arr.append(o);
    }
    return arr;
}

bool Database::updateUserRole(int userId, const QString& newRole) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("UPDATE users SET role = :role WHERE id = :id");
    q.bindValue(":role", newRole);
    q.bindValue(":id", userId);
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::updateUserRole Error:" << q.lastError().text();
    return ok;
}

bool Database::toggleUserBan(int userId, bool isBanned) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("UPDATE users SET is_banned = :isBanned WHERE id = :id");
    q.bindValue(":isBanned", isBanned);
    q.bindValue(":id", userId);
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::toggleUserBan Error:" << q.lastError().text();
    return ok;
}

bool Database::toggleUserBlog(int userId, bool canBlog) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("UPDATE users SET can_blog = :cb WHERE id = :id");
    q.bindValue(":cb", canBlog);
    q.bindValue(":id", userId);
    bool ok = q.exec();
    if (!ok) qCritical() << "Database::toggleUserBlog Error:" << q.lastError().text();
    return ok;
}

QJsonObject Database::getUserProfile(int userId, int requestorId, const QString& requestorRole) {
    QJsonObject o;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT username, name, email, rating, can_blog FROM users WHERE id = :id");
    q.bindValue(":id", userId);
    if (q.exec() && q.next()) {
        o["username"] = q.value(0).toString();
        o["name"] = q.value(1).toString();
        if (userId == requestorId || requestorRole == "admin" || requestorRole == "superadmin" || requestorRole == "moderator") {
            o["email"] = q.value(2).toString();
        }
        o["rating"] = q.value(3).toInt();
        o["can_blog"] = q.value(4).toBool();
        o["id"] = userId;
    }
    return o;
}

QJsonArray Database::searchUsers(const QString& queryText) {
    QJsonArray arr;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    QString pattern = "%" + queryText + "%";
    q.prepare("SELECT id, username, name, rating FROM users WHERE username ILIKE :q OR name ILIKE :q ORDER BY rating DESC");
    q.bindValue(":q", pattern);
    q.exec();
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["username"] = q.value(1).toString();
        o["name"] = q.value(2).toString();
        o["rating"] = q.value(3).toInt();
        arr.append(o);
    }
    return arr;
}

bool Database::addFriend(int userId, int friendId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO friends (user_id, friend_id) VALUES (:u, :f) ON CONFLICT DO NOTHING");
    q.bindValue(":u", userId); q.bindValue(":f", friendId);
    return q.exec();
}

bool Database::removeFriend(int userId, int friendId) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("DELETE FROM friends WHERE user_id = :u AND friend_id = :f");
    q.bindValue(":u", userId); q.bindValue(":f", friendId);
    return q.exec();
}

QJsonArray Database::getFriends(int userId) {
    QJsonArray arr;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT u.id, u.username, u.name, u.rating FROM friends f JOIN users u ON f.friend_id = u.id WHERE f.user_id = :u ORDER BY u.rating DESC");
    q.bindValue(":u", userId);
    q.exec();
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["username"] = q.value(1).toString();
        o["name"] = q.value(2).toString();
        o["rating"] = q.value(3).toInt();
        arr.append(o);
    }
    return arr;
}

int Database::addBlogPost(int userId, const QString& content) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO blog_posts (user_id, content) VALUES (:u, :c) RETURNING id");
    q.bindValue(":u", userId); q.bindValue(":c", content);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return -1;
}

QJsonArray Database::getBlogPosts(int userId) {
    QJsonArray arr;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT id, content, created_at FROM blog_posts WHERE user_id = :u ORDER BY created_at DESC");
    q.bindValue(":u", userId);
    q.exec();
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["content"] = q.value(1).toString();
        o["created_at"] = q.value(2).toDateTime().toString(Qt::ISODate);
        arr.append(o);
    }
    return arr;
}

int Database::addBlogComment(int postId, int userId, const QString& content) {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("INSERT INTO blog_comments (post_id, user_id, content) VALUES (:p, :u, :c) RETURNING id");
    q.bindValue(":p", postId); q.bindValue(":u", userId); q.bindValue(":c", content);
    if (q.exec() && q.next()) return q.value(0).toInt();
    return -1;
}

QJsonArray Database::getBlogComments(int postId) {
    QJsonArray arr;
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT c.id, c.user_id, u.username, c.content, c.created_at FROM blog_comments c JOIN users u ON c.user_id = u.id WHERE c.post_id = :p ORDER BY c.created_at ASC");
    q.bindValue(":p", postId);
    q.exec();
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value(0).toInt();
        o["user_id"] = q.value(1).toInt();
        o["username"] = q.value(2).toString();
        o["content"] = q.value(3).toString();
        o["created_at"] = q.value(4).toDateTime().toString(Qt::ISODate);
        arr.append(o);
    }
    return arr;
}

void Database::autoRateContests() {
    QSqlDatabase db = QSqlDatabase::database(getThreadLocalConnection());
    QSqlQuery q(db);
    q.prepare("SELECT id FROM contests WHERE is_rated = FALSE AND end_time < CURRENT_TIMESTAMP");
    if (q.exec()) {
        QList<int> toRate;
        while (q.next()) toRate.append(q.value(0).toInt());
        for (int cid : toRate) {
            qInfo() << "Auto-rating contest:" << cid;
            rateContest(cid);
        }
    }
}


