#pragma once
#include <QLabel>
#include <QListWidget>
#include <QPdfDocument>
#include <QPdfView>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>
#include "mvc/ActiveContestPresenter.h"

class ActiveContestTab : public QWidget {
  Q_OBJECT
public:
  ActiveContestTab(const QString &token, const QString &role,
                   QWidget *parent = nullptr);
  void loadContest(int contestId, const QString &title);
private slots:
  void submit();
  void compileAndShowPdf(const QString &typstCode);
  void compileRealtime(const QString &typstCode);
  void loadSubmissions(int taskId);
  void showAllSubmissions();

private:
  QString m_token;
  QString m_role;
  int m_contestId = -1;
  QListWidget *m_tasks;
  QTextEdit *m_answer;
  QLabel *m_desc;
  QTableWidget *m_submissionsTable;
  QTableWidget *m_hacksTable;
  QMap<int, QString> m_taskMap;
  QMap<int, QString> m_editorialMap;
  class QPushButton *m_btnShowEditorial;
  class QPushButton *m_btnAllSubmissions;

  QTimer *m_compileTimer;
  QPdfView *m_pdfView;
  QPdfDocument *m_pdfDoc;
  QTemporaryFile *m_pdfTempFile;
  
  ActiveContestPresenter* m_presenter;
  bool m_isLoadingDraft = false;
};
