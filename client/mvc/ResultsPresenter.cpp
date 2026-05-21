#include "ResultsPresenter.h"
#include <QVariantList>
#include <QJsonObject>

ResultsPresenter::ResultsPresenter(ResultsModel* model, const QString& token, QObject* parent) 
    : QObject(parent), m_model(model), m_token(token), m_lastContestId(-1) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::resultsLoaded, this, &ResultsPresenter::onResultsLoaded);
    connect(m_apiClient, &ApiClient::contestRated, this, &ResultsPresenter::onContestRated);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &ResultsPresenter::onApiError);
}

void ResultsPresenter::loadResults(int contestId) {
    m_lastContestId = contestId;
    m_apiClient->fetchResults(contestId);
}

void ResultsPresenter::rateContest(int contestId) {
    m_apiClient->rateContest(m_token, contestId);
}

void ResultsPresenter::onResultsLoaded(const QJsonArray& data) {
    QVariantList list;
    for(int i = 0; i < data.size(); ++i) {
        list.append(data[i].toObject().toVariantMap());
    }
    m_model->setResults(list);
}

void ResultsPresenter::onContestRated() {
    if (m_lastContestId != -1) loadResults(m_lastContestId);
}

void ResultsPresenter::onApiError(const QString& errorStr) {
    emit errorOccurred(errorStr);
}
