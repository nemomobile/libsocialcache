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

#include "facebookimagesdatabase.h"
#include "abstractsocialcachedatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QtDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QFile>

static const char *DB_NAME = "facebook.db";
static const int VERSION = 3;

static const char *THUMBNAIL_FILE_KEY = "thumbnailFile";
static const char *IMAGE_FILE_KEY = "imageFile";

struct FacebookUserPrivate
{
    explicit FacebookUserPrivate(const QString &fbUserId, const QDateTime &updatedTime,
                                 const QString &userName, int count = -1);
    QString fbUserId;
    QDateTime updatedTime;
    QString userName;
    int count;
};

FacebookUserPrivate::FacebookUserPrivate(const QString &fbUserId, const QDateTime &updatedTime,
                                         const QString &userName, int count)
    : fbUserId(fbUserId), updatedTime(updatedTime), userName(userName), count(count)
{

}

FacebookUser::FacebookUser(const QString &fbUserId, const QDateTime &updatedTime,
                           const QString &userName, int count)
    : d_ptr(new FacebookUserPrivate(fbUserId, updatedTime, userName, count))
{
}

FacebookUser::~FacebookUser()
{
}

FacebookUser::Ptr FacebookUser::create(const QString &fbUserId, const QDateTime &updatedTime,
                                       const QString &userName, int count)
{
    return FacebookUser::Ptr(new FacebookUser(fbUserId, updatedTime, userName, count));
}

QString FacebookUser::fbUserId() const
{
    Q_D(const FacebookUser);
    return d->fbUserId;
}

QDateTime FacebookUser::updatedTime() const
{
    Q_D(const FacebookUser);
    return d->updatedTime;
}

QString FacebookUser::userName() const
{
    Q_D(const FacebookUser);
    return d->userName;
}

int FacebookUser::count() const
{
    Q_D(const FacebookUser);
    return d->count;
}

struct FacebookAlbumPrivate
{
    explicit FacebookAlbumPrivate(const QString &fbAlbumId, const QString &fbUserId,
                                  const QDateTime &createdTime, const QDateTime &updatedTime,
                                  const QString &albumName, int imageCount);
    QString fbAlbumId;
    QString fbUserId;
    QDateTime createdTime;
    QDateTime updatedTime;
    QString albumName;
    int imageCount;
};

FacebookAlbumPrivate::FacebookAlbumPrivate(const QString &fbAlbumId, const QString &fbUserId,
                                           const QDateTime &createdTime, const QDateTime &updatedTime,
                                           const QString &albumName, int imageCount)
    : fbAlbumId(fbAlbumId), fbUserId(fbUserId), createdTime(createdTime)
    , updatedTime(updatedTime), albumName(albumName), imageCount(imageCount)
{

}

FacebookAlbum::FacebookAlbum(const QString &fbAlbumId, const QString &fbUserId,
                             const QDateTime &createdTime, const QDateTime &updatedTime,
                             const QString &albumName, int imageCount)
    : d_ptr(new FacebookAlbumPrivate(fbAlbumId, fbUserId, createdTime, updatedTime,
                                     albumName, imageCount))
{
}

FacebookAlbum::~FacebookAlbum()
{
}

FacebookAlbum::Ptr FacebookAlbum::create(const QString &fbAlbumId, const QString &fbUserId,
                                         const QDateTime &createdTime, const QDateTime &updatedTime,
                                         const QString &albumName, int imageCount)
{
    return FacebookAlbum::Ptr(new FacebookAlbum(fbAlbumId, fbUserId, createdTime, updatedTime,
                                                albumName, imageCount));
}

QString FacebookAlbum::fbAlbumId() const
{
    Q_D(const FacebookAlbum);
    return d->fbAlbumId;
}

QString FacebookAlbum::fbUserId() const
{
    Q_D(const FacebookAlbum);
    return d->fbUserId;
}

QDateTime FacebookAlbum::createdTime() const
{
    Q_D(const FacebookAlbum);
    return d->createdTime;
}

QDateTime FacebookAlbum::updatedTime() const
{
    Q_D(const FacebookAlbum);
    return d->updatedTime;
}

QString FacebookAlbum::albumName() const
{
    Q_D(const FacebookAlbum);
    return d->albumName;
}

int FacebookAlbum::imageCount() const
{
    Q_D(const FacebookAlbum);
    return d->imageCount;
}

struct FacebookImagePrivate
{
    explicit FacebookImagePrivate(const QString &fbImageId, const QString &fbAlbumId,
                                  const QString &fbUserId, const QDateTime &createdTime,
                                  const QDateTime &updatedTime, const QString &imageName,
                                  int width, int height, const QString &thumbnailUrl,
                                  const QString &imageUrl, const QString &thumbnailFile,
                                  const QString &imageFile, int account = -1);
    QString fbImageId;
    QString fbAlbumId;
    QString fbUserId;
    QDateTime createdTime;
    QDateTime updatedTime;
    QString imageName;
    int width;
    int height;
    QString thumbnailUrl;
    QString imageUrl;
    QString thumbnailFile;
    QString imageFile;
    int account;
};

FacebookImagePrivate::FacebookImagePrivate(const QString &fbImageId, const QString &fbAlbumId,
                                           const QString &fbUserId, const QDateTime &createdTime,
                                           const QDateTime &updatedTime, const QString &imageName,
                                           int width, int height, const QString &thumbnailUrl,
                                           const QString &imageUrl, const QString &thumbnailFile,
                                           const QString &imageFile, int account)
    : fbImageId(fbImageId), fbAlbumId(fbAlbumId), fbUserId(fbUserId)
    , createdTime(createdTime), updatedTime(updatedTime), imageName(imageName)
    , width(width), height(height), thumbnailUrl(thumbnailUrl)
    , imageUrl(imageUrl), thumbnailFile(thumbnailFile), imageFile(imageFile), account(account)
{
}

FacebookImage::FacebookImage(const QString &fbImageId, const QString &fbAlbumId,
                             const QString &fbUserId, const QDateTime &createdTime,
                             const QDateTime &updatedTime, const QString &imageName,
                             int width, int height, const QString &thumbnailUrl,
                             const QString &imageUrl, const QString &thumbnailFile,
                             const QString &imageFile, int account)
    : d_ptr(new FacebookImagePrivate(fbImageId, fbAlbumId, fbUserId, createdTime,
                                     updatedTime, imageName, width, height,
                                     thumbnailUrl, imageUrl, thumbnailFile,
                                     imageFile, account))
{
}

FacebookImage::~FacebookImage()
{
}

FacebookImage::Ptr FacebookImage::create(const QString &fbImageId, const QString &fbAlbumId,
                                         const QString &fbUserId, const QDateTime &createdTime,
                                         const QDateTime &updatedTime, const QString &imageName,
                                         int width, int height, const QString &thumbnailUrl,
                                         const QString &imageUrl, const QString &thumbnailFile,
                                         const QString &imageFile, int account)
{
    return FacebookImage::Ptr(new FacebookImage(fbImageId, fbAlbumId, fbUserId, createdTime,
                                                updatedTime, imageName, width, height,
                                                thumbnailUrl, imageUrl, thumbnailFile,
                                                imageFile, account));
}

QString FacebookImage::fbImageId() const
{
    Q_D(const FacebookImage);
    return d->fbImageId;
}

QString FacebookImage::fbAlbumId() const
{
    Q_D(const FacebookImage);
    return d->fbAlbumId;
}

QString FacebookImage::fbUserId() const
{
    Q_D(const FacebookImage);
    return d->fbUserId;
}

QDateTime FacebookImage::createdTime() const
{
    Q_D(const FacebookImage);
    return d->createdTime;
}

QDateTime FacebookImage::updatedTime() const
{
    Q_D(const FacebookImage);
    return d->updatedTime;
}

QString FacebookImage::imageName() const
{
    Q_D(const FacebookImage);
    return d->imageName;
}

int FacebookImage::width() const
{
    Q_D(const FacebookImage);
    return d->width;
}

int FacebookImage::height() const
{
    Q_D(const FacebookImage);
    return d->height;
}

QString FacebookImage::thumbnailUrl() const
{
    Q_D(const FacebookImage);
    return d->thumbnailUrl;
}

QString FacebookImage::imageUrl() const
{
    Q_D(const FacebookImage);
    return d->imageUrl;
}

QString FacebookImage::thumbnailFile() const
{
    Q_D(const FacebookImage);
    return d->thumbnailFile;
}

QString FacebookImage::imageFile() const
{
    Q_D(const FacebookImage);
    return d->imageFile;
}

int FacebookImage::account() const
{
    Q_D(const FacebookImage);
    return d->account;
}

class FacebookImagesDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit FacebookImagesDatabasePrivate(FacebookImagesDatabase *q);
private:
    Q_DECLARE_PUBLIC(FacebookImagesDatabase)
    static void createUsersEntries(const QMap<QString, FacebookUser::ConstPtr> &users,
                                   QStringList &keys,
                                   QMap<QString, QVariantList> &entries);
    static void createAlbumsEntries(const QMap<QString, FacebookAlbum::ConstPtr> &albums,
                                    QStringList &keys,
                                    QMap<QString, QVariantList> &entries);
    static void createImagesEntries(const QMap<QString, FacebookImage::ConstPtr> &images,
                                    QStringList &keys,
                                    QMap<QString, QVariantList> &entries);
    static void createUpdatedEntries(const QMap<QString, QMap<QString, QVariant> > &input,
                                     const QString &primary,
                                     QMap<QString, QVariantList> &entries);
    static void clearCachedImages(QSqlQuery &query);
    QList<FacebookImage::ConstPtr> queryImages(const QString &fbUserId, const QString &fbAlbumId);
    QMap<QString, FacebookUser::ConstPtr> queuedUsers;
    QMap<QString, FacebookAlbum::ConstPtr> queuedAlbums;
    QMap<QString, FacebookImage::ConstPtr> queuedImages;

    QMap<QString, QMap<QString, QVariant> > queuedUpdatedUsers;
    QMap<QString, QMap<QString, QVariant> > queuedUpdatedImages;
};

FacebookImagesDatabasePrivate::FacebookImagesDatabasePrivate(FacebookImagesDatabase *q)
    : AbstractSocialCacheDatabasePrivate(q)
{
}

void FacebookImagesDatabasePrivate::createUsersEntries(const QMap<QString, FacebookUser::ConstPtr> &users,
                                                       QStringList &keys,
                                                       QMap<QString, QVariantList> &entries)
{
    keys.clear();
    keys << QLatin1String("fbUserId") << QLatin1String("updatedTime")
         << QLatin1String("userName");

    entries.clear();

    foreach (const FacebookUser::ConstPtr &user, users) {
        entries[QLatin1String("fbUserId")].append(user->fbUserId());
        entries[QLatin1String("updatedTime")].append(user->updatedTime().toTime_t());
        entries[QLatin1String("userName")].append(user->userName());
    }
}

void FacebookImagesDatabasePrivate::createAlbumsEntries(const QMap<QString, FacebookAlbum::ConstPtr> &albums,
                                                        QStringList &keys,
                                                        QMap<QString, QVariantList> &entries)
{
    keys.clear();
    keys << QLatin1String("fbAlbumId") << QLatin1String("fbUserId")
         << QLatin1String("createdTime") << QLatin1String("updatedTime")
         << QLatin1String("albumName") << QLatin1String("imageCount");
    entries.clear();

    foreach (const FacebookAlbum::ConstPtr &album, albums) {
        entries[QLatin1String("fbAlbumId")].append(album->fbAlbumId());
        entries[QLatin1String("fbUserId")].append(album->fbUserId());
        entries[QLatin1String("createdTime")].append(album->createdTime().toTime_t());
        entries[QLatin1String("updatedTime")].append(album->updatedTime().toTime_t());
        entries[QLatin1String("albumName")].append(album->albumName());
        entries[QLatin1String("imageCount")].append(album->imageCount());
    }
}

void FacebookImagesDatabasePrivate::createImagesEntries(const QMap<QString, FacebookImage::ConstPtr> &images,
                                                        QStringList &keys,
                                                        QMap<QString, QVariantList> &entries)
{
    keys.clear();
    keys << QLatin1String("fbImageId") << QLatin1String("fbAlbumId") << QLatin1String("fbUserId")
         << QLatin1String("createdTime") << QLatin1String("updatedTime")
         << QLatin1String("imageName") << QLatin1String("width") << QLatin1String("height")
         << QLatin1String("thumbnailUrl") << QLatin1String("imageUrl")
         << QLatin1String("thumbnailFile") << QLatin1String("imageFile");

    entries.clear();

    foreach (const FacebookImage::ConstPtr &image, images) {
        entries[QLatin1String("fbImageId")].append(image->fbImageId());
        entries[QLatin1String("fbAlbumId")].append(image->fbAlbumId());
        entries[QLatin1String("fbUserId")].append(image->fbUserId());
        entries[QLatin1String("createdTime")].append(image->createdTime().toTime_t());
        entries[QLatin1String("updatedTime")].append(image->updatedTime().toTime_t());
        entries[QLatin1String("imageName")].append(image->imageName());
        entries[QLatin1String("width")].append(image->width());
        entries[QLatin1String("height")].append(image->height());
        entries[QLatin1String("thumbnailUrl")].append(image->thumbnailUrl());
        entries[QLatin1String("imageUrl")].append(image->imageUrl());
        entries[QLatin1String("thumbnailFile")].append(image->thumbnailFile());
        entries[QLatin1String("imageFile")].append(image->imageFile());
    }
}

void FacebookImagesDatabasePrivate::createUpdatedEntries(const QMap<QString, QMap<QString, QVariant> > &input,
                                                         const QString &primary,
                                                         QMap<QString, QVariantList> &entries)
{
    entries.clear();

    // Check if we have the same data
    QStringList keys;
    for (QMap<QString, QMap<QString, QVariant> >::const_iterator i = input.begin();
         i != input.end(); i++) {
        const QMap<QString, QVariant> &inputEntry = i.value();
        // TODO: instead of return, we should only insert entries that "works" to
        // the list.
        if (inputEntry.isEmpty()) {
            return;
        }

        if (keys.isEmpty()) {
            keys = inputEntry.keys();
        } else if (inputEntry.keys() != keys) {
            return;
        }
    }

    foreach (const QString &key, keys) {
        entries.insert(key, QVariantList());
    }
    entries.insert(primary, QVariantList());

    for (QMap<QString, QMap<QString, QVariant> >::const_iterator i = input.begin();
         i != input.end(); i++) {
        entries[primary].append(i.key());

        const QMap<QString, QVariant> &inputEntry = i.value();
        foreach (const QString &key, inputEntry.keys()) {
            entries[key].append(inputEntry.value(key));
        }
    }
}

void FacebookImagesDatabasePrivate::clearCachedImages(QSqlQuery &query)
{
    while (query.next()) {
        QString thumb = query.value(0).toString();
        QString image = query.value(1).toString();

        if (!thumb.isEmpty()) {
            QFile thumbFile (thumb);
            if (thumbFile.exists()) {
                thumbFile.remove();
            }
        }

        if (!image.isEmpty()) {
            QFile imageFile (image);
            if (imageFile.exists()) {
                imageFile.remove();
            }
        }
    }

}

QList<FacebookImage::ConstPtr> FacebookImagesDatabasePrivate::queryImages(const QString &fbUserId,
                                                                          const QString &fbAlbumId)
{
    QList<FacebookImage::ConstPtr> data;

    if (!fbUserId.isEmpty() && !fbAlbumId.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Cannot select images in both an album and for an user";
        return data;
    }

    QString queryString = QLatin1String("SELECT images.fbImageId, images.fbAlbumId, "\
                                        "images.fbUserId, images.createdTime, "\
                                        "images.updatedTime, images.imageName, images.width, "\
                                        "images.height, images.thumbnailUrl, images.imageUrl, "\
                                        "images.thumbnailFile, images.imageFile, "\
                                        "accounts.accountId "\
                                        "FROM images "\
                                        "INNER JOIN accounts "\
                                        "ON accounts.fbUserId = images.fbUserId%1 "\
                                        "ORDER BY images.updatedTime %2");



    if (!fbUserId.isEmpty()) {
        queryString = queryString.arg(QLatin1String(" WHERE images.fbUserId = :fbUserId"),
                                      QLatin1String("DESC"));
    } else if (!fbAlbumId.isEmpty()){
        queryString = queryString.arg(QLatin1String(" WHERE images.fbAlbumId = :fbAlbumId"),
                                      QString());
    } else {
        queryString = queryString.arg(QString(), QLatin1String("DESC"));
    }

    QSqlQuery query (db);
    query.prepare(queryString);
    if (!fbUserId.isEmpty()) {
        query.bindValue(":fbUserId", fbUserId);
    }
    if (!fbAlbumId.isEmpty()) {
        query.bindValue(":fbAlbumId", fbAlbumId);
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query all albums:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookImage::create(query.value(0).toString(), query.value(1).toString(),
                                          query.value(2).toString(),
                                          QDateTime::fromTime_t(query.value(3).toUInt()),
                                          QDateTime::fromTime_t(query.value(4).toUInt()),
                                          query.value(5).toString(),
                                          query.value(6).toInt(), query.value(7).toInt(),
                                          query.value(8).toString(), query.value(9).toString(),
                                          query.value(10).toString(), query.value(11).toString(),
                                          query.value(12).toInt()));
    }

    return data;
}

bool operator==(const FacebookUser::ConstPtr &user1, const FacebookUser::ConstPtr &user2)
{
    return user1->fbUserId() == user2->fbUserId();
}

bool operator==(const FacebookAlbum::ConstPtr &album1, const FacebookAlbum::ConstPtr &album2)
{
    return album1->fbAlbumId() == album2->fbAlbumId() && album1->fbUserId() == album2->fbUserId();
}

bool operator==(const FacebookImage::ConstPtr &image1, const FacebookImage::ConstPtr &image2)
{
    return image1->fbImageId() == image2->fbImageId() && image1->fbAlbumId() == image2->fbAlbumId()
            && image1->fbUserId() == image2->fbUserId();
}

// Note
//
// Insertion operations needs to use write(), while Delete
// operations are automatically using transactions and
// don't need write().

FacebookImagesDatabase::FacebookImagesDatabase()
    : AbstractSocialCacheDatabase(*(new FacebookImagesDatabasePrivate(this)))
{
}

FacebookImagesDatabase::~FacebookImagesDatabase()
{
}

// Call dbInit, but can be used to delay database initialization
void FacebookImagesDatabase::initDatabase()
{
    dbInit(SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
           SocialSyncInterface::dataType(SocialSyncInterface::Images),
           QLatin1String(DB_NAME), VERSION);
}

bool FacebookImagesDatabase::syncAccount(int accountId, const QString &fbUserId)
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return false;
    }
    QSqlQuery query (d->db);
    query.prepare("INSERT OR REPLACE INTO accounts (accountId, fbUserId) "\
                  "VALUES (:accountId, :fbUserId)");
    query.bindValue(":accountId", accountId);
    query.bindValue(":fbUserId", fbUserId);
    bool ok = query.exec();
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "Error writing accounts:" << query.lastError();
        dbRollbackTransaction();
        return false;
    }

    if (!dbCommitTransaction()) {
        dbRollbackTransaction();
        return false;
    }

    return ok;
}

void FacebookImagesDatabase::purgeAccount(int accountId)
{
    Q_D(FacebookImagesDatabase);
    // We will kill all data linked to an an account id.
    // If it kills data that should not be killed, another
    // sync will bring them back.

    if (!dbBeginTransaction()) {
        return;
    }

    // We first search for all users that have the given account id
    QSqlQuery query (d->db);
    query.prepare("SELECT fbUserId FROM accounts WHERE accountId = :accountId");
    query.bindValue(":accountId", accountId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query account" << accountId;
        return;
    }

    QVariantList userIds;
    while (query.next()) {
        userIds.append(query.value(0).toString());
    }

    // Clean accounts for a given user
    query.prepare("DELETE FROM accounts WHERE accountId = :accountId");
    query.bindValue(":accountId", accountId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean account" << accountId;
    }

    // Clean users
    query.prepare("DELETE FROM users WHERE fbUserId = ?");
    query.addBindValue(userIds);
    if (!query.execBatch()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean users for account" << accountId;
    }

    // Clean albums
    query.prepare("DELETE FROM albums WHERE fbUserId = ?");
    query.addBindValue(userIds);
    if (!query.execBatch()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean albums for account" << accountId;
    }

    // Clean images
    foreach (const QVariant &userId, userIds) {
        query.prepare("SELECT thumbnailFile, imageFile FROM images WHERE fbUserId = :fbUserId");
        query.bindValue(":fbUserId", userId);
        d->clearCachedImages(query);
    }

    query.prepare("DELETE FROM images WHERE fbUserId = ?");
    query.addBindValue(userIds);
    if (!query.execBatch()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean images for account" << accountId;
    }

    if (!dbCommitTransaction()) {
        return;
    }
}

// Returns the user but do not return a count
// if you want a count, better consider users
FacebookUser::ConstPtr FacebookImagesDatabase::user(const QString &fbUserId) const
{
    Q_D(const FacebookImagesDatabase);
    QSqlQuery query(d->db);
    query.prepare("SELECT fbUserId, updatedTime, userName "\
                  "FROM users WHERE fbUserId = :fbUserId");
    query.bindValue(":fbUserId", fbUserId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from users table:" << query.lastError();
        return FacebookUser::Ptr();
    }

    // If we have the user, we have a result, otherwise we won't have the user
    if (!query.next()) {
        return FacebookUser::Ptr();
    }

    return FacebookUser::create(query.value(0).toString(),
                                QDateTime::fromTime_t(query.value(1).toUInt()),
                                query.value(2).toString());
}

void FacebookImagesDatabase::addUser(const QString &fbUserId, const QDateTime &updatedTime,
                                     const QString &userName)
{
    Q_D(FacebookImagesDatabase);
    FacebookUser::Ptr user = FacebookUser::create(fbUserId, updatedTime, userName);
    if (!d->queuedUsers.contains(fbUserId)) {
        d->queuedUsers.insert(fbUserId, user);
    }
}

void FacebookImagesDatabase::removeUser(const QString &fbUserId)
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return;
    }

    // Clean accounts
    QSqlQuery query (d->db);
    query.prepare("DELETE FROM accounts WHERE fbUserId = :fbUserId");
    query.bindValue(":fbUserId", fbUserId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean accounts for user" << fbUserId;
        dbRollbackTransaction();
        return;
    }

    // Clean users
    query.prepare("DELETE FROM users WHERE fbUserId = :fbUserId");
    query.bindValue(":fbUserId", fbUserId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean users for user" << fbUserId;
        dbRollbackTransaction();
        return;
    }

    // Clean albums
    query.prepare("DELETE FROM albums WHERE fbUserId = :fbUserId");
    query.bindValue(":fbUserId", fbUserId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean albums for user" << fbUserId;
        dbRollbackTransaction();
        return ;
    }

    // Clean images
    query.prepare("SELECT thumbnailFile, imageFile FROM images WHERE fbUserId = :fbUserId");
    query.bindValue(":fbUserId", fbUserId);
    d->clearCachedImages(query);

    query.prepare("DELETE FROM images WHERE fbUserId = :fbUserId");
    query.bindValue(":fbUserId", fbUserId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean images for user" << fbUserId;
        dbRollbackTransaction();
        return;
    }

    if (!dbCommitTransaction()) {
        dbRollbackTransaction();
        return;
    }
}

QList<FacebookUser::ConstPtr> FacebookImagesDatabase::users() const
{
    Q_D(const FacebookImagesDatabase);
    QList<FacebookUser::ConstPtr> data;

    QSqlQuery query (d->db);
    query.prepare("SELECT users.fbUserId, users.updatedTime, users.userName, "\
                  "COUNT(fbImageId) as count "\
                  "FROM users "\
                  "LEFT JOIN images ON images.fbUserId = users.fbUserId "\
                  "GROUP BY users.fbUserId "\
                  "ORDER BY users.fbUserId");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query all users:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookUser::create(query.value(0).toString(),
                                         QDateTime::fromTime_t(query.value(1).toUInt()),
                                         query.value(2).toString(), query.value(3).toInt()));
    }

    return data;
}

QStringList FacebookImagesDatabase::allAlbumIds(bool *ok) const
{
    Q_D(const FacebookImagesDatabase);
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query(d->db);
    query.prepare("SELECT DISTINCT fbAlbumId FROM albums ORDER BY updatedTime DESC");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to fetch all albums" << query.lastError().text();
        return ids;
    }

    while (query.next()) {
        ids.append(query.value(0).toString());
    }

    if (ok) {
        *ok = true;
    }

    return ids;
}

FacebookAlbum::ConstPtr FacebookImagesDatabase::album(const QString &fbAlbumId) const
{
    Q_D(const FacebookImagesDatabase);
    QSqlQuery query(d->db);
    query.prepare("SELECT fbAlbumId, fbUserId, createdTime, updatedTime, albumName, "\
                  "imageCount, coverImageId, thumbnailFile "\
                  "FROM albums WHERE fbAlbumId = :fbAlbumId");
    query.bindValue(":fbAlbumId", fbAlbumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return FacebookAlbum::Ptr();
    }

    // If we have the album, we have a result, otherwise we won't have the album
    if (!query.next()) {
        return FacebookAlbum::Ptr();
    }

    return FacebookAlbum::create(query.value(0).toString(),  query.value(1).toString(),
                                 QDateTime::fromTime_t(query.value(2).toUInt()),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 query.value(4).toString(), query.value(5).toInt());
}

void FacebookImagesDatabase::addAlbum(const QString &fbAlbumId, const QString &fbUserId,
                                      const QDateTime &createdTime, const QDateTime &updatedTime,
                                      const QString &albumName, int imageCount)
{
    Q_D(FacebookImagesDatabase);
    FacebookAlbum::Ptr album = FacebookAlbum::create(fbAlbumId, fbUserId, createdTime,
                                                     updatedTime, albumName, imageCount);
    if (!d->queuedAlbums.contains(fbAlbumId)) {
        d->queuedAlbums.insert(fbAlbumId, album);
    }
}

void FacebookImagesDatabase::removeAlbum(const QString &fbAlbumId)
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return;
    }

    // Clean albums
    QSqlQuery query (d->db);
    query.prepare("DELETE FROM albums WHERE fbAlbumId = :fbAlbumId");
    query.bindValue(":fbAlbumId", fbAlbumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean albums for album" << fbAlbumId;
        dbRollbackTransaction();
        return;
    }

    // Clean images
    query.prepare("SELECT thumbnailFile, imageFile FROM images WHERE fbAlbumId = :fbAlbumId");
    query.bindValue(":fbAlbumId", fbAlbumId);
    d->clearCachedImages(query);

    query.prepare("DELETE FROM images WHERE fbAlbumId = :fbAlbumId");
    query.bindValue(":fbAlbumId", fbAlbumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean images for album" << fbAlbumId;
        dbRollbackTransaction();
        return;
    }

    if (!dbCommitTransaction()) {
        return;
    }
}

void FacebookImagesDatabase::removeAlbums(const QStringList &fbAlbumIds)
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return;
    }

    // Clean albums
    QSqlQuery query (d->db);
    QVariantList fbAlbumIdList;
    foreach (const QString &fbAlbumId, fbAlbumIds) {
        fbAlbumIdList.append(fbAlbumId);
    }
    QMap<QString, QVariantList> entries;
    entries.insert(QLatin1String("fbAlbumId"), fbAlbumIdList);
    QStringList keys;
    keys.append(QLatin1String("fbAlbumId"));
    if (!dbWrite(QLatin1String("albums"), keys, entries, Delete)) {
        qWarning() << Q_FUNC_INFO << "Failed to clean albums" << fbAlbumIds;
        dbRollbackTransaction();
        return;
    }

    // Clean images
    foreach (const QString &fbAlbumId, fbAlbumIds) {
        query.prepare("SELECT thumbnailFile, imageFile FROM images WHERE fbAlbumId = :fbAlbumId");
        query.bindValue(":fbAlbumId", fbAlbumId);
        d->clearCachedImages(query);
    }

    if (!dbWrite(QLatin1String("images"), keys, entries, Delete)) {
        qWarning() << Q_FUNC_INFO << "Failed to clean images for albums" << fbAlbumIds;
        dbRollbackTransaction();
        return;
    }

    if (!dbCommitTransaction()) {
        return;
    }
}

QList<FacebookAlbum::ConstPtr> FacebookImagesDatabase::albums(const QString &fbUserId)
{
    Q_D(const FacebookImagesDatabase);
    QList<FacebookAlbum::ConstPtr> data;

    QString queryString = QLatin1String("SELECT fbAlbumId, fbUserId, createdTime, updatedTime, "\
                                        "albumName, imageCount "\
                                        "FROM albums%1 ORDER BY updatedTime DESC");
    if (!fbUserId.isEmpty()) {
        queryString = queryString.arg(QLatin1String(" WHERE fbUserId = :fbUserId"));
    } else {
        queryString = queryString.arg(QString());
    }

    QSqlQuery query (d->db);
    query.prepare(queryString);
    if (!fbUserId.isEmpty()) {
        query.bindValue(":fbUserId", fbUserId);
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query all albums:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookAlbum::create(query.value(0).toString(), query.value(1).toString(),
                                          QDateTime::fromTime_t(query.value(2).toUInt()),
                                          QDateTime::fromTime_t(query.value(3).toUInt()),
                                          query.value(4).toString(), query.value(5).toInt()));
    }

    return data;
}

QStringList FacebookImagesDatabase::allImageIds(bool *ok) const
{
    Q_D(const FacebookImagesDatabase);
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query(d->db);
    query.prepare("SELECT DISTINCT fbImageId FROM images ORDER BY updatedTime DESC");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to fetch all images" << query.lastError().text();
        return ids;
    }

    while (query.next()) {
        ids.append(query.value(0).toString());
    }

    if (ok) {
        *ok = true;
    }

    return ids;
}

QStringList FacebookImagesDatabase::imageIds(const QString &fbAlbumId, bool *ok) const
{
    Q_D(const FacebookImagesDatabase);
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query(d->db);
    query.prepare("SELECT DISTINCT fbImageId FROM images WHERE fbAlbumId = :fbAlbumId");
    query.bindValue(":fbAlbumId", fbAlbumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to fetch images for album" << fbAlbumId
                   << query.lastError().text();
        return ids;
    }

    while (query.next()) {
        ids.append(query.value(0).toString());
    }

    if (ok) {
        *ok = true;
    }

    return ids;
}

FacebookImage::ConstPtr FacebookImagesDatabase::image(const QString &fbImageId) const
{
    Q_D(const FacebookImagesDatabase);
    QSqlQuery query(d->db);
    query.prepare("SELECT fbImageId, fbAlbumId, fbUserId, createdTime, updatedTime, imageName, "\
                  "width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile "\
                  "FROM images WHERE fbImageId = :fbImageId");
    query.bindValue(":fbImageId", fbImageId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return FacebookImage::Ptr();
    }

    // If we have the image, we have a result, otherwise we won't have the image
    if (!query.next()) {
        return FacebookImage::Ptr();
    }

    return FacebookImage::create(query.value(0).toString(),  query.value(1).toString(),
                                 query.value(2).toString(),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 QDateTime::fromTime_t(query.value(4).toUInt()),
                                 query.value(5).toString(),
                                 query.value(6).toInt(), query.value(7).toInt(),
                                 query.value(8).toString(), query.value(9).toString(),
                                 query.value(10).toString(), query.value(11).toString());
}

void FacebookImagesDatabase::removeImage(const QString &fbImageId)
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return;
    }
    // Clean images
    QSqlQuery query (d->db);

    query.prepare("SELECT thumbnailFile, imageFile FROM images WHERE fbImageId = :fbImageId");
    query.bindValue(":fbAlbumId", fbImageId);
    d->clearCachedImages(query);

    query.prepare("DELETE FROM images WHERE fbImageId = :fbImageId");
    query.bindValue(":fbImageId", fbImageId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to clean images for image" << fbImageId;
        dbRollbackTransaction();
        return;
    }

    if (!dbCommitTransaction()) {
        dbRollbackTransaction();
        return;
    }
}

void FacebookImagesDatabase::removeImages(const QStringList &fbImageIds)
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return;
    }
    // Clean images
    QSqlQuery query (d->db);

    foreach (const QString &fbImageId, fbImageIds) {
        query.prepare("SELECT thumbnailFile, imageFile FROM images WHERE fbImageId = :fbImageId");
        query.bindValue(":fbAlbumId", fbImageId);
        d->clearCachedImages(query);
    }

    QVariantList fbImageIdList;
    foreach (const QString &fbImageId, fbImageIds) {
        fbImageIdList.append(fbImageId);
    }
    QMap<QString, QVariantList> entries;
    entries.insert(QLatin1String("fbImageId"), fbImageIdList);
    QStringList keys;
    keys.append(QLatin1String("fbImageId"));
    if (!dbWrite(QLatin1String("images"), keys, entries, Delete)) {
        qWarning() << Q_FUNC_INFO << "Failed to clean images" << fbImageIds;
        dbRollbackTransaction();
        return;
    }

    if (!dbCommitTransaction()) {
        dbRollbackTransaction();
        return;
    }
}

QList<FacebookImage::ConstPtr> FacebookImagesDatabase::userImages(const QString &fbUserId)
{
    Q_D(FacebookImagesDatabase);
    return d->queryImages(fbUserId, QString());
}

QList<FacebookImage::ConstPtr> FacebookImagesDatabase::albumImages(const QString &fbAlbumId)
{
    Q_D(FacebookImagesDatabase);
    return d->queryImages(QString(), fbAlbumId);
}


void FacebookImagesDatabase::addImage(const QString &fbImageId, const QString &fbAlbumId,
                                      const QString &fbUserId, const QDateTime &createdTime,
                                      const QDateTime &updatedTime, const QString &imageName,
                                      int width, int height, const QString &thumbnailUrl,
                                      const QString &imageUrl)
{
    Q_D(FacebookImagesDatabase);
    FacebookImage::Ptr image = FacebookImage::create(fbImageId, fbAlbumId, fbUserId, createdTime,
                                                     updatedTime, imageName, width, height,
                                                     thumbnailUrl, imageUrl, QString(), QString());
    if (!d->queuedImages.contains(fbImageId)) {
        d->queuedImages.insert(fbImageId, image);
    }
}

void FacebookImagesDatabase::updateImageThumbnail(const QString &fbImageId,
                                                  const QString &thumbnailFile)
{
    Q_D(FacebookImagesDatabase);
    if (!d->queuedUpdatedImages.contains(fbImageId)) {
        QMap<QString, QVariant> data;
        data.insert(QLatin1String(THUMBNAIL_FILE_KEY), thumbnailFile);
        data.insert(QLatin1String(IMAGE_FILE_KEY), image(fbImageId)->imageFile());
        d->queuedUpdatedImages.insert(fbImageId, data);
    } else {
        d->queuedUpdatedImages[fbImageId].insert(QLatin1String(THUMBNAIL_FILE_KEY), thumbnailFile);
    }
}

void FacebookImagesDatabase::updateImageFile(const QString &fbImageId, const QString &imageFile)
{
    Q_D(FacebookImagesDatabase);
    if (!d->queuedUpdatedImages.contains(fbImageId)) {
        QMap<QString, QVariant> data;
        data.insert(QLatin1String(THUMBNAIL_FILE_KEY), image(fbImageId)->thumbnailFile());
        data.insert(QLatin1String(IMAGE_FILE_KEY), imageFile);
        d->queuedUpdatedImages.insert(fbImageId, data);
    } else {
        d->queuedUpdatedImages[fbImageId].insert(QLatin1String(IMAGE_FILE_KEY), imageFile);
    }
}

// Beware, if you write update, you have to have queued
// data that contains exactly the same keys
bool FacebookImagesDatabase::write()
{
    Q_D(FacebookImagesDatabase);
    if (!dbBeginTransaction()) {
        return false;
    }

    qWarning() << "Queued users being saved:" << d->queuedUsers.count();
    qWarning() << "Queued albums being saved:" << d->queuedAlbums.count();
    qWarning() << "Queued images being saved:" << d->queuedImages.count();
    qWarning() << "Queued users being updated:" << d->queuedUpdatedUsers.count();
    qWarning() << "Queued images being updated:" << d->queuedUpdatedImages.count();

    QMap<QString, QVariantList> entries;
    QStringList keys;

    // Start by writing new users
    d->createUsersEntries(d->queuedUsers, keys, entries);
    if (!dbWrite(QLatin1String("users"), keys, entries, InsertOrReplace)) {
        dbRollbackTransaction();
        return false;
    }

    // Write new albums
    d->createAlbumsEntries(d->queuedAlbums, keys, entries);
    if (!dbWrite(QLatin1String("albums"), keys, entries, InsertOrReplace)) {
        dbRollbackTransaction();
        return false;
    }

    // Write new images
    d->createImagesEntries(d->queuedImages, keys, entries);
    if (!dbWrite(QLatin1String("images"), keys, entries, InsertOrReplace)) {
        dbRollbackTransaction();
        return false;
    }

    // Write updated users
    d->createUpdatedEntries(d->queuedUpdatedUsers, QLatin1String("fbUserId"), entries);
    if (!dbWrite(QLatin1String("users"), QStringList(), entries, Update,
                 QLatin1String("fbUserId"))) {
        dbRollbackTransaction();
        return false;
    }

    // Write updated images
    d->createUpdatedEntries(d->queuedUpdatedImages, QLatin1String("fbImageId"), entries);
    if (!dbWrite(QLatin1String("images"), QStringList(), entries, Update,
                 QLatin1String("fbImageId"))) {
        dbRollbackTransaction();
        return false;
    }

    if (!dbCommitTransaction()) {
        return false;
    }

    d->queuedUsers.clear();
    d->queuedAlbums.clear();
    d->queuedImages.clear();

    d->queuedUpdatedUsers.clear();
    d->queuedUpdatedImages.clear();

    return true;
}

bool FacebookImagesDatabase::dbCreateTables()
{
    Q_D(FacebookImagesDatabase);
    // create the facebook image db tables
    // images = fbImageId, fbAlbumId, fbUserId, createdTime, updatedTime, imageName, width, height,
    //          thumbnailUrl, imageUrl, thumbnailFile, imageFile
    // albums = fbAlbumId, fbUserId, createdTime, updatedTime, albumName, imageCount, coverImageId,
    //          thumbnailFile
    // users = fbUserId, updatedTime, userName, thumbnailUrl, imageUrl, thumbnailFile, imageFile
    QSqlQuery query(d->db);
    query.prepare( "CREATE TABLE IF NOT EXISTS images ("
                   "fbImageId TEXT UNIQUE PRIMARY KEY,"
                   "fbAlbumId TEXT,"
                   "fbUserId TEXT,"
                   "createdTime INTEGER,"
                   "updatedTime INTEGER,"
                   "imageName TEXT,"
                   "width INTEGER,"
                   "height INTEGER,"
                   "thumbnailUrl TEXT,"
                   "imageUrl TEXT,"
                   "thumbnailFile TEXT,"
                   "imageFile TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS albums ("
                   "fbAlbumId TEXT UNIQUE PRIMARY KEY,"
                   "fbUserId TEXT,"
                   "createdTime INTEGER,"
                   "updatedTime INTEGER,"
                   "albumName TEXT,"
                   "imageCount INTEGER)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create albums table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS users ("
                   "fbUserId TEXT UNIQUE PRIMARY KEY,"
                   "updatedTime INTEGER,"
                   "userName TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create users table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS accounts ("
                   "accountId INTEGER UNIQUE PRIMARY KEY,"
                   "fbUserId TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create accounts table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    if (!dbCreatePragmaVersion(VERSION)) {
        return false;
    }

    return true;
}

bool FacebookImagesDatabase::dbDropTables()
{
    Q_D(FacebookImagesDatabase);
    QSqlQuery query(d->db);
    query.prepare("DROP TABLE IF EXISTS images");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete images table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS albums");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete albums table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS users");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete users table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS accounts");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete accounts table:" << query.lastError().text();
        d->db.close();
        return false;
    }

    return true;
}
