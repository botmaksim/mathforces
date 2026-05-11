#pragma once
#include <QWidget>
#include <QTableWidget>

class ResultsTab : public QWidget {
    Q_OBJECT
public:
    ResultsTab(const QString& token, QWidget* parent = nullptr);
    void loadResults(int contestId);
private:
    QString m_token;
    QTableWidget* m_table;
    int m_currentContest = -1;
};
