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

#ifndef VKPOSTSDATABASE_H
#define VKPOSTSDATABASE_H

#include "abstractsocialpostcachedatabase.h"

class VKPostsDatabase: public AbstractSocialPostCacheDatabase
{
    Q_OBJECT
public:
    explicit VKPostsDatabase();
    ~VKPostsDatabase();

    class Comments
    {
    public:
        Comments(const Comments &other);
        Comments &operator=(const Comments &other);
        Comments();
        ~Comments();

        int count;
        bool userCanComment;
    };

    class Likes
    {
    public:
        Likes();
        ~Likes();
        Likes(const Likes &other);
        Likes &operator=(const Likes &other);

        int count;
        bool userLikes;
        bool userCanLike;
        bool userCanPublish;
    };

    // has data if this post has been reposted elsewhere
    class Reposts
    {
    public:
        Reposts();
        ~Reposts();
        Reposts(const Reposts &other);
        Reposts &operator=(const Reposts &other);

        int count;
        bool userReposted;
    };

    class PostSource
    {
    public:
        PostSource();
        ~PostSource();
        PostSource(const PostSource &other);
        PostSource &operator=(const PostSource &other);

        QString type;
        QString data;
    };

    class GeoLocation
    {
    public:
        GeoLocation();
        ~GeoLocation();
        GeoLocation(const GeoLocation &other);
        GeoLocation &operator=(const GeoLocation &other);

        int placeId;
        QString title;
        QString type;
        int countryId;
        int cityId;
        QString address;
        bool showMap;
    };

    // has data if this post itself is a repost
    class CopyPost
    {
    public:
        CopyPost();
        ~CopyPost();
        CopyPost(const CopyPost &other);
        CopyPost &operator=(const CopyPost &other);

        QDateTime createdTime;
        QString type;
        int ownerId;
        int postId;
        QString text;
    };

    class Post
    {
    public:
        Post();
        ~Post();

        Post(const Post &other);
        Post &operator=(const Post &other);

        Comments comments;
        Likes likes;
        Reposts reposts;
        PostSource postSource;
        GeoLocation geo;
        CopyPost copyPost;

        int fromId;
        int toId;
        QString postType;
        int replyOwnerId;
        int replyPostId;
        int signerId;
        bool friendsOnly;

        typedef QSharedPointer<Post> Ptr;
        typedef QSharedPointer<const Post> ConstPtr;

        static Post::Ptr create(const SocialPost::ConstPtr &socialPost);
    };

    void addVKPost(const QString &identifier,
                   const QDateTime &createdTime,
                   const QString &body,
                   const Post &post,
                   const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                   const QString &personName,
                   const QString &personIcon,
                   int accountId);

    // TODO need to return a list of post objects from db
};

#endif // VKPOSTSDATABASE_H
