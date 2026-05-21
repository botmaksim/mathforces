#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QStackedWidget>
#include <QString>
#include "mvc/AuthPresenter.h"

class AuthDialog : public QDialog {
  Q_OBJECT
public:
  explicit AuthDialog(QWidget *parent = nullptr);
  QString getToken() const { return m_token; }
  QString getRole() const { return m_role; }

private slots:
  void onEmailLogin();
  void onEmailRegister();
  void onGoogleLogin();

private:
  void completeRegistration(const QString &code);

  QStackedWidget *m_stackedWidget;

  // Login fields
  QLineEdit *m_emailLogin;
  QLineEdit *m_passLogin;

  // Registration fields
  QLineEdit *m_emailReg;
  QLineEdit *m_usernameReg;
  QLineEdit *m_nameReg;
  QLineEdit *m_passReg;

  QString m_token;
  QString m_role;
  
  AuthPresenter *m_presenter;
};
