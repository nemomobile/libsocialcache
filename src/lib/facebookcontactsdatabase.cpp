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

#include "facebookcontactsdatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtDebug>

static const char *DB_NAME = "facebook.db";
static const int VERSION = 3;

static const char *PICTURE_FILE_KEY = "pictureFile";
static const char *COVER_FILE_KEY = "coverFile";

struct FacebookContactPrivate
{
    explicit FacebookContactPrivate(const QString &fbFriendId, int accountId,
                                    const QString &pictureUrl, const QString &coverUrl,
                                    const QString &pictureFile, const QString &coverFile);
    QString fbFriendId;
    int accountId;
    QString pictureUrl;
    QString coverUrl;
    QString pictureFile;
    QString coverFile;
};

FacebookContactPrivate::FacebookContactPrivate(const QString &fbFriendId, int accountId,
                                               const QString &pictureUrl, const QString &coverUrl,
                                               const QString &pictureFile, const QString &coverFile)
    : fbFriendId(fbFriendId), accountId(accountId), pictureUrl(pictureUrl)
    , coverUrl(coverUrl), pictureFile(pictureFile), coverFile(coverFile)
{
}

FacebookContact::FacebookContact(const QString &fbFriendId, int accountId,
                                 const QString &pictureUrl, const QString &coverUrl,
                                 const QString &pictureFile, const QString &coverFile)
    : d_ptr(new FacebookContactPrivate(fbFriendId, accountId, pictureUrl, coverUrl,
                                       pictureFile, coverFile))
{
}

FacebookContact::~FacebookContact()
{
}

FacebookContact::Ptr FacebookContact::create(const QString &fbFriendId, int accountId,
                                             const QString &pictureUrl, const QString &coverUrl,
                                             const QString &pictureFile, const QString &coverFile)
{
    return FacebookContact::Ptr(new FacebookContact(fbFriendId, accountId, pictureUrl, coverUrl,
                                                    pictureFile, coverFile));
}

QString FacebookContact::fbFriendId() const
{
    Q_D(const FacebookContact);
    return d->fbFriendId;
}

int FacebookContact::accountId() const
{
    Q_D(const FacebookContact);
    return d->accountId;
}

QString FacebookContact::pictureUrl() const
{
    Q_D(const FacebookContact);
    return d->pictureUrl;
}

QString FacebookContact::coverUrl() const
{
    Q_D(const FacebookContact);
    return d->coverUrl;
}

QString FacebookContact::pictureFile() const
{
    Q_D(const FacebookContact);
    return d->pictureFile;
}

QString FacebookContact::coverFile() const
{
    Q_D(const FacebookContact);
    return d->coverFile;
}

class FacebookContactsDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit FacebookContactsDatabasePrivate(FacebookContactsDatabase *q);

    void clearCachedImages(QSqlQuery &query);

    struct {
        QList<int> removeAccounts;
        QStringList removeContacts;
        QList<FacebookContact::ConstPtr> insertContacts;
        QMap<QString, QString> updatePictures;
        QMap<QString, QString> updateCovers;
    } queue;
};

FacebookContactsDatabasePrivate::FacebookContactsDatabasePrivate(FacebookContactsDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
            SocialSyncInterface::dataType(SocialSyncInterface::Contacts),
            QLatin1String(DB_NAME),
            VERSION)
{
}

void FacebookContactsDatabasePrivate::clearCachedImages(QSqlQuery &query)
{
    while (query.next()) {
        QString picture = query.value(0).toString();
        QString cover = query.value(1).toString();

        if (!picture.isEmpty()) {
            QFile pictureFile (picture);
            if (pictureFile.exists()) {
                pictureFile.remove();
            }
        }

        if (!cover.isEmpty()) {
            QFile coverFile (cover);
            if (coverFile.exists()) {
                coverFile.remove();
            }
        }
    }
}

FacebookContactsDatabase::FacebookContactsDatabase()
    : AbstractSocialCacheDatabase(*(new FacebookContactsDatabasePrivate(this)))
{
}

FacebookContactsDatabase::~FacebookContactsDatabase()
{
    wait();
}

bool FacebookContactsDatabase::removeContacts(int accountId)
{
    Q_D(FacebookContactsDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.removeAccounts.append(accountId);

    return true;
}

bool FacebookContactsDatabase::removeContacts(const QStringList &fbFriendIds)
{
    Q_D(FacebookContactsDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.removeContacts += fbFriendIds;

    return true;
}

FacebookContact::ConstPtr FacebookContactsDatabase::contact(const QString &fbFriendId,
                                                            int accountId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                                "SELECT fbFriendId, accountId, pictureUrl, coverUrl, "\
                                "pictureFile, coverFile "\
                                "FROM friends "\
                                "WHERE fbFriendId = :fbFriendId "\
                                "AND accountId = :accountId"));
    query.bindValue(":fbFriendId", fbFriendId);
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query contacts" << query.lastError().text();
        return FacebookContact::ConstPtr();
    }

    if (!query.next()) {
        return FacebookContact::ConstPtr();
    }

    FacebookContact::ConstPtr contact = FacebookContact::create(
                                   query.value(0).toString(), query.value(1).toInt(),
                                   query.value(2).toString(), query.value(3).toString(),
                                   query.value(4).toString(), query.value(5).toString());
    query.finish();

    return contact;
}

QList<FacebookContact::ConstPtr> FacebookContactsDatabase::contacts(int accountId) const
{
    QList<FacebookContact::ConstPtr> data;

    QSqlQuery query = prepare(QStringLiteral(
                                "SELECT fbFriendId, accountId, pictureUrl, coverUrl, "\
                                "pictureFile, coverFile "\
                                "FROM friends "\
                                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query contacts" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookContact::create(query.value(0).toString(), query.value(1).toInt(),
                                            query.value(2).toString(), query.value(3).toString(),
                                            query.value(4).toString(), query.value(5).toString()));
    }

    return data;
}

QStringList FacebookContactsDatabase::contactIds(int accountId) const
{
    QStringList data;

    QSqlQuery query = prepare(QStringLiteral(
                                "SELECT fbFriendId FROM friends "\
                                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query contacts" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(query.value(0).toString());
    }

    return data;
}

void FacebookContactsDatabase::addSyncedContact(const QString &fbFriendId, int accountId,
                                                const QString &pictureUrl, const QString &coverUrl)
{
    Q_D(FacebookContactsDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.insertContacts.append(FacebookContact::create(fbFriendId, accountId, pictureUrl, coverUrl,
                                                     QString(), QString()));
}

void FacebookContactsDatabase::updatePictureFile(const QString &fbFriendId,
                                                 const QString &pictureFile)
{
    Q_D(FacebookContactsDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updatePictures.insert(fbFriendId, pictureFile);
}

void FacebookContactsDatabase::updateCoverFile(const QString &fbFriendId, const QString &coverFile)
{
    Q_D(FacebookContactsDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateCovers.insert(fbFriendId, coverFile);
}

void FacebookContactsDatabase::commit()
{
    executeWrite();
}

bool FacebookContactsDatabase::write()
{
    Q_D(FacebookContactsDatabase);

    QMutexLocker locker(&d->mutex);

    const QList<int> removeAccounts = d->queue.removeAccounts;
    const QStringList removeContacts = d->queue.removeContacts;
    const QList<FacebookContact::ConstPtr> insertContacts = d->queue.insertContacts;
    const QMap<QString, QString> updatePictures = d->queue.updatePictures;
    const QMap<QString, QString> updateCovers = d->queue.updateCovers;

    d->queue.removeAccounts.clear();
    d->queue.removeContacts.clear();
    d->queue.insertContacts.clear();
    d->queue.updatePictures.clear();
    d->queue.updateCovers.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!removeAccounts.isEmpty()) {
        QVariantList accountIds;

        query = prepare(QStringLiteral(
                    "SELECT pictureFile, coverFile "
                    "FROM friends "
                    "WHERE accountId = :accountId"));
        Q_FOREACH (int accountId, removeAccounts) {
            accountIds.append(accountId);

            query.bindValue(QStringLiteral(":accountId"), accountId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached contacts selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM friends "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeContacts.isEmpty()) {
        QVariantList friendIds;

        query = prepare(QStringLiteral(
                    "SELECT pictureFile, coverFile "
                    "FROM friends "
                    "WHERE fbFriendId = :fbFriendId"));
        Q_FOREACH (const QString &friendId, removeContacts) {
            friendIds.append(friendId);

            query.bindValue(QStringLiteral(":fbFriendId"), friendId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached contacts selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM friends "
                    "WHERE fbFriendId = :fbFriendId"));
        query.bindValue(QStringLiteral(":fbFriendId"), friendIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertContacts.isEmpty()) {
        QVariantList friendIds, accountIds;
        QVariantList pictureUrls, coverUrls;
        QVariantList pictureFiles, coverFiles;

        Q_FOREACH (const FacebookContact::ConstPtr &contact, insertContacts) {
            friendIds.append(contact->fbFriendId());
            accountIds.append(contact->accountId());
            pictureUrls.append(contact->pictureUrl());
            coverUrls.append(contact->coverUrl());
            pictureFiles.append(contact->pictureFile());
            coverFiles.append(contact->coverFile());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO friends ("
                    " fbFriendId, accountId, pictureUrl, coverUrl, pictureFile, coverUrl) "
                    "VALUES ("
                    " :fbFriendId, :accountId, :pictureUrl, :coverUrl, :pictureFile, :coverUrl)"));
        query.bindValue(QStringLiteral(":fbFriendId"), friendIds);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":pictureUrl"), pictureUrls);
        query.bindValue(QStringLiteral(":coverUrl"), coverUrls);
        query.bindValue(QStringLiteral(":pictureFile"), pictureFiles);
        query.bindValue(QStringLiteral(":coverFile"), coverFiles);
        executeBatchSocialCacheQuery(query);
    }

    if (!updatePictures.isEmpty()) {
        QVariantList friendIds;
        QVariantList pictureFiles;

        for (QMap<QString, QString>::const_iterator it = updatePictures.begin();
                it != updatePictures.end();
                ++it) {
            friendIds.append(it.key());
            pictureFiles.append(it.value());
        }

        query = prepare(QStringLiteral(
                    "UPDATE friends "
                    "SET pictureFile = :pictureFile "
                    "WHERE fbFriendId = :fbFriendId"));
        query.bindValue(QStringLiteral(":pictureFile"), pictureFiles);
        query.bindValue(QStringLiteral(":fbFriendId"), friendIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!updateCovers.isEmpty()) {
        QVariantList friendIds;
        QVariantList coverFiles;

        for (QMap<QString, QString>::const_iterator it = updateCovers.begin();
                it != updateCovers.end();
                ++it) {
            friendIds.append(it.key());
            coverFiles.append(it.value());
        }

        query = prepare(QStringLiteral(
                    "UPDATE friends "
                    "SET coverFile = :coverFile "
                    "WHERE fbFriendId = :fbFriendId"));
        query.bindValue(QStringLiteral(":coverFile"), coverFiles);
        query.bindValue(QStringLiteral(":fbFriendId"), friendIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool FacebookContactsDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare( "CREATE TABLE IF NOT EXISTS friends ("
                   "fbFriendId TEXT,"
                   "accountId INTEGER,"
                   "pictureUrl TEXT,"
                   "coverUrl TEXT,"
                   "pictureFile TEXT,"
                   "coverFile TEXT,"
                   "CONSTRAINT id PRIMARY KEY (fbFriendId, accountId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create friends table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool FacebookContactsDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);

    query.prepare("DROP TABLE IF EXISTS friends");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete friends table:" << query.lastError().text();
        return false;
    }
    return true;
}
