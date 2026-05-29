#include "main_window.h"

#include "active_contest_tab.h"
#include "admin_tab.h"
#include "api_config.h"
#include "app_style.h"
#include "archive_tab.h"
#include "contests_tab.h"
#include "friends_tab.h"
#include "network_utils.h"
#include "ratings_tab.h"
#include "results_tab.h"
#include "toast.h"
#include "users_tab.h"

#include <QApplication>
#include <QDebug>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QScreen>
#include <QSizePolicy>
#include <QStyle>
#include <QTabBar>
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

  const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
  const QSize startSize(qMin(1280, available.width() - 80),
                        qMin(820, available.height() - 80));
  resize(startSize.expandedTo(QSize(860, 580)));
  setMinimumSize(760, 520);

  QWidget *root = new QWidget(this);
  root->setObjectName("mainShell");
  QVBoxLayout *rootLayout = new QVBoxLayout(root);
  rootLayout->setContentsMargins(20, 20, 20, 20);
  rootLayout->setSpacing(14);

  QFrame *topBar = new QFrame(root);
  topBar->setObjectName("topBar");
  QHBoxLayout *topLayout = new QHBoxLayout(topBar);
  topLayout->setContentsMargins(22, 16, 22, 16);
  topLayout->setSpacing(14);

  QLabel *brandIcon = new QLabel(QStringLiteral("∑"), topBar);
  brandIcon->setObjectName("brandIcon");
  topLayout->addWidget(brandIcon, 0, Qt::AlignVCenter);

  QVBoxLayout *titleLayout = new QVBoxLayout();
  titleLayout->setContentsMargins(0, 0, 0, 0);
  titleLayout->setSpacing(2);

  QLabel *title = new QLabel("MathForces", topBar);
  title->setObjectName("appTitle");
  QLabel *subtitle = new QLabel(
      "Платформа для математических контестов, решений, разборов и рейтингов",
      topBar);
  subtitle->setObjectName("appSubtitle");
  subtitle->setWordWrap(true);
  titleLayout->addWidget(title);
  titleLayout->addWidget(subtitle);
  topLayout->addLayout(titleLayout, 1);

  QLabel *roleBadge = new QLabel("Роль: " + readableRole(role), topBar);
  roleBadge->setObjectName("roleBadge");
  topLayout->addWidget(roleBadge, 0, Qt::AlignVCenter);

  m_themeButton = new QPushButton(topBar);
  m_themeButton->setObjectName("themeToggle");
  updateThemeButtonText();
  topLayout->addWidget(m_themeButton, 0, Qt::AlignVCenter);
  connect(m_themeButton, &QPushButton::clicked, this, [this]() {
    AppStyle::toggleTheme(*qApp);
    updateThemeButtonText();
    Toast::show(this, "Тема переключена: " + AppStyle::themeLabel(AppStyle::savedTheme()));
  });

  rootLayout->addWidget(topBar);

  m_tabs = new QTabWidget(root);
  m_tabs->setObjectName("mainTabs");
  m_tabs->setDocumentMode(true);
  m_tabs->setMovable(false);
  m_tabs->setUsesScrollButtons(true);
  m_tabs->tabBar()->setExpanding(false);
  rootLayout->addWidget(m_tabs, 1);

  setCentralWidget(root);

  m_contestsTab = new ContestsTab(token, this);
  m_activeTab = new ActiveContestTab(token, role, this);
  m_resultsTab = new ResultsTab(token, role, this);
  m_friendsTab = new FriendsTab(token, role, this);
  m_ratingsTab = new RatingsTab(token, role, this);
  m_archiveTab = new ArchiveTab(token, this);

  m_tabs->addTab(m_contestsTab, AppStyle::standardIcon(QStyle::SP_FileDialogListView), "Контесты");
  m_tabs->addTab(m_activeTab, AppStyle::standardIcon(QStyle::SP_MediaPlay), "Текущий контест");
  m_tabs->addTab(m_resultsTab, AppStyle::standardIcon(QStyle::SP_FileDialogDetailedView), "Результаты");
  m_tabs->addTab(m_friendsTab, AppStyle::standardIcon(QStyle::SP_DirHomeIcon), "Сообщество");
  m_tabs->addTab(m_ratingsTab, AppStyle::standardIcon(QStyle::SP_ArrowUp), "Рейтинг");
  m_tabs->addTab(m_archiveTab, AppStyle::standardIcon(QStyle::SP_DirIcon), "Архив задач");

  if (role == "admin" || role == "superadmin") {
    m_adminTab = new AdminTab(token, this);
    m_tabs->addTab(m_adminTab, AppStyle::standardIcon(QStyle::SP_ComputerIcon), "Управление");
  }

  if (role == "superadmin" || role == "moderator") {
    m_usersTab = new UsersTab(token, role, this);
    m_tabs->addTab(m_usersTab, AppStyle::standardIcon(QStyle::SP_FileDialogInfoView), "Пользователи");
  }

  connect(m_contestsTab, &ContestsTab::contestSelected, this,
          &MainWindow::openContest);
  connect(m_contestsTab, &ContestsTab::startVirtualParticipation, this,
          [this](int cid) {
            QNetworkAccessManager *m = new QNetworkAccessManager(this);
            QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/contests/virtual"));
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            req.setRawHeader("Authorization", m_token.toUtf8());
            QJsonObject j;
            j["contest_id"] = cid;
            QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
            connect(r, &QNetworkReply::finished, [this, r, m, cid]() {
              if (r->error() == QNetworkReply::NoError) {
                Toast::show(this, "Виртуальное участие начато");
                openContest(cid, "Виртуальное участие");
              } else {
                NetworkUtils::showError(this, "Не удалось начать виртуальное участие", r);
              }
              r->deleteLater();
              m->deleteLater();
            });
          });
}

void MainWindow::updateThemeButtonText() {
  if (!m_themeButton)
    return;
  const bool dark = AppStyle::savedTheme() == AppStyle::Theme::Dark;
  m_themeButton->setText(dark ? QStringLiteral("☀ Светлая") : QStringLiteral("🌙 Тёмная"));
}

void MainWindow::openContest(int contestId, const QString &title) {
  m_activeTab->loadContest(contestId, title);
  m_resultsTab->loadResults(contestId);
  m_tabs->setCurrentWidget(m_activeTab);
}
