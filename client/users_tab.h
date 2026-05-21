#ifndef USERS_TAB_H
#define USERS_TAB_H

#include <QString>
#include <QTableView>
#include <QWidget>
#include "mvc/UsersTableModel.h"
#include "mvc/UsersPresenter.h"

class UsersTab : public QWidget {
  Q_OBJECT
public:
  UsersTab(const QString &token, const QString &myRole,
           QWidget *parent = nullptr);

public slots:
  void loadUsers();

private slots:
  void applyRoleChange(int row);
  void applyBanChange(int row);
  void applyBlogChange(int row);
  void onCustomContextMenuRequired(const QPoint& pos);

private:
  QString m_token;
  QString m_myRole;
  QTableView *m_tableView;
  UsersTableModel *m_model;
  UsersPresenter *m_presenter;
};

#endif // USERS_TAB_H
