#include "UsersTableModel.h"

UsersTableModel::UsersTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int UsersTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_users.size();
}

int UsersTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 8; // ID, Email, Username, Name, Elo, Role, Ban, Blog
}

QVariant UsersTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_users.size()) return QVariant();
    
    QVariantMap map = m_users.at(index.row()).toMap();
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return map["id"];
            case 1: return map["email"];
            case 2: return map["username"];
            case 3: return map["name"];
            case 4: return map["rating"];
            case 5: return map["role"];
            case 6: return map["is_banned"].toBool() ? "Да" : "Нет";
            case 7: return map["can_blog"].toBool() ? "Да" : "Нет";
        }
    } else if (role == Qt::UserRole) {
        return map;
    }
    return QVariant();
}

QVariant UsersTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "ID";
            case 1: return "Email";
            case 2: return "Username";
            case 3: return "Имя";
            case 4: return "Эло";
            case 5: return "Роль";
            case 6: return "Бан";
            case 7: return "Блог";
        }
    }
    return QVariant();
}

void UsersTableModel::setUsers(const QVariantList& users) {
    beginResetModel();
    m_users = users;
    endResetModel();
}

int UsersTableModel::getUserId(int row) const {
    if (row >= 0 && row < m_users.size()) return m_users.at(row).toMap()["id"].toInt();
    return -1;
}

QString UsersTableModel::getUserRole(int row) const {
    if (row >= 0 && row < m_users.size()) return m_users.at(row).toMap()["role"].toString();
    return "";
}

bool UsersTableModel::isUserBanned(int row) const {
    if (row >= 0 && row < m_users.size()) return m_users.at(row).toMap()["is_banned"].toBool();
    return false;
}

bool UsersTableModel::canUserBlog(int row) const {
    if (row >= 0 && row < m_users.size()) return m_users.at(row).toMap()["can_blog"].toBool();
    return false;
}
