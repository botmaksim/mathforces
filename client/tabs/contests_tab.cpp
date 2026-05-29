#include "contests_tab.h"
#include "api_config.h"
#include "network_utils.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ContestsTab::ContestsTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
  QVBoxLayout *l = new QVBoxLayout(this);
  l->setContentsMargins(4, 4, 4, 4);
  l->setSpacing(14);

  QLabel *title = new QLabel("Ближайшие контесты", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel("Дважды нажмите на контест, чтобы открыть задачи. Для тренировки выберите виртуальное участие.", this);
  hint->setObjectName("mutedLabel");
  hint->setWordWrap(true);

  QHBoxLayout *top = new QHBoxLayout();
  top->setSpacing(10);
  QPushButton *btn = new QPushButton("Обновить список", this);
  QPushButton *btnVirtual = new QPushButton("Начать виртуально", this);
  top->addWidget(btn);
  top->addWidget(btnVirtual);
  top->addStretch();

  QLineEdit *search = new QLineEdit(this);
  search->setClearButtonEnabled(true);
  search->setPlaceholderText("Поиск контеста...");

  m_list = new QListWidget(this);
  l->addWidget(title);
  l->addWidget(hint);
  l->addLayout(top);
  l->addWidget(search);
  l->addWidget(m_list);
  connect(btn, &QPushButton::clicked, this, &ContestsTab::load);
  connect(search, &QLineEdit::textChanged, this, [this](const QString &text) {
    for (int i = 0; i < m_list->count(); ++i) {
      QListWidgetItem *item = m_list->item(i);
      item->setHidden(!text.trimmed().isEmpty() &&
                      !item->text().contains(text, Qt::CaseInsensitive));
    }
  });
  connect(m_list, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *it) {
    qDebug() << "Client: Selected contest ID:"
             << it->data(Qt::UserRole).toInt();
    emit contestSelected(it->data(Qt::UserRole).toInt(), it->text());
  });

  connect(btnVirtual, &QPushButton::clicked, [this]() {
    if (!m_list->currentItem())
      return;
    int cid = m_list->currentItem()->data(Qt::UserRole).toInt();
    emit startVirtualParticipation(cid);
  });

  load();
}

void ContestsTab::load() {
  qDebug() << "Client: Loading contests";
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/contests"));
  if (!m_token.isEmpty())
    req.setRawHeader("Authorization", m_token.toUtf8());
  QNetworkReply *r = m->get(req);
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    m_list->clear();
    if (r->error() == QNetworkReply::NoError) {
      qDebug() << "Client: Contests loaded successfully";
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      for (auto v : arr) {
        QJsonObject o = v.toObject();
        QListWidgetItem *item = new QListWidgetItem(
            o["title"].toString() + " (" + o["start_time"].toString() + ")");
        item->setData(Qt::UserRole, o["id"].toInt());
        m_list->addItem(item);
      }
    } else {
      qDebug() << "Client Error loading contests:" << r->errorString();
      NetworkUtils::showError(this, "Не удалось загрузить контесты", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}
