#pragma once
#include <QObject>
#include "ResultsModel.h"
#include "ApiClient.h"

class ResultsPresenter : public QObject {
    Q_OBJECT
public:
    explicit ResultsPresenter(ResultsModel* model, const QString& token, QObject* parent = nullptr);
    void loadResults(int contestId);
    void rateContest(int contestId);
    void setToken(const QString& token) { m_token = token; }

signals:
    void errorOccurred(const QString& errMsg);

private slots:
    void onResultsLoaded(const QJsonArray& data);
    void onContestRated();
    void onApiError(const QString& errorStr);

private:
    ResultsModel* m_model;
    ApiClient* m_apiClient;
    QString m_token;
    int m_lastContestId;
};
