#pragma once
#include <QObject>
#include <QJsonArray>
#include <QNetworkAccessManager>

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);
    void fetchContests(const QString& token);
    void fetchRatings(const QString& token);
    void fetchArchiveTasks(const QString& token, const QString& tags, const QString& minDiff, const QString& maxDiff);
    void searchUsers(const QString& token, const QString& query);
    void fetchFriends(const QString& token);
    void addFriend(const QString& token, int friendId);
    void removeFriend(const QString& token, int friendId);
    void login(const QString& email, const QString& password);
    void requestCode(const QString& email);
    void registerUser(const QString& code, const QString& email, const QString& username, const QString& name, const QString& password);
    void fetchUsers(const QString& token);
    void changeUserRole(const QString& token, int userId, const QString& role);
    void changeUserBan(const QString& token, int userId, bool isBanned);
    void changeUserBlog(const QString& token, int userId, bool canBlog);
    void fetchResults(int contestId);
    void rateContest(const QString& token, int contestId);
    void fetchMyContests(const QString& token);
    void createDraftContest(const QString& token);
    void updateContest(const QString& token, int contestId, const QString& title, const QString& start, double duration, const QString& desc, bool isPublished);
    void createTask(const QString& token, int contestId, const QString& title, int maxScore, int maxSubmissions, const QString& desc, const QString& type, const QString& correctAnswer, const QString& editorial, bool sendEditorial, const QString& aiComment, const QString& tags, int difficulty);
    void compileTypst(const QString& code, bool realtime);
    void fetchContestTasks(const QString& token, int contestId);
    void submitAnswer(const QString& token, int taskId, const QString& answer);
    void fetchMySubmissions(const QString& token, int taskId);
    void fetchAllSubmissions(const QString& token, int taskId);
    void submitHack(const QString& token, int submissionId, const QString& hackText);
    void fetchProfile(const QString& token, int targetUserId = -1);
    void fetchBlogPosts(const QString& token, int userId);
    void addBlogPost(const QString& token, const QString& content);
    void fetchComments(const QString& token, int postId);
    void addComment(const QString& token, int postId, const QString& content);
    void startVirtualParticipation(const QString& token, int contestId);
signals:
    void contestsLoaded(const QJsonArray& data);
    void ratingsLoaded(const QJsonArray& data);
    void archiveTasksLoaded(const QJsonArray& data);
    void usersSearched(const QJsonArray& data);
    void friendsLoaded(const QJsonArray& data);
    void friendAdded();
    void friendRemoved();
    void usersLoaded(const QJsonArray& data);
    void userUpdated();
    void resultsLoaded(const QJsonArray& data);
    void contestRated();
    void loginSuccessful(const QString& token, const QString& role);
    void codeRequested();
    void myContestsLoaded(const QJsonArray& data);
    void draftCreated();
    void contestUpdated();
    void taskCreated();
    void typstCompiled(const QByteArray& pdfData);
    void realtimeTypstCompiled(const QByteArray& pdfData);
    void contestTasksLoaded(const QJsonArray& data);
    void submissionSuccessful();
    void mySubmissionsLoaded(const QJsonArray& data);
    void allSubmissionsLoaded(const QJsonArray& data);
    void hackSuccessful();
    void profileLoaded(const QJsonObject& data, int targetUserId);
    void blogPostsLoaded(const QJsonArray& data);
    void blogPostAdded();
    void commentsLoaded(const QJsonArray& data);
    void commentAdded();
    void virtualParticipationStarted(int contestId);
    void errorOccurred(const QString& errorStr);
private:
    QNetworkAccessManager* m_manager;
};
