#include "api_config.h"
#include "results_tab.h"
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
#include <QDebug>

ResultsTab::ResultsTab(const QString& token, const QString& myRole, QWidget* parent) : QWidget(parent), m_token(token), m_myRole(myRole) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QPushButton* btn = new QPushButton("Обновить", this);
    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({"Место", "Участник", "Баллы", "Штраф (мин)", "Официальный"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    l->addWidget(btn); l->addWidget(m_table);
    
    QHBoxLayout* adminL = new QHBoxLayout();
    QPushButton* btnRate = new QPushButton("Пересчитать Эло (только для Админов)", this);
    adminL->addWidget(btnRate);
    adminL->addStretch();
    if (m_myRole == "admin" || m_myRole == "superadmin") {
        l->addLayout(adminL);
    } else {
        btnRate->hide();
    }
    
    connect(btn, &QPushButton::clicked, [this](){ if (m_currentContest != -1) loadResults(m_currentContest); });
    connect(btnRate, &QPushButton::clicked, this, &ResultsTab::rateContest);
    connect(m_table, &QTableWidget::cellDoubleClicked, [this](int row, int /*col*/){
        int uId = m_table->item(row, 1)->data(Qt::UserRole).toInt();
        if (uId > 0) {
            ProfileDialog d(m_token, uId, m_myRole, this);
            d.exec();
        }
    });

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, [this]() {
        if (m_currentContest != -1) {
            loadResults(m_currentContest);
        }
    });
    m_timer->start(10000); // Poll every 10 seconds
}

void ResultsTab::loadResults(int contestId) {
    qDebug() << "Client: Loading results for contest ID:" << contestId;
    m_currentContest = contestId;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkReply* r = m->get(QNetworkRequest(QUrl(QString(ApiConfig::baseUrl + "/api/results?contest_id=%1").arg(contestId))));
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Results loaded successfully";
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            m_table->setRowCount(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject o = arr[i].toObject();
                m_table->setItem(i, 0, new QTableWidgetItem(QString::number(o["place"].toInt())));
                QTableWidgetItem* uItem = new QTableWidgetItem(o["username"].toString());
                uItem->setData(Qt::UserRole, o["user_id"].toInt());
                m_table->setItem(i, 1, uItem);
                m_table->setItem(i, 2, new QTableWidgetItem(QString::number(o["total_score"].toInt())));
                m_table->setItem(i, 3, new QTableWidgetItem(QString::number(o["penalty"].toInt())));
                m_table->setItem(i, 4, new QTableWidgetItem(o["is_official"].toBool() ? "Да" : "Нет"));
            }
        } else {
            qDebug() << "Client Error loading results:" << r->errorString();
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ResultsTab::rateContest() {
    if (m_currentContest == -1) return;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/admin/rate_contest"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["contest_id"] = m_currentContest;
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            loadResults(m_currentContest);
        }
        r->deleteLater(); m->deleteLater();
    });
}
