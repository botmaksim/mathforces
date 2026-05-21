#pragma once
#include <QAbstractTableModel>
#include <QVariantList>
#include <QVariantMap>

class UsersTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit UsersTableModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    void setUsers(const QVariantList& users);
    int getUserId(int row) const;
    QString getUserRole(int row) const;
    bool isUserBanned(int row) const;
    bool canUserBlog(int row) const;

private:
    QVariantList m_users;
};
