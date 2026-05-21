#include "results_tab.h"
#include "profile_dialog.h"
#include <QDebug>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
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
  
  m_tableView = new QTableView(this);
  m_model = new ResultsModel(this);
  m_presenter = new ResultsPresenter(m_model, m_token, this);
  
  m_tableView->setModel(m_model);
  m_tableView->horizontalHeader()->setStretchLastSection(true);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_tableView->setAlternatingRowColors(true);
  l->addWidget(title);
  l->addWidget(hint);
  l->addWidget(btn, 0, Qt::AlignLeft);
  l->addWidget(m_tableView);

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
  connect(m_tableView, &QTableView::doubleClicked,
          [this](const QModelIndex& index) {
            int uId = m_model->getUserId(index.row());
            if (uId > 0) {
              ProfileDialog d(m_token, uId, m_myRole, this);
              d.exec();
            }
          });
          
  connect(m_presenter, &ResultsPresenter::errorOccurred, this, [this](const QString& err){
      QMessageBox::warning(this, "Ошибка", err);
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
  m_currentContest = contestId;
  m_presenter->loadResults(contestId);
}

void ResultsTab::rateContest() {
  if (m_currentContest != -1) {
      m_presenter->rateContest(m_currentContest);
  }
}
