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

#include "twitternotificationsdatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlError>
#include <QtCore/QtDebug>

static const char *DB_NAME = "twitterNotifications.db";
static const int VERSION = 1;

class TwitterNotificationsDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit TwitterNotificationsDatabasePrivate(TwitterNotificationsDatabase *q);

    // in the future, we may support more notification information.
    // for now, we just support retweet and follower information.
    QMap<int, QHash<QString, int> > setRetweetedTweetCounts;
    QMap<int, QSet<QString> > setFollowerIds;
    struct {
        QMap<int, QHash<QString, int> > setRetweetedTweetCounts;
        QMap<int, QSet<QString> > setFollowerIds;
    } queue;
};

TwitterNotificationsDatabasePrivate::TwitterNotificationsDatabasePrivate(TwitterNotificationsDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Twitter),
            SocialSyncInterface::dataType(SocialSyncInterface::Notifications),
            QLatin1String(DB_NAME),
            VERSION)
{
}

TwitterNotificationsDatabase::TwitterNotificationsDatabase()
    : AbstractSocialCacheDatabase(*(new TwitterNotificationsDatabasePrivate(this)))
{
}

TwitterNotificationsDatabase::~TwitterNotificationsDatabase()
{
    wait();
}

void TwitterNotificationsDatabase::setFollowerIds(int accountId, const QSet<QString> &followerIds)
{
    Q_D(TwitterNotificationsDatabase);
    QMutexLocker locker(&d->mutex);
    d->setFollowerIds[accountId] = followerIds;
}

void TwitterNotificationsDatabase::setRetweetedTweetCounts(int accountId, const QHash<QString, int> &tweetCounts)
{
    Q_D(TwitterNotificationsDatabase);
    QMutexLocker locker(&d->mutex);
    d->setRetweetedTweetCounts[accountId] = tweetCounts;
}

void TwitterNotificationsDatabase::sync()
{
    Q_D(TwitterNotificationsDatabase);
    {
        // queue changes to write later in write()
        QMutexLocker locker(&d->mutex);
        if (d->setFollowerIds.size()) {
            d->queue.setFollowerIds = d->setFollowerIds;
            d->setFollowerIds.clear();
        }
        if (d->setRetweetedTweetCounts.size()) {
            d->queue.setRetweetedTweetCounts = d->setRetweetedTweetCounts;
            d->setRetweetedTweetCounts.clear();
        }
    }
    executeWrite();
}

QSet<QString> TwitterNotificationsDatabase::followerIds(int accountId)
{
    QSqlQuery query;
    query = prepare(QStringLiteral(
                "SELECT followerId "
                "FROM followerIds "
                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);
    QSet<QString> retn;
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query follower ids" << query.lastError().text();
        return retn;
    }

    while (query.next()) {
        retn.insert(query.value(0).toString());
    }

    return retn;
}

QHash<QString, int> TwitterNotificationsDatabase::retweetedTweetCounts(int accountId)
{
    QSqlQuery query;
    query = prepare(QStringLiteral(
                "SELECT retweetedTweetId, retweetsCount "
                "FROM retweetedTweets "
                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);
    QHash<QString, int> retn;
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query retweeted tweet counts:" << query.lastError().text();
        return retn;
    }

    while (query.next()) {
        retn.insert(query.value(0).toString(), query.value(1).toInt());
    }

    return retn;
}

void TwitterNotificationsDatabase::readFinished()
{
    // emit followersChanged();
}

bool TwitterNotificationsDatabase::write()
{
    Q_D(TwitterNotificationsDatabase);
    // write changes queued during sync()
    QMutexLocker locker(&d->mutex);
    QMap<int, QSet<QString> > setFollowerIds = d->queue.setFollowerIds;
    d->queue.setFollowerIds.clear();
    QMap<int, QHash<QString, int> > setRetweetedTweetCounts = d->queue.setRetweetedTweetCounts;
    d->queue.setRetweetedTweetCounts.clear();
    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!setFollowerIds.isEmpty()) {
        Q_FOREACH (int accountId, setFollowerIds.keys()) {
            // first step: delete all data associated with the account.
            query = prepare(QStringLiteral("DELETE FROM followerIds WHERE accountId = :accountId"));
            query.bindValue(":accountId", accountId);
            executeSocialCacheQuery(query);

            // second step: insert the data set.
            QVariantList followerIdsForQuery;
            QVariantList accountIdsForQuery;
            QSet<QString> followerIdsSet = setFollowerIds[accountId];
            Q_FOREACH (const QString &fid, followerIdsSet.toList()) {
                followerIdsForQuery.append(fid);
                accountIdsForQuery.append(accountId);
            }
            query = prepare(QStringLiteral("INSERT INTO followerIds VALUES (:accountId, :followerId)"));
            query.bindValue(":accountId", accountIdsForQuery);
            query.bindValue(":followerId", followerIdsForQuery);
            executeBatchSocialCacheQuery(query);
        }
    }

    if (!setRetweetedTweetCounts.isEmpty()) {
        Q_FOREACH (int accountId, setRetweetedTweetCounts.keys()) {
            // first step: delete all data associated with the account.
            query = prepare(QStringLiteral("DELETE FROM retweetedTweets WHERE accountId = :accountId"));
            query.bindValue(":accountId", accountId);
            executeSocialCacheQuery(query);

            // second step: insert the data set.
            QVariantList retweetedTweetIdsForQuery;
            QVariantList retweetCountsForQuery;
            QVariantList accountIdsForQuery;
            QList<QString> retweetedTweetIds = setRetweetedTweetCounts[accountId].keys();
            Q_FOREACH (const QString &tweetId, retweetedTweetIds) {
                retweetedTweetIdsForQuery.append(tweetId);
                retweetCountsForQuery.append(setRetweetedTweetCounts[accountId][tweetId]);
                accountIdsForQuery.append(accountId);
            }
            query = prepare(QStringLiteral("INSERT INTO retweetedTweets VALUES (:accountId, :retweetedTweetId, :retweetCount)"));
            query.bindValue(":accountId", accountIdsForQuery);
            query.bindValue(":retweetedTweetId", retweetedTweetIdsForQuery);
            query.bindValue(":retweetCount", retweetCountsForQuery);
            executeBatchSocialCacheQuery(query);
        }
    }

    return success;
}

bool TwitterNotificationsDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    query.prepare("CREATE TABLE IF NOT EXISTS followerIds ("
                  "accountId INTEGER NOT NULL,"
                  "followerId TEXT NOT NULL,"
                  "PRIMARY KEY (accountId, followerId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create followerIds table: " << query.lastError().text();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS retweetedTweets ("
                  "accountId INTEGER NOT NULL,"
                  "retweetedTweetId TEXT NOT NULL,"
                  "retweetsCount INTEGER NOT NULL,"
                  "PRIMARY KEY (accountId, retweetedTweetId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create followerIds table: " << query.lastError().text();
        return false;
    }

    return true;
}

bool TwitterNotificationsDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS followerIds"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete followerIds table: " << query.lastError().text();
        return false;
    }

    if (!query.exec(QStringLiteral("DROP TABLE IF EXISTS retweetedTweets"))) {
        qWarning() << Q_FUNC_INFO << "Unable to delete retweetedTweets table: " << query.lastError().text();
        return false;
    }

    return true;
}
