#include "users_tab.h"
#include "profile_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QHeaderView>
#include <QDebug>

UsersTab::UsersTab(const QString& token, const QString& myRole, QWidget* parent) 
    : QWidget(parent), m_token(token), m_myRole(myRole) 
{
    QVBoxLayout* l = new QVBoxLayout(this);
    
    QPushButton* btnRefresh = new QPushButton("Обновить", this);
    l->addWidget(btnRefresh);
    
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({"ID", "Username", "Email", "Имя", "Роль", "Забанен", "Hidden Prob", "Блог"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    l->addWidget(m_table);
    
    connect(btnRefresh, &QPushButton::clicked, this, &UsersTab::loadUsers);
    connect(m_table, &QTableWidget::cellDoubleClicked, [this](int row, int /*col*/) {
        int id = m_table->item(row, 0)->text().toInt();
        ProfileDialog d(m_token, id, m_myRole, this);
        d.exec();
    });
    
    loadUsers();
}

void UsersTab::loadUsers() {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/users"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            m_table->setRowCount(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject o = arr[i].toObject();
                int id = o["id"].toInt();
                
                m_table->setItem(i, 0, new QTableWidgetItem(QString::number(id)));
                m_table->setItem(i, 1, new QTableWidgetItem(o["username"].toString()));
                m_table->setItem(i, 2, new QTableWidgetItem(o["email"].toString()));
                m_table->setItem(i, 3, new QTableWidgetItem(o["name"].toString()));
                
                // Role widget
                QWidget* roleWidget = new QWidget(this);
                QHBoxLayout* roleLayout = new QHBoxLayout(roleWidget);
                roleLayout->setContentsMargins(0, 0, 0, 0);
                
                if (m_myRole == "superadmin") {
                    QComboBox* cb = new QComboBox(roleWidget);
                    cb->addItems({"student", "admin", "moderator", "superadmin"});
                    cb->setCurrentText(o["role"].toString());
                    QPushButton* btn = new QPushButton("Save", roleWidget);
                    cb->setProperty("userId", id);
                    roleWidget->setProperty("cb", QVariant::fromValue((void*)cb));
                    connect(btn, &QPushButton::clicked, [this, i]() { applyRoleChange(i); });
                    roleLayout->addWidget(cb);
                    roleLayout->addWidget(btn);
                } else {
                    roleLayout->addWidget(new QLabel(o["role"].toString()));
                }
                m_table->setCellWidget(i, 4, roleWidget);
                
                // Ban widget
                QWidget* banWidget = new QWidget(this);
                QHBoxLayout* banLayout = new QHBoxLayout(banWidget);
                banLayout->setContentsMargins(0, 0, 0, 0);
                QCheckBox* chk = new QCheckBox("Banned");
                chk->setChecked(o["is_banned"].toBool());
                QPushButton* btnBan = new QPushButton("Save");
                chk->setProperty("userId", id);
                banWidget->setProperty("chk", QVariant::fromValue((void*)chk));
                connect(btnBan, &QPushButton::clicked, [this, i]() { applyBanChange(i); });
                banLayout->addWidget(chk);
                banLayout->addWidget(btnBan);
                m_table->setCellWidget(i, 5, banWidget);
                
                // Hidden Prob
                QString hp = o.contains("hidden_probability") ? QString::number(o["hidden_probability"].toDouble()) : "-";
                m_table->setItem(i, 6, new QTableWidgetItem(hp));
                
                // Blog
                QWidget* blogWidget = new QWidget(this);
                QHBoxLayout* blogLayout = new QHBoxLayout(blogWidget);
                blogLayout->setContentsMargins(0,0,0,0);
                QCheckBox* chkBlog = new QCheckBox("Писать в блог", blogWidget);
                chkBlog->setChecked(o["can_blog"].toBool());
                chkBlog->setProperty("userId", id);
                blogWidget->setProperty("chk", QVariant::fromValue((void*)chkBlog));
                QPushButton* btnBlog = new QPushButton("Ok", blogWidget);
                if (m_myRole == "superadmin" || m_myRole == "moderator") {
                    connect(btnBlog, &QPushButton::clicked, [this, i]() { applyBlogChange(i); });
                } else {
                    chkBlog->setEnabled(false);
                    btnBlog->setEnabled(false);
                }
                blogLayout->addWidget(chkBlog);
                blogLayout->addWidget(btnBlog);
                m_table->setCellWidget(i, 7, blogWidget);
            }
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось загрузить пользователей");
        }
        r->deleteLater(); m->deleteLater();
    });
}

void UsersTab::applyRoleChange(int row) {
    QWidget* w = m_table->cellWidget(row, 4);
    QComboBox* cb = (QComboBox*)w->property("cb").value<void*>();
    if (!cb) return;
    int id = cb->property("userId").toInt();
    QString role = cb->currentText();
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/users/role"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["user_id"] = id; j["role"] = role;
    
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Ок", "Роль обновлена");
            loadUsers();
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка обновления роли: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}

void UsersTab::applyBanChange(int row) {
    QWidget* w = m_table->cellWidget(row, 5);
    QCheckBox* chk = (QCheckBox*)w->property("chk").value<void*>();
    if (!chk) return;
    int id = chk->property("userId").toInt();
    bool ban = chk->isChecked();
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/users/ban"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["user_id"] = id; j["is_banned"] = ban;
    
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Ок", "Статус бана обновлен");
            loadUsers();
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}

void UsersTab::applyBlogChange(int row) {
    QWidget* w = m_table->cellWidget(row, 7);
    QCheckBox* chk = (QCheckBox*)w->property("chk").value<void*>();
    if (!chk) return;
    int id = chk->property("userId").toInt();
    bool canBlog = chk->isChecked();
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/users/blog_access"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["user_id"] = id; j["can_blog"] = canBlog;
    
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Ок", "Право на блог обновлено");
            loadUsers();
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}
