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

#include "facebookpostsmodel.h"
#include "abstractsocialcachemodel_p.h"
#include "facebookpostsdatabase.h"
#include <QtCore/QDebug>
#include "postimagehelper_p.h"

class FacebookPostsWorkerObject: public AbstractWorkerObject
{
    Q_OBJECT

public:
    explicit FacebookPostsWorkerObject();
    ~FacebookPostsWorkerObject();

    void refresh();
    void finalCleanup();

private:
    FacebookPostsDatabase m_db;
    bool m_enabled;
};

FacebookPostsWorkerObject::FacebookPostsWorkerObject()
    : AbstractWorkerObject(), m_enabled(false)
{
}

FacebookPostsWorkerObject::~FacebookPostsWorkerObject()
{
}

void FacebookPostsWorkerObject::finalCleanup()
{
    m_db.closeDatabase();
}

void FacebookPostsWorkerObject::refresh()
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
        eventMap.insert(FacebookPostsModel::FacebookId, post->identifier());
        eventMap.insert(FacebookPostsModel::Name, post->name());
        eventMap.insert(FacebookPostsModel::Body, post->body());
        eventMap.insert(FacebookPostsModel::Timestamp, post->timestamp());
        eventMap.insert(FacebookPostsModel::Icon, post->icon());

        QVariantList images;
        foreach (const SocialPostImage::ConstPtr &image, post->images()) {
            images.append(createImageData(image));
        }
        eventMap.insert(FacebookPostsModel::Images, images);

        eventMap.insert(FacebookPostsModel::AttachmentName, m_db.attachmentName(post));
        eventMap.insert(FacebookPostsModel::AttachmentCaption, m_db.attachmentCaption(post));
        eventMap.insert(FacebookPostsModel::AttachmentDescription,
                        m_db.attachmentDescription(post));
        eventMap.insert(FacebookPostsModel::AttachmentUrl, m_db.attachmentUrl(post));
        eventMap.insert(FacebookPostsModel::AllowLike, m_db.allowLike(post));
        eventMap.insert(FacebookPostsModel::AllowComment, m_db.allowComment(post));
        eventMap.insert(FacebookPostsModel::ClientId, m_db.clientId(post));

        QVariantList accountsVariant;
        foreach (int account, post->accounts()) {
            accountsVariant.append(account);
        }
        eventMap.insert(FacebookPostsModel::Accounts, accountsVariant);
        data.append(eventMap);
    }

    emit dataUpdated(data);
}

class FacebookPostsModelPrivate: public AbstractSocialCacheModelPrivate
{
public:
    explicit FacebookPostsModelPrivate(FacebookPostsModel *q);

private:
    Q_DECLARE_PUBLIC(FacebookPostsModel)
};

FacebookPostsModelPrivate::FacebookPostsModelPrivate(FacebookPostsModel *q)
    : AbstractSocialCacheModelPrivate(q)
{
}

FacebookPostsModel::FacebookPostsModel(QObject *parent)
    : AbstractSocialCacheModel(*(new FacebookPostsModelPrivate(this)), parent)
{
    Q_D(FacebookPostsModel);
    d->initWorkerObject(new FacebookPostsWorkerObject());
}

QHash<int, QByteArray> FacebookPostsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(FacebookId, "facebookId");
    roleNames.insert(Name, "name");
    roleNames.insert(Body, "body");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Icon, "icon");
    roleNames.insert(Images, "images");
    roleNames.insert(AttachmentName, "attachmentName");
    roleNames.insert(AttachmentCaption, "attachmentCaption");
    roleNames.insert(AttachmentDescription, "attachmentDescription");
    roleNames.insert(AttachmentUrl, "attachmentUrl");
    roleNames.insert(AllowLike, "allowLike");
    roleNames.insert(AllowComment, "allowComment");
    roleNames.insert(ClientId, "clientId");
    roleNames.insert(Accounts, "accounts");
    return roleNames;
}

#include "facebookpostsmodel.moc"
