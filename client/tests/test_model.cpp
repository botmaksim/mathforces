#include <QTest>
#include "../mvc/ContestModel.h"

// Модульные тесты для слоя Model (Требование 5)
class TestModel : public QObject {
    Q_OBJECT
private slots:
    void testInitialRowCount() {
        ContestModel model;
        QCOMPARE(model.rowCount(), 0);
    }

    void testSetContests() {
        ContestModel model;
        QVariantList list;
        QVariantMap map;
        map["id"] = 42;
        map["title"] = "Test Contest Integration";
        list.append(map);
        model.setContests(list);
        
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.getId(0), 42);
        QCOMPARE(model.getTitle(0), QString("Test Contest Integration"));
    }
};

QTEST_MAIN(TestModel)
#include "test_model.moc"
