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

#ifndef FACEBOOKPOSTSDATABASE_H
#define FACEBOOKPOSTSDATABASE_H

#include "abstractsocialpostcachedatabase.h"

class FacebookPostsDatabase: public AbstractSocialPostCacheDatabase
{
public:
    explicit FacebookPostsDatabase();
    ~FacebookPostsDatabase();

    void addFacebookPost(const QString &identifier, const QString &name, const QString &body,
                         const QDateTime &timestamp,
                         const QString &icon,
                         const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                         const QString &attachmentName,  const QString &attachmentCaption,
                         const QString &attachmentDescription, const QString &attachmentUrl,
                         bool allowLike, bool allowComment,
                         const QString &clientId, int account);

    static QString attachmentName(const SocialPost::ConstPtr &post);
    static QString attachmentCaption(const SocialPost::ConstPtr &post);
    static QString attachmentDescription(const SocialPost::ConstPtr &post);
    static QString attachmentUrl(const SocialPost::ConstPtr &post);
    static bool allowLike(const SocialPost::ConstPtr &post);
    static bool allowComment(const SocialPost::ConstPtr &post);
    static QString clientId(const SocialPost::ConstPtr &post);
};

#endif // FACEBOOKPOSTSDATABASE_H
