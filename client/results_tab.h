#pragma once
#include <QTableView>
#include <QTimer>
#include <QWidget>
#include "mvc/ResultsModel.h"
#include "mvc/ResultsPresenter.h"

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
  QTableView *m_tableView;
  ResultsModel *m_model;
  ResultsPresenter *m_presenter;
  
  int m_currentContest = -1;
  QTimer *m_timer;
};
