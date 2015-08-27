/*
 * Copyright (C) 2014 Jolla Ltd.
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

#include "facebooknotificationsmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "facebooknotificationsdatabase.h"

class FacebookNotificationsModelPrivate : public AbstractSocialCacheModelPrivate
{
public:
    explicit FacebookNotificationsModelPrivate(FacebookNotificationsModel *q);

    FacebookNotificationsDatabase database;

private:
    Q_DECLARE_PUBLIC(FacebookNotificationsModel)
};

FacebookNotificationsModelPrivate::FacebookNotificationsModelPrivate(FacebookNotificationsModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

FacebookNotificationsModel::FacebookNotificationsModel(QObject *parent)
    : AbstractSocialCacheModel(*(new FacebookNotificationsModelPrivate(this)), parent)
{
    Q_D(FacebookNotificationsModel);

    connect(&d->database, SIGNAL(notificationsChanged()), this, SLOT(notificationsChanged()));
    connect(&d->database, SIGNAL(accountIdFilterChanged()), this, SIGNAL(accountIdFilterChanged()));
}

QHash<int, QByteArray> FacebookNotificationsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(NotificationId, "notificationId");
    roleNames.insert(From, "from");
    roleNames.insert(To, "to");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Title, "title");
    roleNames.insert(Link, "link");
    roleNames.insert(AppId, "appId");
    roleNames.insert(Object, "object");
    roleNames.insert(Unread, "unread");
    roleNames.insert(Accounts, "accounts");
    roleNames.insert(ClientId, "clientId");

    return roleNames;
}

QVariantList FacebookNotificationsModel::accountIdFilter() const
{
    Q_D(const FacebookNotificationsModel);

    return d->database.accountIdFilter();
}

void FacebookNotificationsModel::setAccountIdFilter(const QVariantList &accountIds)
{
    Q_D(FacebookNotificationsModel);

    d->database.setAccountIdFilter(accountIds);
}

void FacebookNotificationsModel::refresh()
{
    notificationsChanged();
}

void FacebookNotificationsModel::remove(const QString &notificationId)
{
    Q_D(FacebookNotificationsModel);
    for (int i=0; i<count(); i++) {
        if (getField(i, FacebookNotificationsModel::NotificationId).toString() == notificationId) {
            d->removeRange(i, 1);
            d->database.removeNotification(notificationId);
            d->database.sync();
            break;
        }
    }
}

void FacebookNotificationsModel::clear()
{
    Q_D(FacebookNotificationsModel);
    d->clearData();
    d->database.removeAllNotifications();
}

void FacebookNotificationsModel::notificationsChanged()
{
    Q_D(FacebookNotificationsModel);

    SocialCacheModelData data;
    QList<FacebookNotification::ConstPtr> notificationsData = d->database.notifications();
    Q_FOREACH (const FacebookNotification::ConstPtr &notification, notificationsData) {
        QMap<int, QVariant> eventMap;

        eventMap.insert(FacebookNotificationsModel::NotificationId, notification->facebookId());
        eventMap.insert(FacebookNotificationsModel::From, notification->from());
        eventMap.insert(FacebookNotificationsModel::To, notification->to());
        eventMap.insert(FacebookNotificationsModel::Timestamp, notification->updatedTime());
        eventMap.insert(FacebookNotificationsModel::Title, notification->title());
        eventMap.insert(FacebookNotificationsModel::Link, notification->link());
        eventMap.insert(FacebookNotificationsModel::AppId, notification->application());
        eventMap.insert(FacebookNotificationsModel::Object, notification->object());
        eventMap.insert(FacebookNotificationsModel::Unread, notification->unread());
        eventMap.insert(FacebookNotificationsModel::Accounts, notification->accountId());

        QVariantList accountsVariant;
        accountsVariant.append(notification->accountId());
        eventMap.insert(FacebookNotificationsModel::Accounts, accountsVariant);

        eventMap.insert(FacebookNotificationsModel::ClientId, notification->clientId());

        data.append(eventMap);
    }

    updateData(data);
}
