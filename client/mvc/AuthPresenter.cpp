#include "AuthPresenter.h"

AuthPresenter::AuthPresenter(QObject* parent) : QObject(parent) {
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::loginSuccessful, this, &AuthPresenter::onLoginSuccessful);
    connect(m_apiClient, &ApiClient::codeRequested, this, &AuthPresenter::onCodeRequested);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &AuthPresenter::onApiError);
}

void AuthPresenter::login(const QString& email, const QString& password) {
    m_apiClient->login(email, password);
}

void AuthPresenter::requestCode(const QString& email) {
    m_apiClient->requestCode(email);
}

void AuthPresenter::registerUser(const QString& code, const QString& email, const QString& username, const QString& name, const QString& password) {
    m_apiClient->registerUser(code, email, username, name, password);
}

void AuthPresenter::onLoginSuccessful(const QString& token, const QString& role) {
    emit loginSuccessful(token, role);
}

void AuthPresenter::onCodeRequested() {
    emit codeRequested();
}

void AuthPresenter::onApiError(const QString& errorStr) {
    emit errorOccurred(errorStr);
}
