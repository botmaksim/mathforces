#include "RatingsPresenter.h"
#include "LocalDb.h"
#include <QVariantList>
#include <QJsonObject>
#include <QDebug>

RatingsPresenter::RatingsPresenter(RatingModel* model, const QString& token, QObject* parent) 
    : QObject(parent), m_model(model), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::ratingsLoaded, this, &RatingsPresenter::onRatingsLoaded);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &RatingsPresenter::onApiError);

    // Initial load from cache
    QVariantList cached = LocalDb::getCachedRatings();
    if (!cached.isEmpty()) {
        m_model->setRatings(cached);
    }
}

void RatingsPresenter::loadRatings() {
    m_apiClient->fetchRatings(m_token);
}

void RatingsPresenter::onRatingsLoaded(const QJsonArray& data) {
    QVariantList list;
    for(int i = 0; i < data.size(); ++i) {
        list.append(data[i].toObject().toVariantMap());
    }
    m_model->setRatings(list);
    LocalDb::cacheRatings(list);
}

void RatingsPresenter::onApiError(const QString& errorStr) {
    emit errorOccurred(errorStr);
}
