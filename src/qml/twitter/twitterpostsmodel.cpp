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

#include "twitterpostsmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "twitterpostsdatabase.h"
#include <QtCore/QDebug>
#include "postimagehelper_p.h"

class TwitterPostsModelPrivate: public AbstractSocialCacheModelPrivate
{
public:
    explicit TwitterPostsModelPrivate(TwitterPostsModel *q);

    TwitterPostsDatabase database;

private:
    Q_DECLARE_PUBLIC(TwitterPostsModel)
};

TwitterPostsModelPrivate::TwitterPostsModelPrivate(TwitterPostsModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

TwitterPostsModel::TwitterPostsModel(QObject *parent)
    : AbstractSocialCacheModel(*(new TwitterPostsModelPrivate(this)), parent)
{
    Q_D(TwitterPostsModel);

     connect(&d->database, &AbstractSocialPostCacheDatabase::postsChanged,
             this, &TwitterPostsModel::postsChanged);
}

QHash<int, QByteArray> TwitterPostsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(TwitterId, "twitterId");
    roleNames.insert(Name, "name");
    roleNames.insert(ScreenName, "screenName");
    roleNames.insert(Body, "body");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Icon, "icon");
    roleNames.insert(Images, "images");
    roleNames.insert(Retweeter, "retweeter");
    roleNames.insert(ConsumerKey, "consumerKey");
    roleNames.insert(ConsumerSecret, "consumerSecret");
    roleNames.insert(Accounts, "accounts");
    return roleNames;
}

void TwitterPostsModel::refresh()
{
    Q_D(TwitterPostsModel);

    d->database.refresh();
}

void TwitterPostsModel::clear()
{
    Q_D(TwitterPostsModel);

    d->database.removeAllPosts();
    d->database.commit();
}

void TwitterPostsModel::postsChanged()
{
    Q_D(TwitterPostsModel);

    SocialCacheModelData data;
    QList<SocialPost::ConstPtr> postsData = d->database.posts();
    Q_FOREACH (const SocialPost::ConstPtr &post, postsData) {
        QMap<int, QVariant> eventMap;
        eventMap.insert(TwitterPostsModel::TwitterId, post->identifier());
        eventMap.insert(TwitterPostsModel::Name, post->name());
        eventMap.insert(TwitterPostsModel::Body, post->body());
        eventMap.insert(TwitterPostsModel::Timestamp, post->timestamp());
        eventMap.insert(TwitterPostsModel::Icon, post->icon());

        QVariantList images;
        Q_FOREACH (const SocialPostImage::ConstPtr &image, post->images()) {
            images.append(createImageData(image));
        }
        eventMap.insert(TwitterPostsModel::Images, images);

        eventMap.insert(TwitterPostsModel::ScreenName, d->database.screenName(post));
        eventMap.insert(TwitterPostsModel::Retweeter, d->database.retweeter(post));
        eventMap.insert(TwitterPostsModel::ConsumerKey, d->database.consumerKey(post));
        eventMap.insert(TwitterPostsModel::ConsumerSecret, d->database.consumerSecret(post));

        QVariantList accountsVariant;
        Q_FOREACH (int account, post->accounts()) {
            accountsVariant.append(account);
        }
        eventMap.insert(TwitterPostsModel::Accounts, accountsVariant);
        data.append(eventMap);
    }

    updateData(data);
}
