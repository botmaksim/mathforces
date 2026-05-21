#include "archive_tab.h"
#include <QHeaderView>

ArchiveTab::ArchiveTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(4, 4, 4, 4);
  mainLayout->setSpacing(14);

  QHBoxLayout *filterLayout = new QHBoxLayout();
  filterLayout->setSpacing(10);
  m_filterTags = new QLineEdit(this);
  m_filterTags->setPlaceholderText("Теги (через запятую)...");

  m_filterMinDiff = new QLineEdit(this);
  m_filterMinDiff->setPlaceholderText("Мин. сложность");

  m_filterMaxDiff = new QLineEdit(this);
  m_filterMaxDiff->setPlaceholderText("Макс. сложность");

  m_btnFilter = new QPushButton("Найти задачи", this);

  filterLayout->addWidget(m_filterTags);
  filterLayout->addWidget(m_filterMinDiff);
  filterLayout->addWidget(m_filterMaxDiff);
  filterLayout->addWidget(m_btnFilter);

  m_tableView = new QTableView(this);
  m_model = new ArchiveModel(this);
  m_presenter = new ArchivePresenter(m_model, m_token, this);
  
  m_tableView->setModel(m_model);
  m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_tableView->setAlternatingRowColors(true);

  mainLayout->addLayout(filterLayout);
  mainLayout->addWidget(m_tableView);

  connect(m_btnFilter, &QPushButton::clicked, this, &ArchiveTab::applyFilter);
  connect(m_presenter, &ArchivePresenter::errorOccurred, this, [](const QString& err){
      qDebug() << "Archive error:" << err;
  });
  connect(m_tableView, &QTableView::doubleClicked, [this](const QModelIndex& index) {
      int taskId = m_model->getTaskId(index.row());
      if (taskId > 0) openTask(taskId);
  });
}

void ArchiveTab::loadTasks() { applyFilter(); }

void ArchiveTab::applyFilter() {
  m_presenter->loadTasks(m_filterTags->text(), m_filterMinDiff->text(), m_filterMaxDiff->text());
}

void ArchiveTab::openTask(int taskId) {
  // В данном прототипе покажем ID задачи при двойном клике
  QMessageBox::information(this, "Задача",
                           "Открытие задачи ID: " + QString::number(taskId));
}
