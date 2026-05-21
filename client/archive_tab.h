#ifndef ARCHIVE_TAB_H
#define ARCHIVE_TAB_H

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>
#include "mvc/ArchiveModel.h"
#include "mvc/ArchivePresenter.h"

class ArchiveTab : public QWidget {
  Q_OBJECT
public:
  ArchiveTab(const QString &token, QWidget *parent = nullptr);
  void loadTasks();
private slots:
  void applyFilter();
  void openTask(int taskId);

private:
  QString m_token;
  QTableView *m_tableView;
  ArchiveModel *m_model;
  ArchivePresenter *m_presenter;
  
  QLineEdit *m_filterTags;
  QLineEdit *m_filterMinDiff;
  QLineEdit *m_filterMaxDiff;
  QPushButton *m_btnFilter;
};

#endif // ARCHIVE_TAB_H
