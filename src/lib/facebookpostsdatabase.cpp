/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote 
 *     products derived from this software without specific prior written 
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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

void FacebookPostsDatabase::addFacebookEvent(const QString &identifier, const QString &title,
                                             const QString &body, const QDateTime &timestamp,
                                             const QString &footer,
                                             const QMap<int, SocialPostImage::ConstPtr> &images,
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

    addEvent(identifier, title, body, timestamp, footer, images, extra, account);
}

QString FacebookPostsDatabase::attachmentName(const SocialPost::ConstPtr &event)
{
    return event->extra().value(ATTACHMENT_NAME_KEY).toString();
}

QString FacebookPostsDatabase::attachmentCaption(const SocialPost::ConstPtr &event)
{
    return event->extra().value(ATTACHMENT_CAPTION_KEY).toString();
}

QString FacebookPostsDatabase::attachmentDescription(const SocialPost::ConstPtr &event)
{
    return event->extra().value(ATTACHMENT_DESCRIPTION_KEY).toString();
}

bool FacebookPostsDatabase::allowLike(const SocialPost::ConstPtr &event)
{
    return event->extra().value(ALLOW_LIKE_KEY).toBool();
}

bool FacebookPostsDatabase::allowComment(const SocialPost::ConstPtr &event)
{
    return event->extra().value(ALLOW_COMMENT_KEY).toBool();
}

QString FacebookPostsDatabase::clientId(const SocialPost::ConstPtr &event)
{
    return event->extra().value(CLIENT_ID_KEY).toString();
}
