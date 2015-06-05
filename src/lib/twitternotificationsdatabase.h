/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
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

#ifndef TWITTERNOTIFICATIONSDATABASE_H
#define TWITTERNOTIFICATIONSDATABASE_H

#include "abstractsocialcachedatabase.h"

#include <QtCore/QSharedPointer>
#include <QString>
#include <QDateTime>
#include <QSet>

class TwitterNotificationsDatabasePrivate;
class TwitterNotificationsDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT

public:
    explicit TwitterNotificationsDatabase();
    ~TwitterNotificationsDatabase();

    QHash<QString, int> retweetedTweetCounts(int accountId);
    QSet<QString> followerIds(int accountId);

    void setRetweetedTweetCounts(int accountId, const QHash<QString, int> &retweetCounts);
    void setFollowerIds(int accountId, const QSet<QString> &followerIds);
    void sync();

protected:
    void readFinished();
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(TwitterNotificationsDatabase)
};

#endif // TWITTERNOTIFICATIONSDATABASE_H
