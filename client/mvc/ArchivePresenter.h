#pragma once
#include <QObject>
#include "ArchiveModel.h"
#include "ApiClient.h"

class ArchivePresenter : public QObject {
    Q_OBJECT
public:
    explicit ArchivePresenter(ArchiveModel* model, const QString& token, QObject* parent = nullptr);
    void loadTasks(const QString& tags, const QString& minDiff, const QString& maxDiff);
    void setToken(const QString& token) { m_token = token; }

signals:
    void errorOccurred(const QString& errMsg);

private slots:
    void onTasksLoaded(const QJsonArray& data);
    void onApiError(const QString& errorStr);

private:
    ArchiveModel* m_model;
    ApiClient* m_apiClient;
    QString m_token;
};
