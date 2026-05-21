#ifndef FRIENDS_TAB_H
#define FRIENDS_TAB_H

#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include "mvc/UsersListModel.h"
#include "mvc/FriendsPresenter.h"

class FriendsTab : public QWidget {
  Q_OBJECT
public:
  explicit FriendsTab(const QString &token, const QString &myRole,
                      QWidget *parent = nullptr);

private slots:
  void searchUsers();
  void loadFriends();
  void onSearchUserDoubleClicked(const QModelIndex& index);
  void onFriendDoubleClicked(const QModelIndex& index);
  void addFriend();
  void removeFriend();

private:
  QString m_token;
  QString m_myRole;

  QLineEdit *m_searchEdit;
  QPushButton *m_btnSearch;
  QListView *m_searchResults;
  UsersListModel *m_searchModel;

  QPushButton *m_btnAddFriend;

  QListView *m_friendsList;
  UsersListModel *m_friendsModel;
  QPushButton *m_btnRemoveFriend;

  FriendsPresenter *m_presenter;
};

#endif // FRIENDS_TAB_H
