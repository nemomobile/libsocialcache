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

class TwitterPostsWorkerObject: public AbstractWorkerObject
{
    Q_OBJECT

public:
    explicit TwitterPostsWorkerObject();
    ~TwitterPostsWorkerObject();

    void refresh();
    void finalCleanup();

private:
    TwitterPostsDatabase m_db;
    bool m_enabled;
};

TwitterPostsWorkerObject::TwitterPostsWorkerObject()
    : AbstractWorkerObject(), m_enabled(false)
{
}

TwitterPostsWorkerObject::~TwitterPostsWorkerObject()
{
}

void TwitterPostsWorkerObject::finalCleanup()
{
    m_db.closeDatabase();
}

void TwitterPostsWorkerObject::refresh()
{
    // We initialize the database when refresh is called
    // When it is called, we are sure that the object is already in a different thread
    if (!m_enabled) {
        m_db.initDatabase();
        m_enabled = true;
    }

    SocialCacheModelData data;
    QList<SocialPost::ConstPtr> postsData = m_db.posts();
    foreach (const SocialPost::ConstPtr &post, postsData) {
        QMap<int, QVariant> eventMap;
        eventMap.insert(TwitterPostsModel::TwitterId, post->identifier());
        eventMap.insert(TwitterPostsModel::Name, post->name());
        eventMap.insert(TwitterPostsModel::Body, post->body());
        eventMap.insert(TwitterPostsModel::Timestamp, post->timestamp());
        eventMap.insert(TwitterPostsModel::Icon, post->icon());

        QVariantList images;
        foreach (const SocialPostImage::ConstPtr &image, post->images()) {
            images.append(createImageData(image));
        }
        eventMap.insert(TwitterPostsModel::Images, images);

        eventMap.insert(TwitterPostsModel::ScreenName, m_db.screenName(post));
        eventMap.insert(TwitterPostsModel::Retweeter, m_db.retweeter(post));
        eventMap.insert(TwitterPostsModel::ConsumerKey, m_db.consumerKey(post));
        eventMap.insert(TwitterPostsModel::ConsumerSecret, m_db.consumerSecret(post));

        QVariantList accountsVariant;
        foreach (int account, post->accounts()) {
            accountsVariant.append(account);
        }
        eventMap.insert(TwitterPostsModel::Accounts, accountsVariant);
        data.append(eventMap);
    }

    emit dataUpdated(data);
}

class TwitterPostsModelPrivate: public AbstractSocialCacheModelPrivate
{
public:
    explicit TwitterPostsModelPrivate(TwitterPostsModel *q);
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
    d->initWorkerObject(new TwitterPostsWorkerObject());
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

#include "twitterpostsmodel.moc"
