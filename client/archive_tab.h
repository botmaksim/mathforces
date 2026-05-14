#ifndef ARCHIVE_TAB_H
#define ARCHIVE_TAB_H

#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

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
  QTableWidget *m_table;
  QLineEdit *m_filterTags;
  QLineEdit *m_filterMinDiff;
  QLineEdit *m_filterMaxDiff;
  QPushButton *m_btnFilter;
};

#endif // ARCHIVE_TAB_H
