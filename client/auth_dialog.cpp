#include "api_config.h"
#include "auth_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QUrlQuery>
#include <QDebug>
#include <QInputDialog>
#include <QStackedWidget>

AuthDialog::AuthDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Авторизация Mathforces");
    setMinimumWidth(300);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);
    
    // --- Login Widget ---
    QWidget* loginWidget = new QWidget(this);
    QVBoxLayout* loginLayout = new QVBoxLayout(loginWidget);
    m_emailLogin = new QLineEdit(this); m_emailLogin->setPlaceholderText("Email");
    m_passLogin = new QLineEdit(this); m_passLogin->setEchoMode(QLineEdit::Password); m_passLogin->setPlaceholderText("Пароль");
    
    QPushButton *btnLog = new QPushButton("Вход", this);
    QPushButton *btnGoogle = new QPushButton("Войти через Google", this);
    QPushButton *btnGoToReg = new QPushButton("Нет аккаунта? Зарегистрируйтесь", this);
    btnGoToReg->setFlat(true);
    
    loginLayout->addWidget(new QLabel("Вход в систему", this));
    loginLayout->addWidget(m_emailLogin);
    loginLayout->addWidget(m_passLogin);
    loginLayout->addWidget(btnLog);
    loginLayout->addWidget(btnGoogle);
    loginLayout->addWidget(btnGoToReg);
    loginLayout->addStretch();
    
    // --- Register Widget ---
    QWidget* regWidget = new QWidget(this);
    QVBoxLayout* regLayout = new QVBoxLayout(regWidget);
    m_emailReg = new QLineEdit(this); m_emailReg->setPlaceholderText("Email (student@example.com)");
    m_usernameReg = new QLineEdit(this); m_usernameReg->setPlaceholderText("Username (уникальный)");
    m_nameReg = new QLineEdit(this); m_nameReg->setPlaceholderText("Полное имя");
    m_passReg = new QLineEdit(this); m_passReg->setEchoMode(QLineEdit::Password); m_passReg->setPlaceholderText("Пароль");
    
    QPushButton *btnReg = new QPushButton("Отправить код подтверждения", this);
    QPushButton *btnGoToLog = new QPushButton("Уже есть аккаунт? Войти", this);
    btnGoToLog->setFlat(true);
    
    regLayout->addWidget(new QLabel("Регистрация", this));
    regLayout->addWidget(m_emailReg);
    regLayout->addWidget(m_usernameReg);
    regLayout->addWidget(m_nameReg);
    regLayout->addWidget(m_passReg);
    regLayout->addWidget(btnReg);
    regLayout->addWidget(btnGoToLog);
    regLayout->addStretch();
    
    m_stackedWidget->addWidget(loginWidget);
    m_stackedWidget->addWidget(regWidget);
    
    connect(btnLog, &QPushButton::clicked, this, &AuthDialog::onEmailLogin);
    connect(btnReg, &QPushButton::clicked, this, &AuthDialog::onEmailRegister);
    connect(btnGoogle, &QPushButton::clicked, this, &AuthDialog::onGoogleLogin);
    
    connect(btnGoToReg, &QPushButton::clicked, [this]() { m_stackedWidget->setCurrentIndex(1); });
    connect(btnGoToLog, &QPushButton::clicked, [this]() { m_stackedWidget->setCurrentIndex(0); });
}

void AuthDialog::onEmailLogin() {
    qDebug() << "Client: Attempting login for:" << m_emailLogin->text();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/login/email"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j; j["email"] = m_emailLogin->text(); j["password"] = m_passLogin->text();
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
    if (m_emailReg->text().isEmpty() || m_passReg->text().isEmpty() || m_usernameReg->text().isEmpty()) {
         QMessageBox::warning(this, "Ошибка", "Заполните все поля");
         return;
    }
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/register/request_code"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j; j["email"] = m_emailReg->text();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            bool ok;
            QString code = QInputDialog::getText(this, "Подтверждение Email", "Код отправлен на вашу почту (проверьте консоль сервера).\nВведите код подтверждения:", QLineEdit::Normal, "", &ok);
            if (ok && !code.isEmpty()) {
                completeRegistration(code);
            }
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось запросить код: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}

void AuthDialog::completeRegistration(const QString& code) {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/register/email"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j; 
    j["email"] = m_emailReg->text(); 
    j["password"] = m_passReg->text(); 
    j["username"] = m_usernameReg->text(); 
    j["name"] = m_nameReg->text();
    j["code"] = code;
    
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QMessageBox::information(this, "Ок", "Регистрация успешна! Теперь вы можете войти.");
            m_stackedWidget->setCurrentIndex(0); // Go to login
            m_emailLogin->setText(m_emailReg->text());
        } else {
            QString err = QJsonDocument::fromJson(r->readAll()).object()["error"].toString();
            QMessageBox::warning(this, "Ошибка", "Ошибка регистрации: " + (err.isEmpty() ? r->errorString() : err));
        }
        r->deleteLater(); m->deleteLater();
    });
}

void AuthDialog::onGoogleLogin() {
    QString clientId = qEnvironmentVariable("GOOGLE_CLIENT_ID", "170919746104-iqpvnoialm0enaf8g9fkibd5gcrrn91d.apps.googleusercontent.com");
    
    QUrl url("https://accounts.google.com/o/oauth2/v2/auth");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("redirect_uri", "http://127.0.0.1:8080/api/oauth_callback_client");
    query.addQueryItem("response_type", "token");
    query.addQueryItem("scope", "email profile");
    url.setQuery(query);
    
    QDesktopServices::openUrl(url);
    
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

