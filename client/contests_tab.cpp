#include "api_config.h"
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
    QHBoxLayout* top = new QHBoxLayout();
    QPushButton* btn = new QPushButton("Обновить", this);
    QPushButton* btnVirtual = new QPushButton("Виртуальное участие", this);
    top->addWidget(btn); top->addWidget(btnVirtual);
    
    m_list = new QListWidget(this);
    l->addLayout(top); l->addWidget(m_list);
    connect(btn, &QPushButton::clicked, this, &ContestsTab::load);
    connect(m_list, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* it) {
        qDebug() << "Client: Selected contest ID:" << it->data(Qt::UserRole).toInt();
        emit contestSelected(it->data(Qt::UserRole).toInt(), it->text());
    });
    
    connect(btnVirtual, &QPushButton::clicked, [this]() {
        if (!m_list->currentItem()) return;
        int cid = m_list->currentItem()->data(Qt::UserRole).toInt();
        emit startVirtualParticipation(cid);
    });
    
    load();
}

void ContestsTab::load() {
    qDebug() << "Client: Loading contests";
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/contests"));
    if (!m_token.isEmpty())
        req.setRawHeader("Authorization", m_token.toUtf8());
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
