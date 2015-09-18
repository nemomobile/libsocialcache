/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Jonni Rainisto <jonni.rainisto@jollamobile.com>
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

#include "dropboximagesdatabase.h"
#include "abstractsocialcachedatabase.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QFile>

#include <QtDebug>

static const char *DB_NAME = "dropbox.db";
static const int VERSION = 1;

struct DropboxUserPrivate
{
    explicit DropboxUserPrivate(const QString &userId, const QDateTime &updatedTime,
                                 const QString &userName, int count = -1);
    QString userId;
    QDateTime updatedTime;
    QString userName;
    int count;
};

DropboxUserPrivate::DropboxUserPrivate(const QString &userId, const QDateTime &updatedTime,
                                         const QString &userName, int count)
    : userId(userId), updatedTime(updatedTime), userName(userName), count(count)
{

}

DropboxUser::DropboxUser(const QString &userId, const QDateTime &updatedTime,
                           const QString &userName, int count)
    : d_ptr(new DropboxUserPrivate(userId, updatedTime, userName, count))
{
}

DropboxUser::~DropboxUser()
{
}

DropboxUser::Ptr DropboxUser::create(const QString &userId, const QDateTime &updatedTime,
                                       const QString &userName, int count)
{
    return DropboxUser::Ptr(new DropboxUser(userId, updatedTime, userName, count));
}

QString DropboxUser::userId() const
{
    Q_D(const DropboxUser);
    return d->userId;
}

QDateTime DropboxUser::updatedTime() const
{
    Q_D(const DropboxUser);
    return d->updatedTime;
}

QString DropboxUser::userName() const
{
    Q_D(const DropboxUser);
    return d->userName;
}

int DropboxUser::count() const
{
    Q_D(const DropboxUser);
    return d->count;
}

struct DropboxAlbumPrivate
{
    explicit DropboxAlbumPrivate(const QString &albumId, const QString &userId,
                                  const QDateTime &createdTime, const QDateTime &updatedTime,
                                  const QString &albumName, int imageCount, const QString &hash);
    QString albumId;
    QString userId;
    QDateTime createdTime;
    QDateTime updatedTime;
    QString albumName;
    int imageCount;
    QString hash;
};

DropboxAlbumPrivate::DropboxAlbumPrivate(const QString &albumId, const QString &userId,
                                           const QDateTime &createdTime, const QDateTime &updatedTime,
                                           const QString &albumName, int imageCount, const QString &hash)
    : albumId(albumId), userId(userId), createdTime(createdTime)
    , updatedTime(updatedTime), albumName(albumName), imageCount(imageCount), hash(hash)
{

}

DropboxAlbum::DropboxAlbum(const QString &albumId, const QString &userId,
                             const QDateTime &createdTime, const QDateTime &updatedTime,
                             const QString &albumName, int imageCount, const QString &hash)
    : d_ptr(new DropboxAlbumPrivate(albumId, userId, createdTime, updatedTime,
                                     albumName, imageCount, hash))
{
}

DropboxAlbum::~DropboxAlbum()
{
}

DropboxAlbum::Ptr DropboxAlbum::create(const QString &albumId, const QString &userId,
                                         const QDateTime &createdTime, const QDateTime &updatedTime,
                                         const QString &albumName, int imageCount, const QString &hash)
{
    return DropboxAlbum::Ptr(new DropboxAlbum(albumId, userId, createdTime, updatedTime,
                                                albumName, imageCount, hash));
}

QString DropboxAlbum::albumId() const
{
    Q_D(const DropboxAlbum);
    return d->albumId;
}

QString DropboxAlbum::userId() const
{
    Q_D(const DropboxAlbum);
    return d->userId;
}

QDateTime DropboxAlbum::createdTime() const
{
    Q_D(const DropboxAlbum);
    return d->createdTime;
}

QDateTime DropboxAlbum::updatedTime() const
{
    Q_D(const DropboxAlbum);
    return d->updatedTime;
}

QString DropboxAlbum::albumName() const
{
    Q_D(const DropboxAlbum);
    return d->albumName;
}

int DropboxAlbum::imageCount() const
{
    Q_D(const DropboxAlbum);
    return d->imageCount;
}

QString DropboxAlbum::hash() const
{
    Q_D(const DropboxAlbum);
    return d->hash;
}

struct DropboxImagePrivate
{
    explicit DropboxImagePrivate(const QString &imageId, const QString &albumId,
                                  const QString &userId, const QDateTime &createdTime,
                                  const QDateTime &updatedTime, const QString &imageName,
                                  int width, int height, const QString &thumbnailUrl,
                                  const QString &imageUrl, const QString &thumbnailFile,
                                  const QString &imageFile, int account = -1,
                                  const QString &accessToken = QString());
    QString imageId;
    QString albumId;
    QString userId;
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
    QString accessToken;
};

DropboxImagePrivate::DropboxImagePrivate(const QString &imageId, const QString &albumId,
                                           const QString &userId, const QDateTime &createdTime,
                                           const QDateTime &updatedTime, const QString &imageName,
                                           int width, int height, const QString &thumbnailUrl,
                                           const QString &imageUrl, const QString &thumbnailFile,
                                           const QString &imageFile, int account,
                                           const QString &accessToken)
    : imageId(imageId), albumId(albumId), userId(userId)
    , createdTime(createdTime), updatedTime(updatedTime), imageName(imageName)
    , width(width), height(height), thumbnailUrl(thumbnailUrl)
    , imageUrl(imageUrl), thumbnailFile(thumbnailFile), imageFile(imageFile), account(account)
    , accessToken(accessToken)
{
}

DropboxImage::DropboxImage(const QString &imageId, const QString &albumId,
                             const QString &userId, const QDateTime &createdTime,
                             const QDateTime &updatedTime, const QString &imageName,
                             int width, int height, const QString &thumbnailUrl,
                             const QString &imageUrl, const QString &thumbnailFile,
                             const QString &imageFile, int account,
                             const QString &accessToken)
    : d_ptr(new DropboxImagePrivate(imageId, albumId, userId, createdTime,
                                     updatedTime, imageName, width, height,
                                     thumbnailUrl, imageUrl, thumbnailFile,
                                     imageFile, account, accessToken))
{
}

DropboxImage::~DropboxImage()
{
}

DropboxImage::Ptr DropboxImage::create(const QString &imageId, const QString &albumId,
                                         const QString &userId, const QDateTime &createdTime,
                                         const QDateTime &updatedTime, const QString &imageName,
                                         int width, int height, const QString &thumbnailUrl,
                                         const QString &imageUrl, const QString &thumbnailFile,
                                         const QString &imageFile, int account,
                                         const QString &accessToken)
{
    return DropboxImage::Ptr(new DropboxImage(imageId, albumId, userId, createdTime,
                                                updatedTime, imageName, width, height,
                                                thumbnailUrl, imageUrl, thumbnailFile,
                                                imageFile, account, accessToken));
}

QString DropboxImage::imageId() const
{
    Q_D(const DropboxImage);
    return d->imageId;
}

QString DropboxImage::albumId() const
{
    Q_D(const DropboxImage);
    return d->albumId;
}

QString DropboxImage::userId() const
{
    Q_D(const DropboxImage);
    return d->userId;
}

QDateTime DropboxImage::createdTime() const
{
    Q_D(const DropboxImage);
    return d->createdTime;
}

QDateTime DropboxImage::updatedTime() const
{
    Q_D(const DropboxImage);
    return d->updatedTime;
}

QString DropboxImage::imageName() const
{
    Q_D(const DropboxImage);
    return d->imageName;
}

int DropboxImage::width() const
{
    Q_D(const DropboxImage);
    return d->width;
}

int DropboxImage::height() const
{
    Q_D(const DropboxImage);
    return d->height;
}

QString DropboxImage::thumbnailUrl() const
{
    Q_D(const DropboxImage);
    return d->thumbnailUrl;
}

QString DropboxImage::imageUrl() const
{
    Q_D(const DropboxImage);
    return d->imageUrl;
}

QString DropboxImage::thumbnailFile() const
{
    Q_D(const DropboxImage);
    return d->thumbnailFile;
}

QString DropboxImage::imageFile() const
{
    Q_D(const DropboxImage);
    return d->imageFile;
}

int DropboxImage::account() const
{
    Q_D(const DropboxImage);
    return d->account;
}

QString DropboxImage::accessToken() const
{
    Q_D(const DropboxImage);
    return d->accessToken;
}

class DropboxImagesDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    enum QueryType {
        Users,
        Albums,
        UserImages,
        AlbumImages
    };

    explicit DropboxImagesDatabasePrivate(DropboxImagesDatabase *q);
    ~DropboxImagesDatabasePrivate();

private:
    Q_DECLARE_PUBLIC(DropboxImagesDatabase)

    static void clearCachedImages(QSqlQuery &query);

    QList<DropboxUser::ConstPtr> queryUsers() const;
    QList<DropboxAlbum::ConstPtr> queryAlbums(const QString &userId) const;
    QList<DropboxImage::ConstPtr> queryImages(const QString &userId, const QString &albumId);

    struct {
        QList<int> purgeAccounts;

        QStringList removeUsers;
        QStringList removeAlbums;
        QStringList removeImages;

        QMap<QString, DropboxUser::ConstPtr> insertUsers;
        QMap<QString, DropboxAlbum::ConstPtr> insertAlbums;
        QMap<QString, DropboxImage::ConstPtr> insertImages;

        QMap<int, QString> syncAccounts;

        QMap<QString, QString> updateThumbnailFiles;
        QMap<QString, QString> updateImageFiles;
    } queue;

    struct {
        QueryType type;
        QString id;
        QList<DropboxUser::ConstPtr> users;
        QList<DropboxAlbum::ConstPtr> albums;
        QList<DropboxImage::ConstPtr> images;
    } query;

    struct {
        QList<DropboxUser::ConstPtr> users;
        QList<DropboxAlbum::ConstPtr> albums;
        QList<DropboxImage::ConstPtr> images;
    } result;
};

DropboxImagesDatabasePrivate::DropboxImagesDatabasePrivate(DropboxImagesDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Dropbox),
            SocialSyncInterface::dataType(SocialSyncInterface::Images),
            QLatin1String(DB_NAME),
            VERSION)
{
}

DropboxImagesDatabasePrivate::~DropboxImagesDatabasePrivate()
{
}

void DropboxImagesDatabasePrivate::clearCachedImages(QSqlQuery &query)
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

QList<DropboxImage::ConstPtr> DropboxImagesDatabasePrivate::queryImages(const QString &userId,
                                                                          const QString &albumId)
{
    Q_Q(DropboxImagesDatabase);

    QList<DropboxImage::ConstPtr> data;

    if (!userId.isEmpty() && !albumId.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Cannot select images in both an album and for an user";
        return data;
    }

    QString queryString = QLatin1String("SELECT images.imageId, images.albumId, "\
                                        "images.userId, images.createdTime, "\
                                        "images.updatedTime, images.imageName, images.width, "\
                                        "images.height, images.thumbnailUrl, images.imageUrl, "\
                                        "images.thumbnailFile, images.imageFile, "\
                                        "accounts.accountId, images.accessToken "\
                                        "FROM images "\
                                        "INNER JOIN accounts "\
                                        "ON accounts.userId = images.userId%1 "\
                                        "ORDER BY images.updatedTime %2");

    if (!userId.isEmpty()) {
        queryString = queryString.arg(QLatin1String(" WHERE images.userId = :userId"),
                                      QLatin1String("DESC"));
    } else if (!albumId.isEmpty()){
        queryString = queryString.arg(QLatin1String(" WHERE images.albumId = :albumId"),
                                      QString());
    } else {
        queryString = queryString.arg(QString(), QLatin1String("DESC"));
    }

    QSqlQuery query = q->prepare(queryString);
    if (!userId.isEmpty()) {
        query.bindValue(":userId", userId);
    }
    if (!albumId.isEmpty()) {
        query.bindValue(":albumId", albumId);
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query all albums:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(DropboxImage::create(query.value(0).toString(), query.value(1).toString(),
                                          query.value(2).toString(),
                                          QDateTime::fromTime_t(query.value(3).toUInt()),
                                          QDateTime::fromTime_t(query.value(4).toUInt()),
                                          query.value(5).toString(),
                                          query.value(6).toInt(), query.value(7).toInt(),
                                          query.value(8).toString(), query.value(9).toString(),
                                          query.value(10).toString(), query.value(11).toString(),
                                          query.value(12).toInt(), query.value(13).toString()));
    }

    return data;
}

bool operator==(const DropboxUser::ConstPtr &user1, const DropboxUser::ConstPtr &user2)
{
    return user1->userId() == user2->userId();
}

bool operator==(const DropboxAlbum::ConstPtr &album1, const DropboxAlbum::ConstPtr &album2)
{
    return album1->albumId() == album2->albumId() && album1->userId() == album2->userId();
}

bool operator==(const DropboxImage::ConstPtr &image1, const DropboxImage::ConstPtr &image2)
{
    return image1->imageId() == image2->imageId() && image1->albumId() == image2->albumId()
            && image1->userId() == image2->userId();
}

// Note
//
// Insertion operations needs to use write(), while Delete
// operations are automatically using transactions and
// don't need write().

DropboxImagesDatabase::DropboxImagesDatabase()
    : AbstractSocialCacheDatabase(*(new DropboxImagesDatabasePrivate(this)))
{
}

DropboxImagesDatabase::~DropboxImagesDatabase()
{
    wait();
}

bool DropboxImagesDatabase::syncAccount(int accountId, const QString &userId)
{
    Q_D(DropboxImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.syncAccounts.insert(accountId, userId);

    return true;
}

void DropboxImagesDatabase::purgeAccount(int accountId)
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.purgeAccounts.append(accountId);
}

// Returns the user but do not return a count
// if you want a count, better consider users
DropboxUser::ConstPtr DropboxImagesDatabase::user(const QString &userId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT userId, updatedTime, userName "
                "FROM users WHERE userId = :userId"));
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from users table:" << query.lastError();
        return DropboxUser::Ptr();
    }

    // If we have the user, we have a result, otherwise we won't have the user
    if (!query.next()) {
        return DropboxUser::Ptr();
    }

    DropboxUser::ConstPtr user = DropboxUser::create(
                                query.value(0).toString(),
                                QDateTime::fromTime_t(query.value(1).toUInt()),
                                query.value(2).toString());

    query.finish();

    return user;
}

void DropboxImagesDatabase::addUser(const QString &userId, const QDateTime &updatedTime,
                                     const QString &userName)
{
    Q_D(DropboxImagesDatabase);

    DropboxUser::Ptr user = DropboxUser::create(userId, updatedTime, userName);

    QMutexLocker locker(&d->mutex);

    d->queue.insertUsers.insert(userId, user);
}

void DropboxImagesDatabase::removeUser(const QString &userId)
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeUsers.append(userId);
}

QList<DropboxUser::ConstPtr> DropboxImagesDatabasePrivate::queryUsers() const
{
    QList<DropboxUser::ConstPtr> data;

    QSqlQuery query = q_func()->prepare(QStringLiteral(
                "SELECT users.userId, users.updatedTime, users.userName, "
                "SUM(albums.imageCount) as count "
                "FROM users "
                "LEFT JOIN albums ON albums.userId = users.userId "
                "GROUP BY users.userId "
                "ORDER BY users.userId"));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query all users:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(DropboxUser::create(query.value(0).toString(),
                                         QDateTime::fromTime_t(query.value(1).toUInt()),
                                         query.value(2).toString(), query.value(3).toInt()));
    }
    return data;
}

QStringList DropboxImagesDatabase::allAlbumIds(bool *ok) const
{
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT DISTINCT albumId "
                "FROM albums "
                "ORDER BY updatedTime DESC"));
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


QMap<int,QString> DropboxImagesDatabase::accounts(bool *ok) const
{
    if (ok) {
        *ok = false;
    }

    QMap<int,QString> result;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT accountId, userId "
                "FROM accounts "));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to fetch account mappings" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result[query.value(0).toInt()] = query.value(1).toString();
    }

    if (ok) {
        *ok = true;
    }
    return result;
}


DropboxAlbum::ConstPtr DropboxImagesDatabase::album(const QString &albumId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT albumId, userId, createdTime, updatedTime, albumName, "
                "imageCount, hash "
                "FROM albums WHERE albumId = :albumId"));
    query.bindValue(":albumId", albumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return DropboxAlbum::Ptr();
    }

    // If we have the album, we have a result, otherwise we won't have the album
    if (!query.next()) {
        return DropboxAlbum::Ptr();
    }

    DropboxAlbum::ConstPtr album = DropboxAlbum::create(query.value(0).toString(),  query.value(1).toString(),
                                 QDateTime::fromTime_t(query.value(2).toUInt()),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 query.value(4).toString(), query.value(5).toInt(),
                                 query.value(6).toString());

    query.finish();
    return album;
}

void DropboxImagesDatabase::addAlbum(const QString &albumId, const QString &userId,
                                      const QDateTime &createdTime, const QDateTime &updatedTime,
                                      const QString &albumName, int imageCount, const QString &hash)
{
    Q_D(DropboxImagesDatabase);
    DropboxAlbum::Ptr album = DropboxAlbum::create(albumId, userId, createdTime,
                                                     updatedTime, albumName, imageCount, hash);

    QMutexLocker locker(&d->mutex);

    d->queue.insertAlbums.insert(albumId, album);
}

void DropboxImagesDatabase::removeAlbum(const QString &albumId)
{
    Q_D(DropboxImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.removeAlbums.append(albumId);
}

void DropboxImagesDatabase::removeAlbums(const QStringList &albumIds)
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeAlbums += albumIds;
}

QList<DropboxAlbum::ConstPtr> DropboxImagesDatabasePrivate::queryAlbums(const QString &userId) const
{
    QList<DropboxAlbum::ConstPtr> data;

    QString queryString = QLatin1String("SELECT albumId, userId, createdTime, updatedTime, "\
                                        "albumName, imageCount, hash "\
                                        "FROM albums%1 ORDER BY updatedTime DESC");
    if (!userId.isEmpty()) {
        queryString = queryString.arg(QLatin1String(" WHERE userId = :userId"));
    } else {
        queryString = queryString.arg(QString());
    }

    QSqlQuery query = q_func()->prepare(queryString);
    if (!userId.isEmpty()) {
        query.bindValue(":userId", userId);
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query all albums:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(DropboxAlbum::create(query.value(0).toString(), query.value(1).toString(),
                                          QDateTime::fromTime_t(query.value(2).toUInt()),
                                          QDateTime::fromTime_t(query.value(3).toUInt()),
                                          query.value(4).toString(), query.value(5).toInt(),
                                          query.value(6).toString()));
    }

    return data;
}

QStringList DropboxImagesDatabase::allImageIds(bool *ok) const
{
    if (ok) {
        *ok = false;
    }
    QStringList ids;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT DISTINCT imageId "
                "FROM images "
                "ORDER BY updatedTime DESC"));
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

QStringList DropboxImagesDatabase::imageIds(const QString &albumId, bool *ok) const
{
    if (ok) {
        *ok = false;
    }
    QStringList ids;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT DISTINCT imageId "
                "FROM images WHERE albumId = :albumId"));
    query.bindValue(":albumId", albumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to fetch images for album" << albumId
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

DropboxImage::ConstPtr DropboxImagesDatabase::image(const QString &imageId) const
{
    QSqlQuery query = prepare(
                "SELECT imageId, albumId, userId, createdTime, updatedTime, imageName, "
                "width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile, accessToken "
                "FROM images WHERE imageId = :imageId");
    query.bindValue(":imageId", imageId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return DropboxImage::Ptr();
    }

    // If we have the image, we have a result, otherwise we won't have the image
    if (!query.next()) {
        return DropboxImage::Ptr();
    }

    return DropboxImage::create(query.value(0).toString(),  query.value(1).toString(),
                                 query.value(2).toString(),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 QDateTime::fromTime_t(query.value(4).toUInt()),
                                 query.value(5).toString(),
                                 query.value(6).toInt(), query.value(7).toInt(),
                                 query.value(8).toString(), query.value(9).toString(),
                                 query.value(10).toString(), query.value(11).toString(),-1,
                                 query.value(12).toString());
}

void DropboxImagesDatabase::removeImage(const QString &imageId)
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages.append(imageId);
}

void DropboxImagesDatabase::removeImages(const QStringList &imageIds)
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages += imageIds;
}

void DropboxImagesDatabase::addImage(const QString &imageId, const QString &albumId,
                                      const QString &userId, const QDateTime &createdTime,
                                      const QDateTime &updatedTime, const QString &imageName,
                                      int width, int height, const QString &thumbnailUrl,
                                      const QString &imageUrl, const QString &accessToken)
{
    Q_D(DropboxImagesDatabase);
    DropboxImage::Ptr image = DropboxImage::create(imageId, albumId, userId, createdTime,
                                                     updatedTime, imageName, width, height,
                                                     thumbnailUrl, imageUrl, QString(), QString(),-1,accessToken);
    QMutexLocker locker(&d->mutex);

    d->queue.insertImages.insert(imageId, image);
}

void DropboxImagesDatabase::updateImageThumbnail(const QString &imageId,
                                                  const QString &thumbnailFile)
{
    Q_D(DropboxImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateThumbnailFiles.insert(imageId, thumbnailFile);
}

void DropboxImagesDatabase::updateImageFile(const QString &imageId, const QString &imageFile)
{
    Q_D(DropboxImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateImageFiles.insert(imageId, imageFile);
}

void DropboxImagesDatabase::commit()
{
    executeWrite();
}

QList<DropboxUser::ConstPtr> DropboxImagesDatabase::users() const
{
    return d_func()->result.users;
}

QList<DropboxImage::ConstPtr> DropboxImagesDatabase::images() const
{
    return d_func()->result.images;
}

QList<DropboxAlbum::ConstPtr> DropboxImagesDatabase::albums() const
{
    return d_func()->result.albums;
}

void DropboxImagesDatabase::queryUsers()
{
    Q_D(DropboxImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = DropboxImagesDatabasePrivate::Users;
    }
    executeRead();
}

void DropboxImagesDatabase::queryAlbums(const QString &userId)
{
    Q_D(DropboxImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = DropboxImagesDatabasePrivate::Albums;
        d->query.id = userId;
    }
    executeRead();
}

void DropboxImagesDatabase::queryUserImages(const QString &userId)
{
    Q_D(DropboxImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = DropboxImagesDatabasePrivate::UserImages;
        d->query.id = userId;
    }
    executeRead();
}

void DropboxImagesDatabase::queryAlbumImages(const QString &albumId)
{
    Q_D(DropboxImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = DropboxImagesDatabasePrivate::AlbumImages;
        d->query.id = albumId;
    }
    executeRead();
}

bool DropboxImagesDatabase::read()
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    switch (d->query.type) {
    case DropboxImagesDatabasePrivate::Users: {
        locker.unlock();
        QList<DropboxUser::ConstPtr> users = d->queryUsers();
        locker.relock();
        d->query.users = users;
        return true;
    }
    case DropboxImagesDatabasePrivate::Albums: {
        const QString userId = d->query.id;
        locker.unlock();
        QList<DropboxAlbum::ConstPtr> albums = d->queryAlbums(userId);
        locker.relock();
        d->query.albums = albums;
        return true;
    }
    case DropboxImagesDatabasePrivate::UserImages:
    case DropboxImagesDatabasePrivate::AlbumImages: {
        const QString userId = d->query.type == DropboxImagesDatabasePrivate::UserImages
                ? d->query.id
                : QString();
        const QString albumId = d->query.type == DropboxImagesDatabasePrivate::AlbumImages
                ? d->query.id
                : QString();
        locker.unlock();
        QList<DropboxImage::ConstPtr> images = d->queryImages(userId, albumId);
        locker.relock();
        d->query.images = images;
        return true;
    }
    default:
        return false;
    }
}

void DropboxImagesDatabase::readFinished()
{
    Q_D(DropboxImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);

        d->result.users = d->query.users;
        d->result.albums = d->query.albums;
        d->result.images = d->query.images;

        d->query.users.clear();
        d->query.albums.clear();
        d->query.images.clear();
    }
    emit queryFinished();
}

bool DropboxImagesDatabase::write()
{
    Q_D(DropboxImagesDatabase);
    QMutexLocker locker(&d->mutex);

    qWarning() << "Queued users being saved:" << d->queue.insertUsers.count();
    qWarning() << "Queued albums being saved:" << d->queue.insertAlbums.count();
    qWarning() << "Queued images being saved:" << d->queue.insertImages.count();
    qWarning() << "Queued thumbnail files being updated:" << d->queue.updateThumbnailFiles.count();
    qWarning() << "Queued image files being updated:" << d->queue.updateImageFiles.count();

    const QList<int> purgeAccounts = d->queue.purgeAccounts;

    QStringList removeUsers = d->queue.removeUsers;
    const QStringList removeAlbums = d->queue.removeAlbums;
    const QStringList removeImages = d->queue.removeImages;

    const QMap<QString, DropboxUser::ConstPtr> insertUsers = d->queue.insertUsers;
    const QMap<QString, DropboxAlbum::ConstPtr> insertAlbums = d->queue.insertAlbums;
    const QMap<QString, DropboxImage::ConstPtr> insertImages = d->queue.insertImages;

    const QMap<int, QString> syncAccounts = d->queue.syncAccounts;

    const QMap<QString, QString> updateThumbnailFiles = d->queue.updateThumbnailFiles;
    const QMap<QString, QString> updateImageFiles = d->queue.updateImageFiles;

    d->queue.purgeAccounts.clear();

    d->queue.removeUsers.clear();
    d->queue.removeAlbums.clear();
    d->queue.removeImages.clear();

    d->queue.insertUsers.clear();
    d->queue.insertAlbums.clear();
    d->queue.insertImages.clear();

    d->queue.syncAccounts.clear();

    d->queue.updateThumbnailFiles.clear();
    d->queue.updateImageFiles.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!purgeAccounts.isEmpty()) {
        query = prepare(QStringLiteral(
                    "SELECT userId "
                    "FROM accounts "
                    "WHERE accountId = :accountId"));

        Q_FOREACH (int accountId, purgeAccounts) {
            query.bindValue(QStringLiteral(":accountId"), accountId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec account id selection query:"
                           << query.lastError().text();
                success = false;
            } else while (query.next()) {
                removeUsers.append(query.value(0).toString());
            }
        }
    }

    if (!removeUsers.isEmpty()) {
        QVariantList userIds;

        query = prepare(QStringLiteral(
                    "SELECT thumbnailFile, imageFile "
                    "FROM images "
                    "WHERE userId = :userId"));
        Q_FOREACH (const QString &userId, removeUsers) {
            userIds.append(userId);

            query.bindValue(QStringLiteral(":userId"), userId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM accounts "
                    "WHERE userId = :userId"));
        query.bindValue(QStringLiteral(":userId"), userIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM users "
                    "WHERE userId = :userId"));
        query.bindValue(QStringLiteral(":userId"), userIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE userId = :userId"));
        query.bindValue(QStringLiteral(":userId"), userIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE userId = :userId"));
        query.bindValue(QStringLiteral(":userId"), userIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeAlbums.isEmpty()) {
        QVariantList albumIds;

        query = prepare(QStringLiteral(
                    "SELECT thumbnailFile, imageFile "
                    "FROM images "
                    "WHERE albumId = :albumId"));
        Q_FOREACH (const QString &albumId, removeAlbums) {
            albumIds.append(albumId);

            query.bindValue(QStringLiteral(":albumId"), albumId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE albumId = :albumId"));
        query.bindValue(QStringLiteral(":albumId"), albumIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE albumId = :albumId"));
        query.bindValue(QStringLiteral(":albumId"), albumIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeImages.isEmpty()) {
        QVariantList imageIds;

        query = prepare(QStringLiteral(
                    "SELECT thumbnailFile, imageFile "
                    "FROM images "
                    "WHERE imageId = :imageId"));
        Q_FOREACH (const QString &imageId, removeImages) {
            imageIds.append(imageId);

            query.bindValue(QStringLiteral(":imageId"), imageId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE imageId = :imageId"));
        query.bindValue(QStringLiteral(":imageId"), imageIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertUsers.isEmpty()) {
        QVariantList userIds;
        QVariantList updatedTimes;
        QVariantList usernames;

        Q_FOREACH (const DropboxUser::ConstPtr &user, insertUsers) {
            userIds.append(user->userId());
            updatedTimes.append(user->updatedTime().toTime_t());
            usernames.append(user->userName());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO users ("
                    " userId, updatedTime, userName) "
                    "VALUES ("
                    " :userId, :updatedTime, :userName);"));
        query.bindValue(QStringLiteral(":userId"), userIds);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":userName"), usernames);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertAlbums.isEmpty()) {
        QVariantList albumIds, userIds;
        QVariantList createdTimes, updatedTimes;
        QVariantList albumNames;
        QVariantList imageCounts;
        QVariantList hashes;

        Q_FOREACH (const DropboxAlbum::ConstPtr &album, insertAlbums) {
            albumIds.append(album->albumId());
            userIds.append(album->userId());
            createdTimes.append(album->createdTime().toTime_t());
            updatedTimes.append(album->updatedTime().toTime_t());
            albumNames.append(album->albumName());
            imageCounts.append(album->imageCount());
            hashes.append(album->hash());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO albums("
                    " albumId, userId, createdTime, updatedTime, albumName, imageCount, hash) "
                    "VALUES ("
                    " :albumId, :userId, :createdTime, :updatedTime, :albumName, :imageCount, :hash)"));
        query.bindValue(QStringLiteral(":albumId"), albumIds);
        query.bindValue(QStringLiteral(":userId"), userIds);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":albumName"), albumNames);
        query.bindValue(QStringLiteral(":imageCount"), imageCounts);
        query.bindValue(QStringLiteral(":hash"), hashes);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertImages.isEmpty()) {
        QVariantList imageIds, albumIds, userIds;
        QVariantList createdTimes, updatedTimes;
        QVariantList imageNames;
        QVariantList widths, heights;
        QVariantList thumbnailUrls, imageUrls;
        QVariantList thumbnailFiles, imageFiles;
        QVariantList accessTokens;

        Q_FOREACH (const DropboxImage::ConstPtr &image, insertImages) {
            imageIds.append(image->imageId());
            albumIds.append(image->albumId());
            userIds.append(image->userId());
            createdTimes.append(image->createdTime().toTime_t());
            updatedTimes.append(image->updatedTime().toTime_t());
            imageNames.append(image->imageName());
            widths.append(image->width());
            heights.append(image->height());
            thumbnailUrls.append(image->thumbnailUrl());
            imageUrls.append(image->imageUrl());
            thumbnailFiles.append(image->thumbnailFile());
            imageFiles.append(image->imageFile());
            accessTokens.append(image->accessToken());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO images ("
                    " imageId, albumId, userId, createdTime, updatedTime, imageName,"
                    " width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile, accessToken) "
                    "VALUES ("
                    " :imageId, :albumId, :userId, :createdTime, :updatedTime, :imageName,"
                    " :width, :height, :thumbnailUrl, :imageUrl, :thumbnailFile, :imageFile, :accessToken)"));
        query.bindValue(QStringLiteral(":imageId"), imageIds);
        query.bindValue(QStringLiteral(":albumId"), albumIds);
        query.bindValue(QStringLiteral(":userId"), userIds);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":imageName"), imageNames);
        query.bindValue(QStringLiteral(":width"), widths);
        query.bindValue(QStringLiteral(":height"), heights);
        query.bindValue(QStringLiteral(":thumbnailUrl"), thumbnailUrls);
        query.bindValue(QStringLiteral(":imageUrl"), imageUrls);
        query.bindValue(QStringLiteral(":thumbnailFile"), thumbnailFiles);
        query.bindValue(QStringLiteral(":imageFile"), imageFiles);
        query.bindValue(QStringLiteral(":accessToken"), accessTokens);
        executeBatchSocialCacheQuery(query);
    }

    if (!syncAccounts.isEmpty()) {
        QVariantList accountIds;
        QVariantList userIds;

        for (QMap<int, QString>::const_iterator it = syncAccounts.begin();
                it != syncAccounts.end();
                ++it) {
            accountIds.append(it.key());
            userIds.append(it.value());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO accounts ("
                    " accountId, userId) "
                    "VALUES("
                    " :accountId, :userId)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":userId"), userIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!updateThumbnailFiles.isEmpty()) {
        QVariantList imageIds;
        QVariantList thumbnailFiles;

        for (QMap<QString, QString>::const_iterator it = updateThumbnailFiles.begin();
                it != updateThumbnailFiles.end();
                ++it) {
            imageIds.append(it.key());
            thumbnailFiles.append(it.value());
        }

        query = prepare(QStringLiteral(
                    "UPDATE images "
                    "SET thumbnailFile = :thumbnailFile "
                    "WHERE imageId = :imageId"));
        query.bindValue(QStringLiteral(":thumbnailFile"), thumbnailFiles);
        query.bindValue(QStringLiteral(":imageId"), imageIds);
        executeBatchSocialCacheQuery(query);
    }


    if (!updateImageFiles.isEmpty()) {
        QVariantList imageIds;
        QVariantList imageFiles;

        for (QMap<QString, QString>::const_iterator it = updateImageFiles.begin();
                it != updateImageFiles.end();
                ++it) {
            imageIds.append(it.key());
            imageFiles.append(it.value());
        }

        query = prepare(QStringLiteral(
                    "UPDATE images "
                    "SET imageFile = :imageFile "
                    "WHERE imageId = :imageId"));
        query.bindValue(QStringLiteral(":imageFile"), imageFiles);
        query.bindValue(QStringLiteral(":imageId"), imageIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool DropboxImagesDatabase::createTables(QSqlDatabase database) const
{
    // create the Dropbox image db tables
    // images = imageId, albumId, userId, createdTime, updatedTime, imageName, width, height,
    //          thumbnailUrl, imageUrl, thumbnailFile, imageFile
    // albums = albumId, userId, createdTime, updatedTime, albumName, imageCount, hash
    // users = userId, updatedTime, userName, thumbnailUrl, imageUrl, thumbnailFile, imageFile
    QSqlQuery query(database);
    query.prepare( "CREATE TABLE IF NOT EXISTS images ("
                   "imageId TEXT UNIQUE PRIMARY KEY,"
                   "albumId TEXT,"
                   "userId TEXT,"
                   "createdTime INTEGER,"
                   "updatedTime INTEGER,"
                   "imageName TEXT,"
                   "width INTEGER,"
                   "height INTEGER,"
                   "thumbnailUrl TEXT,"
                   "imageUrl TEXT,"
                   "thumbnailFile TEXT,"
                   "imageFile TEXT,"
                   "accessToken TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table:" << query.lastError().text();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS albums ("
                   "albumId TEXT UNIQUE PRIMARY KEY,"
                   "userId TEXT,"
                   "createdTime INTEGER,"
                   "updatedTime INTEGER,"
                   "albumName TEXT,"
                   "imageCount INTEGER,"
                   "hash TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create albums table:" << query.lastError().text();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS users ("
                   "userId TEXT UNIQUE PRIMARY KEY,"
                   "updatedTime INTEGER,"
                   "userName TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create users table:" << query.lastError().text();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS accounts ("
                   "accountId INTEGER UNIQUE PRIMARY KEY,"
                   "userId TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create accounts table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DropboxImagesDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare("DROP TABLE IF EXISTS images");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete images table:" << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS albums");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete albums table:" << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS users");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete users table:" << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS accounts");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete accounts table:" << query.lastError().text();
        return false;
    }

    return true;
}
