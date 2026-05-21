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
  m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
  l->addWidget(m_tableView);

  connect(btnRefresh, &QPushButton::clicked, this, &UsersTab::loadUsers);
  connect(m_tableView, &QTableView::doubleClicked, [this](const QModelIndex& index) {
      int id = m_model->getUserId(index.row());
      if (id > 0) {
          ProfileDialog d(m_token, id, m_myRole, this);
          d.exec();
      }
  });
  connect(m_tableView, &QTableView::customContextMenuRequested, this, &UsersTab::onCustomContextMenuRequired);
  
  connect(m_presenter, &UsersPresenter::errorOccurred, this, [this](const QString& err){
      QMessageBox::warning(this, "Ошибка", err);
  });

  loadUsers();
}

void UsersTab::loadUsers() {
    m_presenter->loadUsers();
}

void UsersTab::onCustomContextMenuRequired(const QPoint& pos) {
    QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) return;
    int row = index.row();
    QMenu menu(this);
    
    QAction* actRole = nullptr;
    if (m_myRole == "superadmin") {
        actRole = menu.addAction("Изменить роль...");
        connect(actRole, &QAction::triggered, [this, row](){ applyRoleChange(row); });
    }
    QAction* actBanToggle = menu.addAction(m_model->isUserBanned(row) ? "Разбанить" : "Забанить");
    connect(actBanToggle, &QAction::triggered, [this, row](){ applyBanChange(row); });
    
    QAction* actBlogToggle = nullptr;
    if (m_myRole == "superadmin" || m_myRole == "moderator") {
        actBlogToggle = menu.addAction(m_model->canUserBlog(row) ? "Забрать право на блог" : "Дать право на блог");
        connect(actBlogToggle, &QAction::triggered, [this, row](){ applyBlogChange(row); });
    }
    
    menu.exec(m_tableView->viewport()->mapToGlobal(pos));
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
