#ifndef ARCHIVE_TAB_H
#define ARCHIVE_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

class ArchiveTab : public QWidget {
    Q_OBJECT
public:
    ArchiveTab(const QString& token, QWidget* parent = nullptr);
    void loadTasks();
private slots:
    void applyFilter();
    void openTask(int taskId);
private:
    QString m_token;
    QTableWidget* m_table;
    QLineEdit* m_filterTags;
    QLineEdit* m_filterMinDiff;
    QLineEdit* m_filterMaxDiff;
    QPushButton* m_btnFilter;
};

#endif // ARCHIVE_TAB_H
