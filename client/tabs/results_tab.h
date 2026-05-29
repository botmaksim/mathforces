#pragma once
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

class ResultsTab : public QWidget {
  Q_OBJECT
public:
  ResultsTab(const QString &token, const QString &myRole,
             QWidget *parent = nullptr);
  void loadResults(int contestId);
  void rateContest();

private:
  QString m_token;
  QString m_myRole;
  QTableWidget *m_table;
  int m_currentContest = -1;
  QTimer *m_timer;
};
