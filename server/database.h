#pragma once
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>
#include <QPair>

class Database {
public:
    static bool init(const QString& dbName, const QString& user, const QString& pass, const QString& host = "127.0.0.1", int port = 5432);
    static QString getThreadLocalConnection();

    static QJsonObject authenticate(const QString& username, const QString& password);
    static QJsonObject authenticateByEmail(const QString& email, const QString& password);
    static QJsonObject authenticateOAuth(const QString& email, const QString& googleId, const QString& name);
    static bool registerByEmail(const QString& email, const QString& password, const QString& username, const QString& name);
    static QJsonArray getContests();
    static QJsonArray getTasks(int contestId);
    static QJsonArray getResults(int contestId);
    
    static int savePendingSubmission(int taskId, int userId, const QString& answer);
    static void updateSubmissionResult(int submissionId, int score, const QString& feedback, const QString& thinking, float probability = 0.0);
    static QJsonObject getTaskDetails(int taskId);

    static bool createContest(int authorId, const QString& title, const QString& description, const QString& start, float durationHours);
    static bool createTask(int contestId, const QString& type, const QString& title, const QString& description, int maxScore, int maxSubmissions, const QString& correctAnswer, const QString& editorial, const QString& aiComment, bool sendEditorial);
};
