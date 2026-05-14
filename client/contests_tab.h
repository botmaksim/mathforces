#pragma once
#include <QListWidget>
#include <QWidget>

class ContestsTab : public QWidget {
  Q_OBJECT
public:
  ContestsTab(const QString &token, QWidget *parent = nullptr);
signals:
  void contestSelected(int id, const QString &title);
  void startVirtualParticipation(int contestId);
private slots:
  void load();

private:
  QString m_token;
  QListWidget *m_list;
};
