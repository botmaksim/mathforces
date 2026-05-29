#include "table_utils.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QVariant>

namespace TableUtils {

void prepareTable(QTableWidget *table) {
  table->setSortingEnabled(true);
  table->setAlternatingRowColors(true);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  table->horizontalHeader()->setSectionsClickable(true);
  table->horizontalHeader()->setSortIndicatorShown(true);
  table->horizontalHeader()->setStretchLastSection(true);
  table->verticalHeader()->setVisible(false);
}

QLineEdit *attachSearch(QTableWidget *table, QWidget *parent,
                        const QString &placeholder) {
  QLineEdit *search = new QLineEdit(parent);
  search->setClearButtonEnabled(true);
  search->setPlaceholderText(placeholder.isEmpty() ? QStringLiteral("Поиск по таблице...")
                                                   : placeholder);
  QObject::connect(search, &QLineEdit::textChanged, table,
                   [table](const QString &query) {
                     const QString needle = query.trimmed();
                     for (int row = 0; row < table->rowCount(); ++row) {
                       bool match = needle.isEmpty();
                       for (int col = 0; col < table->columnCount() && !match;
                            ++col) {
                         QTableWidgetItem *item = table->item(row, col);
                         if (item && item->text().contains(needle, Qt::CaseInsensitive))
                           match = true;
                       }
                       table->setRowHidden(row, !match);
                     }
                   });
  return search;
}

QTableWidgetItem *numericItem(int value) {
  auto *item = new QTableWidgetItem;
  item->setData(Qt::DisplayRole, value);
  return item;
}

QTableWidgetItem *textItem(const QString &text) {
  return new QTableWidgetItem(text);
}

} // namespace TableUtils
