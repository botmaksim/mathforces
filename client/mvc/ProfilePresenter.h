#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "ApiClient.h"

class ProfilePresenter : public QObject {
    Q_OBJECT
public:
    explicit ProfilePresenter(const QString& token, QObject* parent = nullptr);
    void fetchMyId();
    void loadProfile(int targetUserId);
    void loadBlogPosts(int targetUserId);
    void addBlogPost(const QString& content);
    void loadComments(int postId);
    void addComment(int postId, const QString& content);

signals:
    void myIdLoaded(int myId);
    void profileLoaded(const QJsonObject& data);
    void blogPostsLoaded(const QJsonArray& data);
    void blogPostAdded();
    void commentsLoaded(const QJsonArray& data);
    void commentAdded();
    void errorOccurred(const QString& errorStr);

private:
    ApiClient* m_apiClient;
    QString m_token;
};
