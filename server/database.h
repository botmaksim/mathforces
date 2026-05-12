#pragma once
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>
#include <QPair>

class Database {
public:
    static bool init(const QString& dbName, const QString& user, const QString& pass, const QString& host = "127.0.0.1", int port = 5432);
    static void createInitialUsers();
    
    static QString getThreadLocalConnection();

    static QJsonObject authenticate(const QString& username, const QString& password);
    static QJsonObject authenticateByEmail(const QString& email, const QString& password);
    static QJsonObject authenticateOAuth(const QString& email, const QString& googleId, const QString& name);
    static bool registerByEmail(const QString& email, const QString& password, const QString& username, const QString& name);
    
    static QJsonArray getContests(int userId, bool showAll = false);
    static bool registerContest(int contestId, int userId, bool isOfficial);
    static bool registerVirtualParticipation(int contestId, int userId);
    static bool isVirtualParticipating(int contestId, int userId, QDateTime& virtualStart);
    static QJsonObject getContestContextForTask(int taskId);
    static bool isRegistered(int contestId, int userId);
    static void rateContest(int contestId);
    static void autoRateContests();
    static QJsonArray getGlobalRatings();

    static QJsonArray getMyContests(int authorId);
    static QJsonArray getTasks(int contestId);
    static QJsonArray getArchiveTasks(const QString& tags, int minDiff, int maxDiff);
    static QJsonArray getResults(int contestId);
    
    static int savePendingSubmission(int taskId, int userId, const QString& answer, bool isUpsolving);
    static void updateSubmissionResult(int submissionId, int score, const QString& feedback, const QString& thinking, float probability = 0.0);
    static QJsonObject getTaskDetails(int taskId);
    static int getUserSubmissionsCount(int taskId, int userId);
    static QJsonArray getAllSubmissions(int taskId);
    static int saveHack(int hackerId, int submissionId, const QString& hackText);
    static void updateHackStatus(int hackId, bool isSuccessful, const QString& explanation);
    static QJsonObject getHackContext(int hackId);

    static int createContestInitial(int authorId);
    static bool updateContest(int contestId, int authorId, const QString& title, const QString& description, const QString& start, float durationHours, bool isPublished);
    static bool createTask(int contestId, const QString& type, const QString& title, const QString& description, int maxScore, int maxSubmissions, const QString& correctAnswer, const QString& editorial, const QString& aiComment, bool sendEditorial, const QString& tags, int difficulty);
    
    // User management
    static QJsonArray getUsers(const QString& requestorRole);
    static bool updateUserRole(int userId, const QString& newRole);
    static bool toggleUserBan(int userId, bool isBanned);
    static bool toggleUserBlog(int userId, bool canBlog);
    
    // Social / Blog
    static QJsonObject getUserProfile(int userId, int requestorId, const QString& requestorRole);
    static QJsonArray searchUsers(const QString& queryText);
    static bool addFriend(int userId, int friendId);
    static bool removeFriend(int userId, int friendId);
    static QJsonArray getFriends(int userId);
    
    static int addBlogPost(int userId, const QString& content);
    static QJsonArray getBlogPosts(int userId);
    static int addBlogComment(int postId, int userId, const QString& content);
    static QJsonArray getBlogComments(int postId);
};
