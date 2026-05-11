#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <functional>

class LlmClient : public QObject {
    Q_OBJECT
public:
    explicit LlmClient(QObject* parent = nullptr);
    void evaluate(int submissionId, const QString& taskDesc, const QString& aiComment, const QString& answer, std::function<void(int, QJsonObject)> callback);
private:
    QNetworkAccessManager* manager;
};
