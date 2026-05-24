#include "ContestModel.h"

ContestModel::ContestModel(QObject* parent) : QAbstractTableModel(parent) {}

int ContestModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_contests.size();
}

int ContestModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return 3;
}

QVariant ContestModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "ID";
            case 1: return "Title";
            case 2: return "Start Time";
        }
    }
    return QVariant();
}

QVariant ContestModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return QVariant();
    QVariantMap map = m_contests[index.row()].toMap();
    switch (index.column()) {
        case 0: return map["id"];
        case 1: return map["title"];
        case 2: return map["start_time"];
    }
    return QVariant();
}

void ContestModel::setContests(const QVariantList& contests) {
    beginResetModel();
    m_contests = contests;
    endResetModel();
}

int ContestModel::getId(int row) const {
    return m_contests[row].toMap()["id"].toInt();
}

QString ContestModel::getTitle(int row) const {
    return m_contests[row].toMap()["title"].toString();
}
