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

#include "vkpostsmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "vkpostsdatabase.h"
#include <QtCore/QDebug>
#include "postimagehelper_p.h"

class VKPostsModelPrivate: public AbstractSocialCacheModelPrivate
{
public:
    explicit VKPostsModelPrivate(VKPostsModel *q);

    VKPostsDatabase database;

private:
    Q_DECLARE_PUBLIC(VKPostsModel)
};

VKPostsModelPrivate::VKPostsModelPrivate(VKPostsModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

VKPostsModel::VKPostsModel(QObject *parent)
    : AbstractSocialCacheModel(*(new VKPostsModelPrivate(this)), parent)
{
    Q_D(VKPostsModel);

    connect(&d->database, &AbstractSocialPostCacheDatabase::postsChanged,
            this, &VKPostsModel::postsChanged);
}

QHash<int, QByteArray> VKPostsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(VkId, "vkId");
    roleNames.insert(Name, "name");
    roleNames.insert(Body, "body");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Icon, "icon");
    roleNames.insert(Images, "images");
    roleNames.insert(Accounts, "accounts");
    return roleNames;
}

void VKPostsModel::refresh()
{
    Q_D(VKPostsModel);
    d->database.refresh();
}

void VKPostsModel::postsChanged()
{
    Q_D(VKPostsModel);

    SocialCacheModelData data;
    QList<SocialPost::ConstPtr> postsData = d->database.posts();

    Q_FOREACH (const SocialPost::ConstPtr &post, postsData) {
        QMap<int, QVariant> eventMap;
        eventMap.insert(VKPostsModel::VkId, post->identifier());
        eventMap.insert(VKPostsModel::Name, post->name());
        eventMap.insert(VKPostsModel::Body, post->body());
        eventMap.insert(VKPostsModel::Timestamp, post->timestamp());
        eventMap.insert(VKPostsModel::Icon, post->icon());

        QVariantList images;
        Q_FOREACH (const SocialPostImage::ConstPtr &image, post->images()) {
            images.append(createImageData(image));
        }
        eventMap.insert(VKPostsModel::Images, images);

        QVariantList accountsVariant;
        Q_FOREACH (int account, post->accounts()) {
            accountsVariant.append(account);
        }
        eventMap.insert(VKPostsModel::Accounts, accountsVariant);
        data.append(eventMap);
    }

    updateData(data);
}
