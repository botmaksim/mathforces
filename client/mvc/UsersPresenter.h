#pragma once
#include <QObject>
#include "UsersTableModel.h"
#include "ApiClient.h"

class UsersPresenter : public QObject {
    Q_OBJECT
public:
    explicit UsersPresenter(UsersTableModel* model, const QString& token, QObject* parent = nullptr);
    void loadUsers();
    void changeRole(int userId, const QString& role);
    void changeBan(int userId, bool isBanned);
    void changeBlog(int userId, bool canBlog);
    void setToken(const QString& token) { m_token = token; }

signals:
    void errorOccurred(const QString& errMsg);

private slots:
    void onUsersLoaded(const QJsonArray& data);
    void onUserUpdated();
    void onApiError(const QString& errorStr);

private:
    UsersTableModel* m_model;
    ApiClient* m_apiClient;
    QString m_token;
};
