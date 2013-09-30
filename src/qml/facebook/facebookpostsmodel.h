/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Lucien Xu <lucien.xu@jollamobile.com>
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

#ifndef FACEBOOKPOSTSMODEL_H
#define FACEBOOKPOSTSMODEL_H

#include "abstractsocialcachemodel.h"

class FacebookPostsModelPrivate;
class FacebookPostsModel: public AbstractSocialCacheModel
{
    Q_OBJECT
public:
    enum FacebookPostsRole {
        FacebookId = 0,
        Name,
        Body,
        Timestamp,
        Icon,
        Images,
        AttachmentName,
        AttachmentCaption,
        AttachmentDescription,
        AllowLike,
        AllowComment,
        ClientId,
        Accounts
    };
    explicit FacebookPostsModel(QObject *parent = 0);
    QHash<int, QByteArray> roleNames() const;
private:
    Q_DECLARE_PRIVATE(FacebookPostsModel)
};

#endif // FACEBOOKPOSTSMODEL_H
