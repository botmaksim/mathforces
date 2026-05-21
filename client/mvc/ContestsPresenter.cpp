#include "ContestsPresenter.h"
#include "LocalDb.h"
#include <QDebug>
#include <QJsonObject>

ContestsPresenter::ContestsPresenter(ContestModel* model, const QString& token, QObject* parent)
 : QObject(parent), m_model(model), m_token(token) {
    m_api = new ApiClient(this);
    connect(m_api, &ApiClient::contestsLoaded, this, &ContestsPresenter::onNetworkResponse);
    connect(m_api, &ApiClient::errorOccurred, this, &ContestsPresenter::onNetworkError);
    connect(m_api, &ApiClient::virtualParticipationStarted, this, &ContestsPresenter::virtualParticipationStarted);
}

void ContestsPresenter::loadContests() {
    // 1. Оффлайн готовность: Загружаем из локального SQL-кэша сразу, чтобы UI показал данные мгновенно
    QVariantList cache = LocalDb::getCachedContests();
    if (!cache.isEmpty()) {
        m_model->setContests(cache);
    }
    // 2. Делаем асинхронный сетевой запрос, чтобы освежить данные
    m_api->fetchContests(m_token);
}

void ContestsPresenter::startVirtualParticipation(int contestId) {
    m_api->startVirtualParticipation(m_token, contestId);
}

void ContestsPresenter::onNetworkResponse(const QJsonArray& data) {
    QVariantList list;
    for (const auto& v : data) {
        list.append(v.toObject().toVariantMap());
    }
    // Кэшируем свежие данные в локальную БД
    LocalDb::cacheContests(list);
    // Обновляем модель
    m_model->setContests(list);
}

void ContestsPresenter::onNetworkError(const QString& errMsg) {
    qWarning() << "Contests API Error:" << errMsg << "(Operating in offline cache mode)";
}
