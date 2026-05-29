#pragma once
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QPdfDocument>
#include <QPdfView>
#include <QResizeEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class ActiveContestTab : public QWidget {
  Q_OBJECT
public:
  ActiveContestTab(const QString &token, const QString &role,
                   QWidget *parent = nullptr);
  void loadContest(int contestId, const QString &title);

protected:
  void resizeEvent(QResizeEvent *event) override;

private slots:
  void submit();
  void compileAndShowPdf(const QString &typstCode);
  void compileRealtime(const QString &typstCode);
  void loadSubmissions(int taskId);
  void showAllSubmissions();

private:
  void setCompactMode(bool compact);

  QString m_token;
  QString m_role;
  int m_contestId = -1;
  QStackedWidget *m_stack;
  QWidget *m_contestPage;
  QSplitter *m_mainSplitter;
  QSplitter *m_editorSplitter;
  QListWidget *m_tasks;
  QTextEdit *m_answer;
  QLabel *m_desc;
  QLabel *m_contestTitle;
  QTableWidget *m_submissionsTable;
  QMap<int, QString> m_taskMap;
  QMap<int, QString> m_editorialMap;
  class QPushButton *m_btnShowEditorial;
  class QPushButton *m_btnAllSubmissions;

  QTimer *m_compileTimer;
  QPdfView *m_pdfView;
  QPdfDocument *m_pdfDoc;
  QTemporaryFile *m_pdfTempFile;
  bool m_compact = false;
};
