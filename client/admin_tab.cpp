#include "admin_tab.h"
#include "math_highlighter.h"
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialog>

AdminTab::AdminTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
    
  m_presenter = new AdminPresenter(m_token, this);
  
  QVBoxLayout *mainVert = new QVBoxLayout(this);
  mainVert->setContentsMargins(4, 4, 4, 4);
  mainVert->setSpacing(14);

  QHBoxLayout *topL = new QHBoxLayout();
  topL->setSpacing(10);
  topL->addWidget(new QLabel("Текущий контест:"));
  m_selectContest = new QComboBox();
  topL->addWidget(m_selectContest);
  m_btnCreateDraft = new QPushButton("Создать черновик");
  topL->addWidget(m_btnCreateDraft);
  mainVert->addLayout(topL);

  QHBoxLayout *ML = new QHBoxLayout();
  ML->setSpacing(14);
  mainVert->addLayout(ML);

  QGroupBox *g1 = new QGroupBox("Параметры контеста");
  QVBoxLayout *l1 = new QVBoxLayout(g1);
  m_cTitle = new QLineEdit();
  m_cTitle->setPlaceholderText("Название");

  m_cStart = new QDateTimeEdit(QDateTime::currentDateTime());
  m_cStart->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

  m_cDuration = new QDoubleSpinBox();
  m_cDuration->setRange(0.1, 720.0);
  m_cDuration->setValue(2.0);
  m_cDuration->setSuffix(" ч.");
  m_cDuration->setSingleStep(0.5);

  m_cDesc = new QTextEdit();
  m_cDesc->setPlaceholderText("Описание...");
  new MathHighlighter(m_cDesc->document());

  m_cIsPublished = new QCheckBox("Опубликовать контест (сразу доступен)");

  QPushButton *b1 = new QPushButton("Сохранить контест");
  l1->addWidget(new QLabel("Название:"));
  l1->addWidget(m_cTitle);
  l1->addWidget(new QLabel("Начало:"));
  l1->addWidget(m_cStart);
  l1->addWidget(new QLabel("Продолжительность:"));
  l1->addWidget(m_cDuration);
  l1->addWidget(new QLabel("Описание:"));
  l1->addWidget(m_cDesc);
  l1->addWidget(m_cIsPublished);
  l1->addWidget(b1);

  QGroupBox *g2 = new QGroupBox("Новая задача");
  QVBoxLayout *l2 = new QVBoxLayout(g2);
  m_tTitle = new QLineEdit();
  m_tTitle->setPlaceholderText("Название");
  m_tScore = new QLineEdit();
  m_tScore->setPlaceholderText("Макс Балл (100)");
  m_tMaxSubmissions = new QLineEdit();
  m_tMaxSubmissions->setPlaceholderText("Макс посылок (напр. 10)");

  m_tType = new QComboBox();
  m_tType->addItem("Только ответ", "answer_only");
  m_tType->addItem("Решение", "solution");

  m_tDesc = new QTextEdit();
  m_tDesc->setPlaceholderText("Условие задачи (LaTeX/Typst)...");
  new MathHighlighter(m_tDesc->document());

  m_tCorrectAnswer = new QLineEdit();
  m_tCorrectAnswer->setPlaceholderText("Правильный ответ (для answer_only)");

  m_tEditorial = new QTextEdit();
  m_tEditorial->setPlaceholderText("Решение (разбор) задачи...");
  m_tSendEditorialToAi = new QCheckBox("Отправлять решение ИИ для проверки?");

  m_tAiComment = new QTextEdit();
  m_tAiComment->setPlaceholderText(
      "Комментарий для нейросети (подсказки, критерии проверки)...");
  m_tAiComment->setMaximumHeight(60);

  m_tTags = new QLineEdit();
  m_tTags->setPlaceholderText("Теги (через запятую)...");
  m_tDifficulty = new QLineEdit();
  m_tDifficulty->setPlaceholderText("Сложность (например 1200)");

  QPushButton *btnPreviewEditorial =
      new QPushButton("Предпросмотр разбора (Typst)", this);

  QPushButton *b2 = new QPushButton("Добавить задачу");

  l2->addWidget(m_tTitle);
  l2->addWidget(m_tScore);
  l2->addWidget(m_tMaxSubmissions);
  l2->addWidget(new QLabel("Тип:"));
  l2->addWidget(m_tType);
  l2->addWidget(m_tTags);
  l2->addWidget(m_tDifficulty);
  l2->addWidget(new QLabel("Условие:"));
  l2->addWidget(m_tDesc);
  l2->addWidget(m_tCorrectAnswer);
  l2->addWidget(new QLabel("Авторское решение:"));
  l2->addWidget(m_tEditorial);
  l2->addWidget(btnPreviewEditorial);
  l2->addWidget(m_tSendEditorialToAi);
  l2->addWidget(m_tAiComment);
  l2->addWidget(b2);

  m_pdfDoc = new QPdfDocument(this);
  m_pdfView = new QPdfView(this);
  m_pdfView->setDocument(m_pdfDoc);
  m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
  m_pdfTempFile = nullptr;

  m_compileTimer = new QTimer(this);
  m_compileTimer->setSingleShot(true);

  connect(m_tDesc, &QTextEdit::textChanged, [this]() {
    m_compileTimer->setProperty("target", "desc");
    m_compileTimer->start(1000);
  });
  connect(m_tEditorial, &QTextEdit::textChanged, [this]() {
    m_compileTimer->setProperty("target", "editorial");
    m_compileTimer->start(1000);
  });
  connect(m_compileTimer, &QTimer::timeout, [this]() {
    if (m_compileTimer->property("target").toString() == "desc") {
      compileRealtime(m_tDesc->toPlainText());
    } else {
      compileRealtime(m_tEditorial->toPlainText());
    }
  });

  QGroupBox *g3 = new QGroupBox("Живой предпросмотр Typst");
  QVBoxLayout *l3 = new QVBoxLayout(g3);
  l3->addWidget(m_pdfView);

  ML->addWidget(g1);
  ML->addWidget(g2);
  ML->addWidget(g3);
  
  connect(m_presenter, &AdminPresenter::myContestsLoaded, this, [this](const QJsonArray& arr){
      m_selectContest->clear();
      m_currentContestsArray = arr;
      for (auto v : m_currentContestsArray) {
        QJsonObject o = v.toObject();
        m_selectContest->addItem(QString("%1 (ID: %2)").arg(o["title"].toString()).arg(o["id"].toInt()), o["id"].toInt());
      }
  });
  
  connect(m_presenter, &AdminPresenter::draftCreated, this, [this](){
      QMessageBox::information(this, "Ок", "Черновик контеста создан");
      loadMyContests();
  });
  
  connect(m_presenter, &AdminPresenter::contestUpdated, this, [this](){
      QMessageBox::information(this, "Ок", "Параметры сохранены");
      loadMyContests();
  });
  
  connect(m_presenter, &AdminPresenter::taskCreated, this, [this](){
      QMessageBox::information(this, "Ок", "Задача создана!");
  });
  
  connect(m_presenter, &AdminPresenter::errorOccurred, this, [this](const QString& err){
      QMessageBox::warning(this, "Ошибка", "Ошибка: " + err);
  });
  
  connect(m_presenter, &AdminPresenter::typstCompiled, this, [this](const QByteArray& pdfData){
        QTemporaryFile *tf = new QTemporaryFile(this);
        if (tf->open()) {
          tf->write(pdfData);
          tf->flush();
          QDialog *d = new QDialog(this);
          d->resize(800, 600);
          QVBoxLayout *l = new QVBoxLayout(d);
          QPdfDocument *doc = new QPdfDocument(d);
          doc->load(tf->fileName());
          QPdfView *view = new QPdfView(d);
          view->setDocument(doc);
          view->setPageMode(QPdfView::PageMode::MultiPage);
          l->addWidget(view);
          d->exec();
        }
  });
  
  connect(m_presenter, &AdminPresenter::realtimeTypstCompiled, this, [this](const QByteArray& pdfData){
      if (m_pdfTempFile) m_pdfTempFile->deleteLater();
      m_pdfTempFile = new QTemporaryFile(this);
      if (m_pdfTempFile->open()) {
        m_pdfTempFile->write(pdfData);
        m_pdfTempFile->flush();
        m_pdfDoc->load(m_pdfTempFile->fileName());
      }
  });

  connect(m_btnCreateDraft, &QPushButton::clicked, this, &AdminTab::createDraftContest);
  connect(b1, &QPushButton::clicked, this, &AdminTab::updateContest);
  connect(b2, &QPushButton::clicked, this, &AdminTab::createTask);
  connect(m_selectContest, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &AdminTab::onContestSelectionChanged);

  connect(btnPreviewEditorial, &QPushButton::clicked, [this]() {
    QString typstCode = m_tEditorial->toPlainText();
    if (typstCode.isEmpty()) return;
    m_presenter->compileTypst(typstCode);
  });

  connect(m_tType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &AdminTab::onTaskTypeChanged);
  onTaskTypeChanged(0);
  loadMyContests();
}

void AdminTab::loadMyContests() {
    m_presenter->loadMyContests();
}

void AdminTab::onContestSelectionChanged(int index) {
  if (index < 0 || index >= m_currentContestsArray.size())
    return;
  QJsonObject o = m_currentContestsArray[index].toObject();
  m_cTitle->setText(o["title"].toString());
  m_cDesc->setPlainText(o["description"].toString());
  m_cStart->setDateTime(
      QDateTime::fromString(o["start_time"].toString(), Qt::ISODate));
  m_cDuration->setValue(o["duration_hours"].toDouble());
  m_cIsPublished->setChecked(o["is_published"].toBool());
}

void AdminTab::createDraftContest() {
    m_presenter->createDraftContest();
}

void AdminTab::updateContest() {
  int cIdx = m_selectContest->currentIndex();
  if (cIdx < 0) return;
  int cId = m_selectContest->itemData(cIdx).toInt();
  m_presenter->updateContest(cId, m_cTitle->text(), m_cStart->dateTime().toString("yyyy-MM-dd HH:mm:ss"), m_cDuration->value(), m_cDesc->toPlainText(), m_cIsPublished->isChecked());
}

void AdminTab::compileRealtime(const QString &typstCode) {
  if (typstCode.isEmpty()) {
    m_pdfDoc->close();
    return;
  }
  m_presenter->compileRealtime(typstCode);
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

void AdminTab::createTask() {
  int cIdx = m_selectContest->currentIndex();
  if (cIdx < 0)
    return;
  int cId = m_selectContest->itemData(cIdx).toInt();
  
  m_presenter->createTask(cId, m_tTitle->text(), m_tScore->text().toInt(), m_tMaxSubmissions->text().toInt(), m_tDesc->toPlainText(), m_tType->currentData().toString(), m_tCorrectAnswer->text(), m_tEditorial->toPlainText(), m_tSendEditorialToAi->isChecked(), m_tAiComment->toPlainText(), m_tTags->text(), m_tDifficulty->text().toInt());
}
