#pragma once
#include <QAbstractListModel>
#include <QVariantList>
#include <QVariantMap>

class UsersListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit UsersListModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    
    void setUsers(const QVariantList& users);
    int getUserId(int row) const;

private:
    QVariantList m_users;
};
