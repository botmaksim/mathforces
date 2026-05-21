#pragma once
#include <QObject>
#include "UsersListModel.h"
#include "ApiClient.h"

class FriendsPresenter : public QObject {
    Q_OBJECT
public:
    explicit FriendsPresenter(UsersListModel* searchModel, UsersListModel* friendsModel, const QString& token, QObject* parent = nullptr);
    void searchUsers(const QString& query);
    void loadFriends();
    void addFriend(int friendId);
    void removeFriend(int friendId);
    void setToken(const QString& token) { m_token = token; }

signals:
    void errorOccurred(const QString& errMsg);

private slots:
    void onUsersSearched(const QJsonArray& data);
    void onFriendsLoaded(const QJsonArray& data);
    void onFriendAdded();
    void onFriendRemoved();
    void onApiError(const QString& errorStr);

private:
    UsersListModel* m_searchModel;
    UsersListModel* m_friendsModel;
    ApiClient* m_apiClient;
    QString m_token;
};
