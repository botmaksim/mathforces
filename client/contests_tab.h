#pragma once
#include <QWidget>
#include <QListWidget>

class ContestsTab : public QWidget {
    Q_OBJECT
public:
    ContestsTab(const QString& token, QWidget* parent = nullptr);
signals:
    void contestSelected(int id, const QString& title);
private slots:
    void load();
private:
    QString m_token;
    QListWidget* m_list;
};
