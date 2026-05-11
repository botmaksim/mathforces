#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>

class AdminTab : public QWidget {
    Q_OBJECT
public:
    AdminTab(const QString& token, QWidget* parent = nullptr);
private slots:
    void createContest();
    void createTask();
private:
    QString m_token;
    QLineEdit* m_cTitle; QLineEdit* m_cStart; QLineEdit* m_cEnd; QTextEdit* m_cDesc;
    QLineEdit* m_tContestId; QLineEdit* m_tTitle; QLineEdit* m_tScore; QTextEdit* m_tDesc; QTextEdit* m_tAiComment;
};
