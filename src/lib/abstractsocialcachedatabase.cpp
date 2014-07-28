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

#include "abstractsocialcachedatabase.h"
#include "abstractsocialcachedatabase_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

#include <QtDebug>

// AbstractSocialCacheDatabase
// This class is the base class for all classes
// that deals with database access.
//
// It provides a set of useful methods that should
// be used to initialize the database, and write into
// it. dbBeginTransaction, dbWrite and dbCommitTransaction
// are particularly handy, and can make write operations
// into db very fast.

QThreadStorage<QHash<QString, AbstractSocialCacheDatabasePrivate::ThreadData> > AbstractSocialCacheDatabasePrivate::globalThreadData;

namespace {
class ProcessMutexCleanup
{
public:
    ProcessMutexCleanup(AbstractSocialCacheDatabasePrivate::ThreadData *threadData)
        : threadData(threadData)
    {
    }

    ~ProcessMutexCleanup()
    {
        if (threadData) {
            threadData->mutex->unlock();
            delete threadData->mutex;
            threadData->mutex = 0;
        }
    }

    void finalize()
    {
        threadData->mutex->unlock();
        threadData = 0;
    }

private:
    AbstractSocialCacheDatabasePrivate::ThreadData *threadData;
};
}

AbstractSocialCacheDatabasePrivate::AbstractSocialCacheDatabasePrivate(
        AbstractSocialCacheDatabase *q,
        const QString &serviceName,
        const QString &dataType,
        const QString &databaseFile,
        int version)
    : q_ptr(q)
    , serviceName(serviceName)
    , dataType(dataType)
    , filePath(QString(QLatin1String("%1/%2/%3")).arg(PRIVILEGED_DATA_DIR, dataType, databaseFile))
    , version(version)
    , readStatus(AbstractSocialCacheDatabase::Null)
    , writeStatus(AbstractSocialCacheDatabase::Null)
    , asyncReadStatus(Null)
    , asyncWriteStatus(Null)
    , running(false)
{
    setAutoDelete(false);
}

AbstractSocialCacheDatabasePrivate::~AbstractSocialCacheDatabasePrivate()
{
}

bool AbstractSocialCacheDatabasePrivate::initializeThreadData(ThreadData *threadData) const
{
    Q_Q(const AbstractSocialCacheDatabase);

    const QUuid uuid = QUuid::createUuid();

    threadData->threadId = uuid.toByteArray().toBase64();
    const QString connectionName = QString(QLatin1String("socialcache/%1/%2/%3")).arg(
                serviceName, dataType, uuid.toString());

    threadData->mutex = new ProcessMutex(connectionName);
    if (!threadData->mutex->lock()) {
        qWarning() << Q_FUNC_INFO << "Error: unable to acquire mutex lock during database initialisation";
        delete threadData->mutex;
        threadData->mutex = 0;
        return false;
    }

    ProcessMutexCleanup processMutexCleanup(threadData);

    bool createTables = false;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        createTables = true;

        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QFile dbfile(filePath);
        if (!dbfile.open(QIODevice::ReadWrite)) {
            qWarning() << Q_FUNC_INFO << "Unable to create database" << filePath << "Service"
                       << serviceName << "with data type" << dataType << "will be inactive";
            return false;
        }
        dbfile.close();
    }

    // open the database in which we store our synced image information
    threadData->database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    threadData->database.setDatabaseName(filePath);

    if (!threadData->database.open()) {
        qWarning() << Q_FUNC_INFO << "Unable to open database" << filePath << "Service"
                   << serviceName << "with data type" << dataType << "will be inactive";
        return false;
    }

    QSqlQuery query(threadData->database);

    query.exec(QStringLiteral("PRAGMA temp_store = MEMORY;"));
    query.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));

    if (!query.exec(QLatin1String("PRAGMA user_version")) || !query.next()) {
        qWarning() << Q_FUNC_INFO << "Failed to query pragma_user version. Service"
                   << serviceName << "with data type" << dataType << "will be inactive. Error"
                   << query.lastError().text();
        threadData->database.close();
        return false;
    }

    const int databaseVersion = query.value(0).toInt();
    query.finish();

    if (databaseVersion < version) {
        createTables = true;
        qWarning() << Q_FUNC_INFO << "Version required is" << version
                   << "while database is using" << databaseVersion;

        // DB needs to be recreated
        if (!q->dropTables(threadData->database)) {
            qWarning() << Q_FUNC_INFO << "Failed to update database" << filePath
                       << "It is probably broken and need to be removed manually";
            threadData->database.close();
            return false;
        }
    }

    if (createTables) {
        if (!q->createTables(threadData->database)) {
            qWarning() << Q_FUNC_INFO << "Failed to update database" << filePath
                       << "It is probably broken and need to be removed manually";
            threadData->database.close();
            return false;
        } else if (!query.exec(QString(QLatin1String("PRAGMA user_version=%1")).arg(version))) {
            qWarning()
                    << Q_FUNC_INFO
                    << "Failed to set database version"
                    << filePath
                    << query.lastError();
        }
    }

    processMutexCleanup.finalize();

    return true;
}

void AbstractSocialCacheDatabasePrivate::run()
{
    Q_Q(AbstractSocialCacheDatabase);

    ThreadData &threadData = globalThreadData.localData()[filePath];

    if (!threadData.mutex && !initializeThreadData(&threadData)) {
        return;
    }

    QMutexLocker locker(&mutex);
    for (;;) {
        if (asyncWriteStatus == Queued) {
            if (writeStatus == AbstractSocialCacheDatabase::Null) {
                asyncWriteStatus = Null;
                continue;
            }

            asyncWriteStatus = Executing;

            locker.unlock();

            if (!threadData.mutex->lock()) {
                // Warning
                qWarning() << Q_FUNC_INFO << "Failed to acquire a lock on the database";
                locker.relock();
                break;
            }

            if (!threadData.database.transaction()) {
                qWarning() << Q_FUNC_INFO << "Failed to start a database transaction";

                threadData.mutex->unlock();
                locker.relock();
                break;
            }

            bool success = q->write();

            if (!success) {
                threadData.database.rollback();
            } else if (!threadData.database.commit()) {
                qWarning() << Q_FUNC_INFO << "Failed to commit a database transaction";
                qWarning() << threadData.database.lastError();
                success = false;
            }

            threadData.mutex->unlock();
            locker.relock();

            if (asyncWriteStatus == Executing) {
                asyncWriteStatus = success ? Finished : Error;
            }
        } else if (asyncReadStatus == Queued) {
            if (readStatus == AbstractSocialCacheDatabase::Null) {
                asyncReadStatus = Null;
                continue;
            }
            asyncReadStatus = Executing;

            locker.unlock();

            bool success = q->read();

            locker.relock();

            if (asyncReadStatus == Executing) {
                asyncReadStatus = success ? Finished : Error;
            }
        } else {
            running = false;
            QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
            condition.wakeOne();
            return;
        }
    }
}

AbstractSocialCacheDatabase::AbstractSocialCacheDatabase(
        const QString &serviceName,
        const QString &dataType,
        const QString &databaseFile,
        int version)
    : d_ptr(new AbstractSocialCacheDatabasePrivate(
                this, serviceName, dataType, databaseFile, version))
{
}

AbstractSocialCacheDatabase::AbstractSocialCacheDatabase(AbstractSocialCacheDatabasePrivate &dd)
    : d_ptr(&dd)
{
}

AbstractSocialCacheDatabase::~AbstractSocialCacheDatabase()
{
}

bool AbstractSocialCacheDatabase::isValid() const
{
    Q_D(const AbstractSocialCacheDatabase);

    QSqlQuery query = prepare(QStringLiteral("PRAGMA user_version"));
    if (query.exec() && query.next()) {
        int userVersion = query.value(0).toInt();

        query.finish();

        return userVersion == d->version;
    }
    return false;
}

AbstractSocialCacheDatabase::Status AbstractSocialCacheDatabase::readStatus() const
{
    return d_func()->readStatus;
}

AbstractSocialCacheDatabase::Status AbstractSocialCacheDatabase::writeStatus() const
{
    return d_func()->writeStatus;
}

bool AbstractSocialCacheDatabase::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        Q_D(AbstractSocialCacheDatabase);

        bool readDone = false;
        bool writeDone = false;

        QMutexLocker locker(&d->mutex);

        if (d->asyncReadStatus >= AbstractSocialCacheDatabasePrivate::Finished) {
            if (d->readStatus != Null) {
                d->readStatus = d->asyncReadStatus == AbstractSocialCacheDatabasePrivate::Finished
                        ? Finished
                        : Error;
                d->asyncReadStatus = AbstractSocialCacheDatabasePrivate::Null;
                readDone = true;
            } else {
                d->asyncReadStatus = AbstractSocialCacheDatabasePrivate::Null;
            }
        }
        if (d->asyncWriteStatus >= AbstractSocialCacheDatabasePrivate::Finished) {
            if (d->writeStatus != Null) {
                d->writeStatus = d->asyncWriteStatus == AbstractSocialCacheDatabasePrivate::Finished
                        ? Finished
                        : Error;
                d->asyncWriteStatus = AbstractSocialCacheDatabasePrivate::Null;
                writeDone = true;
            } else {
                d->asyncWriteStatus = AbstractSocialCacheDatabasePrivate::Null;
            }
        }

        locker.unlock();

        if (readDone) {
            readFinished();
        }
        if (writeDone) {
            writeFinished();
        }

        return true;
    } else {
        return QObject::event(event);
    }
}

void AbstractSocialCacheDatabase::executeRead()
{
    Q_D(AbstractSocialCacheDatabase);
    QMutexLocker locker(&d->mutex);

    d->readStatus = Executing;
    d->asyncReadStatus = AbstractSocialCacheDatabasePrivate::Queued;

    if (!d->running) {
        d->running = true;
        QThreadPool::globalInstance()->start(d);
    }
}

void AbstractSocialCacheDatabase::cancelRead()
{
    Q_D(AbstractSocialCacheDatabase);
    QMutexLocker locker(&d->mutex);

    d->readStatus = Null;
}

void AbstractSocialCacheDatabase::executeWrite()
{
    Q_D(AbstractSocialCacheDatabase);
    QMutexLocker locker(&d->mutex);

    d->writeStatus = Executing;
    d->asyncWriteStatus = AbstractSocialCacheDatabasePrivate::Queued;

    if (!d->running) {
        d->running = true;
        QThreadPool::globalInstance()->start(d);
    }
}

void AbstractSocialCacheDatabase::cancelWrite()
{
    Q_D(AbstractSocialCacheDatabase);
    QMutexLocker locker(&d->mutex);

    d->writeStatus = Null;
}

bool AbstractSocialCacheDatabase::read()
{
    return false;
}

bool AbstractSocialCacheDatabase::write()
{
    return false;
}

void AbstractSocialCacheDatabase::readFinished()
{
}

void AbstractSocialCacheDatabase::writeFinished()
{
}

void AbstractSocialCacheDatabase::wait()
{
    Q_D(AbstractSocialCacheDatabase);

    bool readDone = false;
    bool writeDone = false;

    QMutexLocker locker(&d->mutex);

    while (d->running) {
        d->condition.wait(&d->mutex);
    }

    if (d->asyncReadStatus >= AbstractSocialCacheDatabasePrivate::Finished) {
        d->readStatus = d->asyncReadStatus == AbstractSocialCacheDatabasePrivate::Finished
                ? Finished
                : Error;
        d->asyncReadStatus = AbstractSocialCacheDatabasePrivate::Null;
        readDone = true;
    }
    if (d->asyncWriteStatus >= AbstractSocialCacheDatabasePrivate::Finished) {
        d->writeStatus = d->asyncWriteStatus == AbstractSocialCacheDatabasePrivate::Finished
                ? Finished
                : Error;
        d->asyncWriteStatus = AbstractSocialCacheDatabasePrivate::Null;
        writeDone = true;
    }

    locker.unlock();

    if (readDone) {
        readFinished();
    }
    if (writeDone) {
        writeFinished();
    }
}

QSqlQuery AbstractSocialCacheDatabase::prepare(const QString &query) const
{
    Q_D(const AbstractSocialCacheDatabase);

    AbstractSocialCacheDatabasePrivate::ThreadData &threadData = AbstractSocialCacheDatabasePrivate::globalThreadData.localData()[d->filePath];

    if (!threadData.mutex && ! d_func()->initializeThreadData(&threadData)) {
        return QSqlQuery();
    }

    QHash<QString, QSqlQuery>::const_iterator it = threadData.preparedQueries.constFind(query);
    if (it != threadData.preparedQueries.constEnd()) {
        return *it;
    }

    QSqlQuery preparedQuery(threadData.database);
    if (!preparedQuery.prepare(query)) {
        qWarning() << Q_FUNC_INFO << "Failed to prepare query";
        qWarning() << query;
        qWarning() << preparedQuery.lastError();
        return QSqlQuery();
    } else {
        threadData.preparedQueries.insert(query, preparedQuery);
        return preparedQuery;
    }
}

