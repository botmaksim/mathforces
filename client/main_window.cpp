#include "main_window.h"
#include "contests_tab.h"
#include "active_contest_tab.h"
#include "results_tab.h"
#include "admin_tab.h"
#include "users_tab.h"
#include "friends_tab.h"
#include "ratings_tab.h"
#include "archive_tab.h"

MainWindow::MainWindow(const QString& token, const QString& role, QWidget *parent) 
    : QMainWindow(parent), m_token(token), m_role(role) 
{
    setWindowTitle("Mathforces Client");
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_contestsTab = new ContestsTab(token, this);
    m_activeTab = new ActiveContestTab(token, this);
    m_resultsTab = new ResultsTab(token, role, this);
    m_friendsTab = new FriendsTab(token, role, this);
    m_ratingsTab = new RatingsTab(token, role, this);
    m_archiveTab = new ArchiveTab(token, this);

    m_tabs->addTab(m_contestsTab, "Контесты");
    m_tabs->addTab(m_activeTab, "Текущий Контест");
    m_tabs->addTab(m_resultsTab, "Результаты");
    m_tabs->addTab(m_friendsTab, "Сообщество");
    m_tabs->addTab(m_ratingsTab, "Рейтинг");
    m_tabs->addTab(m_archiveTab, "Архив");
    
    // Вкладка активного контеста пока скрыта (или просто недоступна), 
    // пока пользователь не войдет в контест. Для простоты оставим их все видимыми,
    // но заполняются они при клике.
    
    if (role == "admin" || role == "superadmin") {
        m_adminTab = new AdminTab(token, this);
        m_tabs->addTab(m_adminTab, "Создание и Управление");
    }
    
    if (role == "superadmin" || role == "moderator") {
        m_usersTab = new UsersTab(token, role, this);
        m_tabs->addTab(m_usersTab, "Пользователи");
    }

    connect(m_contestsTab, &ContestsTab::contestSelected, this, &MainWindow::openContest);
    connect(m_contestsTab, &ContestsTab::startVirtualParticipation, this, [this](int cid) {
        // Here we could call API to register virtual participation
        QNetworkAccessManager* m = new QNetworkAccessManager(this);
        QNetworkRequest req(QUrl("http://localhost:8080/api/contests/virtual"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", m_token.toUtf8());
        QJsonObject j; j["contest_id"] = cid;
        QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
        connect(r, &QNetworkReply::finished, [this, r, m, cid]() {
            if (r->error() == QNetworkReply::NoError) {
                openContest(cid, "Virtual Participation");
            } else {
                qDebug() << "Failed to start virtual participation";
            }
            r->deleteLater(); m->deleteLater();
        });
    });
}

void MainWindow::openContest(int contestId, const QString& title) {
    m_activeTab->loadContest(contestId, title);
    m_resultsTab->loadResults(contestId);
    m_tabs->setCurrentWidget(m_activeTab);
}
