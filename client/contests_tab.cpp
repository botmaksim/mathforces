#include "contests_tab.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

ContestsTab::ContestsTab(const QString& token, QWidget* parent) : QWidget(parent), m_token(token) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QPushButton* btn = new QPushButton("Обновить", this);
    m_list = new QListWidget(this);
    l->addWidget(btn); l->addWidget(m_list);
    connect(btn, &QPushButton::clicked, this, &ContestsTab::load);
    connect(m_list, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* it) {
        qDebug() << "Client: Selected contest ID:" << it->data(Qt::UserRole).toInt();
        emit contestSelected(it->data(Qt::UserRole).toInt(), it->text());
    });
    load();
}

void ContestsTab::load() {
    qDebug() << "Client: Loading contests";
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/contests"));
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        m_list->clear();
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Contests loaded successfully";
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            for (auto v : arr) {
                QJsonObject o = v.toObject();
                QListWidgetItem* item = new QListWidgetItem(o["title"].toString() + " (" + o["start_time"].toString() + ")");
                item->setData(Qt::UserRole, o["id"].toInt());
                m_list->addItem(item);
            }
        } else {
            qDebug() << "Client Error loading contests:" << r->errorString();
        }
        r->deleteLater(); m->deleteLater();
    });
}
