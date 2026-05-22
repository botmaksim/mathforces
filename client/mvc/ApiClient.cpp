#include "ApiClient.h"
#include "../api_config.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

ApiClient::ApiClient(QObject *parent) : QObject(parent) {
  m_manager = new QNetworkAccessManager(this);
}

void ApiClient::fetchContests(const QString &token) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/contests"));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());

  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
      emit contestsLoaded(arr);
    } else {
      emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
  });
}

void ApiClient::fetchArchiveTasks(const QString &token, const QString &tags,
                                  const QString &minDiff,
                                  const QString &maxDiff) {
  QString urlStr = ApiConfig::baseUrl + "/api/archive/tasks?";
  if (!tags.isEmpty()) {
    urlStr += "tags=" + tags + "&";
  }
  if (!minDiff.isEmpty()) {
    urlStr += "min_diff=" + minDiff + "&";
  }
  if (!maxDiff.isEmpty()) {
    urlStr += "max_diff=" + maxDiff + "&";
  }

  QNetworkRequest req((QUrl(urlStr)));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());

  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
      emit archiveTasksLoaded(arr);
    } else {
      emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
  });
}

void ApiClient::searchUsers(const QString &token, const QString &query) {
  QNetworkRequest req(
      QUrl(QString(ApiConfig::baseUrl + "/api/users/search?q=%1").arg(query)));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit usersSearched(QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchFriends(const QString &token) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/friends/list"));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit friendsLoaded(QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::addFriend(const QString &token, int friendId) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/friends/add"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["friend_id"] = friendId;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit friendAdded();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::removeFriend(const QString &token, int friendId) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/friends/remove"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["friend_id"] = friendId;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit friendRemoved();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::login(const QString &email, const QString &password) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/login/email"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QJsonObject j;
  j["email"] = email;
  j["password"] = password;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
      emit loginSuccessful(o["token"].toString(), o["role"].toString());
    } else
      emit errorOccurred("Логин не удался");
    reply->deleteLater();
  });
}

void ApiClient::requestCode(const QString &email) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/register/request_code"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QJsonObject j;
  j["email"] = email;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit codeRequested();
    } else
      emit errorOccurred(
          "Ошибка при запросе кода (возможно email занят / не существует)");
    reply->deleteLater();
  });
}

void ApiClient::registerUser(const QString &code, const QString &email,
                             const QString &username, const QString &name,
                             const QString &password) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/register/email"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QJsonObject j;
  j["code"] = code;
  j["email"] = email;
  j["username"] = username;
  j["name"] = name;
  j["password"] = password;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
      emit loginSuccessful(o["token"].toString(), "user");
    } else {
      QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
      QString s =
          o.contains("error") ? o["error"].toString() : reply->errorString();
      emit errorOccurred("Регистрация не удалась: " + s);
    }
    reply->deleteLater();
  });
}

void ApiClient::fetchUsers(const QString &token) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users"));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit usersLoaded(QJsonDocument::fromJson(reply->readAll()).array());
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::changeUserRole(const QString &token, int userId,
                               const QString &role) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/role"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["user_id"] = userId;
  j["role"] = role;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit userUpdated();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::changeUserBan(const QString &token, int userId, bool isBanned) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/ban"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["user_id"] = userId;
  j["is_banned"] = isBanned;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit userUpdated();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::changeUserBlog(const QString &token, int userId, bool canBlog) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/blog_access"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["user_id"] = userId;
  j["can_blog"] = canBlog;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit userUpdated();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchResults(int contestId) {
  QNetworkRequest req(
      QUrl(QString(ApiConfig::baseUrl + "/api/results?contest_id=%1")
               .arg(contestId)));
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit resultsLoaded(QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::rateContest(const QString &token, int contestId) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/rate_contest"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["contest_id"] = contestId;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit contestRated();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchMyContests(const QString &token) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/my_contests"));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit myContestsLoaded(QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::createDraftContest(const QString &token) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/contest"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit draftCreated();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::updateContest(const QString &token, int contestId,
                              const QString &title, const QString &start,
                              double duration, const QString &desc,
                              bool isPublished) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/contest"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["id"] = contestId;
  j["title"] = title;
  j["start"] = start;
  j["duration_hours"] = duration;
  j["description"] = desc;
  j["is_published"] = isPublished;
  QNetworkReply *reply = m_manager->put(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit contestUpdated();
    else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::createTask(const QString &token, int contestId,
                           const QString &title, int maxScore,
                           int maxSubmissions, const QString &desc,
                           const QString &type, const QString &correctAnswer,
                           const QString &editorial, bool sendEditorial,
                           const QString &aiComment, const QString &tags,
                           int difficulty) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/task"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["contest_id"] = contestId;
  j["title"] = title;
  j["max_score"] = maxScore;
  j["max_submissions"] = maxSubmissions;
  j["description"] = desc;
  j["task_type"] = type;
  if (type == "answer_only") {
    j["correct_answer"] = correctAnswer;
  } else {
    j["editorial"] = editorial;
    j["send_editorial_to_ai"] = sendEditorial;
    j["ai_comment"] = aiComment;
  }
  j["tags"] = tags;
  j["difficulty"] = difficulty;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
      emit taskCreated();
    else
      emit errorOccurred(reply->errorString() + "-" + reply->readAll());
    reply->deleteLater();
  });
}

void ApiClient::compileTypst(const QString &code, bool realtime) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/compile_typst"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QJsonObject j;
  j["code"] = code;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply, realtime]() {
    if (reply->error() == QNetworkReply::NoError) {
      if (realtime)
        emit realtimeTypstCompiled(reply->readAll());
      else
        emit typstCompiled(reply->readAll());
    } else {
      if (!realtime)
        emit errorOccurred(reply->errorString() + "\n" + reply->readAll());
    }
    reply->deleteLater();
  });
}

void ApiClient::fetchContestTasks(const QString &token, int contestId) {
  QNetworkRequest req(QUrl(
      QString(ApiConfig::baseUrl + "/api/tasks?contest_id=%1").arg(contestId)));
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit contestTasksLoaded(
          QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::submitAnswer(const QString &token, int taskId,
                             const QString &answer) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/submit"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["task_id"] = taskId;
  j["answer"] = answer;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit submissionSuccessful();
    } else
      emit errorOccurred(reply->errorString() + "\n" + reply->readAll());
    reply->deleteLater();
  });
}

void ApiClient::fetchMySubmissions(const QString &token, int taskId) {
  QNetworkRequest req(QUrl(
      QString(ApiConfig::baseUrl + "/api/submissions?task_id=%1").arg(taskId)));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit mySubmissionsLoaded(
          QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchAllSubmissions(const QString &token, int taskId) {
  QNetworkRequest req(
      QUrl(QString(ApiConfig::baseUrl + "/api/submissions/all?task_id=%1")
               .arg(taskId)));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit allSubmissionsLoaded(
          QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::submitHack(const QString &token, int submissionId,
                           const QString &hackText) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/hacks"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["submission_id"] = submissionId;
  j["hack_text"] = hackText;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit hackSuccessful();
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchProfile(const QString &token, int targetUserId) {
  QUrl url(ApiConfig::baseUrl + "/api/users/profile");
  if (targetUserId != -1)
    url = QUrl(QString(ApiConfig::baseUrl + "/api/users/profile?id=%1")
                   .arg(targetUserId));
  QNetworkRequest req(url);
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply, targetUserId]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit profileLoaded(QJsonDocument::fromJson(reply->readAll()).object(),
                         targetUserId);
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchBlogPosts(const QString &token, int userId) {
  QNetworkRequest req(QUrl(
      QString(ApiConfig::baseUrl + "/api/blog/posts?user_id=%1").arg(userId)));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit blogPostsLoaded(QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::addBlogPost(const QString &token, const QString &content) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/blog/posts"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["content"] = content;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit blogPostAdded();
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchComments(const QString &token, int postId) {
  QNetworkRequest req(
      QUrl(QString(ApiConfig::baseUrl + "/api/blog/comments?post_id=%1")
               .arg(postId)));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit commentsLoaded(QJsonDocument::fromJson(reply->readAll()).array());
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::addComment(const QString &token, int postId,
                           const QString &content) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/blog/comments"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["post_id"] = postId;
  j["content"] = content;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit commentAdded();
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::startVirtualParticipation(const QString &token, int contestId) {
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/contests/virtual"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());
  QJsonObject j;
  j["contest_id"] = contestId;
  QNetworkReply *reply = m_manager->post(req, QJsonDocument(j).toJson());
  connect(reply, &QNetworkReply::finished, [this, reply, contestId]() {
    if (reply->error() == QNetworkReply::NoError) {
      emit virtualParticipationStarted(contestId);
    } else
      emit errorOccurred(reply->errorString());
    reply->deleteLater();
  });
}

void ApiClient::fetchRatings(const QString &token) {

  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/ratings"));
  if (!token.isEmpty())
    req.setRawHeader("Authorization", token.toUtf8());

  QNetworkReply *reply = m_manager->get(req);
  connect(reply, &QNetworkReply::finished, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
      emit ratingsLoaded(arr);
    } else {
      emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
  });
}
