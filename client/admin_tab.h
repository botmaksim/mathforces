#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>

class AdminTab : public QWidget {
    Q_OBJECT
public:
    AdminTab(const QString& token, QWidget* parent = nullptr);
private slots:
    void createContest();
    void createTask();
    void onTaskTypeChanged(int index);
private:
    QString m_token;
    QLineEdit* m_cTitle; QLineEdit* m_cStart; QLineEdit* m_cDuration; QTextEdit* m_cDesc;
    QLineEdit* m_tContestId; QLineEdit* m_tTitle; QLineEdit* m_tScore; QComboBox* m_tType;
    QTextEdit* m_tDesc; QLineEdit* m_tCorrectAnswer; QTextEdit* m_tEditorial;
    QCheckBox* m_tSendEditorialToAi; QTextEdit* m_tAiComment; QLineEdit* m_tMaxSubmissions;
};
