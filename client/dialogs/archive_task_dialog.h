#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QPdfDocument>
#include <QPdfView>
#include <QSplitter>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTimer>

class QLabel;

class ArchiveTaskDialog : public QDialog {
  Q_OBJECT
public:
  ArchiveTaskDialog(const QString &token, int taskId, QWidget *parent = nullptr);

private slots:
  void loadTask();
  void submit();
  void loadSubmissions();
  void compileRealtime(const QString &typstCode);
  void compileAndShowPdf(const QString &typstCode);

private:
  void renderTask(const QJsonObject &task);

  QString m_token;
  int m_taskId;
  QLabel *m_title;
  QLabel *m_meta;
  QTextEdit *m_description;
  QTextEdit *m_answer;
  QTableWidget *m_submissionsTable;
  QSplitter *m_editorSplitter;
  QTimer *m_compileTimer;
  QPdfView *m_pdfView;
  QPdfDocument *m_pdfDoc;
  QTemporaryFile *m_pdfTempFile;
};
