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
    sysMsg["content"] = "Ты беспристрастный и точный автогрейдер по математике. Участник прислал решение. Твоя задача — оценить его правильность (ответ и логику решения). Если предоставлено авторское решение - сверяйся с ним. "
                        "Оцени решение от 0 до 100 баллов. Верни СТРОГИЙ JSON формат ответа, без маркдауна и оберток: {\"score\": 100, \"feedback\": \"Отличная работа (или описание ошибки)...\", \"thinking\": \"твои рассуждения\", \"probability\": 0.0}. " + aiComment;

    QJsonObject userMsg; userMsg["role"] = "user";
    userMsg["content"] = QString("Условие задачи и возможно авторское решение: %1\n\nРешение участника: %2").arg(taskDesc, answer);

    QJsonObject payload;
    payload["model"] = qEnvironmentVariable("OPENROUTER_MODEL", "openai/gpt-4o-mini");
    payload["messages"] = QJsonArray{sysMsg, userMsg};
    payload["temperature"] = 0.0;
    
    QJsonObject format; format["type"] = "json_object";
    payload["response_format"] = format;

    QNetworkReply* reply = manager->post(req, QJsonDocument(payload).toJson());
    qInfo() << "LlmClient::evaluate - Request sent to OpenRouter API for submission ID:" << submissionId;
    connect(reply, &QNetworkReply::finished, [reply, submissionId, callback]() {
        QJsonObject resultJson;
        if (reply->error() == QNetworkReply::NoError) {
            qInfo() << "LlmClient::evaluate - Successful API response received for submission ID:" << submissionId;
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString().trimmed();
            if (text.startsWith("```json")) text = text.mid(7);
            else if (text.startsWith("```")) text = text.mid(3);
            if (text.endsWith("```")) text = text.chopped(3);
            
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

void LlmClient::evaluateHack(int hackId, const QString& editorial, const QString& answer, const QString& hackText, std::function<void(int, bool, const QString&)> callback) {
    QNetworkRequest req(QUrl("https://openrouter.ai/api/v1/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString apiKey = qEnvironmentVariable("OPENROUTER_API_KEY", "sk-or-v1-fake");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());

    QJsonObject sysMsg; sysMsg["role"] = "system";
    sysMsg["content"] = "Ты судья соревнований по математике. Твоя задача — проверить ВЗЛОМ (hack). У тебя есть авторское решение задачи, решение участника (которое нужно взломать) и аргумент/контрпример хакера. Тебе нужно сказать, прав ли хакер. "
                        "Верни только СТРОГИЙ JSON без маркдауна и оберток: {\"is_successful\": true/false, \"explanation\": \"почему хакер прав или не прав\"}.";

    QJsonObject userMsg; userMsg["role"] = "user";
    userMsg["content"] = QString("Авторское решение: %1\n\nРешение участника: %2\n\nПретензия хакера: %3").arg(editorial, answer, hackText);

    QJsonObject payload;
    payload["model"] = qEnvironmentVariable("OPENROUTER_MODEL", "openai/gpt-4o-mini");
    payload["messages"] = QJsonArray{sysMsg, userMsg};
    payload["temperature"] = 0.0;
    
    QJsonObject format; format["type"] = "json_object";
    payload["response_format"] = format;

    QNetworkReply* reply = manager->post(req, QJsonDocument(payload).toJson());
    connect(reply, &QNetworkReply::finished, [reply, hackId, callback]() {
        bool isSuccessful = false;
        QString explanation = "Ошибка API";
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString text = doc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString().trimmed();
            if (text.startsWith("```json")) text = text.mid(7);
            else if (text.startsWith("```")) text = text.mid(3);
            if (text.endsWith("```")) text = text.chopped(3);
            QJsonObject res = QJsonDocument::fromJson(text.toUtf8()).object();
            isSuccessful = res["is_successful"].toBool();
            explanation = res["explanation"].toString();
        } else {
            explanation = reply->errorString();
        }
        callback(hackId, isSuccessful, explanation);
        reply->deleteLater();
    });
}
