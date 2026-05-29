#include "archive_tab.h"

#include "api_config.h"
#include "archive_task_dialog.h"
#include "network_utils.h"
#include "table_utils.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

ArchiveTab::ArchiveTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(14);

  QLabel *title = new QLabel("Архив задач", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel(
      "Ищите задачи по тегам и сложности. Двойной клик открывает полноценную карточку задачи с условием, предпросмотром и отправкой решения.",
      this);
  hint->setObjectName("mutedLabel");
  hint->setWordWrap(true);

  QHBoxLayout *filterLayout = new QHBoxLayout();
  filterLayout->setSpacing(10);
  m_filterTags = new QLineEdit(this);
  m_filterTags->setClearButtonEnabled(true);
  m_filterTags->setPlaceholderText("Теги (geometry, algebra)...");

  m_filterMinDiff = new QLineEdit(this);
  m_filterMinDiff->setClearButtonEnabled(true);
  m_filterMinDiff->setPlaceholderText("Мин. сложность");

  m_filterMaxDiff = new QLineEdit(this);
  m_filterMaxDiff->setClearButtonEnabled(true);
  m_filterMaxDiff->setPlaceholderText("Макс. сложность");

  m_btnFilter = new QPushButton("Найти задачи", this);

  filterLayout->addWidget(m_filterTags, 2);
  filterLayout->addWidget(m_filterMinDiff, 1);
  filterLayout->addWidget(m_filterMaxDiff, 1);
  filterLayout->addWidget(m_btnFilter);

  m_table = new QTableWidget(0, 4, this);
  m_table->setHorizontalHeaderLabels({"ID", "Название", "Теги", "Сложность"});
  TableUtils::prepareTable(m_table);

  m_tableSearch = TableUtils::attachSearch(m_table, this, "Поиск в найденных задачах...");

  mainLayout->addWidget(title);
  mainLayout->addWidget(hint);
  mainLayout->addLayout(filterLayout);
  mainLayout->addWidget(m_tableSearch);
  mainLayout->addWidget(m_table, 1);

  connect(m_btnFilter, &QPushButton::clicked, this, &ArchiveTab::applyFilter);
  connect(m_filterTags, &QLineEdit::returnPressed, this, &ArchiveTab::applyFilter);
  connect(m_filterMinDiff, &QLineEdit::returnPressed, this, &ArchiveTab::applyFilter);
  connect(m_filterMaxDiff, &QLineEdit::returnPressed, this, &ArchiveTab::applyFilter);
  connect(m_table, &QTableWidget::cellDoubleClicked, [this](int row, int) {
    if (m_table->item(row, 0))
      openTask(m_table->item(row, 0)->text().toInt());
  });

  loadTasks();
}

void ArchiveTab::loadTasks() { applyFilter(); }

void ArchiveTab::applyFilter() {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QUrl url(ApiConfig::baseUrl + "/api/archive/tasks");
  QUrlQuery query;
  if (!m_filterTags->text().trimmed().isEmpty())
    query.addQueryItem("tags", m_filterTags->text().trimmed());
  if (!m_filterMinDiff->text().trimmed().isEmpty())
    query.addQueryItem("min_diff", m_filterMinDiff->text().trimmed());
  if (!m_filterMaxDiff->text().trimmed().isEmpty())
    query.addQueryItem("max_diff", m_filterMaxDiff->text().trimmed());
  url.setQuery(query);

  QNetworkRequest req(url);
  req.setRawHeader("Authorization", m_token.toUtf8());
  QNetworkReply *r = m->get(req);

  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_table->setSortingEnabled(false);
      m_table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        m_table->setItem(i, 0, TableUtils::numericItem(o["id"].toInt()));
        m_table->setItem(i, 1, TableUtils::textItem(o["title"].toString()));
        m_table->setItem(i, 2, TableUtils::textItem(o["tags"].toString()));
        m_table->setItem(i, 3, TableUtils::numericItem(o["difficulty"].toInt()));
      }
      m_table->setSortingEnabled(true);
    } else {
      NetworkUtils::showError(this, "Не удалось загрузить архив", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ArchiveTab::openTask(int taskId) {
  ArchiveTaskDialog dialog(m_token, taskId, this);
  dialog.exec();
}
