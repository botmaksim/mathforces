#ifndef RATINGS_TAB_H
#define RATINGS_TAB_H

#include <QString>
#include <QTableWidget>
#include <QWidget>

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
  QTableWidget *m_table;
};

#endif // RATINGS_TAB_H
