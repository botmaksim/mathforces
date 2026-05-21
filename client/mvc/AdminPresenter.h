#pragma once
#include <QObject>
#include <QJsonArray>
#include "ApiClient.h"

class AdminPresenter : public QObject {
    Q_OBJECT
public:
    explicit AdminPresenter(const QString& token, QObject* parent = nullptr);
    void loadMyContests();
    void createDraftContest();
    void updateContest(int contestId, const QString& title, const QString& start, double duration, const QString& desc, bool isPublished);
    void createTask(int contestId, const QString& title, int maxScore, int maxSubmissions, const QString& desc, const QString& type, const QString& correctAnswer, const QString& editorial, bool sendEditorial, const QString& aiComment, const QString& tags, int difficulty);
    void compileTypst(const QString& typstCode);
    void compileRealtime(const QString& typstCode);

signals:
    void myContestsLoaded(const QJsonArray& data);
    void draftCreated();
    void contestUpdated();
    void taskCreated();
    void typstCompiled(const QByteArray& pdfData);
    void realtimeTypstCompiled(const QByteArray& pdfData);
    void errorOccurred(const QString& errorStr);

private slots:
    void onMyContestsLoaded(const QJsonArray& data);
    void onDraftCreated();
    void onContestUpdated();
    void onTaskCreated();
    void onTypstCompiled(const QByteArray& pdfData);
    void onRealtimeTypstCompiled(const QByteArray& pdfData);
    void onApiError(const QString& errorStr);

private:
    ApiClient* m_apiClient;
    QString m_token;
};
