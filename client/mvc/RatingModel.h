#pragma once
#include <QAbstractTableModel>
#include <QVariantList>
#include <QVariantMap>

class RatingModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit RatingModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void setRatings(const QVariantList& ratings);

    int getUserId(int row) const;

private:
    QVariantList m_ratings;
};
