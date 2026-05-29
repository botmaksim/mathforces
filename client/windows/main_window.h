#pragma once
#include <QMainWindow>
#include <QString>
#include <QTabWidget>

class ContestsTab;
class ActiveContestTab;
class ResultsTab;
class AdminTab;
class FriendsTab;
class RatingsTab;
class UsersTab;
class ArchiveTab;
class QPushButton;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(const QString &token, const QString &role,
             QWidget *parent = nullptr);

public slots:
  void openContest(int contestId, const QString &title);

private:
  void updateThemeButtonText();

  QTabWidget *m_tabs;
  QPushButton *m_themeButton;
  ContestsTab *m_contestsTab;
  ActiveContestTab *m_activeTab;
  ResultsTab *m_resultsTab;
  FriendsTab *m_friendsTab;
  RatingsTab *m_ratingsTab;
  ArchiveTab *m_archiveTab;
  AdminTab *m_adminTab = nullptr;
  UsersTab *m_usersTab = nullptr;

  QString m_token;
  QString m_role;
};
