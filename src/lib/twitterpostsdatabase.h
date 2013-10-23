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

#ifndef TWITTERPOSTSDATABASE_H
#define TWITTERPOSTSDATABASE_H

#include "abstractsocialpostcachedatabase.h"

class TwitterPostsDatabase: public AbstractSocialPostCacheDatabase
{
public:
    explicit TwitterPostsDatabase();
    ~TwitterPostsDatabase();

    void initDatabase();

    void addTwitterPost(const QString &identifier, const QString &name, const QString &body,
                        const QDateTime &timestamp,
                        const QString &icon,
                        const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                        const QString &screenName, const QString &retweeter,
                        const QString &consumerKey, const QString &consumerSecret,
                        int account);

    static QString screenName(const SocialPost::ConstPtr &post);
    static QString retweeter(const SocialPost::ConstPtr &post);
    static QString consumerKey(const SocialPost::ConstPtr &post);
    static QString consumerSecret(const SocialPost::ConstPtr &post);
};

#endif // TWITTERPOSTSDATABASE_H
