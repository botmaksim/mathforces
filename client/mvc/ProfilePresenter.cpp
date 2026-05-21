#include "ProfilePresenter.h"

ProfilePresenter::ProfilePresenter(const QString& token, QObject* parent) 
    : QObject(parent), m_token(token) 
{
    m_apiClient = new ApiClient(this);
    connect(m_apiClient, &ApiClient::profileLoaded, this, [this](const QJsonObject& d, int t){
        if (t == -1) {
            emit myIdLoaded(d["id"].toInt());
        } else {
            emit profileLoaded(d);
        }
    });
    connect(m_apiClient, &ApiClient::blogPostsLoaded, this, [this](const QJsonArray& d){ emit blogPostsLoaded(d); });
    connect(m_apiClient, &ApiClient::blogPostAdded, this, [this](){ emit blogPostAdded(); });
    connect(m_apiClient, &ApiClient::commentsLoaded, this, [this](const QJsonArray& d){ emit commentsLoaded(d); });
    connect(m_apiClient, &ApiClient::commentAdded, this, [this](){ emit commentAdded(); });
    connect(m_apiClient, &ApiClient::errorOccurred, this, [this](const QString& e){ emit errorOccurred(e); });
}

void ProfilePresenter::fetchMyId() { m_apiClient->fetchProfile(m_token, -1); }
void ProfilePresenter::loadProfile(int targetUserId) { m_apiClient->fetchProfile(m_token, targetUserId); }
void ProfilePresenter::loadBlogPosts(int targetUserId) { m_apiClient->fetchBlogPosts(m_token, targetUserId); }
void ProfilePresenter::addBlogPost(const QString& content) { m_apiClient->addBlogPost(m_token, content); }
void ProfilePresenter::loadComments(int postId) { m_apiClient->fetchComments(m_token, postId); }
void ProfilePresenter::addComment(int postId, const QString& content) { m_apiClient->addComment(m_token, postId, content); }
