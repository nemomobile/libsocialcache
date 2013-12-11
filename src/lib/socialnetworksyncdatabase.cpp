/*
 * Copyright (C) 2013 Lucien Xu <sfietkonstantin@free.fr>
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

#include "socialnetworksyncdatabase.h"
#include "abstractsocialcachedatabase_p.h"

#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtCore/QMap>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtDebug>

static const char *SERVICE_NAME = "Sync";
static const char *DATA_TYPE = "Sync";
static const char *DB_NAME = "sociald-sync.db";
static const int VERSION = 3;

struct SocialNetworkSyncData
{
    QString serviceName;
    QString dataType;
    int accountId;
    QDateTime timestamp;
};

class SocialNetworkSyncDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit SocialNetworkSyncDatabasePrivate(SocialNetworkSyncDatabase *q);
    virtual ~SocialNetworkSyncDatabasePrivate();
    QList<SocialNetworkSyncData *> queuedData;
};

SocialNetworkSyncDatabasePrivate::SocialNetworkSyncDatabasePrivate(SocialNetworkSyncDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            QLatin1String(SERVICE_NAME),
            QLatin1String(DATA_TYPE),
            QLatin1String(DB_NAME),
            VERSION)
{
}

SocialNetworkSyncDatabasePrivate::~SocialNetworkSyncDatabasePrivate()
{
    qDeleteAll(queuedData);
}


SocialNetworkSyncDatabase::SocialNetworkSyncDatabase()
    : AbstractSocialCacheDatabase(*(new SocialNetworkSyncDatabasePrivate(this)))
{
}

SocialNetworkSyncDatabase::~SocialNetworkSyncDatabase()
{
    wait();
}

QList<int> SocialNetworkSyncDatabase::syncedAccounts(const QString &serviceName,
                                                     const QString &dataType) const
{
    QSqlQuery query = prepare(QStringLiteral(
                  "SELECT DISTINCT accountId FROM syncTimestamps "\
                  "WHERE serviceName = :serviceName "\
                  "AND dataType = :dataType"));
    query.bindValue(":serviceName", serviceName);
    query.bindValue(":dataType", dataType);
    bool success = query.exec();
    if (!success) {
        qWarning() << "Failed to query synced accounts" << query.lastError().text();
        return QList<int>();
    }

    QList<int> accounts;
    while (query.next()) {
        accounts.append(query.value(0).toInt());
    }

    return accounts;
}

QDateTime SocialNetworkSyncDatabase::lastSyncTimestamp(const QString &serviceName,
                                                       const QString &dataType,
                                                       int accountId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                  "SELECT syncTimestamp FROM syncTimestamps "\
                  "WHERE serviceName = :serviceName "\
                  "AND accountId = :accountId "\
                  "AND dataType = :dataType ORDER BY syncTimestamp DESC LIMIT 1"));
    query.bindValue(":serviceName", serviceName);
    query.bindValue(":accountId", accountId);
    query.bindValue(":dataType", dataType);
    bool success = query.exec();
    if (!success) {
        qWarning() << "Failed to query last synced timestamp" << query.lastError().text();
        return QDateTime();
    }

    if (!query.next()) {
        return QDateTime();
    }

    QDateTime dateTime = QDateTime::fromTime_t(query.value(0).toUInt());
    return dateTime;
}

void SocialNetworkSyncDatabase::addSyncTimestamp(const QString &serviceName,
                                                 const QString &dataType, int accountId,
                                                 const QDateTime &timestamp)
{
    Q_D(SocialNetworkSyncDatabase);
    SocialNetworkSyncData *data = new SocialNetworkSyncData;
    data->serviceName = serviceName;
    data->dataType = dataType;
    data->accountId = accountId;
    data->timestamp = timestamp;

    QMutexLocker locker(&d->mutex);

    d->queuedData.append(data);
}

void SocialNetworkSyncDatabase::commit()
{
    executeWrite();
}

bool SocialNetworkSyncDatabase::write()
{
    Q_D(SocialNetworkSyncDatabase);

    QMutexLocker locker(&d->mutex);

    const QList<SocialNetworkSyncData *> insertData = d->queuedData;

    d->queuedData.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!insertData.isEmpty()) {
        QVariantList serviceNames;
        QVariantList dataTypes;
        QVariantList accountIds;
        QVariantList timestamps;

        Q_FOREACH (SocialNetworkSyncData *data, insertData) {
            serviceNames.append(data->serviceName);
            dataTypes.append(data->dataType);
            accountIds.append(data->accountId);
            timestamps.append(data->timestamp.toTime_t());

            delete data;
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO syncTimestamps ("
                    " accountId, serviceName, dataType, syncTimestamp) "
                    "VALUES ("
                    " :accountId, :serviceName, :dataType, :syncTimestamp)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":serviceName"), serviceNames);
        query.bindValue(QStringLiteral(":dataType"), dataTypes);
        query.bindValue(QStringLiteral(":syncTimestamp"), timestamps);
        executeBatchSocialCacheQuery(query);
    }
    return success;
}

bool SocialNetworkSyncDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query (database);

    // Create the sociald db tables
    // syncTimestamps = id, accountId, service, dataType, syncTimestamp
    query.prepare("CREATE TABLE IF NOT EXISTS syncTimestamps ("\
                  "accountId TEXT, "\
                  "serviceName TEXT, "\
                  "dataType TEXT, "\
                  "syncTimestamp INTEGER, "\
                  "CONSTRAINT id PRIMARY KEY (accountId, serviceName, dataType))");
    if (!query.exec()) {
        qWarning() << "Unable to create syncTimestamps table" << query.lastError().text();
        return false;
    }

    return true;
}

bool SocialNetworkSyncDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare("DROP TABLE IF EXISTS syncTimestamps");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete syncTimestamps table"
                   << query.lastError().text();
        return false;
    }

    return true;
}
