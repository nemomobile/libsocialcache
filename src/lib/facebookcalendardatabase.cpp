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

#include "facebookcalendardatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtCore/QStringList>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtDebug>

static const char *DB_NAME = "facebook.db";
static const int VERSION = 3;

struct FacebookEventPrivate
{
    explicit FacebookEventPrivate(const QString &fbEventId, int accountId,
                                  const QString &incidenceId);
    QString fbEventId;
    int accountId;
    QString incidenceId;
};

FacebookEventPrivate::FacebookEventPrivate(const QString &fbEventId, int accountId,
                                                 const QString &incidenceId)
    : fbEventId(fbEventId), accountId(accountId), incidenceId(incidenceId)
{
}

FacebookEvent::FacebookEvent(const QString &fbEventId, int accountId,
                                   const QString &incidenceId)
    : d_ptr(new FacebookEventPrivate(fbEventId, accountId, incidenceId))
{
}

FacebookEvent::~FacebookEvent()
{
}

FacebookEvent::Ptr FacebookEvent::create(const QString &fbEventId, int accountId,
                                               const QString &incidenceId)
{
    return FacebookEvent::Ptr(new FacebookEvent(fbEventId, accountId, incidenceId));
}

QString FacebookEvent::fbEventId() const
{
    Q_D(const FacebookEvent);
    return d->fbEventId;
}

int FacebookEvent::accountId() const
{
    Q_D(const FacebookEvent);
    return d->accountId;
}

QString FacebookEvent::incidenceId() const
{
    Q_D(const FacebookEvent);
    return d->incidenceId;
}

class FacebookCalendarDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit FacebookCalendarDatabasePrivate(FacebookCalendarDatabase *q);
    QList<FacebookEvent::ConstPtr> queuedEvents;
};

FacebookCalendarDatabasePrivate::FacebookCalendarDatabasePrivate(FacebookCalendarDatabase *q)
    : AbstractSocialCacheDatabasePrivate(q)
{
}

FacebookCalendarDatabase::FacebookCalendarDatabase()
    : AbstractSocialCacheDatabase(*(new FacebookCalendarDatabasePrivate(this)))
{
}

FacebookCalendarDatabase::~FacebookCalendarDatabase()
{
}

bool FacebookCalendarDatabase::removeEvents(int accountId)
{
    Q_D(FacebookCalendarDatabase);
    if (!dbBeginTransaction()) {
        return false;
    }

    QSqlQuery query (d->db);
    query.prepare(QLatin1String("DELETE FROM events WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete events" << query.lastError().text();
        dbRollbackTransaction();
        return false;
    }

    d->queuedEvents.clear();

    return dbCommitTransaction();
}

QList<FacebookEvent::ConstPtr> FacebookCalendarDatabase::events(int accountId)
{
    Q_D(FacebookCalendarDatabase);
    QList<FacebookEvent::ConstPtr> data;

    QSqlQuery query (d->db);
    query.prepare(QLatin1String("SELECT fbEventId, accountId, incidenceId FROM events "\
                                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query events" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookEvent::create(query.value(0).toString(), query.value(1).toInt(),
                                          query.value(2).toString()));
    }

    return data;
}

// Add a synced incidence
//
// When synced is called, all old incidences will be
// removed and only newer incidences will be written.
void FacebookCalendarDatabase::addSyncedEvent(const QString &fbEventId, int accountId,
                                              const QString &incidenceId)
{
    Q_D(FacebookCalendarDatabase);
    d->queuedEvents.append(FacebookEvent::create(fbEventId, accountId, incidenceId));
}

bool FacebookCalendarDatabase::sync(int accountId)
{
    Q_D(FacebookCalendarDatabase);

    // We sync for one given account. We should have all the interesting
    // events queued. So we will wipe all events from the account id
    // and only add the events that are queued, and match the account id.
    if (!dbBeginTransaction()) {
        return false;
    }

    QSqlQuery query (d->db);
    query.prepare("DELETE FROM events WHERE accountId = :accountId");
    query.bindValue(":accountId", accountId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to purge events for account" << accountId
                   << query.lastError().text();
        dbRollbackTransaction();
        return false;
    }

    QStringList keys;
    keys << QLatin1String("fbEventId") << QLatin1String("accountId")
         << QLatin1String("incidenceId");
    QMap<QString, QVariantList> data;
    foreach (const FacebookEvent::ConstPtr &event,d->queuedEvents) {
        if (event->accountId() == accountId) {
            data[QLatin1String("fbEventId")].append(event->fbEventId());
            data[QLatin1String("accountId")].append(accountId);
            data[QLatin1String("incidenceId")].append(event->incidenceId());
        }
    }

    if (!dbWrite(QLatin1String("events"), keys, data, InsertOrReplace)) {
        dbRollbackTransaction();
        return false;
    }

    return dbCommitTransaction();
}

void FacebookCalendarDatabase::initDatabase()
{
    dbInit(SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
           SocialSyncInterface::dataType(SocialSyncInterface::Calendars),
           QLatin1String(DB_NAME), VERSION);
}

bool FacebookCalendarDatabase::dbCreateTables()
{
    Q_D(FacebookCalendarDatabase);
    // create the facebook event db tables
    // events = fbEventId, fbUserId, incidenceId
    QSqlQuery query(d->db);
    query.prepare( "CREATE TABLE IF NOT EXISTS events ("
                   "fbEventId TEXT,"
                   "accountId INTEGER,"
                   "incidenceId TEXT,"
                   "CONSTRAINT id PRIMARY KEY (fbEventId, accountId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create events table:" << query.lastError().text();
        return false;
    }

    if (!dbCreatePragmaVersion(VERSION)) {
        return false;
    }
    return true;
}

bool FacebookCalendarDatabase::dbDropTables()
{
    Q_D(FacebookCalendarDatabase);
    QSqlQuery query(d->db);
    query.prepare("DROP TABLE IF EXISTS events");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events table:" << query.lastError().text();
        return false;
    }

    return true;
}
