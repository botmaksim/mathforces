#include "users_tab.h"
#include "profile_dialog.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QInputDialog>

UsersTab::UsersTab(const QString &token, const QString &myRole, QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole) {
  QVBoxLayout *l = new QVBoxLayout(this);
  l->setContentsMargins(4, 4, 4, 4);
  l->setSpacing(14);

  QLabel *title = new QLabel("Управление пользователями", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel("Двойной клик открывает профиль. ПКМ (Правый клик) для управления ролями и банами.", this);
  hint->setObjectName("mutedLabel");
  hint->setWordWrap(true);
  QPushButton *btnRefresh = new QPushButton("Обновить пользователей", this);
  l->addWidget(title);
  l->addWidget(hint);
  l->addWidget(btnRefresh, 0, Qt::AlignLeft);

  m_tableView = new QTableView(this);
  m_model = new UsersTableModel(this);
  m_presenter = new UsersPresenter(m_model, m_token, this);
  m_tableView->setModel(m_model);
  m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_tableView->setAlternatingRowColors(true);
  l->addWidget(m_tableView);

  QHBoxLayout *actionLayout = new QHBoxLayout;
  m_btnRoleEdit = new QPushButton("Изменить роль");
  m_btnBanToggle = new QPushButton("Забанить / Разбанить");
  m_btnBlogToggle = new QPushButton("Право на блог");
  actionLayout->addWidget(m_btnRoleEdit);
  actionLayout->addWidget(m_btnBanToggle);
  actionLayout->addWidget(m_btnBlogToggle);
  actionLayout->addStretch();
  l->addLayout(actionLayout);

  m_btnRoleEdit->hide();
  m_btnBanToggle->hide();
  m_btnBlogToggle->hide();

  if (m_myRole == "superadmin") {
      m_btnRoleEdit->show();
      m_btnBanToggle->show();
      m_btnBlogToggle->show();
  } else if (m_myRole == "moderator") {
      m_btnBanToggle->show();
      m_btnBlogToggle->show();
  }

  m_btnRoleEdit->setEnabled(false);
  m_btnBanToggle->setEnabled(false);
  m_btnBlogToggle->setEnabled(false);

  connect(btnRefresh, &QPushButton::clicked, this, &UsersTab::loadUsers);
  connect(m_tableView, &QTableView::doubleClicked, [this](const QModelIndex& index) {
      int id = m_model->getUserId(index.row());
      if (id > 0) {
          ProfileDialog d(m_token, id, m_myRole, this);
          d.exec();
      }
  });

  connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, [this]() {
      bool hasSelection = m_tableView->selectionModel()->hasSelection();
      m_btnRoleEdit->setEnabled(hasSelection);
      m_btnBanToggle->setEnabled(hasSelection);
      m_btnBlogToggle->setEnabled(hasSelection);

      if (hasSelection) {
          int row = m_tableView->selectionModel()->selectedRows().first().row();
          m_btnBanToggle->setText(m_model->isUserBanned(row) ? "Разбанить" : "Забанить");
          m_btnBlogToggle->setText(m_model->canUserBlog(row) ? "Забрать право на блог" : "Дать право на блог");
      }
  });
  
  connect(m_btnRoleEdit, &QPushButton::clicked, [this]() {
      if (m_tableView->selectionModel()->hasSelection()) {
          applyRoleChange(m_tableView->selectionModel()->selectedRows().first().row());
      }
  });
  connect(m_btnBanToggle, &QPushButton::clicked, [this]() {
      if (m_tableView->selectionModel()->hasSelection()) {
          applyBanChange(m_tableView->selectionModel()->selectedRows().first().row());
      }
  });
  connect(m_btnBlogToggle, &QPushButton::clicked, [this]() {
      if (m_tableView->selectionModel()->hasSelection()) {
          applyBlogChange(m_tableView->selectionModel()->selectedRows().first().row());
      }
  });

  connect(m_presenter, &UsersPresenter::errorOccurred, this, [this](const QString& err){
      QMessageBox::warning(this, "Ошибка", err);
  });

  loadUsers();
}

void UsersTab::loadUsers() {
    m_presenter->loadUsers();
}

void UsersTab::applyRoleChange(int row) {
    bool ok;
    QStringList roles = {"student", "admin", "moderator", "superadmin"};
    QString res = QInputDialog::getItem(this, "Роль", "Выберите новую роль:", roles, roles.indexOf(m_model->getUserRole(row)), false, &ok);
    if (ok && !res.isEmpty()) {
        m_presenter->changeRole(m_model->getUserId(row), res);
    }
}

void UsersTab::applyBanChange(int row) {
    m_presenter->changeBan(m_model->getUserId(row), !m_model->isUserBanned(row));
}

void UsersTab::applyBlogChange(int row) {
    m_presenter->changeBlog(m_model->getUserId(row), !m_model->canUserBlog(row));
}
