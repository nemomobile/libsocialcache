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

#ifndef VKPOSTSMODEL_H
#define VKPOSTSMODEL_H

#include "abstractsocialcachemodel.h"

class VKPostsModelPrivate;
class VKPostsModel: public AbstractSocialCacheModel
{
    Q_OBJECT
    Q_ENUMS(VKPostsRole)
public:
    enum VKPostsRole {
        VkId = 0,
        Name,
        Body,
        Timestamp,
        Icon,
        Images,
        Extra,
        Accounts,
        RepostType,
        RepostOwnerName,
        RepostOwnerAvatar,
        RepostText,
        RepostPhoto,
        RepostVideo,
        RepostLink,
        RepostTimestamp
    };
    explicit VKPostsModel(QObject *parent = 0);
    QHash<int, QByteArray> roleNames() const;

    void refresh();

    Q_INVOKABLE void remove(const QString &postId);
    Q_INVOKABLE void clear();

private Q_SLOTS:
    void postsChanged();

private:
    Q_DECLARE_PRIVATE(VKPostsModel)
};

#endif // VKPOSTSMODEL_H
