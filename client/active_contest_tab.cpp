#include "active_contest_tab.h"
#include "math_highlighter.h"
#include <QDebug>
#include <QDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QPdfDocument>
#include <QPdfView>
#include <QPushButton>
#include <QTemporaryFile>
#include <QVBoxLayout>

ActiveContestTab::ActiveContestTab(const QString &token, const QString &role,
                                   QWidget *parent)
    : QWidget(parent), m_token(token), m_role(role) {
  
  m_presenter = new ActiveContestPresenter(m_token, this);

  QHBoxLayout *mainL = new QHBoxLayout(this);
  mainL->setContentsMargins(4, 4, 4, 4);
  mainL->setSpacing(16);

  m_tasks = new QListWidget(this);
  m_tasks->setObjectName("taskList");

  QVBoxLayout *rightL = new QVBoxLayout();
  rightL->setContentsMargins(0, 0, 0, 0);
  rightL->setSpacing(12);

  m_desc = new QLabel("Выберите задачу слева - здесь появится условие.", this);
  m_desc->setObjectName("infoCard");
  m_desc->setWordWrap(true);

  QPushButton *btnPdfTask = new QPushButton("Открыть условие в PDF", this);

  m_answer = new QTextEdit(this);
  m_answer->setPlaceholderText("Пишите решение здесь. Можно использовать LaTeX/Typst - предпросмотр справа обновится автоматически.");

  // Подключаем подсветку синтаксиса для поля ответа (LaTeX, Typst)
  new MathHighlighter(m_answer->document());

  QPushButton *btnPreviewAnswer =
      new QPushButton("Предпросмотр ответа (PDF / Typst)", this);
  QPushButton *btnSub = new QPushButton("Отправить решение", this);

  m_btnShowEditorial = new QPushButton("Посмотреть разбор (Typst)", this);
  m_btnShowEditorial->hide();

  m_btnAllSubmissions = new QPushButton("Все решения и хаки", this);

  connect(m_btnShowEditorial, &QPushButton::clicked, [this]() {
    if (!m_tasks->currentItem())
      return;
    int id = m_tasks->currentItem()->data(Qt::UserRole).toInt();
    compileAndShowPdf(m_editorialMap[id]);
  });

  connect(m_btnAllSubmissions, &QPushButton::clicked, this,
          &ActiveContestTab::showAllSubmissions);

  m_submissionsTable = new QTableWidget(0, 4, this);
  m_submissionsTable->setHorizontalHeaderLabels(
      {"Оценка", "Обратная связь", "Ответ", "Статус"});
  m_submissionsTable->horizontalHeader()->setStretchLastSection(true);
  m_submissionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_submissionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_submissionsTable->setAlternatingRowColors(true);

  m_pdfDoc = new QPdfDocument(this);
  m_pdfView = new QPdfView(this);
  m_pdfView->setObjectName("pdfPreview");
  m_pdfView->setDocument(m_pdfDoc);
  m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
  m_pdfTempFile = new QTemporaryFile(this);

  m_compileTimer = new QTimer(this);
  m_compileTimer->setSingleShot(true);

  connect(m_answer, &QTextEdit::textChanged, [this]() {
    m_compileTimer->start(1000); // 1s debounce
    if (m_tasks->currentItem() && !m_isLoadingDraft) {
        int id = m_tasks->currentItem()->data(Qt::UserRole).toInt();
        m_presenter->saveDraft(id, m_answer->toPlainText());
    }
  });
  connect(m_compileTimer, &QTimer::timeout,
          [this]() { compileRealtime(m_answer->toPlainText()); });

  QHBoxLayout *subsHeaderL = new QHBoxLayout();
  subsHeaderL->addWidget(new QLabel("Мои посылки по этой задаче:"));
  QPushButton *btnRefreshSubs = new QPushButton("Обновить посылки", this);
  subsHeaderL->addWidget(btnRefreshSubs);
  subsHeaderL->addWidget(m_btnAllSubmissions);
  subsHeaderL->addStretch();

  rightL->addWidget(m_desc);
  rightL->addWidget(btnPdfTask);
  rightL->addWidget(m_btnShowEditorial);

  QHBoxLayout *editorL = new QHBoxLayout();
  editorL->addWidget(m_answer, 1);
  editorL->addWidget(m_pdfView, 1);
  rightL->addLayout(editorL);

  QHBoxLayout *actionsL = new QHBoxLayout();
  actionsL->setSpacing(10);
  actionsL->addWidget(btnPreviewAnswer);
  actionsL->addWidget(btnSub);
  actionsL->addStretch();
  rightL->addLayout(actionsL);
  rightL->addLayout(subsHeaderL);
  rightL->addWidget(m_submissionsTable);

  mainL->addWidget(m_tasks, 1);
  mainL->addLayout(rightL, 2);

  connect(m_tasks, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
    int id = item->data(Qt::UserRole).toInt();
    m_desc->setText(m_taskMap[id]);
    if (m_editorialMap.contains(id) && !m_editorialMap[id].isEmpty())
      m_btnShowEditorial->show();
    else
      m_btnShowEditorial->hide();
    
    m_isLoadingDraft = true;
    m_answer->setText(m_presenter->loadDraft(id));
    m_isLoadingDraft = false;
    
    loadSubmissions(id);
  });
  
  connect(btnRefreshSubs, &QPushButton::clicked, [this]() {
    if (m_tasks->currentItem()) {
      loadSubmissions(m_tasks->currentItem()->data(Qt::UserRole).toInt());
    }
  });
  
  connect(btnPdfTask, &QPushButton::clicked, this,
          [this]() { compileAndShowPdf(m_desc->text()); });
  connect(btnPreviewAnswer, &QPushButton::clicked, this,
          [this]() { compileAndShowPdf(m_answer->toPlainText()); });
  connect(btnSub, &QPushButton::clicked, this, &ActiveContestTab::submit);
  
  // Presenter hooks
  connect(m_presenter, &ActiveContestPresenter::tasksLoaded, this, [this](const QJsonArray& arr){
      for (auto v : arr) {
        QJsonObject o = v.toObject();
        int id = o["id"].toInt();
        m_taskMap[id] = o["description"].toString();
        if (o.contains("editorial"))
          m_editorialMap[id] = o["editorial"].toString();
        QListWidgetItem *item = new QListWidgetItem(o["title"].toString());
        item->setData(Qt::UserRole, id);
        m_tasks->addItem(item);
      }
  });
  
  connect(m_presenter, &ActiveContestPresenter::submissionSuccessful, this, [this](){
      QMessageBox::information(this, "Ок", "Отправлено на проверку ИИ!");
      m_answer->clear();
      if (m_tasks->currentItem()) {
          int id = m_tasks->currentItem()->data(Qt::UserRole).toInt();
          m_presenter->saveDraft(id, ""); // clear draft
          loadSubmissions(id);
      }
  });
  
  connect(m_presenter, &ActiveContestPresenter::mySubmissionsLoaded, this, [this](const QJsonArray& arr){
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
  });
  
  // allSubmissionsLoaded is hooked locally inside showAllSubmissions to a temporary table
  
  connect(m_presenter, &ActiveContestPresenter::errorOccurred, this, [this](const QString& err){
      QMessageBox::warning(this, "Ошибка", "Ошибка со стороны сервера: " + err);
  });
  
  connect(m_presenter, &ActiveContestPresenter::typstCompiled, this, [this](const QByteArray& pdfData){
      QTemporaryFile *tempFile = new QTemporaryFile(this);
      if (tempFile->open()) {
        tempFile->write(pdfData);
        tempFile->flush();
        QDialog *pdfDialog = new QDialog(this);
        pdfDialog->setWindowTitle("PDF Предпросмотр");
        pdfDialog->resize(800, 600);
        QVBoxLayout *layout = new QVBoxLayout(pdfDialog);
        QPdfDocument *doc = new QPdfDocument(pdfDialog);
        doc->load(tempFile->fileName());
        QPdfView *view = new QPdfView(pdfDialog);
        view->setDocument(doc);
        view->setPageMode(QPdfView::PageMode::MultiPage);
        layout->addWidget(view);
        pdfDialog->exec();
      }
  });
  
  connect(m_presenter, &ActiveContestPresenter::realtimeTypstCompiled, this, [this](const QByteArray& pdfData){
      if (m_pdfTempFile) m_pdfTempFile->deleteLater();
      m_pdfTempFile = new QTemporaryFile(this);
      if (m_pdfTempFile->open()) {
        m_pdfTempFile->write(pdfData);
        m_pdfTempFile->flush();
        m_pdfDoc->load(m_pdfTempFile->fileName());
      }
  });
}

void ActiveContestTab::compileRealtime(const QString &typstCode) {
  if (typstCode.isEmpty()) {
    m_pdfDoc->close();
    return;
  }
  m_presenter->compileRealtime(typstCode);
}

void ActiveContestTab::compileAndShowPdf(const QString &typstCode) {
  if (typstCode.isEmpty()) return;
  m_presenter->compileTypst(typstCode);
}

void ActiveContestTab::loadContest(int contestId, const QString &) {
  m_contestId = contestId;
  m_tasks->clear();
  m_taskMap.clear();
  m_editorialMap.clear();
  m_answer->clear();
  m_btnAllSubmissions->setEnabled(false);
  m_presenter->loadTasks(contestId);
}

void ActiveContestTab::submit() {
  if (!m_tasks->currentItem() || m_answer->toPlainText().isEmpty()) return;
  int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();
  m_presenter->submitAnswer(tId, m_answer->toPlainText());
}

void ActiveContestTab::showAllSubmissions() {
  if (!m_tasks->currentItem())
    return;
  int tId = m_tasks->currentItem()->data(Qt::UserRole).toInt();

  QDialog dlg(this);
  dlg.setWindowTitle("Все посылки по задаче (Взломы)");
  dlg.resize(800, 600);
  QVBoxLayout *l = new QVBoxLayout(&dlg);

  QTableWidget *table = new QTableWidget(0, 4, &dlg);
  table->setHorizontalHeaderLabels(
      {"ID Посылки", "Пользователь", "Оценка", "Ответ"});
  table->horizontalHeader()->setStretchLastSection(true);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  l->addWidget(table);

  auto conn = connect(m_presenter, &ActiveContestPresenter::allSubmissionsLoaded, [&dlg, table](const QJsonArray& arr){
      table->setRowCount(arr.size());
      for (int i = 0; i < arr.size(); ++i) {
        QJsonObject o = arr[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(o["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(o["username"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(o["score"].toInt())));
        table->setItem(i, 3, new QTableWidgetItem(o["answer_text"].toString()));
      }
  });

  m_presenter->loadAllSubmissions(tId);

  QPushButton *hackBtn = new QPushButton("Взломать выбранное", &dlg);
  l->addWidget(hackBtn);

  connect(hackBtn, &QPushButton::clicked, [this, &dlg, table]() {
    int row = table->currentRow();
    if (row < 0)
      return;
    int subId = table->item(row, 0)->text().toInt();

    QDialog hackDlg(&dlg);
    hackDlg.setWindowTitle("Взлом решения");
    QVBoxLayout *hl = new QVBoxLayout(&hackDlg);
    QTextEdit *hackText = new QTextEdit(&hackDlg);
    hackText->setPlaceholderText("Опишите ошибку или приведите контрпример...");
    QPushButton *sendBtn = new QPushButton("Отправить взлом", &hackDlg);
    hl->addWidget(hackText);
    hl->addWidget(sendBtn);

    auto hConn = std::make_shared<QMetaObject::Connection>();
    *hConn = connect(m_presenter, &ActiveContestPresenter::hackSuccessful, [&hackDlg, hConn](){
        QMessageBox::information(&hackDlg, "Ок", "Взлом отправлен, ожидайте вердикта ИИ!");
        QObject::disconnect(*hConn);
        hackDlg.accept();
    });

    connect(sendBtn, &QPushButton::clicked, [this, subId, hackText]() {
        m_presenter->submitHack(subId, hackText->toPlainText());
    });
    hackDlg.exec();
  });

  dlg.exec();
  disconnect(conn);
}

void ActiveContestTab::loadSubmissions(int taskId) {
  m_btnAllSubmissions->setEnabled(m_role == "admin" || m_role == "superadmin");
  m_presenter->loadMySubmissions(taskId);
}
