#include "admin_tab.h"
#include "math_highlighter.h"
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QVBoxLayout>

AdminTab::AdminTab(const QString &token, QWidget *parent)
    : QWidget(parent), m_token(token) {
  QHBoxLayout *ML = new QHBoxLayout(this);

  QGroupBox *g1 = new QGroupBox("Новый Контест");
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

  QPushButton *b1 = new QPushButton("Создать контест");
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

  QGroupBox *g2 = new QGroupBox("Новая Задача");
  QVBoxLayout *l2 = new QVBoxLayout(g2);
  m_tContestId = new QComboBox();
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

  QPushButton *b2 = new QPushButton("Создать задачу");

  l2->addWidget(m_tContestId);
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

  QGroupBox *g3 = new QGroupBox("Предпросмотр Typst");
  QVBoxLayout *l3 = new QVBoxLayout(g3);
  l3->addWidget(m_pdfView);

  ML->addWidget(g1);
  ML->addWidget(g2);
  ML->addWidget(g3);
  connect(b1, &QPushButton::clicked, this, &AdminTab::createContest);
  connect(b2, &QPushButton::clicked, this, &AdminTab::createTask);

  connect(btnPreviewEditorial, &QPushButton::clicked, [this]() {
    QString typstCode = m_tEditorial->toPlainText();
    if (typstCode.isEmpty())
      return;
    QNetworkAccessManager *m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://localhost:8080/api/compile_typst"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject j;
    j["code"] = typstCode;
    QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
      if (r->error() == QNetworkReply::NoError) {
        QByteArray pdfData = r->readAll();
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
      } else {
        QMessageBox::warning(this, "Ошибка",
                             "Не удалось скомпилировать PDF. Ошибка: " +
                                 r->errorString());
      }
      r->deleteLater();
      m->deleteLater();
    });
  });

  connect(m_tType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &AdminTab::onTaskTypeChanged);
  onTaskTypeChanged(0);
  loadMyContests();
}

void AdminTab::loadMyContests() {
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl("http://127.0.0.1:8080/api/admin/my_contests"));
  req.setRawHeader("Authorization", m_token.toUtf8());
  QNetworkReply *r = m->get(req);
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      m_tContestId->clear();
      QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
      for (auto v : arr) {
        QJsonObject o = v.toObject();
        m_tContestId->addItem(QString("%1 (ID: %2)")
                                  .arg(o["title"].toString())
                                  .arg(o["id"].toInt()),
                              o["id"].toInt());
      }
    }
    r->deleteLater();
    m->deleteLater();
  });
}

void AdminTab::compileRealtime(const QString &typstCode) {
  if (typstCode.isEmpty()) {
    m_pdfDoc->close();
    return;
  }

  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl("http://localhost:8080/api/compile_typst"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["code"] = typstCode;
  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());

  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      QByteArray pdfData = r->readAll();

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
    r->deleteLater();
    m->deleteLater();
  });
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
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl("http://localhost:8080/api/admin/contest"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());
  QJsonObject j;
  j["title"] = m_cTitle->text();
  j["start"] = m_cStart->dateTime().toString(Qt::ISODate);
  j["duration_hours"] = m_cDuration->value();
  j["description"] = m_cDesc->toPlainText();
  j["is_published"] = m_cIsPublished->isChecked();

  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      qDebug() << "Client: Contest created successfully";
      QMessageBox::information(this, "Ок", "Контест создан!");
      loadMyContests();
    } else {
      qDebug() << "Client Error: Failed to create contest:" << r->errorString()
               << "-" << r->readAll();
      QMessageBox::warning(this, "Ошибка", "Ошибка: " + r->errorString());
    }
    r->deleteLater();
    m->deleteLater();
  });
}
void AdminTab::createTask() {
  qDebug() << "Client: Creating task:" << m_tTitle->text()
           << "for contest ID:" << m_tContestId->currentData().toInt();
  QNetworkAccessManager *m = new QNetworkAccessManager(this);
  QNetworkRequest req(QUrl("http://localhost:8080/api/admin/task"));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setRawHeader("Authorization", m_token.toUtf8());

  QJsonObject j;
  j["contest_id"] = m_tContestId->currentData().toInt();
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

  j["tags"] = m_tTags->text();
  j["difficulty"] = m_tDifficulty->text().toInt();

  QNetworkReply *r = m->post(req, QJsonDocument(j).toJson());
  connect(r, &QNetworkReply::finished, [this, r, m]() {
    if (r->error() == QNetworkReply::NoError) {
      qDebug() << "Client: Task created successfully";
      QMessageBox::information(this, "Ок", "Задача создана!");
    } else {
      qDebug() << "Client Error: Failed to create task:" << r->errorString()
               << "-" << r->readAll();
      QMessageBox::warning(this, "Ошибка", "Ошибка: " + r->errorString());
    }
    r->deleteLater();
    m->deleteLater();
  });
}
