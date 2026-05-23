#pragma once
#include <QObject>
#include <QJsonArray>
#include "ApiClient.h"

class ActiveContestPresenter : public QObject {
    Q_OBJECT
public:
    explicit ActiveContestPresenter(const QString& token, QObject* parent = nullptr);
    void loadTasks(int contestId);
    void submitAnswer(int taskId, const QString& answer);
    void compileTypst(const QString& typstCode);
    void compileRealtime(const QString& typstCode);
    void loadMySubmissions(int taskId);
    void loadAllSubmissions(int taskId);
    void submitHack(int submissionId, const QString& hackText);
    
    void saveDraft(int taskId, const QString& answer);
    QString loadDraft(int taskId);

signals:
    void tasksLoaded(const QJsonArray& data);
    void submissionSuccessful();
    void mySubmissionsLoaded(const QJsonArray& data);
    void allSubmissionsLoaded(const QJsonArray& data);
    void hackSuccessful(const QString& verdict, const QString& comment);
    void typstCompiled(const QByteArray& pdfData);
    void realtimeTypstCompiled(const QByteArray& pdfData);
    void errorOccurred(const QString& errorStr);

private:
    ApiClient* m_apiClient;
    QString m_token;
};
