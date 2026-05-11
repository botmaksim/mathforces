#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QString>

class ContestsTab;
class ActiveContestTab;
class ResultsTab;
class AdminTab;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(const QString& token, const QString& role, QWidget *parent = nullptr);

public slots:
    void openContest(int contestId, const QString& title);

private:
    QTabWidget* m_tabs;
    ContestsTab* m_contestsTab;
    ActiveContestTab* m_activeTab;
    ResultsTab* m_resultsTab;
    AdminTab* m_adminTab;
    
    QString m_token;
    QString m_role;
};
