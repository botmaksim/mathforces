#include "active_contest_tab.h"

#include "api_config.h"
#include "math_highlighter.h"
#include "network_utils.h"
#include "table_utils.h"
#include "toast.h"
#include "welcome_widget.h"

#include <QDebug>
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPdfDocument>
#include <QPdfView>
#include <QPushButton>
#include <QTemporaryFile>
#include <QVBoxLayout>

ActiveContestTab::ActiveContestTab(const QString &token, const QString &role,
                                   QWidget *parent)
    : QWidget(parent), m_token(token), m_role(role) {
  QVBoxLayout *outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  m_stack = new QStackedWidget(this);
  outer->addWidget(m_stack);

  m_stack->addWidget(new WelcomeWidget(this));

  m_contestPage = new QWidget(this);
  QVBoxLayout *pageLayout = new QVBoxLayout(m_contestPage);
  pageLayout->setContentsMargins(4, 4, 4, 4);
  pageLayout->setSpacing(12);

  m_contestTitle = new QLabel("Контест", m_contestPage);
  m_contestTitle->setObjectName("sectionTitle");
  pageLayout->addWidget(m_contestTitle);

  m_mainSplitter = new QSplitter(Qt::Horizontal, m_contestPage);
  m_mainSplitter->setChildrenCollapsible(false);
  pageLayout->addWidget(m_mainSplitter, 1);

  QFrame *leftPanel = new QFrame(m_mainSplitter);
  leftPanel->setObjectName("taskPanel");
  QVBoxLayout *leftL = new QVBoxLayout(leftPanel);
  leftL->setContentsMargins(14, 14, 14, 14);
  leftL->setSpacing(10);
  QLabel *taskLabel = new QLabel("Задачи", leftPanel);
  taskLabel->setObjectName("sectionTitle");
  m_tasks = new QListWidget(leftPanel);
  m_tasks->setObjectName("taskList");
  leftL->addWidget(taskLabel);
  leftL->addWidget(m_tasks, 1);

  QFrame *rightPanel = new QFrame(m_mainSplitter);
  rightPanel->setObjectName("taskPanel");
  QVBoxLayout *rightL = new QVBoxLayout(rightPanel);
  rightL->setContentsMargins(14, 14, 14, 14);
  rightL->setSpacing(12);

  m_desc = new QLabel("Выберите задачу слева — здесь появится условие.", rightPanel);
  m_desc->setObjectName("infoCard");
  m_desc->setWordWrap(true);
  m_desc->setTextInteractionFlags(Qt::TextSelectableByMouse);

  QHBoxLayout *taskActions = new QHBoxLayout();
  QPushButton *btnPdfTask = new QPushButton("Открыть условие в PDF", rightPanel);
  m_btnShowEditorial = new QPushButton("Посмотреть разбор (Typst)", rightPanel);
  m_btnShowEditorial->hide();
  taskActions->addWidget(btnPdfTask);
  taskActions->addWidget(m_btnShowEditorial);
  taskActions->addStretch();

  m_answer = new QTextEdit(rightPanel);
  m_answer->setPlaceholderText("Пишите решение здесь. Можно использовать LaTeX/Typst — предпросмотр справа обновится автоматически.");
  new MathHighlighter(m_answer->document());

  m_pdfDoc = new QPdfDocument(this);
  m_pdfView = new QPdfView(rightPanel);
  m_pdfView->setObjectName("pdfPreview");
  m_pdfView->setDocument(m_pdfDoc);
  m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
  m_pdfTempFile = new QTemporaryFile(this);

  m_editorSplitter = new QSplitter(Qt::Horizontal, rightPanel);
  m_editorSplitter->setChildrenCollapsible(false);
  m_editorSplitter->addWidget(m_answer);
  m_editorSplitter->addWidget(m_pdfView);
  m_editorSplitter->setStretchFactor(0, 1);
  m_editorSplitter->setStretchFactor(1, 1);

  QPushButton *btnPreviewAnswer = new QPushButton("Предпросмотр ответа", rightPanel);
  QPushButton *btnSub = new QPushButton("Отправить решение", rightPanel);
  QHBoxLayout *actionsL = new QHBoxLayout();
  actionsL->setSpacing(10);
  actionsL->addWidget(btnPreviewAnswer);
  actionsL->addWidget(btnSub);
  actionsL->addStretch();

  m_btnAllSubmissions = new QPushButton("Все решения и хаки", rightPanel);
  QPushButton *btnRefreshSubs = new QPushButton("Обновить", rightPanel);
  QHBoxLayout *subsHeaderL = new QHBoxLayout();
  QLabel *subsTitle = new QLabel("Мои посылки по этой задаче:", rightPanel);
  subsTitle->setObjectName("mutedLabel");
  subsHeaderL->addWidget(subsTitle);
  subsHeaderL->addWidget(btnRefreshSubs);
  subsHeaderL->addWidget(m_btnAllSubmissions);
  subsHeaderL->addStretch();

  m_submissionsTable = new QTableWidget(0, 4, rightPanel);
  m_submissionsTable->setHorizontalHeaderLabels(
      {"Оценка", "Обратная связь", "Ответ", "Статус"});
  TableUtils::prepareTable(m_submissionsTable);
  QLineEdit *submissionSearch = TableUtils::attachSearch(
      m_submissionsTable, rightPanel, "Поиск в моих посылках...");

  rightL->addWidget(m_desc);
  rightL->addLayout(taskActions);
  rightL->addWidget(m_editorSplitter, 3);
  rightL->addLayout(actionsL);
  rightL->addLayout(subsHeaderL);
  rightL->addWidget(submissionSearch);
  rightL->addWidget(m_submissionsTable, 2);

  m_mainSplitter->addWidget(leftPanel);
  m_mainSplitter->addWidget(rightPanel);
  m_mainSplitter->setStretchFactor(0, 1);
  m_mainSplitter->setStretchFactor(1, 3);

  m_stack->addWidget(m_contestPage);

  m_compileTimer = new QTimer(this);
  m_compileTimer->setSingleShot(true);
  connect(m_answer, &QTextEdit::textChanged, [this]() { m_compileTimer->start(900); });
  connect(m_compileTimer, &QTimer::timeout,
          [this]() { compileRealtime(m_answer->toPlainText()); });

  connect(m_btnShowEditorial, &QPushButton::clicked, [this]() {
    if (!m_tasks->currentItem())
      return;
    const int id = m_tasks->currentItem()->data(Qt::UserRole).toInt();
    compileAndShowPdf(m_editorialMap[id]);
  });
  connect(m_btnAllSubmissions, &QPushButton::clicked, this,
          &ActiveContestTab::showAllSubmissions);

  connect(m_tasks, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
    const int id = item->data(Qt::UserRole).toInt();
    m_desc->setText(m_taskMap[id]);
    if (m_editorialMap.contains(id) && !m_editorialMap[id].isEmpty())
      m_btnShowEditorial->show();
    else
      m_btnShowEditorial->hide();
    loadSubmissions(id);
  });
  connect(btnRefreshSubs, &QPushButton::clicked, [this]() {
    if (m_tasks->currentItem())
      loadSubmissions(m_tasks->currentItem()->data(Qt::UserRole).toInt());
  });
  connect(btnPdfTask, &QPushButton::clicked, this,
          [this]() { compileAndShowPdf(m_desc->text()); });
  connect(btnPreviewAnswer, &QPushButton::clicked, this,
          [this]() { compileAndShowPdf(m_answer->toPlainText()); });
  connect(btnSub, &QPushButton::clicked, this, &ActiveContestTab::submit);
}

void ActiveContestTab::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  setCompactMode(event->size().width() < 920);
}

void ActiveContestTab::setCompactMode(bool compact) {
  if (m_compact == compact)
    return;
  m_compact = compact;
  if (m_mainSplitter)
    m_mainSplitter->setOrientation(compact ? Qt::Vertical : Qt::Horizontal);
  if (m_editorSplitter)
    m_editorSplitter->setOrientation(compact ? Qt::Vertical : Qt::Horizontal);
}

void ActiveContestTab::compileRealtime(const QString &typstCode) {
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
      const QByteArray pdfData = r->readAll();
      if (m_pdfTempFile)
        m_pdfTempFile->deleteLater();
      m_pdfTempFile = new QTemporaryFile(this);
      if (m_pdfTempFile->open()) {
        m_pdfTempFile->write(pdfData);
        m_pdfTempFile->flush();
        m_pdfDoc->load(m_pdfTempFile->fileName());
      }
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ActiveContestTab::compileAndShowPdf(const QString &typstCode) {
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
      const QByteArray pdfData = r->readAll();
      QTemporaryFile *tempFile = new QTemporaryFile(this);
      if (tempFile->open()) {
        tempFile->write(pdfData);
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

void ActiveContestTab::loadContest(int contestId, const QString &title) {
  qDebug() << "Client: Loading contest tasks for ID:" << contestId;
  m_contestId = contestId;
  m_stack->setCurrentWidget(m_contestPage);
  m_contestTitle->setText(title.isEmpty() ? "Текущий контест" : title);
  m_tasks->clear();
  m_taskMap.clear();
  m_editorialMap.clear();
  m_answer->clear();
  m_pdfDoc->close();
  m_btnAllSubmissions->setEnabled(false);
  m_submissionsTable->setRowCount(0);
  m_desc->setText("Загрузка задач...");

  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkReply *r = m->get(QNetworkRequest(
      QUrl(QString(ApiConfig::baseUrl + "/api/tasks?contest_id=%1").arg(contestId))));
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      for (auto v : arr) {
        QJsonObject o = v.toObject();
        const int id = o["id"].toInt();
        m_taskMap[id] = o["description"].toString();
        if (o.contains("editorial"))
          m_editorialMap[id] = o["editorial"].toString();
        QListWidgetItem *item = new QListWidgetItem(o["title"].toString());
        item->setData(Qt::UserRole, id);
        m_tasks->addItem(item);
      }
      m_desc->setText(arr.isEmpty() ? "В этом контесте пока нет задач." : "Выберите задачу слева — здесь появится условие.");
    } else {
      NetworkUtils::showError(this, "Не удалось загрузить задачи", r);
      m_desc->setText("Не удалось загрузить задачи. Проверьте сеть и сервер.");
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ActiveContestTab::submit() {
  if (!m_tasks->currentItem() || m_answer->toPlainText().trimmed().isEmpty()) {
    Toast::show(this, "Выберите задачу и напишите решение");
    return;
  }
  const int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/submit"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["task_id"] = tId;
  j["answer"] = m_answer->toPlainText();
  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m, tId]() {
    if (r->error() == QNetworkReply::NoError) {
      Toast::show(this, "Решение успешно отправлено на проверку");
      m_answer->clear();
      loadSubmissions(tId);
    } else {
      NetworkUtils::showError(this, "Ошибка отправки решения", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void ActiveContestTab::showAllSubmissions() {
  if (!m_tasks->currentItem())
    return;
  const int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();

  QDialog dlg(this);
  dlg.setWindowTitle("Все посылки по задаче и взломы");
  dlg.resize(900, 650);
  QVBoxLayout *l = new QVBoxLayout(&dlg);

  QTableWidget *table = new QTableWidget(0, 4, &dlg);
  table->setHorizontalHeaderLabels({"ID посылки", "Пользователь", "Оценка", "Ответ"});
  TableUtils::prepareTable(table);
  l->addWidget(TableUtils::attachSearch(table, &dlg, "Поиск по решениям..."));
  l->addWidget(table, 1);

  QNetworkAccessManager *m = new QNetworkAccessManager(&dlg);
  QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/submissions/all?task_id=%1").arg(tId)));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QNetworkReply *r = m->get(req);
  connect(r, &QNetworkReply::finished, [&dlg, table, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      table->setSortingEnabled(false);
      table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        table->setItem(i, 0, TableUtils::numericItem(o["id"].toInt()));
        table->setItem(i, 1, TableUtils::textItem(o["username"].toString()));
        table->setItem(i, 2, TableUtils::numericItem(o["score"].toInt()));
        table->setItem(i, 3, TableUtils::textItem(o["answer_text"].toString()));
      }
      table->setSortingEnabled(true);
    } else {
      NetworkUtils::showError(&dlg, "Не удалось загрузить решения", r);
    }
    r->deleteLater();
    m->deleteLater();
  });

  QPushButton *hackBtn = new QPushButton("Взломать выбранное", &dlg);
  l->addWidget(hackBtn);

  connect(hackBtn, &QPushButton::clicked, [this, &dlg, table]() {
    int row = table->currentRow();
    if (row < 0)
      return;
    const int subId = table->item(row, 0)->text().toInt();

    QDialog hackDlg(&dlg);
    hackDlg.setWindowTitle("Взлом решения");
    QVBoxLayout *hl = new QVBoxLayout(&hackDlg);
    QTextEdit *hackText = new QTextEdit(&hackDlg);
    hackText->setPlaceholderText("Опишите ошибку или приведите контрпример...");
    QPushButton *sendBtn = new QPushButton("Отправить взлом", &hackDlg);
    hl->addWidget(hackText);
    hl->addWidget(sendBtn);

    connect(sendBtn, &QPushButton::clicked, [this, &hackDlg, subId, hackText]() {
      QNetworkAccessManager *hm = new QNetworkAccessManager(&hackDlg);
      QNetworkRequest hreq(QUrl(ApiConfig::baseUrl + "/api/hacks"));
      hreq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
      hreq.setRawHeader("Authorization", m_token.toUtf8());
      QJsonObject hj;
      hj["submission_id"] = subId;
      hj["hack_text"] = hackText->toPlainText();
      QNetworkReply *hr = hm->post(hreq, QJsonDocument(hj).toJson());
      connect(hr, &QNetworkReply::finished, [&hackDlg, hr, hm]() {
        if (hr->error() == QNetworkReply::NoError) {
          Toast::show(&hackDlg, "Взлом отправлен, ожидайте вердикта ИИ");
          hackDlg.accept();
        } else {
          NetworkUtils::showError(&hackDlg, "Ошибка отправки взлома", hr);
        }
        hr->deleteLater();
        hm->deleteLater();
      });
    });
    hackDlg.exec();
  });

  dlg.exec();
}

void ActiveContestTab::loadSubmissions(int taskId) {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  m_btnAllSubmissions->setEnabled(m_role == "admin" || m_role == "superadmin");
  QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/submissions?task_id=%1").arg(taskId)));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QNetworkReply *r = m->get(req);
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      m_submissionsTable->setSortingEnabled(false);
      m_submissionsTable->setRowCount(arr.size());
      bool hasSolved = false;
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        const int score = o["score"].toInt();
        m_submissionsTable->setItem(i, 0, TableUtils::numericItem(score));
        m_submissionsTable->setItem(i, 1, TableUtils::textItem(o["feedback"].toString()));
        m_submissionsTable->setItem(i, 2, TableUtils::textItem(o["answer"].toString()));
        m_submissionsTable->setItem(i, 3, TableUtils::textItem(o["status"].toString()));
        if (score >= 100)
          hasSolved = true;
      }
      if (m_role == "admin" || m_role == "superadmin")
        hasSolved = true;
      m_btnAllSubmissions->setEnabled(hasSolved);
      m_submissionsTable->setSortingEnabled(true);
    } else {
      NetworkUtils::showError(this, "Не удалось загрузить посылки", r);
    }
    r->deleteLater();
    m->deleteLater();
  });
}
