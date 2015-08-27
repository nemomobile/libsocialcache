/*
 * Copyright (C) 2014-2015 Jolla Ltd.
 * Contact: Antti Seppälä <antti.seppala@jollamobile.com>
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

#include "facebooknotificationsdatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlError>
#include <QtCore/QtDebug>

static const char *DB_NAME = "facebookNotifications.db";
static const int VERSION = 1;

struct FacebookNotificationPrivate
{
    explicit FacebookNotificationPrivate(const QString &facebookId, const QString &from, const QString &to,
                                         const QDateTime &createdTime, const QDateTime &updatedTime,
                                         const QString &title, const QString &link,
                                         const QString &application, const QString &object,
                                         bool unread, int accountId, const QString &clientId);

    QString m_facebookId;
    QString m_from;
    QString m_to;
    QDateTime m_createdTime;
    QDateTime m_updatedTime;
    QString m_title;
    QString m_link;
    QString m_application;
    QString m_object;
    bool m_unread;
    int m_accountId;
    QString m_clientId;
};

FacebookNotificationPrivate::FacebookNotificationPrivate(const QString &facebookId, const QString &from, const QString &to,
                                                         const QDateTime &createdTime, const QDateTime &updatedTime,
                                                         const QString &title, const QString &link,
                                                         const QString &application, const QString &object,
                                                         bool unread, int accountId, const QString &clientId)
    : m_facebookId(facebookId)
    , m_from(from)
    , m_to(to)
    , m_createdTime(createdTime)
    , m_updatedTime(updatedTime)
    , m_title(title)
    , m_link(link)
    , m_application(application)
    , m_object(object)
    , m_unread(unread)
    , m_accountId(accountId)
    , m_clientId(clientId)
{
}

FacebookNotification::FacebookNotification(const QString &facebookId, const QString &from, const QString &to,
                                           const QDateTime &createdTime, const QDateTime &updatedTime,
                                           const QString &title, const QString &link,
                                           const QString &application, const QString &object,
                                           bool unread, int accountId, const QString &clientId)
    : d_ptr(new FacebookNotificationPrivate(facebookId, from, to, createdTime, updatedTime, title, link,
                                            application, object, unread, accountId, clientId))
{
}

FacebookNotification::Ptr FacebookNotification::create(const QString &facebookId, const QString &from, const QString &to,
                                                       const QDateTime &createdTime, const QDateTime &updatedTime,
                                                       const QString &title, const QString &link,
                                                       const QString &application, const QString &object,
                                                       bool unread, int accountId, const QString &clientId)
{
    return FacebookNotification::Ptr(new FacebookNotification(facebookId, from, to, createdTime, updatedTime, title, link,
                                                              application, object, unread, accountId, clientId));
}

FacebookNotification::~FacebookNotification()
{
}

QString FacebookNotification::facebookId() const
{
    Q_D(const FacebookNotification);
    return d->m_facebookId;
}

QString FacebookNotification::from() const
{
    Q_D(const FacebookNotification);
    return d->m_from;
}

QString FacebookNotification::to() const
{
    Q_D(const FacebookNotification);
    return d->m_to;
}

QDateTime FacebookNotification::createdTime() const
{
    Q_D(const FacebookNotification);
    return d->m_createdTime;
}

QDateTime FacebookNotification::updatedTime() const
{
    Q_D(const FacebookNotification);
    return d->m_updatedTime;
}

QString FacebookNotification::title() const
{
    Q_D(const FacebookNotification);
    return d->m_title;
}

QString FacebookNotification::link() const
{
    Q_D(const FacebookNotification);
    return d->m_link;
}

QString FacebookNotification::application() const
{
    Q_D(const FacebookNotification);
    return d->m_application;
}

QString FacebookNotification::object() const
{
    Q_D(const FacebookNotification);
    return d->m_object;
}

bool FacebookNotification::unread() const
{
    Q_D(const FacebookNotification);
    return d->m_unread;
}

int FacebookNotification::accountId() const
{
    Q_D(const FacebookNotification);
    return d->m_accountId;
}

QString FacebookNotification::clientId() const
{
    Q_D(const FacebookNotification);
    return d->m_clientId;
}

class FacebookNotificationsDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit FacebookNotificationsDatabasePrivate(FacebookNotificationsDatabase *q);

    QMap<int, QList<FacebookNotification::ConstPtr> > insertNotifications;
    QList<int> removeNotificationsFromAccounts;
    QVariantList accountIdFilter;
    QStringList removeNotifications;
    int purgeTimeLimit;

    struct {
        QMap<int, QList<FacebookNotification::ConstPtr> > insertNotifications;
        QList<int> removeNotificationsFromAccounts;
        QStringList removeNotifications;
        bool removeAll;
    } queue;
};

FacebookNotificationsDatabasePrivate::FacebookNotificationsDatabasePrivate(FacebookNotificationsDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
            SocialSyncInterface::dataType(SocialSyncInterface::Notifications),
            QLatin1String(DB_NAME),
            VERSION)
    , purgeTimeLimit(0)
{
    queue.removeAll = false;
}

FacebookNotificationsDatabase::FacebookNotificationsDatabase()
    : AbstractSocialCacheDatabase(*(new FacebookNotificationsDatabasePrivate(this)))
{
}

FacebookNotificationsDatabase::~FacebookNotificationsDatabase()
{
    wait();
}

QVariantList FacebookNotificationsDatabase::accountIdFilter() const
{
    Q_D(const FacebookNotificationsDatabase);

    return d->accountIdFilter;
}

void FacebookNotificationsDatabase::setAccountIdFilter(const QVariantList &accountIds)
{
    Q_D(FacebookNotificationsDatabase);

    if (d->accountIdFilter != accountIds) {
        d->accountIdFilter = accountIds;
        emit accountIdFilterChanged();
    }
}

void FacebookNotificationsDatabase::addFacebookNotification(const QString &facebookId, const QString &from, const QString &to,
                                                            const QDateTime &createdTime, const QDateTime &updatedTime,
                                                            const QString &title, const QString &link,
                                                            const QString &application, const QString &object,
                                                            bool unread, int accountId, const QString &clientId)
{
    Q_D(FacebookNotificationsDatabase);
    d->insertNotifications[accountId].append(FacebookNotification::create(facebookId, from, to, createdTime, updatedTime, title, link,
                                                                          application, object, unread, accountId, clientId));
}

void FacebookNotificationsDatabase::removeAllNotifications()
{
    Q_D(FacebookNotificationsDatabase);

    {
        QMutexLocker locker(&d->mutex);
        d->queue.insertNotifications.clear();
        d->queue.removeNotificationsFromAccounts.clear();
        d->queue.removeNotifications.clear();
        d->queue.removeAll = true;
    }

    executeWrite();
}

void FacebookNotificationsDatabase::removeNotifications(int accountId)
{
    Q_D(FacebookNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removeNotificationsFromAccounts.contains(accountId)) {
        d->queue.removeNotificationsFromAccounts.append(accountId);
    }
    d->queue.insertNotifications.remove(accountId);
}

void FacebookNotificationsDatabase::removeNotification(const QString &notificationId)
{
    Q_D(FacebookNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    removeNotificationFromQueues(notificationId);
}

void FacebookNotificationsDatabase::removeNotifications(QStringList notificationIds)
{
    Q_D(FacebookNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    Q_FOREACH(const QString notifId, notificationIds) {
        removeNotificationFromQueues(notifId);
    }
}

void FacebookNotificationsDatabase::purgeOldNotifications(int limitInDays)
{
    Q_D(FacebookNotificationsDatabase);
    d->purgeTimeLimit = limitInDays;
}

void FacebookNotificationsDatabase::removeNotificationFromQueues(const QString &notificationId)
{
    Q_D(FacebookNotificationsDatabase);

    if (!d->queue.removeNotifications.contains(notificationId)) {
        d->queue.removeNotifications.append(notificationId);
        Q_FOREACH(int accountId, d->insertNotifications.keys()) {
            for (int i = 0; i < d->insertNotifications[accountId].count(); ++i) {
                if (d->insertNotifications[accountId][i]->facebookId() == notificationId) {
                    d->insertNotifications[accountId].removeAt(i);
                    i--;
                }
            }
        }
    }
}

void FacebookNotificationsDatabase::sync()
{
    Q_D(FacebookNotificationsDatabase);

    {
        QMutexLocker locker(&d->mutex);
        Q_FOREACH(int accountId, d->insertNotifications.keys()) {
            d->queue.insertNotifications.insert(accountId, d->insertNotifications.take(accountId));
        }
        while (d->removeNotificationsFromAccounts.count()) {
            d->queue.removeNotificationsFromAccounts.append(d->removeNotificationsFromAccounts.takeFirst());
        }
        while (d->removeNotifications.count()) {
            d->queue.removeNotifications.append(d->removeNotifications.takeFirst());
        }
    }

    executeWrite();
}

QList<FacebookNotification::ConstPtr> FacebookNotificationsDatabase::notifications()
{
    Q_D(FacebookNotificationsDatabase);

    QList<FacebookNotification::ConstPtr> data;

    QString queryString = QStringLiteral(
                "SELECT facebookId, accountId, fromStr, toStr, createdTime, updatedTime, title, link, application," \
                "objectStr, unread, clientId FROM notifications");
    if (!d->accountIdFilter.isEmpty()) {
        QStringList accountIds;
        for (int i=0; i<d->accountIdFilter.count(); i++) {
            if (d->accountIdFilter[i].type() == QVariant::Int) {
                accountIds << d->accountIdFilter[i].toString();
            }
        }
        if (accountIds.count()) {
            queryString += " WHERE accountId IN (" + accountIds.join(',') + ')';
        }
    }
    queryString += QStringLiteral(" ORDER BY updatedTime DESC");
    QSqlQuery query = prepare(queryString);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query events" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookNotification::create(query.value(0).toString(),                      // facebookId
                                                 query.value(2).toString(),                      // from
                                                 query.value(3).toString(),                      // to
                                                 QDateTime::fromTime_t(query.value(4).toInt()),  // createdTime
                                                 QDateTime::fromTime_t(query.value(5).toInt()),  // updatedTime
                                                 query.value(6).toString(),                      // title
                                                 query.value(7).toString(),                      // link
                                                 query.value(8).toString(),                      // application
                                                 query.value(9).toString(),                      // object
                                                 query.value(10).toBool(),                       // unread
                                                 query.value(1).toInt(),                         // accountId
                                                 query.value(11).toString()));                   // clientId
    }

    return data;
}

void FacebookNotificationsDatabase::readFinished()
{
    emit notificationsChanged();
}

bool FacebookNotificationsDatabase::write()
{
    Q_D(FacebookNotificationsDatabase);

    QMutexLocker locker(&d->mutex);

    const QMap<int, QList<FacebookNotification::ConstPtr> > insertNotifications = d->queue.insertNotifications;
    const QList<int> removeNotificationsFromAccounts = d->queue.removeNotificationsFromAccounts;
    QStringList removeNotifications = d->queue.removeNotifications;
    bool removeAll = d->queue.removeAll;

    d->queue.insertNotifications.clear();
    d->queue.removeNotificationsFromAccounts.clear();
    d->queue.removeNotifications.clear();
    d->queue.removeAll = false;

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (removeAll) {
        QVariantList accountIds;
        accountIds.append(-1);

        query = prepare(QStringLiteral("DELETE FROM notifications WHERE accountId > :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeNotificationsFromAccounts.isEmpty()) {
        QVariantList accountIds;

        Q_FOREACH (const int accountId, removeNotificationsFromAccounts) {
            accountIds.append(accountId);
        }

        query = prepare(QStringLiteral("DELETE FROM notifications WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeNotifications.isEmpty()) {
        QVariantList notifIds;

        Q_FOREACH (const QString notifId, removeNotifications) {
            notifIds.append(notifId);
        }

        query = prepare(QStringLiteral("DELETE FROM notifications WHERE facebookId = :facebookId"));
        query.bindValue(QStringLiteral(":facebookId"), notifIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertNotifications.isEmpty()) {
        QVariantList facebookIds;
        QVariantList accountIds;
        QVariantList fromStrings;
        QVariantList toStrings;
        QVariantList createdTimes;
        QVariantList updatedTimes;
        QVariantList titles;
        QVariantList links;
        QVariantList applications;
        QVariantList unreads;
        QVariantList objects;
        QVariantList clientIds;

        Q_FOREACH (const QList<FacebookNotification::ConstPtr> &notifications, insertNotifications) {
            Q_FOREACH (const FacebookNotification::ConstPtr &notification, notifications) {
                facebookIds.append(notification->facebookId());
                accountIds.append(notification->accountId());
                fromStrings.append(notification->from());
                toStrings.append(notification->to());
                createdTimes.append(notification->createdTime().toTime_t());
                updatedTimes.append(notification->updatedTime().toTime_t());
                titles.append(notification->title());
                links.append(notification->link());
                applications.append(notification->application());
                objects.append(notification->object());
                unreads.append((notification->unread()));
                clientIds.append(notification->clientId());
            }
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO notifications ("
                    " facebookId, accountId, fromStr, toStr, createdTime, updatedTime, title, link, application, objectStr, unread, clientId) "
                    "VALUES("
                    " :facebookId, :accountId, :fromStr, :toStr, :createdTime, :updatedTime, :title, :link, :application, :objectStr, :unread, :clientId)"));
        query.bindValue(QStringLiteral(":facebookId"), facebookIds);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":fromStr"), fromStrings);
        query.bindValue(QStringLiteral(":toStr"), toStrings);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":title"), titles);
        query.bindValue(QStringLiteral(":link"), links);
        query.bindValue(QStringLiteral(":application"), applications);
        query.bindValue(QStringLiteral(":objectStr"), objects);
        query.bindValue(QStringLiteral(":unread"), unreads);
        query.bindValue(QStringLiteral(":clientId"), clientIds);

        executeBatchSocialCacheQuery(query);
    }

    if (d->purgeTimeLimit > 0) {
        QVariantList limits;
        // purge notifications older than expirationTime in days
        const quint32 limit = QDateTime::currentDateTime().toTime_t() - d->purgeTimeLimit * 24 * 60 * 60;
        limits.append(limit);
        query = prepare(QStringLiteral("DELETE FROM notifications WHERE updatedTime < :timeLimit"));
        query.bindValue(QStringLiteral(":timeLimit"), limits);
        executeBatchSocialCacheQuery(query);
        d->purgeTimeLimit = 0;
    }

    return success;
}

bool FacebookNotificationsDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    // create the Facebook notification db tables
    // notifications = facebookId, accountId, from, to, createdTime, updatedTime, title, link, application, objectStr, unread, clientId
    query.prepare("CREATE TABLE IF NOT EXISTS notifications ("
                  "facebookId TEXT UNIQUE PRIMARY KEY,"
                  "accountId INTEGER,"
                  "fromStr TEXT,"
                  "toStr TEXT,"
                  "createdTime INTEGER,"
                  "updatedTime INTEGER,"
                  "title TEXT,"
                  "link TEXT,"
                  "application TEXT,"
                  "objectStr TEXT,"
                  "unread INTEGER,"
                  "clientId TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create notifications table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool FacebookNotificationsDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS notifications"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete notifications table: " << query.lastError().text();
        return false;
    }

    return true;
}
