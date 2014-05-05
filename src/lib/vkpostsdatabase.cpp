/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
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

#include "vkpostsdatabase.h"
#include "socialsyncinterface.h"

#include <QtDebug>

static const char *DB_NAME = "vk.db";

static const char *COMMENT_COUNT_KEY = "comment_count";
static const char *COMMENT_ALLOW_KEY = "allow_comment";

static const char *LIKE_COUNT_KEY = "like_count";
static const char *LIKE_BY_USER = "like_by_user";
static const char *LIKE_ALLOW_KEY = "allow_like";
static const char *LIKE_ALLOW_PUBLISH_KEY = "allow_like_publish";

static const char *REPOST_COUNT_KEY = "repost_count";
static const char *REPOST_BY_USER_KEY = "repost_by_user";

static const char *POST_SOURCE_TYPE_KEY = "post_type";
static const char *POST_SOURCE_DATA_KEY = "post_data";

static const char *GEO_PLACE_ID_KEY = "geo_place_id";
static const char *GEO_TITLE_KEY = "geo_title";
static const char *GEO_TYPE_KEY = "geo_type";
static const char *GEO_COUNTRY_ID_KEY = "geo_country_id";
static const char *GEO_CITY_ID_KEY = "geo_city_id";
static const char *GEO_ADDRESS_KEY = "geo_address";
static const char *GEO_SHOWMAP_KEY = "geo_showmap";

static const char *COPIED_POST_CREATED_TIME_KEY = "copied_post_created_time";
static const char *COPIED_POST_TYPE_KEY = "copied_post_type";
static const char *COPIED_POST_OWNER_ID_KEY = "copied_post_owner_id";
static const char *COPIED_POST_POST_ID_KEY = "copied_post_post_id";
static const char *COPIED_POST_TEXT_KEY = "copied_post_text";

static const char *POST_FROM_ID_KEY = "post_from_id";
static const char *POST_TO_ID_KEY = "post_to_id";
static const char *POST_REPLY_OWNER_ID_KEY = "post_reply_owner_id";
static const char *POST_REPLY_POST_ID_KEY = "post_reply_post_id";
static const char *POST_FRIENDS_ONLY_KEY = "post_friends_only";
static const char *POST_SIGNER_ID_KEY = "post_signer_id";

VKPostsDatabase::Comments::Comments() : count(0), userCanComment(false) {}
VKPostsDatabase::Comments::~Comments() {}

VKPostsDatabase::Comments::Comments(const VKPostsDatabase::Comments &other)
{
    operator=(other);
}

VKPostsDatabase::Comments& VKPostsDatabase::Comments::operator=(const VKPostsDatabase::Comments &other)
{
    if (&other == this) {
        return *this;
    }
    count = other.count;
    userCanComment = other.userCanComment;
    return *this;
}

VKPostsDatabase::Likes::Likes() : count(0), userLikes(false), userCanLike(false), userCanPublish(false) {}
VKPostsDatabase::Likes::~Likes() {}

VKPostsDatabase::Likes::Likes(const VKPostsDatabase::Likes &other)
{
    operator=(other);
}

VKPostsDatabase::Likes& VKPostsDatabase::Likes::operator=(const VKPostsDatabase::Likes &other)
{
    if (&other == this) {
        return *this;
    }
    count = other.count;
    userLikes = other.userLikes;
    userCanLike = other.userCanLike;
    userCanPublish = other.userCanPublish;
    return *this;
}

VKPostsDatabase::Reposts::Reposts() : count(0), userReposted(false) {}
VKPostsDatabase::Reposts::~Reposts() {}

VKPostsDatabase::Reposts::Reposts(const VKPostsDatabase::Reposts &other)
{
    operator=(other);
}

VKPostsDatabase::Reposts& VKPostsDatabase::Reposts::operator=(const VKPostsDatabase::Reposts &other)
{
    if (&other == this) {
        return *this;
    }
    count = other.count;
    userReposted = other.userReposted;
    return *this;
}

VKPostsDatabase::PostSource::PostSource() {}
VKPostsDatabase::PostSource::~PostSource() {}

VKPostsDatabase::PostSource::PostSource(const VKPostsDatabase::PostSource &other)
{
    operator=(other);
}

VKPostsDatabase::PostSource& VKPostsDatabase::PostSource::operator=(const VKPostsDatabase::PostSource &other)
{
    if (&other == this) {
        return *this;
    }
    type = other.type;
    data = other.data;
    return *this;
}


VKPostsDatabase::GeoLocation::GeoLocation() : placeId(0), countryId(0), cityId(0), showMap(false) {}
VKPostsDatabase::GeoLocation::~GeoLocation() {}

VKPostsDatabase::GeoLocation::GeoLocation(const VKPostsDatabase::GeoLocation &other)
{
    operator=(other);
}

VKPostsDatabase::GeoLocation& VKPostsDatabase::GeoLocation::operator=(const VKPostsDatabase::GeoLocation &other)
{
    if (&other == this) {
        return *this;
    }
    placeId = other.placeId;
    title = other.title;
    type = other.type;
    countryId = other.countryId;
    cityId = other.cityId;
    address = other.address;
    showMap = other.showMap;
    return *this;
}

VKPostsDatabase::CopyPost::CopyPost() : ownerId(0), postId(0) {}
VKPostsDatabase::CopyPost::~CopyPost() {}

VKPostsDatabase::CopyPost::CopyPost(const VKPostsDatabase::CopyPost &other)
{
    operator=(other);
}

VKPostsDatabase::CopyPost& VKPostsDatabase::CopyPost::operator=(const VKPostsDatabase::CopyPost &other)
{
    if (&other == this) {
        return *this;
    }
    createdTime = other.createdTime;
    type = other.type;
    ownerId = other.ownerId;
    postId = other.postId;
    return *this;
}

VKPostsDatabase::Post::Post() : fromId(0), toId(0), replyOwnerId(0), replyPostId(0), signerId(0), friendsOnly(0) {}
VKPostsDatabase::Post::~Post() {}

VKPostsDatabase::Post::Post(const VKPostsDatabase::Post &other)
{
    operator=(other);
}

VKPostsDatabase::Post& VKPostsDatabase::Post::operator=(const VKPostsDatabase::Post &other)
{
    if (&other == this) {
        return *this;
    }
    comments = other.comments;
    likes = other.likes;
    reposts = other.reposts;
    geo = other.geo;
    copyPost = other.copyPost;
    fromId = other.fromId;
    toId = other.toId;
    postType = other.postType;
    replyOwnerId = other.replyOwnerId;
    replyPostId = other.replyPostId;
    signerId = other.signerId;
    friendsOnly = other.friendsOnly;
    return *this;
}

VKPostsDatabase::VKPostsDatabase()
    : AbstractSocialPostCacheDatabase(
          SocialSyncInterface::socialNetwork(SocialSyncInterface::VK),
          QLatin1String(DB_NAME))
{
}

VKPostsDatabase::~VKPostsDatabase()
{
}

void VKPostsDatabase::addVKPost(const QString &identifier,
                                const QDateTime &createdTime,
                                const QString &body,
                                const Post &post,
                                const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                                const QString &personName,
                                const QString &personIcon,
                                int accountId)
{
    QVariantMap extra;

    extra.insert(POST_FROM_ID_KEY, post.fromId);
    extra.insert(POST_TO_ID_KEY, post.toId);
    extra.insert(POST_REPLY_OWNER_ID_KEY, post.replyOwnerId);
    extra.insert(POST_REPLY_POST_ID_KEY, post.replyPostId);
    extra.insert(POST_FRIENDS_ONLY_KEY, post.friendsOnly);
    extra.insert(POST_SIGNER_ID_KEY, post.signerId);

    extra.insert(COMMENT_COUNT_KEY, post.comments.count);
    extra.insert(COMMENT_ALLOW_KEY, post.comments.userCanComment);

    extra.insert(LIKE_COUNT_KEY, post.likes.count);
    extra.insert(LIKE_BY_USER, post.likes.userLikes);
    extra.insert(LIKE_ALLOW_KEY, post.likes.userCanLike);
    extra.insert(LIKE_ALLOW_PUBLISH_KEY, post.likes.userCanPublish);

    extra.insert(REPOST_COUNT_KEY, post.reposts.count);
    extra.insert(REPOST_BY_USER_KEY, post.reposts.userReposted);

    extra.insert(POST_SOURCE_TYPE_KEY, post.postSource.type);
    extra.insert(POST_SOURCE_DATA_KEY, post.postSource.data);

    extra.insert(GEO_PLACE_ID_KEY, post.geo.placeId);
    extra.insert(GEO_TITLE_KEY, post.geo.title);
    extra.insert(GEO_TYPE_KEY, post.geo.type);
    extra.insert(GEO_COUNTRY_ID_KEY, post.geo.countryId);
    extra.insert(GEO_CITY_ID_KEY, post.geo.cityId);
    extra.insert(GEO_ADDRESS_KEY, post.geo.address);
    extra.insert(GEO_SHOWMAP_KEY, post.geo.showMap);

    extra.insert(COPIED_POST_CREATED_TIME_KEY, post.copyPost.createdTime);
    extra.insert(COPIED_POST_TYPE_KEY, post.copyPost.type);
    extra.insert(COPIED_POST_OWNER_ID_KEY, post.copyPost.ownerId);
    extra.insert(COPIED_POST_POST_ID_KEY, post.copyPost.postId);
    extra.insert(COPIED_POST_TEXT_KEY, post.copyPost.text);

    addPost(identifier,
            personName,
            body,
            createdTime,
            personIcon,
            images,
            extra,
            accountId);
}

VKPostsDatabase::Post::Ptr VKPostsDatabase::Post::create(const SocialPost::ConstPtr &socialPost)
{
    VKPostsDatabase::Post *vkPost = new VKPostsDatabase::Post;

    const QVariantMap &extra = socialPost->extra();

    vkPost->fromId = extra.value(POST_FROM_ID_KEY).toInt();
    vkPost->toId = extra.value(POST_TO_ID_KEY).toInt();
    vkPost->replyOwnerId = extra.value(POST_REPLY_OWNER_ID_KEY).toInt();
    vkPost->replyPostId = extra.value(POST_REPLY_POST_ID_KEY).toInt();
    vkPost->friendsOnly = extra.value(POST_FRIENDS_ONLY_KEY).toBool();
    vkPost->signerId = extra.value(POST_SIGNER_ID_KEY).toInt();

    vkPost->comments.count = extra.value(COMMENT_COUNT_KEY).toInt();
    vkPost->comments.userCanComment = extra.value(COMMENT_ALLOW_KEY).toBool();

    vkPost->likes.count = extra.value(LIKE_COUNT_KEY).toInt();
    vkPost->likes.userLikes = extra.value(LIKE_BY_USER).toBool();
    vkPost->likes.userCanLike = extra.value(LIKE_ALLOW_KEY).toBool();
    vkPost->likes.userCanPublish = extra.value(LIKE_ALLOW_PUBLISH_KEY).toBool();

    vkPost->reposts.count = extra.value(REPOST_COUNT_KEY).toInt();
    vkPost->reposts.userReposted = extra.value(REPOST_BY_USER_KEY).toBool();

    vkPost->postSource.type = extra.value(POST_SOURCE_TYPE_KEY).toString();
    vkPost->postSource.data = extra.value(POST_SOURCE_DATA_KEY).toString();

    vkPost->geo.placeId = extra.value(GEO_PLACE_ID_KEY).toInt();
    vkPost->geo.title = extra.value(GEO_TITLE_KEY).toString();
    vkPost->geo.type = extra.value(GEO_TYPE_KEY).toString();
    vkPost->geo.countryId = extra.value(GEO_COUNTRY_ID_KEY).toInt();
    vkPost->geo.cityId = extra.value(GEO_CITY_ID_KEY).toInt();
    vkPost->geo.address = extra.value(GEO_ADDRESS_KEY).toString();
    vkPost->geo.showMap = extra.value(GEO_SHOWMAP_KEY).toBool();

    vkPost->copyPost.createdTime = extra.value(COPIED_POST_CREATED_TIME_KEY).toDateTime();
    vkPost->copyPost.type = extra.value(COPIED_POST_TYPE_KEY).toString();
    vkPost->copyPost.ownerId = extra.value(COPIED_POST_OWNER_ID_KEY).toInt();
    vkPost->copyPost.postId = extra.value(COPIED_POST_POST_ID_KEY).toInt();

    return VKPostsDatabase::Post::Ptr(vkPost);
}
