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

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtCore/QMap>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static const char *SERVICE_NAME = "Sync";
static const char *DATA_TYPE = "Sync";
static const char *DB_NAME = "sociald-sync.db"; // TODO: temporary, not to clash with old db
static const int VERSION = 2;

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
    : AbstractSocialCacheDatabasePrivate(q)
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

void SocialNetworkSyncDatabase::initDatabase()
{
    dbInit(QLatin1String(SERVICE_NAME), QLatin1String(DATA_TYPE),
           QLatin1String(DB_NAME), VERSION);
}

QDateTime SocialNetworkSyncDatabase::lastSyncTimestamp(const QString &serviceName,
                                                       const QString &dataType,
                                                       int accountId) const
{
    Q_D(const SocialNetworkSyncDatabase);
    QSqlQuery query (d->db);
    query.prepare("SELECT syncTimestamp FROM syncTimestamps "\
                  "WHERE serviceName = :serviceName "\
                  "AND accountId = :accountId "\
                  "AND dataType = :dataType ORDER BY syncTimestamp DESC LIMIT 1");
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
    d->queuedData.append(data);
}

bool SocialNetworkSyncDatabase::write()
{
    Q_D(SocialNetworkSyncDatabase);
    QMap<QString, QVariantList> entries;
    QStringList keys;

    keys << QLatin1String("accountId") << QLatin1String("serviceName") << QLatin1String("dataType")
         << QLatin1String("syncTimestamp");

    entries.insert(QLatin1String("accountId"), QVariantList());
    entries.insert(QLatin1String("serviceName"), QVariantList());
    entries.insert(QLatin1String("dataType"), QVariantList());
    entries.insert(QLatin1String("syncTimestamp"), QVariantList());

    foreach (SocialNetworkSyncData *data, d->queuedData) {
        entries[QLatin1String("accountId")].append(data->accountId);
        entries[QLatin1String("serviceName")].append(data->serviceName);
        entries[QLatin1String("dataType")].append(data->dataType);
        entries[QLatin1String("syncTimestamp")].append(data->timestamp.toTime_t());
    }

    if (!dbBeginTransaction()) {
        return false;
    }

    bool ok = dbWrite(QLatin1String("syncTimestamps"), keys, entries, InsertOrReplace);

    if (!dbCommitTransaction()) {
        return false;
    }

    qDeleteAll(d->queuedData);
    d->queuedData.clear();

    return ok;
}

bool SocialNetworkSyncDatabase::dbCreateTables()
{
    Q_D(SocialNetworkSyncDatabase);
    QSqlQuery query (d->db);

    // Create the sociald db tables
    // syncTimestamps = id, accountId, service, dataType, syncTimestamp
    query.prepare("CREATE TABLE IF NOT EXISTS syncTimestamps ("\
                  "accountId VARCHAR(50), "\
                  "serviceName VARCHAR(20), "\
                  "dataType VARCHAR(16), "\
                  "syncTimestamp INTEGER, "\
                  "CONSTRAINT id PRIMARY KEY (accountId, serviceName, dataType))");
    if (!query.exec()) {
        qWarning() << "Unable to create syncTimestamps table" << query.lastError().text();
        d->db.close();
        return false;
    }

    if (!dbCreatePragmaVersion(VERSION)) {
        return false;
    }

    return true;
}

bool SocialNetworkSyncDatabase::dbDropTables()
{
    Q_D(SocialNetworkSyncDatabase);
    QSqlQuery query(d->db);
    query.prepare("DROP TABLE IF EXISTS syncTimestamps");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete syncTimestamps table"
                   << query.lastError().text();
        return false;
    }

    return true;
}


