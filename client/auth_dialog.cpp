#include "auth_dialog.h"
#include "api_config.h"
#include <QDebug>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

AuthDialog::AuthDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("MathForces - Login");
  setMinimumSize(520, 560);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(28, 28, 28, 28);
  mainLayout->setSpacing(16);

  QLabel *brand = new QLabel("MathForces", this);
  brand->setObjectName("authLogo");
  brand->setAlignment(Qt::AlignCenter);
  QLabel *subtitle = new QLabel(
      "Log in to solve tasks, participate in contests and view rankings",
      this);
  subtitle->setObjectName("authSubtitle");
  subtitle->setWordWrap(true);
  subtitle->setAlignment(Qt::AlignCenter);

  mainLayout->addWidget(brand);
  mainLayout->addWidget(subtitle);

  m_stackedWidget = new QStackedWidget(this);
  mainLayout->addWidget(m_stackedWidget, 1);

  // --- Login Widget ---
  QFrame *loginWidget = new QFrame(this);
  loginWidget->setObjectName("authCard");
  QVBoxLayout *loginLayout = new QVBoxLayout(loginWidget);
  loginLayout->setContentsMargins(28, 28, 28, 28);
  loginLayout->setSpacing(12);

  QLabel *loginTitle = new QLabel("Welcome back", loginWidget);
  loginTitle->setObjectName("sectionTitle");
  QLabel *loginHint = new QLabel("Enter your email and password.", loginWidget);
  loginHint->setObjectName("mutedLabel");

  m_emailLogin = new QLineEdit(this);
  m_emailLogin->setPlaceholderText("Email");
  m_passLogin = new QLineEdit(this);
  m_passLogin->setEchoMode(QLineEdit::Password);
  m_passLogin->setPlaceholderText("Password");

  QPushButton *btnLog = new QPushButton("Login", this);
  QPushButton *btnGoogle = new QPushButton("Login with Google", this);
  QPushButton *btnGoToReg = new QPushButton("No account? Register", this);
  btnGoToReg->setFlat(true);

  loginLayout->addWidget(loginTitle);
  loginLayout->addWidget(loginHint);
  loginLayout->addSpacing(8);
  loginLayout->addWidget(m_emailLogin);
  loginLayout->addWidget(m_passLogin);
  loginLayout->addWidget(btnLog);
  loginLayout->addWidget(btnGoogle);
  loginLayout->addStretch();
  loginLayout->addWidget(btnGoToReg, 0, Qt::AlignCenter);

  // --- Register Widget ---
  QFrame *regWidget = new QFrame(this);
  regWidget->setObjectName("authCard");
  QVBoxLayout *regLayout = new QVBoxLayout(regWidget);
  regLayout->setContentsMargins(28, 28, 28, 28);
  regLayout->setSpacing(12);

  QLabel *regTitle = new QLabel("Create Account", regWidget);
  regTitle->setObjectName("sectionTitle");
  QLabel *regHint = new QLabel("Fill in details, then confirm email.", regWidget);
  regHint->setObjectName("mutedLabel");

  m_emailReg = new QLineEdit(this);
  m_emailReg->setPlaceholderText("Email (student@example.com)");
  m_usernameReg = new QLineEdit(this);
  m_usernameReg->setPlaceholderText("Username (unique)");
  m_nameReg = new QLineEdit(this);
  m_nameReg->setPlaceholderText("Full Name");
  m_passReg = new QLineEdit(this);
  m_passReg->setEchoMode(QLineEdit::Password);
  m_passReg->setPlaceholderText("Password");

  QPushButton *btnReg = new QPushButton("Send confirmation code", this);
  QPushButton *btnGoToLog = new QPushButton("Already have account? Login", this);
  btnGoToLog->setFlat(true);

  regLayout->addWidget(regTitle);
  regLayout->addWidget(regHint);
  regLayout->addSpacing(8);
  regLayout->addWidget(m_emailReg);
  regLayout->addWidget(m_usernameReg);
  regLayout->addWidget(m_nameReg);
  regLayout->addWidget(m_passReg);
  regLayout->addWidget(btnReg);
  regLayout->addStretch();
  regLayout->addWidget(btnGoToLog, 0, Qt::AlignCenter);

  m_stackedWidget->addWidget(loginWidget);
  m_stackedWidget->addWidget(regWidget);
  
  m_presenter = new AuthPresenter(this);

  connect(btnLog, &QPushButton::clicked, this, &AuthDialog::onEmailLogin);
  connect(btnReg, &QPushButton::clicked, this, &AuthDialog::onEmailRegister);
  connect(btnGoogle, &QPushButton::clicked, this, &AuthDialog::onGoogleLogin);

  connect(btnGoToReg, &QPushButton::clicked,
          [this]() { m_stackedWidget->setCurrentIndex(1); });
  connect(btnGoToLog, &QPushButton::clicked,
          [this]() { m_stackedWidget->setCurrentIndex(0); });
          
  connect(m_presenter, &AuthPresenter::loginSuccessful, this, [this](const QString& token, const QString& role) {
      m_token = token;
      m_role = role;
      if (m_stackedWidget->currentIndex() == 1) { // Was doing reg
          QMessageBox::information(this, "Success", "Registration successful! You may login now.");
          m_stackedWidget->setCurrentIndex(0); // Go to login
          m_emailLogin->setText(m_emailReg->text());
          m_token = ""; m_role = ""; // clear, user must login now or it's auto-login? the old code showed Ok and went to login
      } else {
          accept();
      }
  });

  connect(m_presenter, &AuthPresenter::codeRequested, this, [this]() {
      bool ok;
      QString code = QInputDialog::getText(this, "Email Confirmation",
                            "Confirmation code sent to your email (check server console).\nEnter the code:",
                            QLineEdit::Normal, "", &ok);
      if (ok && !code.isEmpty()) {
        completeRegistration(code);
      }
  });

  connect(m_presenter, &AuthPresenter::errorOccurred, this, [this](const QString& err) {
      QMessageBox::warning(this, "Error", err);
  });
}

void AuthDialog::onEmailLogin() {
  m_presenter->login(m_emailLogin->text(), m_passLogin->text());
}

void AuthDialog::onEmailRegister() {
  if (m_emailReg->text().isEmpty() || m_passReg->text().isEmpty() ||
      m_usernameReg->text().isEmpty()) {
    QMessageBox::warning(this, "Error", "Fill in all fields");
    return;
  }
  m_presenter->requestCode(m_emailReg->text());
}

void AuthDialog::completeRegistration(const QString &code) {
  m_presenter->registerUser(code, m_emailReg->text(), m_usernameReg->text(), m_nameReg->text(), m_passReg->text());
}

void AuthDialog::onGoogleLogin() {
  QString clientId = qEnvironmentVariable(
      "GOOGLE_CLIENT_ID", "170919746104-iqpvnoialm0enaf8g9fkibd5gcrrn91d.apps.googleusercontent.com");
  QUrl url("https://accounts.google.com/o/oauth2/v2/auth");
  QUrlQuery query;
  query.addQueryItem("client_id", clientId);
  query.addQueryItem("redirect_uri", ApiConfig::baseUrl + "/api/oauth_callback_client");
  query.addQueryItem("response_type", "token");
  query.addQueryItem("scope", "email profile");
  url.setQuery(query);
  QDesktopServices::openUrl(url);

  bool ok;
  QString tokenWithRole = QInputDialog::getText(
      this, "OAuth",
      "The authorization page should have opened in your browser.\nIf "
      "authorization is successful, a token will appear.\n\nEnter the received "
      "token (token-role):", QLineEdit::Normal, "", &ok);
  if (ok && !tokenWithRole.isEmpty()) {
    QStringList parts = tokenWithRole.split("-");
    if (parts.size() >= 2) {
      m_token = parts[0];
      m_role = parts[1];
      accept();
    } else QMessageBox::warning(this, "Error", "Invalid token format");
  }
}
