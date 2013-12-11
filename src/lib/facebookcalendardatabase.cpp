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

    QMap<int, QList<FacebookEvent::ConstPtr> > syncedEvents;
    struct {
        QList<int> removeEvents;
        QMap<int, QList<FacebookEvent::ConstPtr> > insertEvents;
    } queue;
};

FacebookCalendarDatabasePrivate::FacebookCalendarDatabasePrivate(FacebookCalendarDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
            SocialSyncInterface::dataType(SocialSyncInterface::Calendars),
            QLatin1String(DB_NAME),
            VERSION)
{
}

FacebookCalendarDatabase::FacebookCalendarDatabase()
    : AbstractSocialCacheDatabase(*(new FacebookCalendarDatabasePrivate(this)))
{
}

FacebookCalendarDatabase::~FacebookCalendarDatabase()
{
    wait();
}

void FacebookCalendarDatabase::removeEvents(int accountId)
{
    Q_D(FacebookCalendarDatabase);

    d->syncedEvents.remove(accountId);

    QMutexLocker locker(&d->mutex);

    d->queue.removeEvents.append(accountId);
    d->queue.insertEvents.remove(accountId);
}

QList<FacebookEvent::ConstPtr> FacebookCalendarDatabase::events(int accountId)
{
    QList<FacebookEvent::ConstPtr> data;

    QSqlQuery query = prepare(QStringLiteral(
                "SELECT fbEventId, accountId, incidenceId FROM events "\
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

    d->syncedEvents[accountId].append(FacebookEvent::create(fbEventId, accountId, incidenceId));
}

void FacebookCalendarDatabase::sync(int accountId)
{
    Q_D(FacebookCalendarDatabase);

    {
        QMutexLocker locker(&d->mutex);

        d->queue.insertEvents.insert(accountId, d->syncedEvents.take(accountId));
    }

    executeWrite();
}

bool FacebookCalendarDatabase::write()
{
    Q_D(FacebookCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    const QList<int> removeEvents = d->queue.removeEvents + d->queue.insertEvents.keys();
    const QMap<int, QList<FacebookEvent::ConstPtr> > insertEvents = d->queue.insertEvents;

    d->queue.removeEvents.clear();
    d->queue.insertEvents.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!removeEvents.isEmpty()) {
        QVariantList accountIds;

        Q_FOREACH (int accountId, removeEvents) {
            accountIds.append(accountId);
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM events "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertEvents.isEmpty()) {
        QVariantList eventIds;
        QVariantList accountIds;
        QVariantList incidenceIds;

        Q_FOREACH (const QList<FacebookEvent::ConstPtr> &events, insertEvents) {
            Q_FOREACH (const FacebookEvent::ConstPtr &event, events) {
                eventIds.append(event->fbEventId());
                accountIds.append(event->accountId());
                incidenceIds.append(event->incidenceId());
            }
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO events ("
                    " fbEventId, accountId, incidenceId) "
                    "VALUES("
                    " :fbEventId, :accountId, :incidenceId)"));
        query.bindValue(QStringLiteral(":fbEventId"), eventIds);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":incidenceId"), incidenceIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool FacebookCalendarDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    // create the facebook event db tables
    // events = fbEventId, fbUserId, incidenceId
    query.prepare( "CREATE TABLE IF NOT EXISTS events ("
                   "fbEventId TEXT,"
                   "accountId INTEGER,"
                   "incidenceId TEXT,"
                   "CONSTRAINT id PRIMARY KEY (fbEventId, accountId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create events table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool FacebookCalendarDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS events"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events table:" << query.lastError().text();
        return false;
    }

    return true;
}
