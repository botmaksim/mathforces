#pragma once
#include <QAbstractTableModel>
#include <QVariantList>
#include <QVariantMap>

class ContestModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ContestModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void setContests(const QVariantList& contests);
    int getId(int row) const;
    QString getTitle(int row) const;
private:
    QVariantList m_contests;
};
