#include "UsersPresenter.h"
#include <QVariantList>
#include <QJsonObject>
#include <QDebug>

UsersPresenter::UsersPresenter(UsersTableModel* model, const QString& token, QObject* parent) 
    : QObject(parent), m_model(model), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::usersLoaded, this, &UsersPresenter::onUsersLoaded);
    connect(m_apiClient, &ApiClient::userUpdated, this, &UsersPresenter::onUserUpdated);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &UsersPresenter::onApiError);
}

void UsersPresenter::loadUsers() {
    m_apiClient->fetchUsers(m_token);
}

void UsersPresenter::changeRole(int userId, const QString& role) {
    if(userId > 0) m_apiClient->changeUserRole(m_token, userId, role);
}

void UsersPresenter::changeBan(int userId, bool isBanned) {
    if(userId > 0) m_apiClient->changeUserBan(m_token, userId, isBanned);
}

void UsersPresenter::changeBlog(int userId, bool canBlog) {
    if(userId > 0) m_apiClient->changeUserBlog(m_token, userId, canBlog);
}

void UsersPresenter::onUsersLoaded(const QJsonArray& data) {
    QVariantList list;
    for(int i = 0; i < data.size(); ++i) {
        list.append(data[i].toObject().toVariantMap());
    }
    m_model->setUsers(list);
}

void UsersPresenter::onUserUpdated() {
    loadUsers();
}

void UsersPresenter::onApiError(const QString& errorStr) {
    emit errorOccurred(errorStr);
}
