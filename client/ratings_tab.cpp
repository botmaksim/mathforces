#include "ratings_tab.h"
#include "profile_dialog.h"
#include <QDebug>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

RatingsTab::RatingsTab(const QString &token, const QString &myRole,
                       QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole) {
  QVBoxLayout *l = new QVBoxLayout(this);
  l->setContentsMargins(4, 4, 4, 4);
  l->setSpacing(14);
  QLabel *title = new QLabel("Global Rating", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel("The rating is updated automatically. Double click to open a participant's profile.", this);
  hint->setObjectName("mutedLabel");
  hint->setWordWrap(true);
  QPushButton *btn = new QPushButton("Refresh Rating", this);
  
  m_tableView = new QTableView(this);
  m_model = new RatingModel(this);
  m_presenter = new RatingsPresenter(m_model, m_token, this);
  
  m_tableView->setModel(m_model);
  m_tableView->horizontalHeader()->setStretchLastSection(true);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_tableView->setAlternatingRowColors(true);
  
  l->addWidget(title);
  l->addWidget(hint);
  l->addWidget(btn, 0, Qt::AlignLeft);
  l->addWidget(m_tableView);

  connect(btn, &QPushButton::clicked, this, &RatingsTab::loadRatings);
  connect(m_presenter, &RatingsPresenter::errorOccurred, this, [](const QString& err){
      qDebug() << "Ratings error:" << err;
  });
  connect(m_tableView, &QTableView::doubleClicked,
          [this](const QModelIndex& index) {
            int uId = m_model->getUserId(index.row());
            if (uId > 0) {
              ProfileDialog d(m_token, uId, m_myRole, this);
              d.exec();
            }
          });

  QTimer *m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this, &RatingsTab::loadRatings);
  m_timer->start(30000); // Poll every 30 seconds
  
  // Initial load
  loadRatings();
}

void RatingsTab::loadRatings() {
  m_presenter->loadRatings();
}
