#include "api_config.h"
#include "active_contest_tab.h"
#include "admin_tab.h"
#include "archive_tab.h"
#include "contests_tab.h"
#include "friends_tab.h"
#include "main_window.h"
#include "ratings_tab.h"
#include "results_tab.h"
#include "users_tab.h"
#include <QDebug>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSizePolicy>
#include <QVBoxLayout>

static QString readableRole(const QString &role) {
  if (role == "superadmin")
    return "Суперадмин";
  if (role == "admin")
    return "Администратор";
  if (role == "moderator")
    return "Модератор";
  return "Участник";
}

MainWindow::MainWindow(const QString &token, const QString &role,
                       QWidget *parent)
    : QMainWindow(parent), m_token(token), m_role(role) {
  setWindowTitle("MathForces - математические контесты");
  setMinimumSize(1100, 720);

  QWidget *root = new QWidget(this);
  root->setObjectName("mainShell");
  QVBoxLayout *rootLayout = new QVBoxLayout(root);
  rootLayout->setContentsMargins(24, 24, 24, 24);
  rootLayout->setSpacing(16);

  QFrame *topBar = new QFrame(root);
  topBar->setObjectName("topBar");
  QHBoxLayout *topLayout = new QHBoxLayout(topBar);
  topLayout->setContentsMargins(22, 18, 22, 18);
  topLayout->setSpacing(16);

  QLabel *brandIcon = new QLabel(QStringLiteral("∑"), topBar);
  brandIcon->setObjectName("brandIcon");
  topLayout->addWidget(brandIcon, 0, Qt::AlignVCenter);

  QVBoxLayout *titleLayout = new QVBoxLayout();
  titleLayout->setContentsMargins(0, 0, 0, 0);
  titleLayout->setSpacing(2);

  QLabel *title = new QLabel("MathForces", topBar);
  title->setObjectName("appTitle");
  QLabel *subtitle = new QLabel(
      "Тёплая платформа для математических контестов, решений и рейтингов",
      topBar);
  subtitle->setObjectName("appSubtitle");
  titleLayout->addWidget(title);
  titleLayout->addWidget(subtitle);
  topLayout->addLayout(titleLayout, 1);

  QLabel *roleBadge = new QLabel("Роль: " + readableRole(role), topBar);
  roleBadge->setObjectName("roleBadge");
  topLayout->addWidget(roleBadge, 0, Qt::AlignVCenter);

  rootLayout->addWidget(topBar);

  m_tabs = new QTabWidget(root);
  m_tabs->setObjectName("mainTabs");
  m_tabs->setDocumentMode(true);
  m_tabs->setMovable(false);
  rootLayout->addWidget(m_tabs, 1);

  setCentralWidget(root);

  m_contestsTab = new ContestsTab(token, this);
  m_activeTab = new ActiveContestTab(token, role, this);
  m_resultsTab = new ResultsTab(token, role, this);
  m_friendsTab = new FriendsTab(token, role, this);
  m_ratingsTab = new RatingsTab(token, role, this);
  m_archiveTab = new ArchiveTab(token, this);

  m_tabs->addTab(m_contestsTab, "Контесты");
  m_tabs->addTab(m_activeTab, "Текущий контест");
  m_tabs->addTab(m_resultsTab, "Результаты");
  m_tabs->addTab(m_friendsTab, "Сообщество");
  m_tabs->addTab(m_ratingsTab, "Рейтинг");
  m_tabs->addTab(m_archiveTab, "Архив задач");

  if (role == "admin" || role == "superadmin") {
    m_adminTab = new AdminTab(token, this);
    m_tabs->addTab(m_adminTab, "Управление");
  }

  if (role == "superadmin" || role == "moderator") {
    m_usersTab = new UsersTab(token, role, this);
    m_tabs->addTab(m_usersTab, "Пользователи");
  }

  connect(m_contestsTab, &ContestsTab::contestSelected, this,
          &MainWindow::openContest);
  connect(
      m_contestsTab, &ContestsTab::virtualReadyToOpen, this,
      [this](int cid) {
          openContest(cid, "Виртуальное участие");
      });
}

void MainWindow::openContest(int contestId, const QString &title) {
  m_activeTab->loadContest(contestId, title);
  m_resultsTab->loadResults(contestId);
  m_tabs->setCurrentWidget(m_activeTab);
}
