#pragma once
#include <QVariantList>
#include <QString>

class LocalDb {
public:
    static void init();
    static void cacheContests(const QVariantList& contests);
    static QVariantList getCachedContests();
    static void cacheRatings(const QVariantList& ratings);
    static QVariantList getCachedRatings();
    
    static void setSetting(const QString& key, const QString& value);
    static QString getSetting(const QString& key, const QString& defaultValue = "");

    static void saveTaskDraft(int taskId, const QString& answer);
    static QString getTaskDraft(int taskId);
};
