#include "results_tab.h"
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

ResultsTab::ResultsTab(const QString& token, QWidget* parent) : QWidget(parent), m_token(token) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QPushButton* btn = new QPushButton("Обновить", this);
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Место", "Участник", "Баллы"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    l->addWidget(btn); l->addWidget(m_table);
    connect(btn, &QPushButton::clicked, [this](){ if (m_currentContest != -1) loadResults(m_currentContest); });
}

void ResultsTab::loadResults(int contestId) {
    qDebug() << "Client: Loading results for contest ID:" << contestId;
    m_currentContest = contestId;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkReply* r = m->get(QNetworkRequest(QUrl(QString("http://localhost:8080/api/results?contest_id=%1").arg(contestId))));
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Results loaded successfully";
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            m_table->setRowCount(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject o = arr[i].toObject();
                m_table->setItem(i, 0, new QTableWidgetItem(QString::number(o["place"].toInt())));
                m_table->setItem(i, 1, new QTableWidgetItem(o["username"].toString()));
                m_table->setItem(i, 2, new QTableWidgetItem(QString::number(o["total_score"].toInt())));
            }
        } else {
            qDebug() << "Client Error loading results:" << r->errorString();
        }
        r->deleteLater(); m->deleteLater();
    });
}
