#include "ArchivePresenter.h"
#include <QVariantList>
#include <QJsonObject>
#include <QDebug>

ArchivePresenter::ArchivePresenter(ArchiveModel* model, const QString& token, QObject* parent) 
    : QObject(parent), m_model(model), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::archiveTasksLoaded, this, &ArchivePresenter::onTasksLoaded);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &ArchivePresenter::onApiError);
}

void ArchivePresenter::loadTasks(const QString& tags, const QString& minDiff, const QString& maxDiff) {
    m_apiClient->fetchArchiveTasks(m_token, tags, minDiff, maxDiff);
}

void ArchivePresenter::onTasksLoaded(const QJsonArray& data) {
    QVariantList list;
    for(int i = 0; i < data.size(); ++i) {
        list.append(data[i].toObject().toVariantMap());
    }
    m_model->setTasks(list);
}

void ArchivePresenter::onApiError(const QString& errorStr) {
    emit errorOccurred(errorStr);
}
