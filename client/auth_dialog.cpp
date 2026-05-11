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
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

AuthDialog::AuthDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Авторизация Mathforces");
    QVBoxLayout *l = new QVBoxLayout(this);
    m_email = new QLineEdit(this); m_email->setPlaceholderText("Email (student@example.com)");
    m_username = new QLineEdit(this); m_username->setPlaceholderText("Username (для регистрации)");
    m_name = new QLineEdit(this); m_name->setPlaceholderText("Имя (для регистрации)");
    m_pass = new QLineEdit(this); m_pass->setEchoMode(QLineEdit::Password); m_pass->setPlaceholderText("Пароль");
    
    QPushButton *btnLog = new QPushButton("Вход по Email", this);
    QPushButton *btnReg = new QPushButton("Регистрация по Email", this);
    QPushButton *btnGoogle = new QPushButton("Вход через Google", this);
    
    l->addWidget(new QLabel("Добро пожаловать в Mathforces", this));
    l->addWidget(m_email); l->addWidget(m_pass); l->addWidget(m_username); l->addWidget(m_name);
    l->addWidget(btnLog); l->addWidget(btnReg); l->addWidget(btnGoogle);
    
    connect(btnLog, &QPushButton::clicked, this, &AuthDialog::onEmailLogin);
    connect(btnReg, &QPushButton::clicked, this, &AuthDialog::onEmailRegister);
    connect(btnGoogle, &QPushButton::clicked, this, &AuthDialog::onGoogleLogin);
}

void AuthDialog::onEmailLogin() {
    qDebug() << "Client: Attempting login for:" << m_email->text();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/login/email"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j; j["email"] = m_email->text(); j["password"] = m_pass->text();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Login successful";
            QJsonObject res = QJsonDocument::fromJson(r->readAll()).object();
            m_token = res["token"].toString(); m_role = res["role"].toString();
            accept();
        } else {
            qDebug() << "Client: Login failed. Error:" << r->errorString();
            QString err = QJsonDocument::fromJson(r->readAll()).object()["error"].toString();
            QMessageBox::warning(this, "Ошибка", "Неверные данные! " + err);
        }
        r->deleteLater(); m->deleteLater();
    });
}

void AuthDialog::onEmailRegister() {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/register/email"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j; j["email"] = m_email->text(); j["password"] = m_pass->text(); 
    j["username"] = m_username->text(); j["name"] = m_name->text();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Ок", "Регистрация успешна! Теперь вы можете войти.");
        } else {
            QMessageBox::warning(this, "Ошибка", "Ошибка регистрации: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}

#include <QInputDialog>

void AuthDialog::onGoogleLogin() {
    QString clientId = "51208074605-s9kj787l62cc20facf4m56uuud5utg9t.apps.googleusercontent.com";
    QString url = QString("https://accounts.google.com/o/oauth2/v2/auth?client_id=%1&redirect_uri=http://localhost:8080/api/oauth_callback_client&response_type=token&scope=email profile").arg(clientId);
    QDesktopServices::openUrl(QUrl(url));
    
    bool ok;
    QString tokenWithRole = QInputDialog::getText(this, "OAuth", "В браузере должна была открыться страница авторизации.\nЕсли авторизация пройдет успешно, появится токен.\n\nВведите полученный токен (token-role):", QLineEdit::Normal, "", &ok);
    if (ok && !tokenWithRole.isEmpty()) {
        QStringList parts = tokenWithRole.split("-");
        if (parts.size() >= 2) {
            m_token = parts[0];
            m_role = parts[1];
            accept();
        } else {
            QMessageBox::warning(this, "Ошибка", "Неверный формат токена");
        }
    }
}

