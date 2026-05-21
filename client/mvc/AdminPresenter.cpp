#include "AdminPresenter.h"
#include <QVariantList>

AdminPresenter::AdminPresenter(const QString& token, QObject* parent) 
    : QObject(parent), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::myContestsLoaded, this, &AdminPresenter::onMyContestsLoaded);
    connect(m_apiClient, &ApiClient::draftCreated, this, &AdminPresenter::onDraftCreated);
    connect(m_apiClient, &ApiClient::contestUpdated, this, &AdminPresenter::onContestUpdated);
    connect(m_apiClient, &ApiClient::taskCreated, this, &AdminPresenter::onTaskCreated);
    connect(m_apiClient, &ApiClient::typstCompiled, this, &AdminPresenter::onTypstCompiled);
    connect(m_apiClient, &ApiClient::realtimeTypstCompiled, this, &AdminPresenter::onRealtimeTypstCompiled);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &AdminPresenter::onApiError);
}

void AdminPresenter::loadMyContests() { m_apiClient->fetchMyContests(m_token); }
void AdminPresenter::createDraftContest() { m_apiClient->createDraftContest(m_token); }
void AdminPresenter::updateContest(int contestId, const QString& title, const QString& start, double duration, const QString& desc, bool isPublished) {
    m_apiClient->updateContest(m_token, contestId, title, start, duration, desc, isPublished);
}
void AdminPresenter::createTask(int contestId, const QString& title, int maxScore, int maxSubmissions, const QString& desc, const QString& type, const QString& correctAnswer, const QString& editorial, bool sendEditorial, const QString& aiComment, const QString& tags, int difficulty) {
    m_apiClient->createTask(m_token, contestId, title, maxScore, maxSubmissions, desc, type, correctAnswer, editorial, sendEditorial, aiComment, tags, difficulty);
}
void AdminPresenter::compileTypst(const QString& typstCode) { m_apiClient->compileTypst(typstCode, false); }
void AdminPresenter::compileRealtime(const QString& typstCode) { m_apiClient->compileTypst(typstCode, true); }

void AdminPresenter::onMyContestsLoaded(const QJsonArray& data) { emit myContestsLoaded(data); }
void AdminPresenter::onDraftCreated() { emit draftCreated(); }
void AdminPresenter::onContestUpdated() { emit contestUpdated(); }
void AdminPresenter::onTaskCreated() { emit taskCreated(); }
void AdminPresenter::onTypstCompiled(const QByteArray& pdfData) { emit typstCompiled(pdfData); }
void AdminPresenter::onRealtimeTypstCompiled(const QByteArray& pdfData) { emit realtimeTypstCompiled(pdfData); }
void AdminPresenter::onApiError(const QString& errorStr) { emit errorOccurred(errorStr); }
