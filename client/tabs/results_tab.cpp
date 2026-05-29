#include "results_tab.h"
#include "api_config.h"
#include "profile_dialog.h"
#include "network_utils.h"
#include "table_utils.h"
#include "toast.h"
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
#include <QVBoxLayout>

ResultsTab::ResultsTab(const QString &token, const QString &myRole,
                       QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole) {
  QVBoxLayout *l = new QVBoxLayout(this);
  l->setContentsMargins(4, 4, 4, 4);
  l->setSpacing(14);
  QLabel *title = new QLabel("Таблица результатов", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel("Дважды нажмите на участника, чтобы открыть профиль.", this);
  hint->setObjectName("mutedLabel");
  QPushButton *btn = new QPushButton("Обновить результаты", this);
  m_table = new QTableWidget(0, 5, this);
  m_table->setHorizontalHeaderLabels(
      {"Место", "Участник", "Баллы", "Штраф (мин)", "Официальный"});
  TableUtils::prepareTable(m_table);
  l->addWidget(title);
  l->addWidget(hint);
  l->addWidget(btn, 0, Qt::AlignLeft);
  l->addWidget(TableUtils::attachSearch(m_table, this, "Поиск по результатам..."));
  l->addWidget(m_table);

  QHBoxLayout *adminL = new QHBoxLayout();
  QPushButton *btnRate =
      new QPushButton("Пересчитать Эло", this);
  adminL->addWidget(btnRate);
  adminL->addStretch();
  if (m_myRole == "admin" || m_myRole == "superadmin") {
    l->addLayout(adminL);
  } else {
    btnRate->hide();
  }

  connect(btn, &QPushButton::clicked, [this]() {
    if (m_currentContest != -1)
      loadResults(m_currentContest);
  });
  connect(btnRate, &QPushButton::clicked, this, &ResultsTab::rateContest);
  connect(m_table, &QTableWidget::cellDoubleClicked,
          [this](int row, int /*col*/) {
            int uId = m_table->item(row, 1)->data(Qt::UserRole).toInt();
            if (uId > 0) {
              ProfileDialog d(m_token, uId, m_myRole, this);
              d.exec();
            }
          });

  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, [this]() {
    if (m_currentContest != -1) {
      loadResults(m_currentContest);
    }
  });
  m_timer->start(10000); // Poll every 10 seconds
}

void ResultsTab::loadResults(int contestId) {
  qDebug() << "Client: Loading results for contest ID:" << contestId;
  m_currentContest = contestId;
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkReply *r = m->get(QNetworkRequest(
      QUrl(QString(ApiConfig::baseUrl + "/api/results?contest_id=%1")
               .arg(contestId))));
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      qDebug() << "Client: Results loaded successfully";
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_table->setSortingEnabled(false);
      m_table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        m_table->setItem(i, 0, TableUtils::numericItem(o["place"].toInt()));
        QTableWidgetItem *uItem =
            new QTableWidgetItem(o["username"].toString());
        uItem->setData(Qt::UserRole, o["user_id"].toInt());
        m_table->setItem(i, 1, uItem);
        m_table->setItem(i, 2, TableUtils::numericItem(o["total_score"].toInt()));
        m_table->setItem(i, 3, TableUtils::numericItem(o["penalty"].toInt()));
        m_table->setItem(i, 4, TableUtils::textItem(o["is_official"].toBool() ? "Да" : "Нет"));
      }
      m_table->setSortingEnabled(true);
    } else {
      qDebug() << "Client Error loading results:" << r->errorString();
      NetworkUtils::showError(this, "Не удалось загрузить результаты", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ResultsTab::rateContest() {
  if (m_currentContest == -1)
    return;
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/rate_contest"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["contest_id"] = m_currentContest;
  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      Toast::show(this, "Рейтинг контеста пересчитан");
      loadResults(m_currentContest);
    } else {
      NetworkUtils::showError(this, "Не удалось пересчитать рейтинг", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}
