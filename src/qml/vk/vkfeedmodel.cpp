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

#include "vkfeedmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "vkpostsdatabase.h"
#include "vknotificationsdatabase.h"
#include <QtCore/QDebug>
#include <QMutex>
#include "postimagehelper_p.h"

class VKFeedModelPrivate: public AbstractSocialCacheModelPrivate
{
public:
    explicit VKFeedModelPrivate(VKFeedModel *q);

    VKPostsDatabase postDatabase;
    VKNotificationsDatabase notificationDatabase;
    QMutex mutex;

private:
    Q_DECLARE_PUBLIC(VKFeedModel)
};

VKFeedModelPrivate::VKFeedModelPrivate(VKFeedModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

VKFeedModel::VKFeedModel(QObject *parent)
    : AbstractSocialCacheModel(*(new VKFeedModelPrivate(this)), parent)
{
    Q_D(VKFeedModel);

    connect(&d->postDatabase, &AbstractSocialPostCacheDatabase::postsChanged, this, &VKFeedModel::postsChanged);
    connect(&d->notificationDatabase, SIGNAL(notificationsChanged()), this, SLOT(notificationsChanged()));
}

QHash<int, QByteArray> VKFeedModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(VkId, "vkId");
    roleNames.insert(Type, "type");
    roleNames.insert(Name, "name");
    roleNames.insert(Body, "body");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Icon, "icon");
    roleNames.insert(Images, "images");
    roleNames.insert(Accounts, "accounts");
    roleNames.insert(NotificationType, "notificationType");
    roleNames.insert(Parent, "parent");
    return roleNames;
}

void VKFeedModel::refresh()
{
    Q_D(VKFeedModel);
    d->postDatabase.refresh();
    notificationsChanged();
}

void VKFeedModel::postsChanged()
{
    Q_D(VKFeedModel);

    d->mutex.lock();
    SocialCacheModelData data;

    // keep existing notifications
    for (int i = 0; i < count(); ++i) {
        QMap<int, QVariant> eventMap = itemData(createIndex(i, 0));
        if (eventMap.value(VKFeedModel::Type) == "notification") {
            data.append(eventMap);
        }
    }

    QList<SocialPost::ConstPtr> postsData = d->postDatabase.posts();
    Q_FOREACH (const SocialPost::ConstPtr &post, postsData) {
        QMap<int, QVariant> eventMap;
        eventMap.insert(VKFeedModel::VkId, post->identifier());
        eventMap.insert(VKFeedModel::Type, "post");
        eventMap.insert(VKFeedModel::Name, post->name());
        eventMap.insert(VKFeedModel::Body, post->body());
        eventMap.insert(VKFeedModel::Timestamp, post->timestamp());
        eventMap.insert(VKFeedModel::Icon, post->icon());

        QVariantList images;
        Q_FOREACH (const SocialPostImage::ConstPtr &image, post->images()) {
            images.append(createImageData(image));
        }
        eventMap.insert(VKFeedModel::Images, images);
        eventMap.insert(VKFeedModel::NotificationType, "");
        eventMap.insert(VKFeedModel::Parent, "");

        QVariantList accountsVariant;
        Q_FOREACH (int account, post->accounts()) {
            accountsVariant.append(account);
        }
        eventMap.insert(VKFeedModel::Accounts, accountsVariant);
        data.append(eventMap);
    }

    updateData(data);
    d->mutex.unlock();
}

void VKFeedModel::notificationsChanged()
{
    Q_D(VKFeedModel);

    d->mutex.lock();
    SocialCacheModelData data;

    // keep existing posts
    for (int i = 0; i < count(); ++i) {
        QMap<int, QVariant> eventMap = itemData(createIndex(i, 0));
        if (eventMap.value(VKFeedModel::Type) == "post") {
            data.append(eventMap);
        }
    }

    QList<VKNotification::ConstPtr> notificationsData = d->notificationDatabase.notifications();
    Q_FOREACH (const VKNotification::ConstPtr &notification, notificationsData) {
        QMap<int, QVariant> eventMap;

        eventMap.insert(VKFeedModel::VkId, notification->identifier());
        eventMap.insert(VKFeedModel::Type, "notification");
        eventMap.insert(VKFeedModel::Name, notification->fromName());
        eventMap.insert(VKFeedModel::Icon, notification->fromIcon());
        eventMap.insert(VKFeedModel::Body, "");
        eventMap.insert(VKFeedModel::Timestamp, notification->createdTime());
        eventMap.insert(VKFeedModel::NotificationType, notification->type());
        eventMap.insert(VKFeedModel::Parent, notification->parent());

        QVariantList images;
        eventMap.insert(VKFeedModel::Images, images);

        QVariantList accountsVariant;
        accountsVariant.append(notification->accountId());
        eventMap.insert(VKFeedModel::Accounts, accountsVariant);

        data.append(eventMap);
    }

    updateData(data);
    d->mutex.unlock();
}
