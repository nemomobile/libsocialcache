/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
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

#include "googlecalendardatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtCore/QVariantList>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtDebug>

static const char *DB_NAME = "google.db";
static const int VERSION = 3;

struct GoogleEventPrivate
{
    explicit GoogleEventPrivate(int accountId,
                                const QString &gcalEventId,
                                const QString &localCalendarId,
                                const QString &localEventId,
                                const QString &localEventRecurrenceId);
    int accountId;
    QString gcalEventId;
    QString localCalendarId;
    QString localEventId;
    QString localEventRecurrenceId;
};

GoogleEventPrivate::GoogleEventPrivate(int accountId,
                                       const QString &gcalEventId,
                                       const QString &localCalendarId,
                                       const QString &localEventId,
                                       const QString &localEventRecurrenceId)
    : accountId(accountId)
    , gcalEventId(gcalEventId)
    , localCalendarId(localCalendarId)
    , localEventId(localEventId)
    , localEventRecurrenceId(localEventRecurrenceId)
{
}

GoogleEvent::GoogleEvent(int accountId,
                         const QString &gcalEventId,
                         const QString &localCalendarId,
                         const QString &localEventId,
                         const QString &localEventRecurrenceId)
    : d_ptr(new GoogleEventPrivate(accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId))
{
}

GoogleEvent::~GoogleEvent()
{
}

GoogleEvent::Ptr GoogleEvent::create(int accountId,
                                     const QString &gcalEventId,
                                     const QString &localCalendarId,
                                     const QString &localEventId,
                                     const QString &localEventRecurrenceId)
{
    return GoogleEvent::Ptr(new GoogleEvent(accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId));
}

QString GoogleEvent::gcalEventId() const
{
    Q_D(const GoogleEvent);
    return d->gcalEventId;
}

int GoogleEvent::accountId() const
{
    Q_D(const GoogleEvent);
    return d->accountId;
}

QString GoogleEvent::localCalendarId() const
{
    Q_D(const GoogleEvent);
    return d->localCalendarId;
}

QString GoogleEvent::localEventId() const
{
    Q_D(const GoogleEvent);
    return d->localEventId;
}

QString GoogleEvent::localEventRecurrenceId() const
{
    Q_D(const GoogleEvent);
    return d->localEventRecurrenceId;
}

class GoogleCalendarDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit GoogleCalendarDatabasePrivate(GoogleCalendarDatabase *q);

    QMap<int, QList<GoogleEvent::ConstPtr> > insertEvents;
    QMap<int, QList<GoogleEvent::ConstPtr> > removeEvents;
    QMap<QString, QPair<QString, int> > insertLastUpdateTimes;
    QList<int> removeLastUpdateTimes;

    // Mutex protect the following queue!
    struct {
        QMap<int, QList<GoogleEvent::ConstPtr> > insertEvents;
        QMap<int, QList<GoogleEvent::ConstPtr> > removeEvents;
        QMap<QString, QPair<QString, int > > insertLastUpdateTimes;
        QList<int> removeLastUpdateTimes;
    } queue;
};

GoogleCalendarDatabasePrivate::GoogleCalendarDatabasePrivate(GoogleCalendarDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Google),
            SocialSyncInterface::dataType(SocialSyncInterface::Calendars),
            QLatin1String(DB_NAME),
            VERSION)
{
}

GoogleCalendarDatabase::GoogleCalendarDatabase()
    : AbstractSocialCacheDatabase(*(new GoogleCalendarDatabasePrivate(this)))
{
}

GoogleCalendarDatabase::~GoogleCalendarDatabase()
{
    wait();
}

QList<GoogleEvent::ConstPtr> GoogleCalendarDatabase::events(int accountId, const QString &localCalendarId)
{
    QList<GoogleEvent::ConstPtr> data;

    QSqlQuery query;

    if (localCalendarId.isEmpty()) {
        query = prepare(QStringLiteral(
                    "SELECT accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId FROM events "\
                    "WHERE accountId = :accountId"));
        query.bindValue(":accountId", accountId);
    } else {
        query = prepare(QStringLiteral(
                    "SELECT accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId FROM events "\
                    "WHERE accountId = :accountId AND localCalendarId = :localCalendarId"));
        query.bindValue(":accountId", accountId);
        query.bindValue(":localCalendarId", localCalendarId);
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query events" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(GoogleEvent::create(query.value(0).toInt(),
                                        query.value(1).toString(),
                                        query.value(2).toString(),
                                        query.value(3).toString(),
                                        query.value(4).toString()));
    }

    return data;
}

QString GoogleCalendarDatabase::gcalEventId(int accountId, const QString &localCalendarId, const QString &localEventId, const QString &localEventRecurrenceId)
{
    Q_D(const GoogleCalendarDatabase);

    // check pre-commit data
    Q_FOREACH(const GoogleEvent::ConstPtr &evt, d->insertEvents[accountId]) {
        if (evt->accountId() == accountId
                && evt->localCalendarId() == localCalendarId
                && evt->localEventId() == localEventId
                && evt->localEventRecurrenceId() == localEventRecurrenceId) {
            return evt->gcalEventId();
        }
    }

    // check committed data
    Q_FOREACH(const GoogleEvent::ConstPtr &evt, events(accountId, localCalendarId)) {
        if (evt->accountId() == accountId
                && evt->localCalendarId() == localCalendarId
                && evt->localEventId() == localEventId
                && evt->localEventRecurrenceId() == localEventRecurrenceId) {
            return evt->gcalEventId();
        }
    }

    return QString();
}

void GoogleCalendarDatabase::insertEvent(int accountId,
                                         const QString &gcalEventId,
                                         const QString &localCalendarId,
                                         const QString &localEventId,
                                         const QString &localEventRecurrenceId)
{
    Q_D(GoogleCalendarDatabase);

    // check to see if it already exists in the pre-commit list
    for (int i = 0; i < d->insertEvents[accountId].size(); ++i) {
        const GoogleEvent::ConstPtr &evt(d->insertEvents[accountId][i]);
        if (evt->accountId() == accountId
                && evt->gcalEventId() == gcalEventId
                && evt->localCalendarId() == localCalendarId
                && evt->localEventId() == localEventId
                && evt->localEventRecurrenceId() == localEventRecurrenceId) {
            return; // already exists in the pre-commit list
        }
    }

    d->insertEvents[accountId].append(GoogleEvent::create(accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId));
}

void GoogleCalendarDatabase::removeEvent(int accountId,
                                         const QString &gcalEventId,
                                         const QString &localCalendarId,
                                         const QString &localEventId,
                                         const QString &localEventRecurrenceId)
{
    Q_D(GoogleCalendarDatabase);

    GoogleEvent::ConstPtr doomed = GoogleEvent::create(accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId);
    d->removeEvents[accountId].append(doomed);

    // and just in case, check the pre-commit insert list and remove it from there too.
    for (int i = 0; i < d->insertEvents[accountId].size(); ++i) {
        const GoogleEvent::ConstPtr &evt(d->insertEvents[accountId][i]);
        if (evt->accountId() == accountId
                && evt->gcalEventId() == gcalEventId
                && (localCalendarId.isEmpty() || evt->localCalendarId() == localCalendarId)
                && (localEventId.isEmpty() || evt->localEventId() == localEventId)
                && ((localEventId.isEmpty() && localEventRecurrenceId.isEmpty()) || evt->localEventRecurrenceId() == localEventRecurrenceId)) {
            d->insertEvents[accountId].removeAt(i);
        }
    }
}

void GoogleCalendarDatabase::removeEvents(int accountId, const QString &localCalendarId)
{
    Q_D(GoogleCalendarDatabase);

    // check pre-commit data, remove any which match
    for (int i = d->insertEvents[accountId].size() - 1; i >= 0; --i) {
        const GoogleEvent::ConstPtr &evt(d->insertEvents[accountId].at(i));
        if (evt->accountId() == accountId
                && (localCalendarId.isEmpty() || evt->localCalendarId() == localCalendarId)) {
            d->insertEvents[accountId].removeAt(i);
        }
    }

    // check committed data, and doom any which match
    Q_FOREACH(const GoogleEvent::ConstPtr &evt, events(accountId, localCalendarId)) {
        if (evt->accountId() == accountId
                && (localCalendarId.isEmpty() || evt->localCalendarId() == localCalendarId)) {
            d->removeEvents[accountId].append(evt);
        }
    }
}

QString GoogleCalendarDatabase::lastUpdateTime(const QString &calendarId, int accountId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT calendarId, lastUpdateTime, accountId "
                "FROM updateTimes WHERE calendarId = :calendarId AND accountId = :accountId"));
    query.bindValue(":calendarId", calendarId);
    query.bindValue(":accountId", accountId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from updateTimes table:" << query.lastError();
        return QString();
    }

    if (!query.next()) {
        return QString();
    }

    return query.value(1).toString();
}

void GoogleCalendarDatabase::setLastUpdateTime(const QString &calendarId, int accountId, const QString &lastUpdateTime)
{
    Q_D(GoogleCalendarDatabase);

    d->insertLastUpdateTimes.remove(calendarId);
    d->insertLastUpdateTimes.insert(calendarId, qMakePair(lastUpdateTime, accountId));
}

void GoogleCalendarDatabase::removeLastUpdateTimes(int accountId)
{
    Q_D(GoogleCalendarDatabase);

    if (!d->removeLastUpdateTimes.contains(accountId)) {
        d->removeLastUpdateTimes.append(accountId);
    }
}

void GoogleCalendarDatabase::sync()
{
    Q_D(GoogleCalendarDatabase);

    {
        QMutexLocker locker(&d->mutex);
        Q_FOREACH(int accountId, d->insertEvents.keys()) {
            d->queue.insertEvents.insert(accountId, d->insertEvents.take(accountId));
        }
        Q_FOREACH(int accountId, d->removeEvents.keys()) {
            d->queue.removeEvents.insert(accountId, d->removeEvents.take(accountId));
        }
        Q_FOREACH(const QString calendarId, d->insertLastUpdateTimes.keys()) {
            d->queue.insertLastUpdateTimes.insert(calendarId, d->insertLastUpdateTimes.take(calendarId));
        }
        Q_FOREACH(int accountId, d->removeLastUpdateTimes) {
            d->queue.removeLastUpdateTimes.append(accountId);
        }
        d->removeLastUpdateTimes.clear();
    }

    executeWrite();
}

bool GoogleCalendarDatabase::write()
{
    Q_D(GoogleCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    const QMap<int, QList<GoogleEvent::ConstPtr> > insertEvents = d->queue.insertEvents;
    const QMap<int, QList<GoogleEvent::ConstPtr> > removeEvents = d->queue.removeEvents;
    const QMap<QString, QPair<QString, int> > insertLastUpdateTimes = d->queue.insertLastUpdateTimes;
    const QList<int> removeLastUpdateTimes = d->queue.removeLastUpdateTimes;

    d->queue.insertEvents.clear();
    d->queue.removeEvents.clear();
    d->queue.insertLastUpdateTimes.clear();
    d->queue.removeLastUpdateTimes.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!removeEvents.isEmpty()) {
        QVariantList accountIds;
        QVariantList gcalEventIds;

        Q_FOREACH (const QList<GoogleEvent::ConstPtr> &events, removeEvents) {
            Q_FOREACH (const GoogleEvent::ConstPtr &event, events) {
                accountIds.append(event->accountId());
                gcalEventIds.append(event->gcalEventId());
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM events "
                    "WHERE accountId = :accountId AND gcalEventId = :gcalEventId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":gcalEventId"), gcalEventIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertEvents.isEmpty()) {
        QVariantList accountIds;
        QVariantList gcalEventIds;
        QVariantList localCalendarIds;
        QVariantList localEventIds;
        QVariantList localEventRecurrenceIds;

        Q_FOREACH (const QList<GoogleEvent::ConstPtr> &events, insertEvents) {
            Q_FOREACH (const GoogleEvent::ConstPtr &event, events) {
                accountIds.append(event->accountId());
                gcalEventIds.append(event->gcalEventId());
                localCalendarIds.append(event->localCalendarId());
                localEventIds.append(event->localEventId());
                localEventRecurrenceIds.append(event->localEventRecurrenceId());
            }
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO events ("
                    " accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId) "
                    "VALUES("
                    " :accountId, :gcalEventId, :localCalendarId, :localEventId, :localEventRecurrenceId)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":gcalEventId"), gcalEventIds);
        query.bindValue(QStringLiteral(":localCalendarId"), localCalendarIds);
        query.bindValue(QStringLiteral(":localEventId"), localEventIds);
        query.bindValue(QStringLiteral(":localEventRecurrenceId"), localEventRecurrenceIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertLastUpdateTimes.isEmpty()) {
        QVariantList calendarIds;
        QVariantList lastUpdateTimes;
        QVariantList accountIds;

        QStringList calendars = insertLastUpdateTimes.keys();
        Q_FOREACH (const QString calendar, calendars) {
            QPair<QString, int> item = insertLastUpdateTimes.value(calendar);
            calendarIds.append(calendar);
            lastUpdateTimes.append(item.first);
            accountIds.append(item.second);
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO updateTimes ("
                    " calendarId, lastUpdateTime, accountId) "
                    "VALUES("
                    " :calendarId, :lastUpdateTime, :accountId)"));
        query.bindValue(QStringLiteral(":calendarId"), calendarIds);
        query.bindValue(QStringLiteral(":lastUpdateTime"), lastUpdateTimes);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeLastUpdateTimes.isEmpty()) {
        QVariantList accountIds;

        Q_FOREACH (int accountId, removeLastUpdateTimes) {
            accountIds.append(accountId);
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM updateTimes "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool GoogleCalendarDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    // create the Google event db tables
    // events = accountId, gcalEventId, localCalendarId, localEventId, localEventRecurrenceId
    query.prepare("CREATE TABLE IF NOT EXISTS events ("
                  "accountId INTEGER,"
                  "gcalEventId TEXT,"
                  "localCalendarId TEXT,"
                  "localEventId TEXT,"
                  "localEventRecurrenceId TEXT,"
                  "CONSTRAINT id PRIMARY KEY (accountId, gcalEventId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create events table:" << query.lastError().text();
        return false;
    }

    // lastUpdateTimes = calendarId, accountId, lastUpdateTime
    query.prepare("CREATE TABLE IF NOT EXISTS updateTimes ("
                  "calendarId TEXT UNIQUE PRIMARY KEY,"
                  "accountId INTEGER,"
                  "lastUpdateTime TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create updateTimes table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool GoogleCalendarDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS events"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events table:" << query.lastError().text();
        return false;
    }

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS updateTimes"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete updateTimes table:" << query.lastError().text();
        return false;
    }

    return true;
}
