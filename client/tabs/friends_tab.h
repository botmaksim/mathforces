#ifndef FRIENDS_TAB_H
#define FRIENDS_TAB_H

#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QWidget>

class FriendsTab : public QWidget {
  Q_OBJECT
public:
  explicit FriendsTab(const QString &token, const QString &myRole,
                      QWidget *parent = nullptr);

private slots:
  void searchUsers();
  void loadFriends();
  void onUserClicked(class QListWidgetItem *item);
  void addFriend();
  void removeFriend();

private:
  QString m_token;
  QString m_myRole;

  QLineEdit *m_searchEdit;
  QPushButton *m_btnSearch;
  QListWidget *m_searchResults;

  QPushButton *m_btnAddFriend;

  QListWidget *m_friendsList;
  QPushButton *m_btnRemoveFriend;
};

#endif // FRIENDS_TAB_H
