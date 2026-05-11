#include "auth_dialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

AuthDialog::AuthDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Авторизация Mathforces");
    QVBoxLayout *l = new QVBoxLayout(this);
    m_login = new QLineEdit(this); m_login->setPlaceholderText("Логин (student/admin)");
    m_pass = new QLineEdit(this); m_pass->setEchoMode(QLineEdit::Password); m_pass->setPlaceholderText("Пароль (12345)");
    QPushButton *btn = new QPushButton("Вход", this);
    l->addWidget(new QLabel("Добро пожаловать в Mathforces", this));
    l->addWidget(m_login); l->addWidget(m_pass); l->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, &AuthDialog::onLogin);
}

void AuthDialog::onLogin() {
    qDebug() << "Client: Attempting login for:" << m_login->text();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/login"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j; j["login"] = m_login->text(); j["password"] = m_pass->text();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Login successful";
            QJsonObject res = QJsonDocument::fromJson(r->readAll()).object();
            m_token = res["token"].toString(); m_role = res["role"].toString();
            accept();
        } else {
            qDebug() << "Client: Login failed. Error:" << r->errorString();
            QMessageBox::warning(this, "Ошибка", "Неверные данные! Или ошибка сети: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}
