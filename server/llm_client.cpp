#include "llm_client.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcessEnvironment>

LlmClient::LlmClient(QObject* parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void LlmClient::evaluate(int submissionId, const QString& taskDesc, const QString& aiComment, const QString& answer, std::function<void(int, QJsonObject)> callback) {
    QNetworkRequest req(QUrl("https://openrouter.ai/api/v1/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // В реальности считывать из файла .env, здесь для упрощения "hardcode" или заглушка, 
    // но по условию берем из окружения:
    QString apiKey = qEnvironmentVariable("OPENROUTER_API_KEY", "sk-or-v1-fake");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QJsonObject sysMsg; sysMsg["role"] = "system";
    sysMsg["content"] = "Ты строгий преподаватель математики. Студент может отправлять код и формулы в формате LaTeX или Typst. Внимательно проверяй математические выкладки на наличие читерства или использования сторонних решателей. Верни ТОЛЬКО СТРОГИЙ JSON без маркдауна: {\"score\": int (0-100), \"feedback\": \"...\", \"thinking\": \"...\", \"probability\": float (0.0-1.0 вероятности читерства)}. " + aiComment;

    QJsonObject userMsg; userMsg["role"] = "user";
    userMsg["content"] = QString("Задача (в т.ч. может содержать LaTeX/Typst): %1\nОтвет и решение: %2").arg(taskDesc, answer);

    QJsonObject payload;
    payload["model"] = "openrouter/openai/gpt-4o-mini";
    payload["messages"] = QJsonArray{sysMsg, userMsg};
    
    QJsonObject format; format["type"] = "json_object";
    payload["response_format"] = format;

    QNetworkReply* reply = manager->post(req, QJsonDocument(payload).toJson());
    qInfo() << "LlmClient::evaluate - Request sent to OpenRouter API for submission ID:" << submissionId;
    connect(reply, &QNetworkReply::finished, [reply, submissionId, callback]() {
        QJsonObject resultJson;
        if (reply->error() == QNetworkReply::NoError) {
            qInfo() << "LlmClient::evaluate - Successful API response received for submission ID:" << submissionId;
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();
            qInfo() << "LlmClient::evaluate - Parsed AI message content size:" << text.size() << "bytes";
            resultJson = QJsonDocument::fromJson(text.toUtf8()).object();
        } else {
            qCritical() << "LlmClient::evaluate - Error for submission ID:" << submissionId << "-" << reply->errorString() << "Body:" << reply->readAll();
            resultJson["score"] = 0; resultJson["feedback"] = "Ошибка проверки ИИ: " + reply->errorString(); resultJson["thinking"] = "";
        }
        callback(submissionId, resultJson);
        reply->deleteLater();
    });
}
