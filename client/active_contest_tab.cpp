#include "api_config.h"
// Force rebuild
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
#include <QHeaderView>
#include <QDialog>
#include <QPdfDocument>
#include <QPdfView>
#include <QTemporaryFile>

ActiveContestTab::ActiveContestTab(const QString& token, const QString& role, QWidget* parent) : QWidget(parent), m_token(token), m_role(role) {
    QHBoxLayout* mainL = new QHBoxLayout(this);
    m_tasks = new QListWidget(this);
    QVBoxLayout* rightL = new QVBoxLayout();
    m_desc = new QLabel("Выберите задачу слева", this); m_desc->setWordWrap(true);
    
    QPushButton* btnPdfTask = new QPushButton("Сгенерировать PDF условия (Typst)", this);
    
    m_answer = new QTextEdit(this);
    
    // Подключаем подсветку синтаксиса для поля ответа (LaTeX, Typst)
    new MathHighlighter(m_answer->document());

    QPushButton* btnPreviewAnswer = new QPushButton("Предпросмотр ответа (PDF / Typst)", this);
    QPushButton* btnSub = new QPushButton("Отправить решение", this);
    
    m_btnShowEditorial = new QPushButton("Посмотреть разбор (Typst)", this);
    m_btnShowEditorial->hide();
    
    m_btnAllSubmissions = new QPushButton("Все решения (Взломать)", this);

    connect(m_btnShowEditorial, &QPushButton::clicked, [this]() {
        if (!m_tasks->currentItem()) return;
        int id = m_tasks->currentItem()->data(Qt::UserRole).toInt();
        compileAndShowPdf(m_editorialMap[id]);
    });
    
    connect(m_btnAllSubmissions, &QPushButton::clicked, this, &ActiveContestTab::showAllSubmissions);

    m_submissionsTable = new QTableWidget(0, 4, this);
    m_submissionsTable->setHorizontalHeaderLabels({"Оценка", "Обратная связь", "Ответ", "Статус"});
    m_submissionsTable->horizontalHeader()->setStretchLastSection(true);
    m_submissionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_submissionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_pdfDoc = new QPdfDocument(this);
    m_pdfView = new QPdfView(this);
    m_pdfView->setDocument(m_pdfDoc);
    m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    m_pdfTempFile = new QTemporaryFile(this);
    
    m_compileTimer = new QTimer(this);
    m_compileTimer->setSingleShot(true);
    
    connect(m_answer, &QTextEdit::textChanged, [this]() {
        m_compileTimer->start(1000); // 1s debounce
    });
    connect(m_compileTimer, &QTimer::timeout, [this]() {
        compileRealtime(m_answer->toPlainText());
    });

    QHBoxLayout* subsHeaderL = new QHBoxLayout();
    subsHeaderL->addWidget(new QLabel("Мои посылки по этой задаче:"));
    QPushButton* btnRefreshSubs = new QPushButton("Обновить посылки", this);
    subsHeaderL->addWidget(btnRefreshSubs);
    subsHeaderL->addWidget(m_btnAllSubmissions);
    subsHeaderL->addStretch();
    
    rightL->addWidget(m_desc); 
    rightL->addWidget(btnPdfTask);
    rightL->addWidget(m_btnShowEditorial);
    
    QHBoxLayout* editorL = new QHBoxLayout();
    editorL->addWidget(m_answer, 1);
    editorL->addWidget(m_pdfView, 1);
    rightL->addLayout(editorL); 
    
    rightL->addWidget(btnSub);
    rightL->addLayout(subsHeaderL);
    rightL->addWidget(m_submissionsTable);
    
    mainL->addWidget(m_tasks, 1); mainL->addLayout(rightL, 2);

    connect(m_tasks, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        int id = item->data(Qt::UserRole).toInt();
        m_desc->setText(m_taskMap[id]);
        if (m_editorialMap.contains(id) && !m_editorialMap[id].isEmpty()) m_btnShowEditorial->show();
        else m_btnShowEditorial->hide();
        loadSubmissions(id);
    });
    connect(btnRefreshSubs, &QPushButton::clicked, [this]() {
        if (m_tasks->currentItem()) {
            loadSubmissions(m_tasks->currentItem()->data(Qt::UserRole).toInt());
        }
    });
    connect(btnPdfTask, &QPushButton::clicked, this, [this]() {
        compileAndShowPdf(m_desc->text());
    });
    connect(btnPreviewAnswer, &QPushButton::clicked, this, [this]() {
        compileAndShowPdf(m_answer->toPlainText());
    });
    connect(btnSub, &QPushButton::clicked, this, &ActiveContestTab::submit);
}

void ActiveContestTab::compileRealtime(const QString& typstCode) {
    if (typstCode.isEmpty()) {
        m_pdfDoc->close();
        return;
    }
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/compile_typst"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["code"] = typstCode;
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QByteArray pdfData = r->readAll();
            
            // Delete old temporary file if it exists, to not leak files
            if (m_pdfTempFile) {
                m_pdfTempFile->deleteLater();
            }
            m_pdfTempFile = new QTemporaryFile(this);
            if (m_pdfTempFile->open()) {
                m_pdfTempFile->write(pdfData);
                m_pdfTempFile->flush();
                m_pdfDoc->load(m_pdfTempFile->fileName());
            }
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ActiveContestTab::compileAndShowPdf(const QString& typstCode) {
    if (typstCode.isEmpty()) return;
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/compile_typst"));
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
            QString errText = r->readAll();
            QMessageBox::warning(this, "Ошибка", "Не удалось скомпилировать PDF. Ошибка: " + r->errorString() + "\n" + errText);
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ActiveContestTab::loadContest(int contestId, const QString&) {
    qDebug() << "Client: Loading contest tasks for ID:" << contestId;
    m_contestId = contestId; m_tasks->clear(); m_taskMap.clear(); m_answer->clear();
    m_btnAllSubmissions->setEnabled(false);
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkReply* r = m->get(QNetworkRequest(QUrl(QString(ApiConfig::baseUrl + "/api/tasks?contest_id=%1").arg(contestId))));
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            qDebug() << "Client: Fetched tasks successfully";
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            for (auto v : arr) {
                QJsonObject o = v.toObject(); int id = o["id"].toInt();
                m_taskMap[id] = o["description"].toString();
                if (o.contains("editorial")) m_editorialMap[id] = o["editorial"].toString();
                QListWidgetItem* item = new QListWidgetItem(o["title"].toString());
                item->setData(Qt::UserRole, id); m_tasks->addItem(item);
            }
        } else {
            qDebug() << "Client Error fetching tasks:" << r->errorString();
        }
        r->deleteLater(); m->deleteLater();
    });
}


void ActiveContestTab::submit() {
    if (!m_tasks->currentItem() || m_answer->toPlainText().isEmpty()) {
        qDebug() << "Client: Submit failed. Task not selected or answer is empty.";
        return;
    }
    int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();
    qDebug() << "Client: Submitting answer for task ID:" << tId;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/submit"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["task_id"] = tId; j["answer"] = m_answer->toPlainText();
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m, tId]() {
        if (r->error() == QNetworkReply::NoError) { 
            qDebug() << "Client: Submission successful";
            QMessageBox::information(this, "Ок", "Отправлено на проверку ИИ!"); 
            m_answer->clear(); 
            loadSubmissions(tId); // Reload submissions
        } else {
            QString errText = r->readAll();
            qDebug() << "Client Error in submission:" << r->errorString() << errText;
            QMessageBox::warning(this, "Ошибка", "Ошибка со стороны сервера: " + r->errorString() + "\n" + errText);
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ActiveContestTab::showAllSubmissions() {
    if (!m_tasks->currentItem()) return;
    int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();
    
    QDialog dlg(this);
    dlg.setWindowTitle("Все посылки по задаче (Взломы)");
    dlg.resize(800, 600);
    QVBoxLayout* l = new QVBoxLayout(&dlg);
    
    QTableWidget* table = new QTableWidget(0, 4, &dlg);
    table->setHorizontalHeaderLabels({"ID Посылки", "Пользователь", "Оценка", "Ответ"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    l->addWidget(table);
    
    QNetworkAccessManager* m = new QNetworkAccessManager(&dlg);
    QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/submissions/all?task_id=%1").arg(tId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [table, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            table->setRowCount(arr.size());
            for (int i=0; i<arr.size(); ++i) {
                QJsonObject o = arr[i].toObject();
                QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(o["id"].toInt()));
                table->setItem(i, 0, idItem);
                table->setItem(i, 1, new QTableWidgetItem(o["username"].toString()));
                table->setItem(i, 2, new QTableWidgetItem(QString::number(o["score"].toInt())));
                table->setItem(i, 3, new QTableWidgetItem(o["answer_text"].toString()));
            }
        }
        r->deleteLater(); m->deleteLater();
    });
    
    QPushButton* hackBtn = new QPushButton("Взломать выбранное", &dlg);
    l->addWidget(hackBtn);
    
    connect(hackBtn, &QPushButton::clicked, [this, &dlg, table]() {
        int row = table->currentRow();
        if (row < 0) return;
        int subId = table->item(row, 0)->text().toInt();
        
        QDialog hackDlg(&dlg);
        hackDlg.setWindowTitle("Взлом решения");
        QVBoxLayout* hl = new QVBoxLayout(&hackDlg);
        QTextEdit* hackText = new QTextEdit(&hackDlg);
        hackText->setPlaceholderText("Опишите ошибку или приведите контрпример...");
        QPushButton* sendBtn = new QPushButton("Отправить взлом", &hackDlg);
        hl->addWidget(hackText); hl->addWidget(sendBtn);
        
        connect(sendBtn, &QPushButton::clicked, [this, &hackDlg, subId, hackText]() {
            QNetworkAccessManager* hm = new QNetworkAccessManager(&hackDlg);
            QNetworkRequest hreq(QUrl(ApiConfig::baseUrl + "/api/hacks"));
            hreq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            hreq.setRawHeader("Authorization", m_token.toUtf8());
            QJsonObject hj; hj["submission_id"] = subId; hj["hack_text"] = hackText->toPlainText();
            QNetworkReply* hr = hm->post(hreq, QJsonDocument(hj).toJson());
            connect(hr, &QNetworkReply::finished, [&hackDlg, hr, hm]() {
                if (hr->error() == QNetworkReply::NoError) {
                    QMessageBox::information(&hackDlg, "Ок", "Взлом отправлен, ожидайте вердикта ИИ!");
                    hackDlg.accept();
                } else {
                    QMessageBox::warning(&hackDlg, "Ошибка", "Ошибка отправки взлома");
                }
                hr->deleteLater(); hm->deleteLater();
            });
        });
        hackDlg.exec();
    });
    
    dlg.exec();
}

void ActiveContestTab::loadSubmissions(int taskId) {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    m_btnAllSubmissions->setEnabled(m_role == "admin" || m_role == "superadmin");
    QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/submissions?task_id=%1").arg(taskId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            m_submissionsTable->setRowCount(arr.size());
            bool hasSolved = false;
            for (int i = 0; i < arr.size(); ++i) {
                QJsonObject o = arr[i].toObject();
                int score = o["score"].toInt();
                m_submissionsTable->setItem(i, 0, new QTableWidgetItem(QString::number(score)));
                m_submissionsTable->setItem(i, 1, new QTableWidgetItem(o["feedback"].toString()));
                m_submissionsTable->setItem(i, 2, new QTableWidgetItem(o["answer"].toString()));
                m_submissionsTable->setItem(i, 3, new QTableWidgetItem(o["status"].toString()));
                if (score >= 100) hasSolved = true;
            }
            if (m_role == "admin" || m_role == "superadmin") hasSolved = true;
            m_btnAllSubmissions->setEnabled(hasSolved);
        }
        r->deleteLater(); m->deleteLater();
    });
}
