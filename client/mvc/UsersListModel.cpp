#include "UsersListModel.h"

UsersListModel::UsersListModel(QObject* parent) : QAbstractListModel(parent) {}

int UsersListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_users.size();
}

QVariant UsersListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_users.size()) return QVariant();
    
    QVariantMap map = m_users.at(index.row()).toMap();
    
    if (role == Qt::DisplayRole) {
        return QString("%1 (%2) - Elo: %3")
                .arg(map["username"].toString())
                .arg(map["name"].toString())
                .arg(map["rating"].toInt());
    } else if (role == Qt::UserRole) {
        return map["id"];
    }
    return QVariant();
}

void UsersListModel::setUsers(const QVariantList& users) {
    beginResetModel();
    m_users = users;
    endResetModel();
}

int UsersListModel::getUserId(int row) const {
    if (row >= 0 && row < m_users.size()) {
        return m_users.at(row).toMap()["id"].toInt();
    }
    return -1;
}
