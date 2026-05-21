#pragma once
#include <QWidget>
#include <QTableView>
#include "mvc/ContestsPresenter.h"
#include "mvc/ContestModel.h"

class ContestsTab : public QWidget {
  Q_OBJECT
public:
  ContestsTab(const QString &token, QWidget *parent = nullptr);
signals:
  void contestSelected(int id, const QString &title);
  void virtualReadyToOpen(int contestId);
private:
  QString m_token;
  QTableView *m_tableView;
  ContestModel *m_model;
  ContestsPresenter *m_presenter;
};
