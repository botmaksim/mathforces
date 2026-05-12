#pragma once
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QJsonArray>
#include <QLineEdit>
#include <QPdfDocument>
#include <QPdfView>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class AdminTab : public QWidget {
  Q_OBJECT
public:
  AdminTab(const QString &token, QWidget *parent = nullptr);
  void loadMyContests();
private slots:
  void createDraftContest();
  void updateContest();
  void createTask();
  void onTaskTypeChanged(int index);
  void compileRealtime(const QString &typstCode);
  void onContestSelectionChanged(int index);

private:
  QJsonArray m_currentContestsArray;
  QString m_token;
  QComboBox *m_selectContest;
  QPushButton *m_btnCreateDraft;
  QLineEdit *m_cTitle;
  QDateTimeEdit *m_cStart;
  QDoubleSpinBox *m_cDuration;
  QTextEdit *m_cDesc;
  QCheckBox *m_cIsPublished;
  QLineEdit *m_tTitle;
  QLineEdit *m_tScore;
  QComboBox *m_tType;
  QTextEdit *m_tDesc;
  QLineEdit *m_tCorrectAnswer;
  QTextEdit *m_tEditorial;
  QCheckBox *m_tSendEditorialToAi;
  QTextEdit *m_tAiComment;
  QLineEdit *m_tMaxSubmissions;
  QLineEdit *m_tTags;
  QLineEdit *m_tDifficulty;

  QTimer *m_compileTimer;
  QPdfView *m_pdfView;
  QPdfDocument *m_pdfDoc;
  QTemporaryFile *m_pdfTempFile;
};
