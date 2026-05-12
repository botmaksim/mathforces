#include "archive_tab.h"
#include <QHeaderView>

ArchiveTab::ArchiveTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  QHBoxLayout *filterLayout = new QHBoxLayout();
  m_filterTags = new QLineEdit(this);
  m_filterTags->setPlaceholderText("Теги (через запятую)...");

  m_filterMinDiff = new QLineEdit(this);
  m_filterMinDiff->setPlaceholderText("Мин. сложность");

  m_filterMaxDiff = new QLineEdit(this);
  m_filterMaxDiff->setPlaceholderText("Макс. сложность");

  m_btnFilter = new QPushButton("Искать", this);

  filterLayout->addWidget(m_filterTags);
  filterLayout->addWidget(m_filterMinDiff);
  filterLayout->addWidget(m_filterMaxDiff);
  filterLayout->addWidget(m_btnFilter);

  m_table = new QTableWidget(0, 4, this);
  m_table->setHorizontalHeaderLabels({"ID", "Название", "Теги", "Сложность"});
  m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

  mainLayout->addLayout(filterLayout);
  mainLayout->addWidget(m_table);

  connect(m_btnFilter, &QPushButton::clicked, this, &ArchiveTab::applyFilter);
  connect(m_table, &QTableWidget::cellDoubleClicked, [this](int row, int col) {
    openTask(m_table->item(row, 0)->text().toInt());
  });
}

void ArchiveTab::loadTasks() { applyFilter(); }

void ArchiveTab::applyFilter() {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QString urlStr = "http://localhost:8080/api/archive/tasks?";

  if (!m_filterTags->text().isEmpty()) {
    urlStr += "tags=" + m_filterTags->text() + "&";
  }
  if (!m_filterMinDiff->text().isEmpty()) {
    urlStr += "min_diff=" + m_filterMinDiff->text() + "&";
  }
  if (!m_filterMaxDiff->text().isEmpty()) {
    urlStr += "max_diff=" + m_filterMaxDiff->text() + "&";
  }

  QUrl url(urlStr);
  QNetworkRequest req(url);
  QNetworkReply *r = m->get(req);

  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        m_table->setItem(
            i, 0, new QTableWidgetItem(QString::number(o["id"].toInt())));
        m_table->setItem(i, 1, new QTableWidgetItem(o["title"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(o["tags"].toString()));
        m_table->setItem(
            i, 3,
            new QTableWidgetItem(QString::number(o["difficulty"].toInt())));
      }
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ArchiveTab::openTask(int taskId) {
  // В данном прототипе покажем ID задачи при двойном клике
  // Полноценное открытие задачи похоже на ActiveContestTab, но вне контеста.
  QMessageBox::information(this, "Задача",
                           "Открытие задачи ID: " + QString::number(taskId));
}
