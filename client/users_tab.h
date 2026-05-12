#ifndef USERS_TAB_H
#define USERS_TAB_H

#include <QWidget>
#include <QString>
#include <QTableWidget>

class UsersTab : public QWidget {
    Q_OBJECT
public:
    UsersTab(const QString& token, const QString& myRole, QWidget* parent = nullptr);

public slots:
    void loadUsers();

private slots:
    void applyRoleChange(int row);
    void applyBanChange(int row);
    void applyBlogChange(int row);

private:
    QString m_token;
    QString m_myRole;
    QTableWidget* m_table;
};

#endif // USERS_TAB_H
