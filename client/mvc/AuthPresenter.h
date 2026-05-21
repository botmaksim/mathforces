#pragma once
#include <QObject>
#include "ApiClient.h"

class AuthPresenter : public QObject {
    Q_OBJECT
public:
    explicit AuthPresenter(QObject* parent = nullptr);
    void login(const QString& email, const QString& password);
    void requestCode(const QString& email);
    void registerUser(const QString& code, const QString& email, const QString& username, const QString& name, const QString& password);

signals:
    void loginSuccessful(const QString& token, const QString& role);
    void codeRequested();
    void errorOccurred(const QString& errorStr);

private slots:
    void onLoginSuccessful(const QString& token, const QString& role);
    void onCodeRequested();
    void onApiError(const QString& errorStr);

private:
    ApiClient* m_apiClient;
};
