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

class FacebookPostsWorkerObject: public AbstractWorkerObject
{
    Q_OBJECT
public:
    explicit FacebookPostsWorkerObject();
    void refresh();
private:
    FacebookPostsDatabase m_db;
    bool m_enabled;
};

FacebookPostsWorkerObject::FacebookPostsWorkerObject()
    : AbstractWorkerObject(), m_enabled(false)
{
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
    QList<SocialPost::ConstPtr> eventsData = m_db.events();
    foreach (const SocialPost::ConstPtr &event, eventsData) {
        QMap<int, QVariant> eventMap;
        eventMap.insert(FacebookPostsModel::FacebookId, event->identifier());
        eventMap.insert(FacebookPostsModel::Title, event->title());
        eventMap.insert(FacebookPostsModel::Body, event->body());
        eventMap.insert(FacebookPostsModel::Timestamp, event->timestamp());
        eventMap.insert(FacebookPostsModel::Footer, event->footer());
        eventMap.insert(FacebookPostsModel::Icon, event->icon()->url());

        QVariantList accountsVariant;
        foreach (int account, event->accounts()) {
            accountsVariant.append(account);
        }
        eventMap.insert(FacebookPostsModel::AttachmentName, m_db.attachmentName(event));
        eventMap.insert(FacebookPostsModel::AttachmentCaption, m_db.attachmentCaption(event));
        eventMap.insert(FacebookPostsModel::AttachmentDescription,
                        m_db.attachmentDescription(event));
        eventMap.insert(FacebookPostsModel::AllowLike, m_db.allowLike(event));
        eventMap.insert(FacebookPostsModel::AllowComment, m_db.allowComment(event));
        eventMap.insert(FacebookPostsModel::ClientId, m_db.clientId(event));
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
    roleNames.insert(Title, "title");
    roleNames.insert(Body, "body");
    roleNames.insert(Timestamp, "timestamp");
    roleNames.insert(Footer, "footer");
    roleNames.insert(Icon, "icon");
    roleNames.insert(Images, "images");
    roleNames.insert(AttachmentName, "attachmentName");
    roleNames.insert(AttachmentCaption, "attachmentCaption");
    roleNames.insert(AttachmentDescription, "attachmentDescription");
    roleNames.insert(AllowLike, "allowLike");
    roleNames.insert(AllowComment, "allowComment");
    roleNames.insert(ClientId, "clientId");
    roleNames.insert(Accounts, "accounts");
    return roleNames;
}

#include "facebookpostsmodel.moc"
