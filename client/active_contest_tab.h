#pragma once
#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>

class ActiveContestTab : public QWidget {
    Q_OBJECT
public:
    ActiveContestTab(const QString& token, QWidget* parent = nullptr);
    void loadContest(int contestId, const QString& title);
private slots:
    void submit();
    void loadFile();
    void compileAndShowPdf(const QString& typstCode);
private:
    QString m_token;
    int m_contestId = -1;
    QListWidget* m_tasks;
    QTextEdit* m_answer;
    QLabel* m_desc;
    QMap<int, QString> m_taskMap;
};
