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

#include "twitterpostsdatabase.h"
#include "socialsyncinterface.h"

#include <QtDebug>

static const char *DB_NAME = "twitter.db";
static const char *SCREEN_NAME_KEY = "screen_name";
static const char *RETWEETER_KEY = "retweeter";
static const char *CONSUMER_KEY_KEY = "consumer_key";
static const char *CONSUMER_SECRET_KEY = "consumer_secret";

TwitterPostsDatabase::TwitterPostsDatabase()
    : AbstractSocialPostCacheDatabase()
{
}

TwitterPostsDatabase::~TwitterPostsDatabase()
{
}

void TwitterPostsDatabase::initDatabase()
{
    dbInit(SocialSyncInterface::socialNetwork(SocialSyncInterface::Twitter),
           SocialSyncInterface::dataType(SocialSyncInterface::Posts),
           QLatin1String(DB_NAME), POST_DB_VERSION);
}

void TwitterPostsDatabase::addTwitterPost(const QString &identifier, const QString &name,
                                          const QString &body, const QDateTime &timestamp,
                                          const QString &icon,
                                          const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                                          const QString &screenName, const QString &retweeter,
                                          const QString &consumerKey, const QString &consumerSecret,
                                          int account)
{
    QVariantMap extra;
    extra.insert(SCREEN_NAME_KEY, screenName);
    extra.insert(RETWEETER_KEY, retweeter);
    extra.insert(CONSUMER_KEY_KEY, consumerKey);
    extra.insert(CONSUMER_SECRET_KEY, consumerSecret);
    addPost(identifier, name, body, timestamp, icon, images, extra, account);
}

QString TwitterPostsDatabase::screenName(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(SCREEN_NAME_KEY).toString();
}

QString TwitterPostsDatabase::retweeter(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(RETWEETER_KEY).toString();
}

QString TwitterPostsDatabase::consumerKey(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(CONSUMER_KEY_KEY).toString();
}

QString TwitterPostsDatabase::consumerSecret(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(CONSUMER_SECRET_KEY).toString();
}
