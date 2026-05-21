#pragma once
#include <QObject>
#include "ContestModel.h"
#include "ApiClient.h"

class ContestsPresenter : public QObject {
    Q_OBJECT
public:
    ContestsPresenter(ContestModel* model, const QString& token, QObject* parent = nullptr);
    void loadContests();
    void startVirtualParticipation(int contestId);
signals:
    void virtualParticipationStarted(int contestId);
    void errorOccurred(const QString& errMsg);
private slots:
    void onNetworkResponse(const QJsonArray& data);
    void onNetworkError(const QString& errMsg);
private:
    ContestModel* m_model;
    QString m_token;
    ApiClient* m_api;
};
