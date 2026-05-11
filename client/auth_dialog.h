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
    void onLogin();

private:
    QLineEdit* m_login;
    QLineEdit* m_pass;
    QString m_token;
    QString m_role;
};
