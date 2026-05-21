#pragma once
#include <QObject>
#include "RatingModel.h"
#include "ApiClient.h"

class RatingsPresenter : public QObject {
    Q_OBJECT
public:
    explicit RatingsPresenter(RatingModel* model, const QString& token, QObject* parent = nullptr);
    void loadRatings();
    void setToken(const QString& token) { m_token = token; }

signals:
    void errorOccurred(const QString& errMsg);

private slots:
    void onRatingsLoaded(const QJsonArray& data);
    void onApiError(const QString& errorStr);

private:
    RatingModel* m_model;
    ApiClient* m_apiClient;
    QString m_token;
};
