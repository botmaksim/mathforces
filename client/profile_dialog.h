#ifndef PROFILE_DIALOG_H
#define PROFILE_DIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include "mvc/ProfilePresenter.h"

class ProfileDialog : public QDialog {
  Q_OBJECT
public:
  ProfileDialog(const QString &token, int targetUserId, const QString &myRole,
                QWidget *parent = nullptr);

private slots:
  void loadProfile();
  void loadBlogPosts();
  void addBlogPost();
  void showComments(int postId);

private:
  QString m_token;
  int m_targetUserId;
  QString m_myRole;
  bool m_isSelf;
  bool m_canBlog;
  int m_myUserId;

  QLabel *m_lblUsername;
  QLabel *m_lblName;
  QLabel *m_lblRating;
  QLabel *m_lblEmail;

  QWidget *m_blogContainer;
  QVBoxLayout *m_blogLayout;

  QTextEdit *m_txtNewPost;
  QPushButton *m_btnPost;

  ProfilePresenter *m_presenter;

  void fetchMyId();
};

#endif // PROFILE_DIALOG_H
