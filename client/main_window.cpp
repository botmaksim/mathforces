#include "main_window.h"
#include "contests_tab.h"
#include "active_contest_tab.h"
#include "results_tab.h"
#include "admin_tab.h"

MainWindow::MainWindow(const QString& token, const QString& role, QWidget *parent) 
    : QMainWindow(parent), m_token(token), m_role(role) 
{
    setWindowTitle("Mathforces Client");
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_contestsTab = new ContestsTab(token, this);
    m_activeTab = new ActiveContestTab(token, this);
    m_resultsTab = new ResultsTab(token, this);

    m_tabs->addTab(m_contestsTab, "Контесты");
    m_tabs->addTab(m_activeTab, "Текущий Контест");
    m_tabs->addTab(m_resultsTab, "Результаты");
    
    // Вкладка активного контеста пока скрыта (или просто недоступна), 
    // пока пользователь не войдет в контест. Для простоты оставим их все видимыми,
    // но заполняются они при клике.
    
    if (role == "admin") {
        m_adminTab = new AdminTab(token, this);
        m_tabs->addTab(m_adminTab, "Администрирование");
    }

    connect(m_contestsTab, &ContestsTab::contestSelected, this, &MainWindow::openContest);
}

void MainWindow::openContest(int contestId, const QString& title) {
    m_activeTab->loadContest(contestId, title);
    m_resultsTab->loadResults(contestId);
    m_tabs->setCurrentWidget(m_activeTab);
}
