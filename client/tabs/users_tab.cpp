#include "users_tab.h"
#include "api_config.h"
#include "profile_dialog.h"
#include "network_utils.h"
#include "table_utils.h"
#include "toast.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

UsersTab::UsersTab(const QString &token, const QString &myRole, QWidget *parent)
    : QWidget(parent), m_token(token), m_myRole(myRole) {
  QVBoxLayout *l = new QVBoxLayout(this);
  l->setContentsMargins(4, 4, 4, 4);
  l->setSpacing(14);

  QLabel *title = new QLabel("Управление пользователями", this);
  title->setObjectName("sectionTitle");
  QLabel *hint = new QLabel("Двойной клик открывает профиль. Изменения ролей, банов и прав на блог сохраняются кнопками в таблице.", this);
  hint->setObjectName("mutedLabel");
  hint->setWordWrap(true);
  QPushButton *btnRefresh = new QPushButton("Обновить пользователей", this);
  l->addWidget(title);
  l->addWidget(hint);
  l->addWidget(btnRefresh, 0, Qt::AlignLeft);

  m_table = new QTableWidget(this);
  m_table->setColumnCount(8);
  m_table->setHorizontalHeaderLabels({"ID", "Username", "Email", "Имя", "Роль",
                                      "Забанен", "Hidden Prob", "Блог"});
  TableUtils::prepareTable(m_table);
  m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  l->addWidget(TableUtils::attachSearch(m_table, this, "Поиск пользователей..."));
  l->addWidget(m_table);

  connect(btnRefresh, &QPushButton::clicked, this, &UsersTab::loadUsers);
  connect(m_table, &QTableWidget::cellDoubleClicked,
          [this](int row, int /*col*/) {
            int id = m_table->item(row, 0)->text().toInt();
            ProfileDialog d(m_token, id, m_myRole, this);
            d.exec();
          });

  loadUsers();
}

void UsersTab::loadUsers() {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());

  QNetworkReply *r = m->get(req);
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_table->setSortingEnabled(false);
      m_table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        int id = o["id"].toInt();

        m_table->setItem(i, 0, TableUtils::numericItem(id));
        m_table->setItem(i, 1, TableUtils::textItem(o["username"].toString()));
        m_table->setItem(i, 2, TableUtils::textItem(o["email"].toString()));
        m_table->setItem(i, 3, TableUtils::textItem(o["name"].toString()));

        // Role widget
        QWidget *roleWidget = new QWidget(this);
        QHBoxLayout *roleLayout = new QHBoxLayout(roleWidget);
        roleLayout->setContentsMargins(0, 0, 0, 0);

        if (m_myRole == "superadmin") {
          QComboBox *cb = new QComboBox(roleWidget);
          cb->addItems({"student", "admin", "moderator", "superadmin"});
          cb->setCurrentText(o["role"].toString());
          QPushButton *btn = new QPushButton("Сохранить", roleWidget);
          cb->setProperty("userId", id);
          roleWidget->setProperty("cb", QVariant::fromValue((void *)cb));
          connect(btn, &QPushButton::clicked, [this, roleWidget]() {
            for (int row = 0; row < m_table->rowCount(); ++row) {
              if (m_table->cellWidget(row, 4) == roleWidget) {
                applyRoleChange(row);
                break;
              }
            }
          });
          roleLayout->addWidget(cb);
          roleLayout->addWidget(btn);
        } else {
          roleLayout->addWidget(new QLabel(o["role"].toString()));
        }
        m_table->setCellWidget(i, 4, roleWidget);

        // Ban widget
        QWidget *banWidget = new QWidget(this);
        QHBoxLayout *banLayout = new QHBoxLayout(banWidget);
        banLayout->setContentsMargins(0, 0, 0, 0);
        QCheckBox *chk = new QCheckBox("Забанен");
        chk->setChecked(o["is_banned"].toBool());
        QPushButton *btnBan = new QPushButton("Сохранить");
        chk->setProperty("userId", id);
        banWidget->setProperty("chk", QVariant::fromValue((void *)chk));
        connect(btnBan, &QPushButton::clicked, [this, banWidget]() {
          for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->cellWidget(row, 5) == banWidget) {
              applyBanChange(row);
              break;
            }
          }
        });
        banLayout->addWidget(chk);
        banLayout->addWidget(btnBan);
        m_table->setCellWidget(i, 5, banWidget);

        // Hidden Prob
        QString hp = o.contains("hidden_probability")
                         ? QString::number(o["hidden_probability"].toDouble())
                         : "-";
        m_table->setItem(i, 6, TableUtils::textItem(hp));

        // Blog
        QWidget *blogWidget = new QWidget(this);
        QHBoxLayout *blogLayout = new QHBoxLayout(blogWidget);
        blogLayout->setContentsMargins(0, 0, 0, 0);
        QCheckBox *chkBlog = new QCheckBox("Писать в блог", blogWidget);
        chkBlog->setChecked(o["can_blog"].toBool());
        chkBlog->setProperty("userId", id);
        blogWidget->setProperty("chk", QVariant::fromValue((void *)chkBlog));
        QPushButton *btnBlog = new QPushButton("Ok", blogWidget);
        if (m_myRole == "superadmin" || m_myRole == "moderator") {
          connect(btnBlog, &QPushButton::clicked, [this, blogWidget]() {
            for (int row = 0; row < m_table->rowCount(); ++row) {
              if (m_table->cellWidget(row, 7) == blogWidget) {
                applyBlogChange(row);
                break;
              }
            }
          });
        } else {
          chkBlog->setEnabled(false);
          btnBlog->setEnabled(false);
        }
        blogLayout->addWidget(chkBlog);
        blogLayout->addWidget(btnBlog);
        m_table->setCellWidget(i, 7, blogWidget);
      }
      m_table->setSortingEnabled(true);
    } else {
      NetworkUtils::showError(this, "Не удалось загрузить пользователей", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void UsersTab::applyRoleChange(int row) {
  QWidget *w = m_table->cellWidget(row, 4);
  QComboBox *cb = (QComboBox *)w->property("cb").value<void *>();
  if (!cb)
    return;
  int id = cb->property("userId").toInt();
  QString role = cb->currentText();

  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/role"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["user_id"] = id;
  j["role"] = role;

  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      Toast::show(this, "Роль обновлена");
      loadUsers();
    } else {
      NetworkUtils::showError(this, "Ошибка обновления роли", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void UsersTab::applyBanChange(int row) {
  QWidget *w = m_table->cellWidget(row, 5);
  QCheckBox *chk = (QCheckBox *)w->property("chk").value<void *>();
  if (!chk)
    return;
  int id = chk->property("userId").toInt();
  bool ban = chk->isChecked();

  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/ban"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["user_id"] = id;
  j["is_banned"] = ban;

  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      Toast::show(this, "Статус бана обновлён");
      loadUsers();
    } else {
      NetworkUtils::showError(this, "Ошибка обновления бана", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void UsersTab::applyBlogChange(int row) {
  QWidget *w = m_table->cellWidget(row, 7);
  QCheckBox *chk = (QCheckBox *)w->property("chk").value<void *>();
  if (!chk)
    return;
  int id = chk->property("userId").toInt();
  bool canBlog = chk->isChecked();

  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/blog_access"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["user_id"] = id;
  j["can_blog"] = canBlog;

  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      Toast::show(this, "Право на блог обновлено");
      loadUsers();
    } else {
      NetworkUtils::showError(this, "Ошибка обновления доступа к блогу", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}
