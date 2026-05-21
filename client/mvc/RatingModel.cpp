#include "RatingModel.h"

RatingModel::RatingModel(QObject* parent) : QAbstractTableModel(parent) {}

int RatingModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_ratings.size();
}

int RatingModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 4; // Place, Username, Name, Rating
}

QVariant RatingModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_ratings.size()) return QVariant();
    
    QVariantMap map = m_ratings.at(index.row()).toMap();
    
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return map["place"];
            case 1: return map["username"];
            case 2: return map["name"];
            case 3: return map["rating"];
        }
    }
    return QVariant();
}

QVariant RatingModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "Место";
            case 1: return "Пользователь";
            case 2: return "Имя";
            case 3: return "Рейтинг";
        }
    }
    return QVariant();
}

void RatingModel::setRatings(const QVariantList& ratings) {
    beginResetModel();
    m_ratings = ratings;
    endResetModel();
}

int RatingModel::getUserId(int row) const {
    if (row >= 0 && row < m_ratings.size()) {
        return m_ratings.at(row).toMap()["id"].toInt();
    }
    return -1;
}
