/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Antti Seppälä <antti.seppala@jollamobile.com>
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

#include "onedriveimagesdatabase.h"
#include "abstractsocialcachedatabase.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QFile>

#include <QtDebug>

static const char *DB_NAME = "onedrive.db";
static const int VERSION = 3;

struct OneDriveUserPrivate
{
    explicit OneDriveUserPrivate(const QString &userId, const QDateTime &updatedTime,
                                 const QString &userName, int accountId, int count = -1);
    QString userId;
    QDateTime updatedTime;
    QString userName;
    int accountId;
    int count;
};

OneDriveUserPrivate::OneDriveUserPrivate(const QString &userId, const QDateTime &updatedTime,
                                         const QString &userName, int accountId, int count)
    : userId(userId), updatedTime(updatedTime), userName(userName),
      accountId(accountId), count(count)
{
}

OneDriveUser::OneDriveUser(const QString &userId, const QDateTime &updatedTime,
                           const QString &userName, int accountId, int count)
    : d_ptr(new OneDriveUserPrivate(userId, updatedTime, userName, accountId, count))
{
}

OneDriveUser::~OneDriveUser()
{
}

OneDriveUser::Ptr OneDriveUser::create(const QString &userId, const QDateTime &updatedTime,
                                       const QString &userName, int accountId, int count)
{
    return OneDriveUser::Ptr(new OneDriveUser(userId, updatedTime, userName, accountId, count));
}

QString OneDriveUser::userId() const
{
    Q_D(const OneDriveUser);
    return d->userId;
}

QDateTime OneDriveUser::updatedTime() const
{
    Q_D(const OneDriveUser);
    return d->updatedTime;
}

QString OneDriveUser::userName() const
{
    Q_D(const OneDriveUser);
    return d->userName;
}

int OneDriveUser::accountId() const
{
    Q_D(const OneDriveUser);
    return d->accountId;
}

int OneDriveUser::count() const
{
    Q_D(const OneDriveUser);
    return d->count;
}

struct OneDriveAlbumPrivate
{
    explicit OneDriveAlbumPrivate(const QString &albumId, const QString &userId,
                                  const QDateTime &createdTime, const QDateTime &updatedTime,
                                  const QString &albumName, int imageCount);
    QString albumId;
    QString userId;
    QDateTime createdTime;
    QDateTime updatedTime;
    QString albumName;
    int imageCount;
};

OneDriveAlbumPrivate::OneDriveAlbumPrivate(const QString &albumId, const QString &userId,
                                           const QDateTime &createdTime, const QDateTime &updatedTime,
                                           const QString &albumName, int imageCount)
    : albumId(albumId), userId(userId), createdTime(createdTime)
    , updatedTime(updatedTime), albumName(albumName), imageCount(imageCount)
{

}

OneDriveAlbum::OneDriveAlbum(const QString &albumId, const QString &userId,
                             const QDateTime &createdTime, const QDateTime &updatedTime,
                             const QString &albumName, int imageCount)
    : d_ptr(new OneDriveAlbumPrivate(albumId, userId, createdTime, updatedTime,
                                     albumName, imageCount))
{
}

OneDriveAlbum::~OneDriveAlbum()
{
}

OneDriveAlbum::Ptr OneDriveAlbum::create(const QString &albumId, const QString &userId,
                                         const QDateTime &createdTime, const QDateTime &updatedTime,
                                         const QString &albumName, int imageCount)
{
    return OneDriveAlbum::Ptr(new OneDriveAlbum(albumId, userId, createdTime, updatedTime,
                                                albumName, imageCount));
}

QString OneDriveAlbum::albumId() const
{
    Q_D(const OneDriveAlbum);
    return d->albumId;
}

QString OneDriveAlbum::userId() const
{
    Q_D(const OneDriveAlbum);
    return d->userId;
}

QDateTime OneDriveAlbum::createdTime() const
{
    Q_D(const OneDriveAlbum);
    return d->createdTime;
}

QDateTime OneDriveAlbum::updatedTime() const
{
    Q_D(const OneDriveAlbum);
    return d->updatedTime;
}

QString OneDriveAlbum::albumName() const
{
    Q_D(const OneDriveAlbum);
    return d->albumName;
}

int OneDriveAlbum::imageCount() const
{
    Q_D(const OneDriveAlbum);
    return d->imageCount;
}

struct OneDriveImagePrivate
{
    explicit OneDriveImagePrivate(const QString &imageId, const QString &albumId,
                                  const QString &userId, const QDateTime &createdTime,
                                  const QDateTime &updatedTime, const QString &imageName,
                                  int width, int height, const QString &thumbnailUrl,
                                  const QString &imageUrl, const QString &thumbnailFile,
                                  const QString &imageFile, const QString &description,
                                  int accountId);
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
    QString description;
    int accountId;
};

OneDriveImagePrivate::OneDriveImagePrivate(const QString &imageId, const QString &albumId,
                                           const QString &userId, const QDateTime &createdTime,
                                           const QDateTime &updatedTime, const QString &imageName,
                                           int width, int height, const QString &thumbnailUrl,
                                           const QString &imageUrl, const QString &thumbnailFile,
                                           const QString &imageFile, const QString & description,
                                           int accountId)
    : imageId(imageId), albumId(albumId), userId(userId)
    , createdTime(createdTime), updatedTime(updatedTime), imageName(imageName)
    , width(width), height(height), thumbnailUrl(thumbnailUrl)
    , imageUrl(imageUrl), thumbnailFile(thumbnailFile), imageFile(imageFile)
    , description(description), accountId(accountId)
{
}

OneDriveImage::OneDriveImage(const QString &imageId, const QString &albumId,
                             const QString &userId, const QDateTime &createdTime,
                             const QDateTime &updatedTime, const QString &imageName,
                             int width, int height, const QString &thumbnailUrl,
                             const QString &imageUrl, const QString &thumbnailFile,
                             const QString &imageFile, const QString &description,
                             int accountId)
    : d_ptr(new OneDriveImagePrivate(imageId, albumId, userId, createdTime,
                                     updatedTime, imageName, width, height,
                                     thumbnailUrl, imageUrl, thumbnailFile,
                                     imageFile, description, accountId))
{
}

OneDriveImage::~OneDriveImage()
{
}

OneDriveImage::Ptr OneDriveImage::create(const QString &imageId, const QString &albumId,
                                         const QString &userId, const QDateTime &createdTime,
                                         const QDateTime &updatedTime, const QString &imageName,
                                         int width, int height, const QString &thumbnailUrl,
                                         const QString &imageUrl, const QString &thumbnailFile,
                                         const QString &imageFile, const QString &description,
                                         int accountId)
{
    return OneDriveImage::Ptr(new OneDriveImage(imageId, albumId, userId, createdTime,
                                                updatedTime, imageName, width, height,
                                                thumbnailUrl, imageUrl, thumbnailFile,
                                                imageFile, description, accountId));
}

QString OneDriveImage::imageId() const
{
    Q_D(const OneDriveImage);
    return d->imageId;
}

QString OneDriveImage::albumId() const
{
    Q_D(const OneDriveImage);
    return d->albumId;
}

QString OneDriveImage::userId() const
{
    Q_D(const OneDriveImage);
    return d->userId;
}

QDateTime OneDriveImage::createdTime() const
{
    Q_D(const OneDriveImage);
    return d->createdTime;
}

QDateTime OneDriveImage::updatedTime() const
{
    Q_D(const OneDriveImage);
    return d->updatedTime;
}

QString OneDriveImage::imageName() const
{
    Q_D(const OneDriveImage);
    return d->imageName;
}

int OneDriveImage::width() const
{
    Q_D(const OneDriveImage);
    return d->width;
}

int OneDriveImage::height() const
{
    Q_D(const OneDriveImage);
    return d->height;
}

QString OneDriveImage::thumbnailUrl() const
{
    Q_D(const OneDriveImage);
    return d->thumbnailUrl;
}

QString OneDriveImage::imageUrl() const
{
    Q_D(const OneDriveImage);
    return d->imageUrl;
}

QString OneDriveImage::thumbnailFile() const
{
    Q_D(const OneDriveImage);
    return d->thumbnailFile;
}

QString OneDriveImage::imageFile() const
{
    Q_D(const OneDriveImage);
    return d->imageFile;
}

QString OneDriveImage::description() const
{
    Q_D(const OneDriveImage);
    return d->description;
}

int OneDriveImage::accountId() const
{
    Q_D(const OneDriveImage);
    return d->accountId;
}

class OneDriveImagesDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    enum QueryType {
        Users,
        Albums,
        UserImages,
        AlbumImages
    };

    explicit OneDriveImagesDatabasePrivate(OneDriveImagesDatabase *q);
    ~OneDriveImagesDatabasePrivate();

private:
    Q_DECLARE_PUBLIC(OneDriveImagesDatabase)

    static void clearCachedImages(QSqlQuery &query);

    QList<OneDriveUser::ConstPtr> queryUsers() const;
    QList<OneDriveAlbum::ConstPtr> queryAlbums(const QString &userId) const;
    QList<OneDriveImage::ConstPtr> queryImages(const QString &userId, const QString &albumId);

    struct {
        QList<int> purgeAccounts;

        QStringList removeUsers;
        QStringList removeAlbums;
        QStringList removeImages;

        QMap<QString, OneDriveUser::ConstPtr> insertUsers;
        QMap<QString, OneDriveAlbum::ConstPtr> insertAlbums;
        QMap<QString, OneDriveImage::ConstPtr> insertImages;

        QMap<int, QString> syncAccounts;

        QMap<QString, QString> updateThumbnailFiles;
        QMap<QString, QString> updateImageFiles;
    } queue;

    struct {
        QueryType type;
        QString id;
        QList<OneDriveUser::ConstPtr> users;
        QList<OneDriveAlbum::ConstPtr> albums;
        QList<OneDriveImage::ConstPtr> images;
    } query;

    struct {
        QList<OneDriveUser::ConstPtr> users;
        QList<OneDriveAlbum::ConstPtr> albums;
        QList<OneDriveImage::ConstPtr> images;
    } result;
};

OneDriveImagesDatabasePrivate::OneDriveImagesDatabasePrivate(OneDriveImagesDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::OneDrive),
            SocialSyncInterface::dataType(SocialSyncInterface::Images),
            QLatin1String(DB_NAME),
            VERSION)
{
}

OneDriveImagesDatabasePrivate::~OneDriveImagesDatabasePrivate()
{
}

void OneDriveImagesDatabasePrivate::clearCachedImages(QSqlQuery &query)
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

QList<OneDriveImage::ConstPtr> OneDriveImagesDatabasePrivate::queryImages(const QString &userId,
                                                                          const QString &albumId)
{
    Q_Q(OneDriveImagesDatabase);

    QList<OneDriveImage::ConstPtr> data;

    if (!userId.isEmpty() && !albumId.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Cannot select images in both an album and for an user";
        return data;
    }

    QString queryString = QLatin1String("SELECT images.imageId, images.albumId, "\
                                        "images.userId, images.createdTime, "\
                                        "images.updatedTime, images.imageName, images.width, "\
                                        "images.height, images.thumbnailUrl, images.imageUrl, "\
                                        "images.thumbnailFile, images.imageFile, images.description, images.accountId,"\
                                        "accounts.accountId "\
                                        "FROM images "\
                                        "INNER JOIN accounts "\
                                        "ON accounts.userId = images.userId%1 "\
                                        "ORDER BY images.updatedTime %2");

    if (!userId.isEmpty()) {
        queryString = queryString.arg(QLatin1String(" WHERE images.userId = :userId"),
                                      QLatin1String("DESC"));
    } else if (!albumId.isEmpty()){
        queryString = queryString.arg(QLatin1String(" WHERE images.albumId = :albumId"),
                                      QLatin1String("DESC"));
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
        data.append(OneDriveImage::create(query.value(0).toString(), query.value(1).toString(),
                                          query.value(2).toString(),
                                          QDateTime::fromTime_t(query.value(3).toUInt()),
                                          QDateTime::fromTime_t(query.value(4).toUInt()),
                                          query.value(5).toString(),
                                          query.value(6).toInt(), query.value(7).toInt(),
                                          query.value(8).toString(), query.value(9).toString(),
                                          query.value(10).toString(), query.value(11).toString(),
                                          query.value(12).toString(), query.value(13).toInt()));
    }

    return data;
}

bool operator==(const OneDriveUser::ConstPtr &user1, const OneDriveUser::ConstPtr &user2)
{
    return user1->userId() == user2->userId();
}

bool operator==(const OneDriveAlbum::ConstPtr &album1, const OneDriveAlbum::ConstPtr &album2)
{
    return album1->albumId() == album2->albumId() && album1->userId() == album2->userId();
}

bool operator==(const OneDriveImage::ConstPtr &image1, const OneDriveImage::ConstPtr &image2)
{
    return image1->imageId() == image2->imageId() && image1->albumId() == image2->albumId()
            && image1->userId() == image2->userId();
}

// Note
//
// Insertion operations needs to use write(), while Delete
// operations are automatically using transactions and
// don't need write().

OneDriveImagesDatabase::OneDriveImagesDatabase()
    : AbstractSocialCacheDatabase(*(new OneDriveImagesDatabasePrivate(this)))
{
}

OneDriveImagesDatabase::~OneDriveImagesDatabase()
{
    wait();
}

bool OneDriveImagesDatabase::syncAccount(int accountId, const QString &userId)
{
    Q_D(OneDriveImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.syncAccounts.insert(accountId, userId);

    return true;
}

void OneDriveImagesDatabase::purgeAccount(int accountId)
{
    Q_D(OneDriveImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.purgeAccounts.append(accountId);
}

// Returns the user but do not return a count
// if you want a count, better consider users
OneDriveUser::ConstPtr OneDriveImagesDatabase::user(const QString &userId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT userId, updatedTime, userName, accountId "
                "FROM users WHERE userId = :userId"));
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from users table:" << query.lastError();
        return OneDriveUser::Ptr();
    }

    // If we have the user, we have a result, otherwise we won't have the user
    if (!query.next()) {
        return OneDriveUser::Ptr();
    }

    OneDriveUser::ConstPtr user = OneDriveUser::create(
                                query.value(0).toString(),
                                QDateTime::fromTime_t(query.value(1).toUInt()),
                                query.value(2).toString(),
                                query.value(3).toInt());

    query.finish();

    return user;
}

void OneDriveImagesDatabase::addUser(const QString &userId, const QDateTime &updatedTime,
                                     const QString &userName, int accountId)
{
    Q_D(OneDriveImagesDatabase);

    OneDriveUser::Ptr user = OneDriveUser::create(userId, updatedTime, userName, accountId);

    QMutexLocker locker(&d->mutex);

    d->queue.insertUsers.insert(userId, user);
}

void OneDriveImagesDatabase::removeUser(const QString &userId)
{
    Q_D(OneDriveImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeUsers.append(userId);
}

QList<OneDriveUser::ConstPtr> OneDriveImagesDatabasePrivate::queryUsers() const
{
    QList<OneDriveUser::ConstPtr> data;

    QSqlQuery query = q_func()->prepare(QStringLiteral(
                "SELECT users.userId, users.updatedTime, users.userName, users.accountId,"
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
        data.append(OneDriveUser::create(query.value(0).toString(),
                                         QDateTime::fromTime_t(query.value(1).toUInt()),
                                         query.value(2).toString(), query.value(3).toInt(),
                                         query.value(4).toInt()));
    }

    return data;
}

QStringList OneDriveImagesDatabase::allAlbumIds(bool *ok) const
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


QMap<int,QString> OneDriveImagesDatabase::accounts(bool *ok) const
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


OneDriveAlbum::ConstPtr OneDriveImagesDatabase::album(const QString &albumId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT albumId, userId, createdTime, updatedTime, albumName, "
                "imageCount "
                "FROM albums WHERE albumId = :albumId"));
    query.bindValue(":albumId", albumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return OneDriveAlbum::Ptr();
    }

    // If we have the album, we have a result, otherwise we won't have the album
    if (!query.next()) {
        return OneDriveAlbum::Ptr();
    }

    OneDriveAlbum::ConstPtr album = OneDriveAlbum::create(query.value(0).toString(),  query.value(1).toString(),
                                 QDateTime::fromTime_t(query.value(2).toUInt()),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 query.value(4).toString(), query.value(5).toInt());

    query.finish();

    return album;
}

void OneDriveImagesDatabase::addAlbum(const QString &albumId, const QString &userId,
                                      const QDateTime &createdTime, const QDateTime &updatedTime,
                                      const QString &albumName, int imageCount)
{
    Q_D(OneDriveImagesDatabase);
    OneDriveAlbum::Ptr album = OneDriveAlbum::create(albumId, userId, createdTime,
                                                     updatedTime, albumName, imageCount);

    QMutexLocker locker(&d->mutex);

    d->queue.insertAlbums.insert(albumId, album);
}

void OneDriveImagesDatabase::removeAlbum(const QString &albumId)
{
    Q_D(OneDriveImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.removeAlbums.append(albumId);
}

void OneDriveImagesDatabase::removeAlbums(const QStringList &albumIds)
{
    Q_D(OneDriveImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeAlbums += albumIds;
}

QList<OneDriveAlbum::ConstPtr> OneDriveImagesDatabasePrivate::queryAlbums(const QString &userId) const
{
    QList<OneDriveAlbum::ConstPtr> data;

    QString queryString = QLatin1String("SELECT albumId, userId, createdTime, updatedTime, "\
                                        "albumName, imageCount "\
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
        data.append(OneDriveAlbum::create(query.value(0).toString(), query.value(1).toString(),
                                          QDateTime::fromTime_t(query.value(2).toUInt()),
                                          QDateTime::fromTime_t(query.value(3).toUInt()),
                                          query.value(4).toString(), query.value(5).toInt()));
    }

    return data;
}

QStringList OneDriveImagesDatabase::allImageIds(bool *ok) const
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

QStringList OneDriveImagesDatabase::imageIds(const QString &albumId, bool *ok) const
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

OneDriveImage::ConstPtr OneDriveImagesDatabase::image(const QString &imageId) const
{
    QSqlQuery query = prepare(
                "SELECT imageId, albumId, userId, createdTime, updatedTime, imageName, "
                "width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile, description, accountId "
                "FROM images WHERE imageId = :imageId");
    query.bindValue(":imageId", imageId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return OneDriveImage::Ptr();
    }

    // If we have the image, we have a result, otherwise we won't have the image
    if (!query.next()) {
        return OneDriveImage::Ptr();
    }

    return OneDriveImage::create(query.value(0).toString(),  query.value(1).toString(),
                                 query.value(2).toString(),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 QDateTime::fromTime_t(query.value(4).toUInt()),
                                 query.value(5).toString(),
                                 query.value(6).toInt(), query.value(7).toInt(),
                                 query.value(8).toString(), query.value(9).toString(),
                                 query.value(10).toString(), query.value(11).toString(),
                                 query.value(12).toString(), query.value(13).toInt());
}

void OneDriveImagesDatabase::removeImage(const QString &imageId)
{
    Q_D(OneDriveImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages.append(imageId);
}

void OneDriveImagesDatabase::removeImages(const QStringList &imageIds)
{
    Q_D(OneDriveImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages += imageIds;
}

void OneDriveImagesDatabase::addImage(const QString &imageId, const QString &albumId,
                                      const QString &userId, const QDateTime &createdTime,
                                      const QDateTime &updatedTime, const QString &imageName,
                                      int width, int height, const QString &thumbnailUrl,
                                      const QString &imageUrl, const QString &description,
                                      int accountId)
{
    Q_D(OneDriveImagesDatabase);
    OneDriveImage::Ptr image = OneDriveImage::create(imageId, albumId, userId, createdTime,
                                                     updatedTime, imageName, width, height,
                                                     thumbnailUrl, imageUrl, QString(), QString(),
                                                     description, accountId);
    QMutexLocker locker(&d->mutex);

    d->queue.insertImages.insert(imageId, image);
}

void OneDriveImagesDatabase::updateImageThumbnail(const QString &imageId,
                                                  const QString &thumbnailFile)
{
    Q_D(OneDriveImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateThumbnailFiles.insert(imageId, thumbnailFile);
}

void OneDriveImagesDatabase::updateImageFile(const QString &imageId, const QString &imageFile)
{
    Q_D(OneDriveImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateImageFiles.insert(imageId, imageFile);
}

void OneDriveImagesDatabase::commit()
{
    executeWrite();
}

QList<OneDriveUser::ConstPtr> OneDriveImagesDatabase::users() const
{
    return d_func()->result.users;
}

QList<OneDriveImage::ConstPtr> OneDriveImagesDatabase::images() const
{
    return d_func()->result.images;
}

QList<OneDriveAlbum::ConstPtr> OneDriveImagesDatabase::albums() const
{
    return d_func()->result.albums;
}

void OneDriveImagesDatabase::queryUsers()
{
    Q_D(OneDriveImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = OneDriveImagesDatabasePrivate::Users;
    }
    executeRead();
}

void OneDriveImagesDatabase::queryAlbums(const QString &userId)
{
    Q_D(OneDriveImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = OneDriveImagesDatabasePrivate::Albums;
        d->query.id = userId;
    }
    executeRead();
}

void OneDriveImagesDatabase::queryUserImages(const QString &userId)
{
    Q_D(OneDriveImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = OneDriveImagesDatabasePrivate::UserImages;
        d->query.id = userId;
    }
    executeRead();
}

void OneDriveImagesDatabase::queryAlbumImages(const QString &albumId)
{
    Q_D(OneDriveImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = OneDriveImagesDatabasePrivate::AlbumImages;
        d->query.id = albumId;
    }
    executeRead();
}

bool OneDriveImagesDatabase::read()
{
    Q_D(OneDriveImagesDatabase);
    QMutexLocker locker(&d->mutex);

    switch (d->query.type) {
    case OneDriveImagesDatabasePrivate::Users: {
        locker.unlock();
        QList<OneDriveUser::ConstPtr> users = d->queryUsers();
        locker.relock();
        d->query.users = users;
        return true;
    }
    case OneDriveImagesDatabasePrivate::Albums: {
        const QString userId = d->query.id;
        locker.unlock();
        QList<OneDriveAlbum::ConstPtr> albums = d->queryAlbums(userId);
        locker.relock();
        d->query.albums = albums;
        return true;
    }
    case OneDriveImagesDatabasePrivate::UserImages:
    case OneDriveImagesDatabasePrivate::AlbumImages: {
        const QString userId = d->query.type == OneDriveImagesDatabasePrivate::UserImages
                ? d->query.id
                : QString();
        const QString albumId = d->query.type == OneDriveImagesDatabasePrivate::AlbumImages
                ? d->query.id
                : QString();
        locker.unlock();
        QList<OneDriveImage::ConstPtr> images = d->queryImages(userId, albumId);
        locker.relock();
        d->query.images = images;
        return true;
    }
    default:
        return false;
    }
}

void OneDriveImagesDatabase::readFinished()
{
    Q_D(OneDriveImagesDatabase);
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

bool OneDriveImagesDatabase::write()
{
    Q_D(OneDriveImagesDatabase);
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

    const QMap<QString, OneDriveUser::ConstPtr> insertUsers = d->queue.insertUsers;
    const QMap<QString, OneDriveAlbum::ConstPtr> insertAlbums = d->queue.insertAlbums;
    const QMap<QString, OneDriveImage::ConstPtr> insertImages = d->queue.insertImages;

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
        QVariantList accountIds;

        Q_FOREACH (const OneDriveUser::ConstPtr &user, insertUsers) {
            userIds.append(user->userId());
            updatedTimes.append(user->updatedTime().toTime_t());
            usernames.append(user->userName());
            accountIds.append(user->accountId());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO users ("
                    " userId, updatedTime, userName, accountId) "
                    "VALUES ("
                    " :userId, :updatedTime, :userName, :accountId);"));
        query.bindValue(QStringLiteral(":userId"), userIds);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":userName"), usernames);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertAlbums.isEmpty()) {
        QVariantList albumIds, userIds;
        QVariantList createdTimes, updatedTimes;
        QVariantList albumNames;
        QVariantList imageCounts;

        Q_FOREACH (const OneDriveAlbum::ConstPtr &album, insertAlbums) {
            albumIds.append(album->albumId());
            userIds.append(album->userId());
            createdTimes.append(album->createdTime().toTime_t());
            updatedTimes.append(album->updatedTime().toTime_t());
            albumNames.append(album->albumName());
            imageCounts.append(album->imageCount());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO albums("
                    " albumId, userId, createdTime, updatedTime, albumName, imageCount) "
                    "VALUES ("
                    " :albumId, :userId, :createdTime, :updatedTime, :albumName, :imageCount)"));
        query.bindValue(QStringLiteral(":albumId"), albumIds);
        query.bindValue(QStringLiteral(":userId"), userIds);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":albumName"), albumNames);
        query.bindValue(QStringLiteral(":imageCount"), imageCounts);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertImages.isEmpty()) {
        QVariantList imageIds, albumIds, userIds;
        QVariantList createdTimes, updatedTimes;
        QVariantList imageNames;
        QVariantList widths, heights;
        QVariantList thumbnailUrls, imageUrls;
        QVariantList thumbnailFiles, imageFiles;
        QVariantList descriptions, accountIds;

        Q_FOREACH (const OneDriveImage::ConstPtr &image, insertImages) {
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
            descriptions.append(image->description());
            accountIds.append(image->accountId());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO images ("
                    " imageId, albumId, userId, createdTime, updatedTime, imageName,"
                    " width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile, description, accountId) "
                    "VALUES ("
                    " :imageId, :albumId, :userId, :createdTime, :updatedTime, :imageName,"
                    " :width, :height, :thumbnailUrl, :imageUrl, :thumbnailFile, :imageFile, :description, :accountId)"));
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
        query.bindValue(QStringLiteral(":description"), descriptions);
        query.bindValue(QStringLiteral(":accountId"), accountIds);
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

bool OneDriveImagesDatabase::createTables(QSqlDatabase database) const
{
    // create the onedrive image db tables
    // images = imageId, albumId, userId, createdTime, updatedTime, imageName, width, height,
    //          thumbnailUrl, imageUrl, thumbnailFile, imageFile
    // albums = albumId, userId, createdTime, updatedTime, albumName, imageCount
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
                   "description TEXT,"
                   "accountId INTEGER)");
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
                   "imageCount INTEGER)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create albums table:" << query.lastError().text();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS users ("
                   "userId TEXT UNIQUE PRIMARY KEY,"
                   "updatedTime INTEGER,"
                   "userName TEXT,"
                   "accountId INTEGER)");
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

bool OneDriveImagesDatabase::dropTables(QSqlDatabase database) const
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
