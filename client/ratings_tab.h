#ifndef RATINGS_TAB_H
#define RATINGS_TAB_H

#include <QString>
#include <QTableView>
#include <QWidget>
#include "mvc/RatingModel.h"
#include "mvc/RatingsPresenter.h"

class RatingsTab : public QWidget {
  Q_OBJECT
public:
  RatingsTab(const QString &token, const QString &myRole,
             QWidget *parent = nullptr);

public slots:
  void loadRatings();

private:
  QString m_token;
  QString m_myRole;
  QTableView *m_tableView;
  RatingModel *m_model;
  RatingsPresenter *m_presenter;
};

#endif // RATINGS_TAB_H
