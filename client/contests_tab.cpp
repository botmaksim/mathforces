#include "contests_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMenu>

ContestsTab::ContestsTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
  QVBoxLayout *l = new QVBoxLayout(this);
  QLabel *title = new QLabel("Upcoming contests", this);
  title->setObjectName("sectionTitle");

  QHBoxLayout *top = new QHBoxLayout();
  QPushButton *btn = new QPushButton("Refresh", this);
  top->addWidget(btn);
  top->addStretch();

  m_tableView = new QTableView(this);
  m_model = new ContestModel(this);
  m_tableView->setModel(m_model);
  m_tableView->horizontalHeader()->setStretchLastSection(true);
  
  m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tableView, &QTableView::customContextMenuRequested, [this](const QPoint &pos) {
      QModelIndex index = m_tableView->indexAt(pos);
      if (index.isValid()) {
          QMenu menu(this);
          QAction* vAct = menu.addAction("Start virtual participation");
          connect(vAct, &QAction::triggered, [this, index]() {
              m_presenter->startVirtualParticipation(m_model->getId(index.row()));
          });
          menu.exec(m_tableView->viewport()->mapToGlobal(pos));
      }
  });
  
  l->addWidget(title);
  l->addLayout(top);
  l->addWidget(m_tableView);

  // Изолированная бизнес-логика
  m_presenter = new ContestsPresenter(m_model, token, this);
  connect(btn, &QPushButton::clicked, m_presenter, &ContestsPresenter::loadContests);
  
  connect(m_presenter, &ContestsPresenter::virtualParticipationStarted, [this](int cid) {
      emit virtualReadyToOpen(cid);
  });

  connect(m_tableView, &QTableView::doubleClicked, [this](const QModelIndex& idx) {
      if (idx.isValid()) {
         int id = m_model->getId(idx.row());
         QString title = m_model->getTitle(idx.row());
         emit contestSelected(id, title);
      }
  });

  // Запуск локального кэша и фонового обновления
  m_presenter->loadContests();
}
