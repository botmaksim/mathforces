#include "friends_tab.h"
#include "profile_dialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDebug>

FriendsTab::FriendsTab(const QString &token, const QString &myRole,
                       QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole) {
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(16);

  // Left side: Search
  QVBoxLayout *leftLayout = new QVBoxLayout();
  leftLayout->setSpacing(12);
  QLabel *searchTitle = new QLabel("Найти участника", this);
  searchTitle->setObjectName("sectionTitle");
  QLabel *searchHint = new QLabel("Ищите по имени или username, затем добавляйте в друзья.", this);
  searchHint->setObjectName("mutedLabel");
  searchHint->setWordWrap(true);
  QHBoxLayout *searchLayout = new QHBoxLayout();
  searchLayout->setSpacing(10);
  m_searchEdit = new QLineEdit();
  m_searchEdit->setPlaceholderText("Имя или Username");
  m_btnSearch = new QPushButton("Искать");
  searchLayout->addWidget(m_searchEdit);
  searchLayout->addWidget(m_btnSearch);

  m_searchResults = new QListView();
  m_searchModel = new UsersListModel(this);
  m_searchResults->setModel(m_searchModel);
  m_searchResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
  
  m_btnAddFriend = new QPushButton("Добавить в друзья");

  leftLayout->addWidget(searchTitle);
  leftLayout->addWidget(searchHint);
  leftLayout->addLayout(searchLayout);
  leftLayout->addWidget(m_searchResults);
  leftLayout->addWidget(m_btnAddFriend);

  // Right side: Friends
  QVBoxLayout *rightLayout = new QVBoxLayout();
  rightLayout->setSpacing(12);
  QLabel *friendsTitle = new QLabel("Мои друзья", this);
  friendsTitle->setObjectName("sectionTitle");
  
  m_friendsList = new QListView();
  m_friendsModel = new UsersListModel(this);
  m_friendsList->setModel(m_friendsModel);
  m_friendsList->setEditTriggers(QAbstractItemView::NoEditTriggers);
  
  m_btnRemoveFriend = new QPushButton("Удалить из друзей");
  QPushButton *btnRefresh = new QPushButton("Обновить друзей");

  rightLayout->addWidget(friendsTitle);
  rightLayout->addWidget(btnRefresh, 0, Qt::AlignLeft);
  rightLayout->addWidget(m_friendsList);
  rightLayout->addWidget(m_btnRemoveFriend);

  mainLayout->addLayout(leftLayout, 1);
  mainLayout->addLayout(rightLayout, 1);

  m_presenter = new FriendsPresenter(m_searchModel, m_friendsModel, m_token, this);

  connect(m_btnSearch, &QPushButton::clicked, this, &FriendsTab::searchUsers);
  connect(btnRefresh, &QPushButton::clicked, this, &FriendsTab::loadFriends);
  connect(m_searchResults, &QListView::doubleClicked, this,
          &FriendsTab::onSearchUserDoubleClicked);
  connect(m_friendsList, &QListView::doubleClicked, this,
          &FriendsTab::onFriendDoubleClicked);
  connect(m_btnAddFriend, &QPushButton::clicked, this, &FriendsTab::addFriend);
  connect(m_btnRemoveFriend, &QPushButton::clicked, this,
          &FriendsTab::removeFriend);
  
  connect(m_presenter, &FriendsPresenter::errorOccurred, this, [](const QString& err){
      qDebug() << "Friends error:" << err;
  });

  loadFriends();
}

void FriendsTab::searchUsers() {
  m_presenter->searchUsers(m_searchEdit->text());
}

void FriendsTab::loadFriends() {
  m_presenter->loadFriends();
}

void FriendsTab::onSearchUserDoubleClicked(const QModelIndex& index) {
  int id = m_searchModel->getUserId(index.row());
  if (id > 0) {
      ProfileDialog d(m_token, id, m_myRole, this);
      d.exec();
  }
}

void FriendsTab::onFriendDoubleClicked(const QModelIndex& index) {
  int id = m_friendsModel->getUserId(index.row());
  if (id > 0) {
      ProfileDialog d(m_token, id, m_myRole, this);
      d.exec();
  }
}

void FriendsTab::addFriend() {
  auto index = m_searchResults->currentIndex();
  if (index.isValid()) {
      m_presenter->addFriend(m_searchModel->getUserId(index.row()));
  }
}

void FriendsTab::removeFriend() {
  auto index = m_friendsList->currentIndex();
  if (index.isValid()) {
      m_presenter->removeFriend(m_friendsModel->getUserId(index.row()));
  }
}
