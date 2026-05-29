#include "ratings_tab.h"
#include "api_config.h"
#include "profile_dialog.h"
#include "network_utils.h"
#include "table_utils.h"
#include <QDebug>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

RatingsTab::RatingsTab(const QString &token, const QString &myRole,
                       QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole) {
  QVBoxLayout *l = new QVBoxLayout(this);
  l->setContentsMargins(4, 4, 4, 4);
  l->setSpacing(14);
  QLabel *title = new QLabel("Глобальный рейтинг", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel("Рейтинг обновляется автоматически. Двойной клик открывает профиль участника.", this);
  hint->setObjectName("mutedLabel");
  hint->setWordWrap(true);
  QPushButton *btn = new QPushButton("Обновить рейтинг", this);
  m_table = new QTableWidget(0, 4, this);
  m_table->setHorizontalHeaderLabels(
      {"Место", "Пользователь", "Имя", "Рейтинг"});
  TableUtils::prepareTable(m_table);
  l->addWidget(title);
  l->addWidget(hint);
  l->addWidget(btn, 0, Qt::AlignLeft);
  l->addWidget(TableUtils::attachSearch(m_table, this, "Поиск по рейтингу..."));
  l->addWidget(m_table);

  connect(btn, &QPushButton::clicked, this, &RatingsTab::loadRatings);
  connect(m_table, &QTableWidget::cellDoubleClicked,
          [this](int row, int /*col*/) {
            int uId = m_table->item(row, 1)->data(Qt::UserRole).toInt();
            if (uId > 0) {
              ProfileDialog d(m_token, uId, m_myRole, this);
              d.exec();
            }
          });

  QTimer *m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this, &RatingsTab::loadRatings);
  m_timer->start(30000); // Poll every 30 seconds
}

void RatingsTab::loadRatings() {
  qDebug() << "Client: Loading ratings";
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkReply *r =
      m->get(QNetworkRequest(QUrl(ApiConfig::baseUrl + "/api/ratings")));
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_table->setSortingEnabled(false);
      m_table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        m_table->setItem(i, 0, TableUtils::numericItem(o["place"].toInt()));
        QTableWidgetItem *uItem =
            new QTableWidgetItem(o["username"].toString());
        uItem->setData(Qt::UserRole, o["id"].toInt());
        m_table->setItem(i, 1, uItem);
        m_table->setItem(i, 2, TableUtils::textItem(o["name"].toString()));
        m_table->setItem(i, 3, TableUtils::numericItem(o["rating"].toInt()));
      }
      m_table->setSortingEnabled(true);
    } else {
      qDebug() << "Client Error loading ratings:" << r->errorString();
      NetworkUtils::showError(this, "Не удалось загрузить рейтинг", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}
