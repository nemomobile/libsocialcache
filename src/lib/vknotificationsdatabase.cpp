/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
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

#include "vknotificationsdatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlError>
#include <QtCore/QtDebug>

static const char *DB_NAME = "vkNotifications.db";
static const int VERSION = 1;

struct VKNotificationPrivate
{
    explicit VKNotificationPrivate(const QString &identifier,
                                   int accountId,
                                   VKNotification::Type type,
                                   const QString &fromId,
                                   const QString &toId,
                                   const QDateTime &createdTime);

    QString m_id;
    int m_accountId;
    VKNotification::Type m_type;
    QString m_fromId;
    QString m_toId;
    QDateTime m_createdTime;
};

VKNotificationPrivate::VKNotificationPrivate(const QString &identifier,
                                             int accountId,
                                             VKNotification::Type type,
                                             const QString &fromId,
                                             const QString &toId,
                                             const QDateTime &createdTime)
    : m_id(identifier)
    , m_accountId(accountId)
    , m_type(type)
    , m_fromId(fromId)
    , m_toId(toId)
    , m_createdTime(createdTime)
{
}

VKNotification::VKNotification(const QString &identifier,
                               int accountId,
                               Type type,
                               const QString &fromId,
                               const QString &toId,
                               const QDateTime &createdTime)
    : d_ptr(new VKNotificationPrivate(identifier, accountId, type, fromId, toId, createdTime))
{
}

VKNotification::Ptr VKNotification::create(const QString &identifier,
                                           int accountId,
                                           Type type,
                                           const QString &fromId,
                                           const QString &toId,
                                           const QDateTime &createdTime)
{
    return VKNotification::Ptr(new VKNotification(identifier, accountId, type, fromId, toId, createdTime));
}

VKNotification::~VKNotification()
{
}

QString VKNotification::identifier() const
{
    Q_D(const VKNotification);
    return d->m_id;
}

VKNotification::Type VKNotification::type() const
{
    Q_D(const VKNotification);
    return d->m_type;
}

QString VKNotification::from() const
{
    Q_D(const VKNotification);
    return d->m_fromId;
}

QString VKNotification::to() const
{
    Q_D(const VKNotification);
    return d->m_toId;
}

QDateTime VKNotification::createdTime() const
{
    Q_D(const VKNotification);
    return d->m_createdTime;
}

int VKNotification::accountId() const
{
    Q_D(const VKNotification);
    return d->m_accountId;
}


class VKNotificationsDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit VKNotificationsDatabasePrivate(VKNotificationsDatabase *q);

    QMap<int, QList<VKNotification::ConstPtr> > insertNotifications;
    QList<int> removeNotificationsFromAccounts;
    QStringList removeNotifications;

    struct {
        QMap<int, QList<VKNotification::ConstPtr> > insertNotifications;
        QList<int> removeNotificationsFromAccounts;
        QStringList removeNotifications;
    } queue;
};

VKNotificationsDatabasePrivate::VKNotificationsDatabasePrivate(VKNotificationsDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::VK),
            SocialSyncInterface::dataType(SocialSyncInterface::Notifications),
            QLatin1String(DB_NAME),
            VERSION)
{
}

VKNotificationsDatabase::VKNotificationsDatabase()
    : AbstractSocialCacheDatabase(*(new VKNotificationsDatabasePrivate(this)))
{
}

VKNotificationsDatabase::~VKNotificationsDatabase()
{
    wait();
}

void VKNotificationsDatabase::addVKNotification(int accountId,
                                                VKNotification::Type type,
                                                const QString &fromId,
                                                const QString &toId,
                                                const QDateTime &createdTime)
{
    Q_D(VKNotificationsDatabase);
    d->insertNotifications[accountId].append(VKNotification::create(QString(), accountId, type, fromId, toId, createdTime));
}

void VKNotificationsDatabase::removeNotifications(int accountId)
{
    Q_D(VKNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removeNotificationsFromAccounts.contains(accountId)) {
        d->queue.removeNotificationsFromAccounts.append(accountId);
    }
    d->queue.insertNotifications.remove(accountId);
}

void VKNotificationsDatabase::removeNotification(const QString &notificationId)
{
    Q_D(VKNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removeNotifications.contains(notificationId)) {
        d->queue.removeNotifications.append(notificationId);
    }
}

void VKNotificationsDatabase::removeNotifications(const QStringList &notificationIds)
{
    Q_D(VKNotificationsDatabase);

    QMutexLocker locker(&d->mutex);
    Q_FOREACH(const QString notifId, notificationIds) {
        removeNotification(notifId);
    }
}

void VKNotificationsDatabase::sync()
{
    Q_D(VKNotificationsDatabase);

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

QList<VKNotification::ConstPtr> VKNotificationsDatabase::notifications()
{
    QList<VKNotification::ConstPtr> data;

    QSqlQuery query;
    query = prepare(QStringLiteral(
                "SELECT identifier, accountId, type, fromStr, toStr, createdTime " \
                "FROM notifications ORDER BY createdTime DESC"));

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query events" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(VKNotification::create(QString::number(query.value(0).toInt()),         // id
                                           query.value(1).toInt(),                          // accountId
                                           VKNotification::Type(query.value(2).toInt()),    // type
                                           query.value(3).toString(),                       // from
                                           query.value(4).toString(),                       // to
                                           QDateTime::fromTime_t(query.value(5).toInt()))); // createdTime
    }

    return data;
}

void VKNotificationsDatabase::readFinished()
{
    emit notificationsChanged();
}

bool VKNotificationsDatabase::write()
{
    Q_D(VKNotificationsDatabase);

    QMutexLocker locker(&d->mutex);

    const QMap<int, QList<VKNotification::ConstPtr> > insertNotifications = d->queue.insertNotifications;
    const QList<int> removeNotificationsFromAccounts = d->queue.removeNotificationsFromAccounts;
    QStringList removeNotifications = d->queue.removeNotifications;

    d->queue.insertNotifications.clear();
    d->queue.removeNotificationsFromAccounts.clear();
    d->queue.removeNotifications.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

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
            notifIds.append(notifId.toInt());
        }

        query = prepare(QStringLiteral("DELETE FROM notifications WHERE identifier = :identifier"));
        query.bindValue(QStringLiteral(":identifier"), notifIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertNotifications.isEmpty()) {
        QVariantList accountIds;
        QVariantList fromStrings;
        QVariantList toStrings;
        QVariantList createdTimes;
        QVariantList types;

        Q_FOREACH (const QList<VKNotification::ConstPtr> &notifications, insertNotifications) {
            Q_FOREACH (const VKNotification::ConstPtr &notification, notifications) {
                accountIds.append(notification->accountId());
                fromStrings.append(notification->from());
                toStrings.append(notification->to());
                createdTimes.append(notification->createdTime().toTime_t());
                types.append(int(notification->type()));
            }
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO notifications ("
                    " accountId, fromStr, toStr, createdTime) "
                    "VALUES("
                    " :accountId, :type, :fromStr, :toStr, :createdTime)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":type"), types);
        query.bindValue(QStringLiteral(":fromStr"), fromStrings);
        query.bindValue(QStringLiteral(":toStr"), toStrings);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);

        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool VKNotificationsDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    // create the VK notification db tables
    // notifications = identifier, accountId, from, to, createdTime
    query.prepare("CREATE TABLE IF NOT EXISTS notifications ("
                  "identifier INTEGER NOT NULL AUTO_INCREMENT,"
                  "accountId INTEGER,"
                  "fromStr TEXT,"
                  "toStr TEXT,"
                  "createdTime INTEGER,"
                  "type INTEGER");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create notifications table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool VKNotificationsDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS notifications"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete notifications table: " << query.lastError().text();
        return false;
    }

    return true;
}
