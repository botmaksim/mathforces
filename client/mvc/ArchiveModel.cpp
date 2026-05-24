#include "ArchiveModel.h"

ArchiveModel::ArchiveModel(QObject* parent) : QAbstractTableModel(parent) {}

int ArchiveModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_tasks.size();
}

int ArchiveModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 4; // ID, Title, Tags, Difficulty
}

QVariant ArchiveModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_tasks.size()) return QVariant();
    
    QVariantMap map = m_tasks.at(index.row()).toMap();
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return map["id"];
            case 1: return map["title"];
            case 2: return map["tags"];
            case 3: return map["difficulty"];
        }
    }
    return QVariant();
}

QVariant ArchiveModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "ID";
            case 1: return "Title";
            case 2: return "Tags";
            case 3: return "Difficulty";
        }
    }
    return QVariant();
}

void ArchiveModel::setTasks(const QVariantList& tasks) {
    beginResetModel();
    m_tasks = tasks;
    endResetModel();
}

int ArchiveModel::getTaskId(int row) const {
    if (row >= 0 && row < m_tasks.size()) {
        return m_tasks.at(row).toMap()["id"].toInt();
    }
    return -1;
}
