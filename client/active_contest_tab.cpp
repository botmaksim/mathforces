#include "active_contest_tab.h"
#include "math_highlighter.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QDialog>
#include <QPdfDocument>
#include <QPdfView>
#include <QTemporaryFile>

ActiveContestTab::ActiveContestTab(const QString& token, QWidget* parent) : QWidget(parent), m_token(token) {
    QHBoxLayout* mainL = new QHBoxLayout(this);
    m_tasks = new QListWidget(this);
    QVBoxLayout* rightL = new QVBoxLayout();
    m_desc = new QLabel("Выберите задачу слева", this); m_desc->setWordWrap(true);
    
    QPushButton* btnPdfTask = new QPushButton("Сгенерировать PDF условия (Typst)", this);
    
    m_answer = new QTextEdit(this);
    
    // Подключаем подсветку синтаксиса для поля ответа (LaTeX, Typst)
    new MathHighlighter(m_answer->document());

    QPushButton* btnPreviewAnswer = new QPushButton("Предпросмотр ответа (PDF / Typst)", this);
    QPushButton* btnFile = new QPushButton("Загрузить файл (.txt, .tex, .typ)", this);
    QPushButton* btnSub = new QPushButton("Отправить решение", this);
    
    rightL->addWidget(m_desc); 
    rightL->addWidget(btnPdfTask);
    rightL->addWidget(m_answer); 
    rightL->addWidget(btnPreviewAnswer);
    rightL->addWidget(btnFile); 
    rightL->addWidget(btnSub);
    
    mainL->addWidget(m_tasks, 1); mainL->addLayout(rightL, 2);

    connect(m_tasks, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        m_desc->setText(m_taskMap[item->data(Qt::UserRole).toInt()]);
    });
    connect(btnPdfTask, &QPushButton::clicked, this, [this]() {
        compileAndShowPdf(m_desc->text());
    });
    connect(btnPreviewAnswer, &QPushButton::clicked, this, [this]() {
        compileAndShowPdf(m_answer->toPlainText());
    });
    connect(btnFile, &QPushButton::clicked, this, &ActiveContestTab::loadFile);
    connect(btnSub, &QPushButton::clicked, this, &ActiveContestTab::submit);
}

void ActiveContestTab::compileAndShowPdf(const QString& typstCode) {
    if (typstCode.isEmpty()) return;
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/compile_typst"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["code"] = typstCode;
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QByteArray pdfData = r->readAll();
            
            QTemporaryFile* tempFile = new QTemporaryFile(this);
            if (tempFile->open()) {
                tempFile->write(pdfData);
                tempFile->flush();
                
                QDialog* pdfDialog = new QDialog(this);
                pdfDialog->setWindowTitle("PDF Предпросмотр");
                pdfDialog->resize(800, 600);
                QVBoxLayout* layout = new QVBoxLayout(pdfDialog);
                
                QPdfDocument* doc = new QPdfDocument(pdfDialog);
                doc->load(tempFile->fileName());
                
                QPdfView* view = new QPdfView(pdfDialog);
                view->setDocument(doc);
                view->setPageMode(QPdfView::PageMode::MultiPage);
                
                layout->addWidget(view);
                pdfDialog->exec();
            }
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось скомпилировать PDF. Ошибка: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ActiveContestTab::loadContest(int contestId, const QString&) {
    qDebug() << "Client: Loading contest tasks for ID:" << contestId;
    m_contestId = contestId; m_tasks->clear(); m_taskMap.clear(); m_answer->clear();
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkReply* r = m->get(QNetworkRequest(QUrl(QString("http://localhost:8080/api/tasks?contest_id=%1").arg(contestId))));
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Fetched tasks successfully";
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            for (auto v : arr) {
                QJsonObject o = v.toObject(); int id = o["id"].toInt();
                m_taskMap[id] = o["description"].toString();
                QListWidgetItem* item = new QListWidgetItem(o["title"].toString());
                item->setData(Qt::UserRole, id); m_tasks->addItem(item);
            }
        } else {
            qDebug() << "Client Error fetching tasks:" << r->errorString();
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ActiveContestTab::loadFile() {
    QString p = QFileDialog::getOpenFileName(this, "Выбор", "", "Math/Text Files (*.txt *.tex *.typ);;All (*)");
    if (!p.isEmpty()) { 
        qDebug() << "Client: Loading file:" << p;
        QFile f(p); 
        if(f.open(QIODevice::ReadOnly)) m_answer->setPlainText(f.readAll()); 
    }
}

void ActiveContestTab::submit() {
    if (!m_tasks->currentItem() || m_answer->toPlainText().isEmpty()) {
        qDebug() << "Client: Submit failed. Task not selected or answer is empty.";
        return;
    }
    int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();
    qDebug() << "Client: Submitting answer for task ID:" << tId;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/submit"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["task_id"] = tId; j["answer"] = m_answer->toPlainText();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) { 
            qDebug() << "Client: Submission successful";
            QMessageBox::information(this, "Ок", "Отправлено на проверку ИИ!"); 
            m_answer->clear(); 
        } else {
            qDebug() << "Client Error in submission:" << r->errorString();
            QMessageBox::warning(this, "Ошибка", "Ошибка со стороны сервера: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}
