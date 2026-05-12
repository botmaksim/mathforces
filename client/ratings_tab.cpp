#include "ratings_tab.h"
#include "profile_dialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

RatingsTab::RatingsTab(const QString& token, const QString& myRole, QWidget* parent) 
    : QWidget(parent), m_token(token), m_myRole(myRole) 
{
    QVBoxLayout* l = new QVBoxLayout(this);
    
    QPushButton* btnRefresh = new QPushButton("Обновить рейтинг", this);
    l->addWidget(btnRefresh);
    
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Место", "Участник", "Имя", "Эло (Рейтинг)"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    l->addWidget(m_table);
    
    connect(btnRefresh, &QPushButton::clicked, this, &RatingsTab::loadRatings);
    connect(m_table, &QTableWidget::cellDoubleClicked, [this](int row, int /*col*/) {
        int uId = m_table->item(row, 1)->data(Qt::UserRole).toInt();
        if (uId > 0) {
            ProfileDialog d(m_token, uId, m_myRole, this);
            d.exec();
        }
    });

    loadRatings();
}

void RatingsTab::loadRatings() {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/ratings"));
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            m_table->setRowCount(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject o = arr[i].toObject();
                m_table->setItem(i, 0, new QTableWidgetItem(QString::number(o["place"].toInt())));
                
                QTableWidgetItem* uItem = new QTableWidgetItem(o["username"].toString());
                uItem->setData(Qt::UserRole, o["id"].toInt());
                
                m_table->setItem(i, 1, uItem);
                m_table->setItem(i, 2, new QTableWidgetItem(o["name"].toString()));
                m_table->setItem(i, 3, new QTableWidgetItem(QString::number(o["rating"].toInt())));
            }
        }
        r->deleteLater(); m->deleteLater();
    });
}
