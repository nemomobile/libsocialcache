/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Lucien Xu <lucien.xu@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ABSTRACTSOCIALCACHEDATABASE_P_H
#define ABSTRACTSOCIALCACHEDATABASE_P_H

#include <QtCore/QtGlobal>
#include <QtCore/QWaitCondition>
#include <QtCore/QRunnable>
#include <QtCore/QThreadStorage>
#include <QtCore/QWaitCondition>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include "semaphore_p.h"
#include "abstractsocialcachedatabase.h"

class AbstractSocialCacheDatabase;
class AbstractSocialCacheDatabasePrivate : public QRunnable
{
protected:
    AbstractSocialCacheDatabase * const q_ptr;

public:
    enum Status
    {
        Null,
        Queued,
        Executing,
        Finished,
        Error
    };

    struct ThreadData
    {
        ThreadData() : mutex(0) {}
        ~ThreadData() { database.close(); delete mutex; }

        QSqlDatabase database;
        QHash<QString, QSqlQuery> preparedQueries;
        QString threadId;
        ProcessMutex *mutex; // Process (and thread) mutex to prevent concurrent write
    };

    explicit AbstractSocialCacheDatabasePrivate(
            AbstractSocialCacheDatabase *q,
            const QString &serviceName,
            const QString &dataType,
            const QString &databaseFile,
            int version);
    virtual ~AbstractSocialCacheDatabasePrivate();

    bool initializeThreadData(ThreadData *threadData) const;

    static QThreadStorage<QHash<QString, ThreadData> > globalThreadData;

    QMutex mutex;
    QWaitCondition condition;

    const QString serviceName;
    const QString dataType;
    const QString filePath;
    const int version;

    AbstractSocialCacheDatabase::Status readStatus;
    AbstractSocialCacheDatabase::Status writeStatus;

    Status asyncReadStatus;
    Status asyncWriteStatus;

    bool running;

    void run();

private:

    Q_DECLARE_PUBLIC(AbstractSocialCacheDatabase)
};

#define executeSocialCacheQuery(query) \
    if (!query.exec()) { \
        qWarning() << Q_FUNC_INFO << "Failed to execute query"; \
        qWarning() << query.lastQuery(); \
        qWarning() << query.lastError(); \
        success = false; \
    } \
    query.finish()

#define executeBatchSocialCacheQuery(query) \
    if (!query.execBatch()) { \
        qWarning() << Q_FUNC_INFO << "Failed to execute query"; \
        qWarning() << query.lastQuery(); \
        qWarning() << query.lastError(); \
        success = false; \
    } \
    query.finish()

#endif // ABSTRACTSOCIALCACHEDATABASE_P_H
