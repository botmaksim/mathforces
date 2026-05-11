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
    static QJsonArray getContests();
    static QJsonArray getTasks(int contestId);
    static QJsonArray getResults(int contestId);
    
    static int savePendingSubmission(int taskId, int userId, const QString& answer);
    static void updateSubmissionResult(int submissionId, int score, const QString& feedback, const QString& thinking);
    static QPair<QString, QString> getTaskDescriptionAndComment(int taskId);

    static bool createContest(const QString& title, const QString& description, const QString& start, const QString& end);
    static bool createTask(int contestId, const QString& title, const QString& description, int maxScore, const QString& aiComment);
};
