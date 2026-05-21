#include "ResultsModel.h"

ResultsModel::ResultsModel(QObject* parent) : QAbstractTableModel(parent) {}

int ResultsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_results.size();
}

int ResultsModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 5;
}

QVariant ResultsModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_results.size()) return QVariant();
    QVariantMap map = m_results.at(index.row()).toMap();
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return map["place"];
            case 1: return map["username"];
            case 2: return map["total_score"];
            case 3: return map["penalty"];
            case 4: return map["is_official"].toBool() ? "Да" : "Нет";
        }
    }
    return QVariant();
}

QVariant ResultsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "Место";
            case 1: return "Участник";
            case 2: return "Баллы";
            case 3: return "Штраф (мин)";
            case 4: return "Официальный";
        }
    }
    return QVariant();
}

void ResultsModel::setResults(const QVariantList& results) {
    beginResetModel();
    m_results = results;
    endResetModel();
}

int ResultsModel::getUserId(int row) const {
    if (row >= 0 && row < m_results.size()) return m_results.at(row).toMap()["user_id"].toInt();
    return -1;
}
