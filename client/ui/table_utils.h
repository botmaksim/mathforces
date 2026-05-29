#pragma once

#include <QLineEdit>
#include <QTableWidget>

namespace TableUtils {

void prepareTable(QTableWidget *table);
QLineEdit *attachSearch(QTableWidget *table, QWidget *parent,
                        const QString &placeholder = QString());
QTableWidgetItem *numericItem(int value);
QTableWidgetItem *textItem(const QString &text);

} // namespace TableUtils
