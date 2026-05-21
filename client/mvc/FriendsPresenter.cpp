#include "FriendsPresenter.h"
#include <QVariantList>
#include <QJsonObject>
#include <QDebug>

FriendsPresenter::FriendsPresenter(UsersListModel* searchModel, UsersListModel* friendsModel, const QString& token, QObject* parent) 
    : QObject(parent), m_searchModel(searchModel), m_friendsModel(friendsModel), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::usersSearched, this, &FriendsPresenter::onUsersSearched);
    connect(m_apiClient, &ApiClient::friendsLoaded, this, &FriendsPresenter::onFriendsLoaded);
    connect(m_apiClient, &ApiClient::friendAdded, this, &FriendsPresenter::onFriendAdded);
    connect(m_apiClient, &ApiClient::friendRemoved, this, &FriendsPresenter::onFriendRemoved);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &FriendsPresenter::onApiError);
}

void FriendsPresenter::searchUsers(const QString& query) {
    if(!query.isEmpty()) m_apiClient->searchUsers(m_token, query);
}

void FriendsPresenter::loadFriends() {
    m_apiClient->fetchFriends(m_token);
}

void FriendsPresenter::addFriend(int friendId) {
    if(friendId > 0) m_apiClient->addFriend(m_token, friendId);
}

void FriendsPresenter::removeFriend(int friendId) {
    if(friendId > 0) m_apiClient->removeFriend(m_token, friendId);
}

void FriendsPresenter::onUsersSearched(const QJsonArray& data) {
    QVariantList list;
    for(int i = 0; i < data.size(); ++i) list.append(data[i].toObject().toVariantMap());
    m_searchModel->setUsers(list);
}

void FriendsPresenter::onFriendsLoaded(const QJsonArray& data) {
    QVariantList list;
    for(int i = 0; i < data.size(); ++i) list.append(data[i].toObject().toVariantMap());
    m_friendsModel->setUsers(list);
}

void FriendsPresenter::onFriendAdded() {
    loadFriends();
}

void FriendsPresenter::onFriendRemoved() {
    loadFriends();
}

void FriendsPresenter::onApiError(const QString& errorStr) {
    emit errorOccurred(errorStr);
}
