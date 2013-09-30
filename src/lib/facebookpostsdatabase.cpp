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

#include "facebookpostsdatabase.h"
#include "socialsyncinterface.h"

static const char *DB_NAME = "facebook.db";
static const char *ATTACHMENT_NAME_KEY = "post_attachment_name";
static const char *ATTACHMENT_CAPTION_KEY = "post_attachment_caption";
static const char *ATTACHMENT_DESCRIPTION_KEY = "post_attachment_description";
static const char *CLIENT_ID_KEY = "client_id";
static const char *ALLOW_LIKE_KEY = "allow_like";
static const char *ALLOW_COMMENT_KEY = "allow_comment";

FacebookPostsDatabase::FacebookPostsDatabase()
    : AbstractSocialPostCacheDatabase()
{
}

void FacebookPostsDatabase::initDatabase()
{
    dbInit(SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
           SocialSyncInterface::dataType(SocialSyncInterface::Posts),
           QLatin1String(DB_NAME), POST_DB_VERSION);
}

void FacebookPostsDatabase::addFacebookPost(const QString &identifier, const QString &name,
                                            const QString &body, const QDateTime &timestamp,
                                            const QString &icon,
                                            const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                                            const QString &attachmentName,
                                            const QString &attachmentCaption,
                                            const QString &attachmentDescription,
                                            bool allowLike, bool allowComment,
                                            const QString &clientId, int account)
{
    QVariantMap extra;
    extra.insert(ATTACHMENT_NAME_KEY, attachmentName);
    extra.insert(ATTACHMENT_CAPTION_KEY, attachmentCaption);
    extra.insert(ATTACHMENT_DESCRIPTION_KEY, attachmentDescription);
    extra.insert(ALLOW_LIKE_KEY, allowLike);
    extra.insert(ALLOW_COMMENT_KEY, allowComment);
    extra.insert(CLIENT_ID_KEY, clientId);

    addPost(identifier, name, body, timestamp, icon, images, extra, account);
}

QString FacebookPostsDatabase::attachmentName(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(ATTACHMENT_NAME_KEY).toString();
}

QString FacebookPostsDatabase::attachmentCaption(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(ATTACHMENT_CAPTION_KEY).toString();
}

QString FacebookPostsDatabase::attachmentDescription(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(ATTACHMENT_DESCRIPTION_KEY).toString();
}

bool FacebookPostsDatabase::allowLike(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return false;
    }
    return post->extra().value(ALLOW_LIKE_KEY).toBool();
}

bool FacebookPostsDatabase::allowComment(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return false;
    }
    return post->extra().value(ALLOW_COMMENT_KEY).toBool();
}

QString FacebookPostsDatabase::clientId(const SocialPost::ConstPtr &post)
{
    if (post.isNull()) {
        return QString();
    }
    return post->extra().value(CLIENT_ID_KEY).toString();
}
