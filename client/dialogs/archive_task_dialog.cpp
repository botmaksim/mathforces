#include "archive_task_dialog.h"

#include "api_config.h"
#include "math_highlighter.h"
#include "network_utils.h"
#include "table_utils.h"
#include "toast.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTemporaryFile>
#include <QVBoxLayout>

ArchiveTaskDialog::ArchiveTaskDialog(const QString &token, int taskId, QWidget *parent)
    : QDialog(parent), m_token(token), m_taskId(taskId) {
  setWindowTitle("Задача из архива");
  resize(1050, 760);
  setMinimumSize(760, 540);

  QVBoxLayout *root = new QVBoxLayout(this);
  root->setContentsMargins(18, 18, 18, 18);
  root->setSpacing(12);

  m_title = new QLabel("Загрузка задачи...", this);
  m_title->setObjectName("sectionTitle");
  m_title->setWordWrap(true);
  m_meta = new QLabel(this);
  m_meta->setObjectName("mutedLabel");
  m_meta->setWordWrap(true);

  m_description = new QTextEdit(this);
  m_description->setObjectName("readOnlyCard");
  m_description->setReadOnly(true);
  m_description->setMinimumHeight(150);

  QHBoxLayout *taskButtons = new QHBoxLayout();
  QPushButton *btnPdfTask = new QPushButton("Открыть условие в PDF", this);
  QPushButton *btnRefresh = new QPushButton("Обновить", this);
  taskButtons->addWidget(btnPdfTask);
  taskButtons->addWidget(btnRefresh);
  taskButtons->addStretch();

  m_answer = new QTextEdit(this);
  m_answer->setPlaceholderText("Решение для upsolving в архиве...");
  new MathHighlighter(m_answer->document());

  m_pdfDoc = new QPdfDocument(this);
  m_pdfView = new QPdfView(this);
  m_pdfView->setDocument(m_pdfDoc);
  m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
  m_pdfTempFile = new QTemporaryFile(this);

  m_editorSplitter = new QSplitter(Qt::Horizontal, this);
  m_editorSplitter->setChildrenCollapsible(false);
  m_editorSplitter->addWidget(m_answer);
  m_editorSplitter->addWidget(m_pdfView);
  m_editorSplitter->setStretchFactor(0, 1);
  m_editorSplitter->setStretchFactor(1, 1);

  QHBoxLayout *actions = new QHBoxLayout();
  QPushButton *btnPreview = new QPushButton("Предпросмотр ответа", this);
  QPushButton *btnSubmit = new QPushButton("Отправить решение", this);
  actions->addWidget(btnPreview);
  actions->addWidget(btnSubmit);
  actions->addStretch();

  m_submissionsTable = new QTableWidget(0, 4, this);
  m_submissionsTable->setHorizontalHeaderLabels({"Оценка", "Обратная связь", "Ответ", "Статус"});
  TableUtils::prepareTable(m_submissionsTable);

  root->addWidget(m_title);
  root->addWidget(m_meta);
  root->addWidget(m_description);
  root->addLayout(taskButtons);
  root->addWidget(m_editorSplitter, 3);
  root->addLayout(actions);
  root->addWidget(TableUtils::attachSearch(m_submissionsTable, this, "Поиск в посылках..."));
  root->addWidget(m_submissionsTable, 2);

  m_compileTimer = new QTimer(this);
  m_compileTimer->setSingleShot(true);
  connect(m_answer, &QTextEdit::textChanged, [this]() { m_compileTimer->start(900); });
  connect(m_compileTimer, &QTimer::timeout, [this]() { compileRealtime(m_answer->toPlainText()); });

  connect(btnRefresh, &QPushButton::clicked, this, &ArchiveTaskDialog::loadTask);
  connect(btnPdfTask, &QPushButton::clicked, this, [this]() { compileAndShowPdf(m_description->toPlainText()); });
  connect(btnPreview, &QPushButton::clicked, this, [this]() { compileAndShowPdf(m_answer->toPlainText()); });
  connect(btnSubmit, &QPushButton::clicked, this, &ArchiveTaskDialog::submit);

  loadTask();
  loadSubmissions();
}

void ArchiveTaskDialog::loadTask() {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkReply *r = m->get(QNetworkRequest(
      QUrl(QString(ApiConfig::baseUrl + "/api/archive/task?id=%1").arg(m_taskId))));
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      renderTask(QJsonDocument::fromJson(r->readAll()).object());
    } else {
      NetworkUtils::showError(this, "Не удалось открыть задачу", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ArchiveTaskDialog::renderTask(const QJsonObject &task) {
  m_title->setText(task.value("title").toString("Задача"));
  const QString tags = task.value("tags").toString();
  const QString contest = task.value("contest_title").toString();
  const int difficulty = task.value("difficulty").toInt();
  const int maxScore = task.value("max_score").toInt();
  m_meta->setText(QStringLiteral("Контест: %1 · Сложность: %2 · Баллы: %3 · Теги: %4")
                      .arg(contest.isEmpty() ? QStringLiteral("без названия") : contest)
                      .arg(difficulty)
                      .arg(maxScore)
                      .arg(tags.isEmpty() ? QStringLiteral("нет") : tags));
  m_description->setPlainText(task.value("description").toString());
}

void ArchiveTaskDialog::submit() {
  if (m_answer->toPlainText().trimmed().isEmpty()) {
    Toast::show(this, "Напишите решение перед отправкой");
    return;
  }

  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/submit"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["task_id"] = m_taskId;
  j["answer"] = m_answer->toPlainText();
  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      Toast::show(this, "Решение отправлено из архива");
      m_answer->clear();
      loadSubmissions();
    } else {
      NetworkUtils::showError(this, "Ошибка отправки решения", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ArchiveTaskDialog::loadSubmissions() {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/submissions?task_id=%1").arg(m_taskId)));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QNetworkReply *r = m->get(req);
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_submissionsTable->setSortingEnabled(false);
      m_submissionsTable->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        m_submissionsTable->setItem(i, 0, TableUtils::numericItem(o["score"].toInt()));
        m_submissionsTable->setItem(i, 1, TableUtils::textItem(o["feedback"].toString()));
        m_submissionsTable->setItem(i, 2, TableUtils::textItem(o["answer"].toString()));
        m_submissionsTable->setItem(i, 3, TableUtils::textItem(o["status"].toString()));
      }
      m_submissionsTable->setSortingEnabled(true);
    } else {
      NetworkUtils::showError(this, "Не удалось загрузить посылки", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ArchiveTaskDialog::compileRealtime(const QString &typstCode) {
  if (typstCode.trimmed().isEmpty()) {
    m_pdfDoc->close();
    return;
  }
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/compile_typst"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["code"] = typstCode;
  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      if (m_pdfTempFile)
        m_pdfTempFile->deleteLater();
      m_pdfTempFile = new QTemporaryFile(this);
      if (m_pdfTempFile->open()) {
        m_pdfTempFile->write(r->readAll());
        m_pdfTempFile->flush();
        m_pdfDoc->load(m_pdfTempFile->fileName());
      }
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ArchiveTaskDialog::compileAndShowPdf(const QString &typstCode) {
  if (typstCode.trimmed().isEmpty())
    return;
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/compile_typst"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["code"] = typstCode;
  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QTemporaryFile *tempFile = new QTemporaryFile(this);
      if (tempFile->open()) {
        tempFile->write(r->readAll());
        tempFile->flush();
        QDialog *pdfDialog = new QDialog(this);
        pdfDialog->setWindowTitle("PDF предпросмотр");
        pdfDialog->resize(900, 700);
        QVBoxLayout *layout = new QVBoxLayout(pdfDialog);
        QPdfDocument *doc = new QPdfDocument(pdfDialog);
        doc->load(tempFile->fileName());
        QPdfView *view = new QPdfView(pdfDialog);
        view->setDocument(doc);
        view->setPageMode(QPdfView::PageMode::MultiPage);
        layout->addWidget(view);
        pdfDialog->exec();
      }
    } else {
      NetworkUtils::showError(this, "Не удалось скомпилировать PDF", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}
