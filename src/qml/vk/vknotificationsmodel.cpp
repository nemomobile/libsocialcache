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

#include "vknotificationsmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "vknotificationsdatabase.h"
#include <QtCore/QDebug>

class VKNotificationsModelPrivate: public AbstractSocialCacheModelPrivate
{
public:
    explicit VKNotificationsModelPrivate(VKNotificationsModel *q);

    VKNotificationsDatabase database;

private:
    Q_DECLARE_PUBLIC(VKNotificationsModel)
};

VKNotificationsModelPrivate::VKNotificationsModelPrivate(VKNotificationsModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

VKNotificationsModel::VKNotificationsModel(QObject *parent)
    : AbstractSocialCacheModel(*(new VKNotificationsModelPrivate(this)), parent)
{
    Q_D(VKNotificationsModel);
    connect(&d->database, SIGNAL(notificationsChanged()), this, SLOT(notificationsChanged()));
}

QHash<int, QByteArray> VKNotificationsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(VkId, "vkId");
    roleNames.insert(Type, "type");
    roleNames.insert(From, "from");
    roleNames.insert(FromName, "fromName");
    roleNames.insert(FromName, "fromIcon");
    roleNames.insert(FromName, "to");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Accounts, "accounts");
    return roleNames;
}

void VKNotificationsModel::refresh()
{
    notificationsChanged();
}

void VKNotificationsModel::notificationsChanged()
{
    Q_D(VKNotificationsModel);

    SocialCacheModelData data;
    QList<VKNotification::ConstPtr> notificationsData = d->database.notifications();
    Q_FOREACH (const VKNotification::ConstPtr &notification, notificationsData) {
        QMap<int, QVariant> eventMap;

        eventMap.insert(VKNotificationsModel::VkId, notification->identifier());
        eventMap.insert(VKNotificationsModel::Type, notification->type());
        eventMap.insert(VKNotificationsModel::From, notification->identifier());
        eventMap.insert(VKNotificationsModel::FromName, notification->identifier());
        eventMap.insert(VKNotificationsModel::FromIcon, notification->identifier());
        eventMap.insert(VKNotificationsModel::To, notification->identifier());
        eventMap.insert(VKNotificationsModel::Timestamp, notification->createdTime());

        QVariantList accountsVariant;
        accountsVariant.append(notification->accountId());
        eventMap.insert(VKNotificationsModel::Accounts, accountsVariant);

        data.append(eventMap);
    }

    updateData(data);
}
