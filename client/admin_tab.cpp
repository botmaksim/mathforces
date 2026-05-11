#include "admin_tab.h"
#include "math_highlighter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QLabel>
#include <QDebug>

AdminTab::AdminTab(const QString& token, QWidget* parent) : QWidget(parent), m_token(token) {
    QHBoxLayout* ML = new QHBoxLayout(this);
    
    QGroupBox* g1 = new QGroupBox("Новый Контест"); QVBoxLayout* l1 = new QVBoxLayout(g1);
    m_cTitle = new QLineEdit(); m_cTitle->setPlaceholderText("Название");
    m_cStart = new QLineEdit(); m_cStart->setPlaceholderText("Начало 2026-01-01 10:00:00 (UTC+3)");
    m_cDuration = new QLineEdit(); m_cDuration->setPlaceholderText("Длительность (часы), напр. 2.5");
    m_cDesc = new QTextEdit(); m_cDesc->setPlaceholderText("Описание...");
    new MathHighlighter(m_cDesc->document());
    QPushButton* b1 = new QPushButton("Создать контест");
    l1->addWidget(m_cTitle); l1->addWidget(m_cStart); l1->addWidget(m_cDuration); l1->addWidget(m_cDesc); l1->addWidget(b1);
    
    QGroupBox* g2 = new QGroupBox("Новая Задача"); QVBoxLayout* l2 = new QVBoxLayout(g2);
    m_tContestId = new QLineEdit(); m_tContestId->setPlaceholderText("ID Контеста");
    m_tTitle = new QLineEdit(); m_tTitle->setPlaceholderText("Название");
    m_tScore = new QLineEdit(); m_tScore->setPlaceholderText("Макс Балл (100)");
    m_tMaxSubmissions = new QLineEdit(); m_tMaxSubmissions->setPlaceholderText("Макс посылок (напр. 10)");
    
    m_tType = new QComboBox();
    m_tType->addItem("Только ответ", "answer_only");
    m_tType->addItem("Решение", "solution");
    
    m_tDesc = new QTextEdit(); m_tDesc->setPlaceholderText("Условие задачи (LaTeX/Typst)...");
    new MathHighlighter(m_tDesc->document());

    m_tCorrectAnswer = new QLineEdit(); m_tCorrectAnswer->setPlaceholderText("Правильный ответ (для answer_only)");
    
    m_tEditorial = new QTextEdit(); m_tEditorial->setPlaceholderText("Решение (разбор) задачи...");
    m_tSendEditorialToAi = new QCheckBox("Отправлять решение ИИ для проверки?");
    
    m_tAiComment = new QTextEdit(); m_tAiComment->setPlaceholderText("Комментарий для нейросети (подсказки, критерии проверки)...");
    m_tAiComment->setMaximumHeight(60);
    
    QPushButton* b2 = new QPushButton("Создать задачу");
    
    l2->addWidget(m_tContestId); l2->addWidget(m_tTitle); l2->addWidget(m_tScore); l2->addWidget(m_tMaxSubmissions);
    l2->addWidget(new QLabel("Тип:")); l2->addWidget(m_tType);
    l2->addWidget(new QLabel("Условие:")); l2->addWidget(m_tDesc);
    l2->addWidget(m_tCorrectAnswer);
    l2->addWidget(new QLabel("Авторское решение (для ИИ):")); l2->addWidget(m_tEditorial); l2->addWidget(m_tSendEditorialToAi);
    l2->addWidget(m_tAiComment); l2->addWidget(b2);

    ML->addWidget(g1); ML->addWidget(g2);
    connect(b1, &QPushButton::clicked, this, &AdminTab::createContest);
    connect(b2, &QPushButton::clicked, this, &AdminTab::createTask);
    connect(m_tType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AdminTab::onTaskTypeChanged);
    onTaskTypeChanged(0);
}

void AdminTab::onTaskTypeChanged(int index) {
    if (m_tType->itemData(index).toString() == "answer_only") {
        m_tCorrectAnswer->setVisible(true);
        m_tEditorial->setVisible(false);
        m_tSendEditorialToAi->setVisible(false);
        m_tAiComment->setVisible(false);
    } else {
        m_tCorrectAnswer->setVisible(false);
        m_tEditorial->setVisible(true);
        m_tSendEditorialToAi->setVisible(true);
        m_tAiComment->setVisible(true);
    }
}

void AdminTab::createContest() {
    qDebug() << "Client: Creating contest:" << m_cTitle->text();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/admin/contest"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["title"] = m_cTitle->text(); j["start"] = m_cStart->text(); j["duration_hours"] = m_cDuration->text().toFloat(); j["description"] = m_cDesc->toPlainText();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m](){
        if(r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Contest created successfully";
            QMessageBox::information(this, "Ок", "Контест создан!");
        } else {
            qDebug() << "Client Error: Failed to create contest:" << r->errorString() << "-" << r->readAll();
            QMessageBox::warning(this, "Ошибка", "Ошибка: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}
void AdminTab::createTask() {
    qDebug() << "Client: Creating task:" << m_tTitle->text() << "for contest ID:" << m_tContestId->text();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/admin/task"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); req.setRawHeader("Authorization", m_token.toUtf8());
    
    QJsonObject j; 
    j["contest_id"] = m_tContestId->text().toInt(); 
    j["title"] = m_tTitle->text(); 
    j["max_score"] = m_tScore->text().toInt(); 
    j["max_submissions"] = m_tMaxSubmissions->text().toInt();
    j["description"] = m_tDesc->toPlainText(); 
    
    QString typeChoice = m_tType->currentData().toString();
    j["task_type"] = typeChoice;
    
    if (typeChoice == "answer_only") {
        j["correct_answer"] = m_tCorrectAnswer->text();
    } else {
        j["editorial"] = m_tEditorial->toPlainText();
        j["send_editorial_to_ai"] = m_tSendEditorialToAi->isChecked();
        j["ai_comment"] = m_tAiComment->toPlainText();
    }
    
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m](){
        if(r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Task created successfully";
            QMessageBox::information(this, "Ок", "Задача создана!");
        } else {
            qDebug() << "Client Error: Failed to create task:" << r->errorString() << "-" << r->readAll();
            QMessageBox::warning(this, "Ошибка", "Ошибка: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}
