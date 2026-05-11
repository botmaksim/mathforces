#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QString>

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
    QLineEdit* m_email;
    QLineEdit* m_pass;
    QLineEdit* m_username;
    QLineEdit* m_name;
    QString m_token;
    QString m_role;
};
