/*
 * Copyright (C) 2014 Jolla Ltd.
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

#ifndef VKNOTIFICATIONSDATABASE_H
#define VKNOTIFICATIONSDATABASE_H

#include "abstractsocialcachedatabase.h"

#include <QtCore/QSharedPointer>
#include <QStringList>
#include <QDateTime>

class VKNotificationPrivate;
class VKNotification
{
public:
    typedef QSharedPointer<VKNotification> Ptr;
    typedef QSharedPointer<const VKNotification> ConstPtr;

    enum Type {
        Follow,
        FriendRequestAccepted,
        Mention,
        MentionComments,
        WallPost,
        CommentPost,
        CommentPhoto,
        CommentVideo,
        ReplyComment,
        ReplyCommentPhoto,
        ReplyCommentVideo,
        ReplyTopic,
        LikePost,
        LikeComment,
        LikePhoto,
        LikeVideo,
        LikeCommentPhoto,
        LikeCommentVideo,
        LikeCommentTopic,
        CopyPost,
        CopyPhoto,
        CopyVideo
    };


    virtual ~VKNotification();

    static VKNotification::Ptr create(const QString &identifier,
                                      int accountId,
                                      Type type,
                                      const QString &fromId,
                                      const QString &toId,
                                      const QDateTime &createdTime);
    QString identifier() const;
    Type type() const;
    QString from() const;
    QString to() const;
    QDateTime createdTime() const;
    int accountId() const;

protected:
    QScopedPointer<VKNotificationPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(VKNotification)
    explicit VKNotification(const QString &identifier,
                            int accountId,
                            Type type,
                            const QString &fromId,
                            const QString &toId,
                            const QDateTime &createdTime);
};


class VKNotificationsDatabasePrivate;
class VKNotificationsDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT

public:
    explicit VKNotificationsDatabase();
    ~VKNotificationsDatabase();

    void addVKNotification(int accountId,
                           VKNotification::Type type,
                           const QString &fromId,
                           const QString &toId,
                           const QDateTime &createdTime);

    void removeNotifications(int accountId);
    void removeNotification(const QString &notificationId);
    void removeNotifications(const QStringList &notificationIds);

    void sync();

    QList<VKNotification::ConstPtr> notifications();

signals:
    void notificationsChanged();

protected:
    void readFinished();
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(VKNotificationsDatabase)
};

#endif // VKNOTIFICATIONSDATABASE_H
