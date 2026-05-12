#include "friends_tab.h"
#include "profile_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

FriendsTab::FriendsTab(const QString& token, const QString& myRole, QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole)
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // Left side: Search
    QVBoxLayout* leftLayout = new QVBoxLayout();
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Имя или Username");
    m_btnSearch = new QPushButton("Искать");
    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(m_btnSearch);
    
    m_searchResults = new QListWidget();
    m_btnAddFriend = new QPushButton("Добавить в друзья");
    
    leftLayout->addLayout(searchLayout);
    leftLayout->addWidget(m_searchResults);
    leftLayout->addWidget(m_btnAddFriend);
    
    // Right side: Friends
    QVBoxLayout* rightLayout = new QVBoxLayout();
    m_friendsList = new QListWidget();
    m_btnRemoveFriend = new QPushButton("Удалить из друзей");
    QPushButton* btnRefresh = new QPushButton("Обновить друзей");
    
    rightLayout->addWidget(btnRefresh);
    rightLayout->addWidget(m_friendsList);
    rightLayout->addWidget(m_btnRemoveFriend);
    
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 1);
    
    connect(m_btnSearch, &QPushButton::clicked, this, &FriendsTab::searchUsers);
    connect(btnRefresh, &QPushButton::clicked, this, &FriendsTab::loadFriends);
    connect(m_searchResults, &QListWidget::itemDoubleClicked, this, &FriendsTab::onUserClicked);
    connect(m_friendsList, &QListWidget::itemDoubleClicked, this, &FriendsTab::onUserClicked);
    connect(m_btnAddFriend, &QPushButton::clicked, this, &FriendsTab::addFriend);
    connect(m_btnRemoveFriend, &QPushButton::clicked, this, &FriendsTab::removeFriend);
    
    loadFriends();
}

void FriendsTab::searchUsers() {
    QString q = m_searchEdit->text();
    if (q.isEmpty()) return;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(QString("http://127.0.0.1:8080/api/users/search?q=%1").arg(q)));
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            m_searchResults->clear();
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            for (auto v : arr) {
                QJsonObject o = v.toObject();
                QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2) - Эло: %3").arg(o["username"].toString(), o["name"].toString(), QString::number(o["rating"].toInt())));
                item->setData(Qt::UserRole, o["id"].toInt());
                m_searchResults->addItem(item);
            }
        }
        r->deleteLater(); m->deleteLater();
    });
}

void FriendsTab::loadFriends() {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/friends/list"));
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            m_friendsList->clear();
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            for (auto v : arr) {
                QJsonObject o = v.toObject();
                QListWidgetItem* item = new QListWidgetItem(QString("%1 (%2) - Эло: %3").arg(o["username"].toString(), o["name"].toString(), QString::number(o["rating"].toInt())));
                item->setData(Qt::UserRole, o["id"].toInt());
                m_friendsList->addItem(item);
            }
        }
        r->deleteLater(); m->deleteLater();
    });
}

void FriendsTab::onUserClicked(QListWidgetItem* item) {
    int id = item->data(Qt::UserRole).toInt();
    ProfileDialog d(m_token, id, m_myRole, this);
    d.exec();
}

void FriendsTab::addFriend() {
    auto item = m_searchResults->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/friends/add"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["friend_id"] = id;
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            loadFriends();
        }
        r->deleteLater(); m->deleteLater();
    });
}

void FriendsTab::removeFriend() {
    auto item = m_friendsList->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/friends/remove"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["friend_id"] = id;
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            loadFriends();
        }
        r->deleteLater(); m->deleteLater();
    });
}
