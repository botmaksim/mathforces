#include "ActiveContestPresenter.h"
#include "LocalDb.h"

ActiveContestPresenter::ActiveContestPresenter(const QString& token, QObject* parent) 
    : QObject(parent), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::contestTasksLoaded, this, [this](const QJsonArray& d){ emit tasksLoaded(d); });
    connect(m_apiClient, &ApiClient::submissionSuccessful, this, [this](){ emit submissionSuccessful(); });
    connect(m_apiClient, &ApiClient::mySubmissionsLoaded, this, [this](const QJsonArray& d){ emit mySubmissionsLoaded(d); });
    connect(m_apiClient, &ApiClient::allSubmissionsLoaded, this, [this](const QJsonArray& d){ emit allSubmissionsLoaded(d); });
    connect(m_apiClient, &ApiClient::hackSuccessful, this, [this](){ emit hackSuccessful(); });
    connect(m_apiClient, &ApiClient::typstCompiled, this, [this](const QByteArray& p){ emit typstCompiled(p); });
    connect(m_apiClient, &ApiClient::realtimeTypstCompiled, this, [this](const QByteArray& p){ emit realtimeTypstCompiled(p); });
    connect(m_apiClient, &ApiClient::errorOccurred, this, [this](const QString& e){ emit errorOccurred(e); });
}

void ActiveContestPresenter::loadTasks(int contestId) { m_apiClient->fetchContestTasks(m_token, contestId); }
void ActiveContestPresenter::submitAnswer(int taskId, const QString& answer) { m_apiClient->submitAnswer(m_token, taskId, answer); }
void ActiveContestPresenter::compileTypst(const QString& typstCode) { m_apiClient->compileTypst(typstCode, false); }
void ActiveContestPresenter::compileRealtime(const QString& typstCode) { m_apiClient->compileTypst(typstCode, true); }
void ActiveContestPresenter::loadMySubmissions(int taskId) { m_apiClient->fetchMySubmissions(m_token, taskId); }
void ActiveContestPresenter::loadAllSubmissions(int taskId) { m_apiClient->fetchAllSubmissions(m_token, taskId); }
void ActiveContestPresenter::submitHack(int submissionId, const QString& hackText) { m_apiClient->submitHack(m_token, submissionId, hackText); }

void ActiveContestPresenter::saveDraft(int taskId, const QString& answer) { LocalDb::saveTaskDraft(taskId, answer); }
QString ActiveContestPresenter::loadDraft(int taskId) { return LocalDb::getTaskDraft(taskId); }
