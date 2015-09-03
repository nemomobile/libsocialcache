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

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QFile>

#include <QtDebug>

static const char *DB_NAME = "facebook.db";
static const int VERSION = 3;

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
    enum QueryType {
        Users,
        Albums,
        UserImages,
        AlbumImages
    };

    explicit FacebookImagesDatabasePrivate(FacebookImagesDatabase *q);
    ~FacebookImagesDatabasePrivate();

private:
    Q_DECLARE_PUBLIC(FacebookImagesDatabase)

    static void clearCachedImages(QSqlQuery &query);

    QList<FacebookUser::ConstPtr> queryUsers() const;
    QList<FacebookAlbum::ConstPtr> queryAlbums(const QString &fbUserId) const;

    QList<FacebookImage::ConstPtr> queryImages(const QString &fbUserId, const QString &fbAlbumId);

    struct {
        QList<int> purgeAccounts;

        QStringList removeUsers;
        QStringList removeAlbums;
        QStringList removeImages;

        QMap<QString, FacebookUser::ConstPtr> insertUsers;
        QMap<QString, FacebookAlbum::ConstPtr> insertAlbums;
        QMap<QString, FacebookImage::ConstPtr> insertImages;

        QMap<int, QString> syncAccounts;

        QMap<QString, QString> updateThumbnailFiles;
        QMap<QString, QString> updateImageFiles;
    } queue;

    struct {
        QueryType type;
        QString id;
        QList<FacebookUser::ConstPtr> users;
        QList<FacebookAlbum::ConstPtr> albums;
        QList<FacebookImage::ConstPtr> images;
    } query;

    struct {
        QList<FacebookUser::ConstPtr> users;
        QList<FacebookAlbum::ConstPtr> albums;
        QList<FacebookImage::ConstPtr> images;
    } result;
};

FacebookImagesDatabasePrivate::FacebookImagesDatabasePrivate(FacebookImagesDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
            SocialSyncInterface::dataType(SocialSyncInterface::Images),
            QLatin1String(DB_NAME),
            VERSION)
{
}

FacebookImagesDatabasePrivate::~FacebookImagesDatabasePrivate()
{
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
    Q_Q(FacebookImagesDatabase);

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

    QSqlQuery query = q->prepare(queryString);
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
    wait();
}

bool FacebookImagesDatabase::syncAccount(int accountId, const QString &fbUserId)
{
    Q_D(FacebookImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.syncAccounts.insert(accountId, fbUserId);

    return true;
}

void FacebookImagesDatabase::purgeAccount(int accountId)
{
    Q_D(FacebookImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.purgeAccounts.append(accountId);
}

// Returns the user but do not return a count
// if you want a count, better consider users
FacebookUser::ConstPtr FacebookImagesDatabase::user(const QString &fbUserId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT fbUserId, updatedTime, userName "
                "FROM users WHERE fbUserId = :fbUserId"));
    query.bindValue(":fbUserId", fbUserId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from users table:" << query.lastError();
        return FacebookUser::Ptr();
    }

    // If we have the user, we have a result, otherwise we won't have the user
    if (!query.next()) {
        return FacebookUser::Ptr();
    }

    FacebookUser::ConstPtr user = FacebookUser::create(
                                query.value(0).toString(),
                                QDateTime::fromTime_t(query.value(1).toUInt()),
                                query.value(2).toString());

    query.finish();

    return user;
}

void FacebookImagesDatabase::addUser(const QString &fbUserId, const QDateTime &updatedTime,
                                     const QString &userName)
{
    Q_D(FacebookImagesDatabase);

    FacebookUser::Ptr user = FacebookUser::create(fbUserId, updatedTime, userName);

    QMutexLocker locker(&d->mutex);

    d->queue.insertUsers.insert(fbUserId, user);
}

void FacebookImagesDatabase::removeUser(const QString &fbUserId)
{
    Q_D(FacebookImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeUsers.append(fbUserId);
}

QList<FacebookUser::ConstPtr> FacebookImagesDatabasePrivate::queryUsers() const
{
    QList<FacebookUser::ConstPtr> data;

    QSqlQuery query = q_func()->prepare(QStringLiteral(
                "SELECT users.fbUserId, users.updatedTime, users.userName, "
                "SUM(albums.imageCount) as count "
                "FROM users "
                "LEFT JOIN albums ON albums.fbUserId = users.fbUserId "
                "GROUP BY users.fbUserId "
                "ORDER BY users.fbUserId"));
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
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT DISTINCT fbAlbumId "
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


QMap<int,QString> FacebookImagesDatabase::accounts(bool *ok) const
{
    if (ok) {
        *ok = false;
    }

    QMap<int,QString> result;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT accountId, fbUserId "
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


FacebookAlbum::ConstPtr FacebookImagesDatabase::album(const QString &fbAlbumId) const
{
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT fbAlbumId, fbUserId, createdTime, updatedTime, albumName, "
                "imageCount "
                "FROM albums WHERE fbAlbumId = :fbAlbumId"));
    query.bindValue(":fbAlbumId", fbAlbumId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from albums table:" << query.lastError();
        return FacebookAlbum::Ptr();
    }

    // If we have the album, we have a result, otherwise we won't have the album
    if (!query.next()) {
        return FacebookAlbum::Ptr();
    }

    FacebookAlbum::ConstPtr album = FacebookAlbum::create(query.value(0).toString(),  query.value(1).toString(),
                                 QDateTime::fromTime_t(query.value(2).toUInt()),
                                 QDateTime::fromTime_t(query.value(3).toUInt()),
                                 query.value(4).toString(), query.value(5).toInt());

    query.finish();

    return album;
}

void FacebookImagesDatabase::addAlbum(const QString &fbAlbumId, const QString &fbUserId,
                                      const QDateTime &createdTime, const QDateTime &updatedTime,
                                      const QString &albumName, int imageCount)
{
    Q_D(FacebookImagesDatabase);
    FacebookAlbum::Ptr album = FacebookAlbum::create(fbAlbumId, fbUserId, createdTime,
                                                     updatedTime, albumName, imageCount);

    QMutexLocker locker(&d->mutex);

    d->queue.insertAlbums.insert(fbAlbumId, album);
}

void FacebookImagesDatabase::removeAlbum(const QString &fbAlbumId)
{
    Q_D(FacebookImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.removeAlbums.append(fbAlbumId);
}

void FacebookImagesDatabase::removeAlbums(const QStringList &fbAlbumIds)
{
    Q_D(FacebookImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeAlbums += fbAlbumIds;
}

QList<FacebookAlbum::ConstPtr> FacebookImagesDatabasePrivate::queryAlbums(const QString &fbUserId) const
{
    QList<FacebookAlbum::ConstPtr> data;

    QString queryString = QLatin1String("SELECT fbAlbumId, fbUserId, createdTime, updatedTime, "\
                                        "albumName, imageCount "\
                                        "FROM albums%1 ORDER BY updatedTime DESC");
    if (!fbUserId.isEmpty()) {
        queryString = queryString.arg(QLatin1String(" WHERE fbUserId = :fbUserId"));
    } else {
        queryString = queryString.arg(QString());
    }

    QSqlQuery query = q_func()->prepare(queryString);
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
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT DISTINCT fbImageId "
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

QStringList FacebookImagesDatabase::imageIds(const QString &fbAlbumId, bool *ok) const
{
    if (ok) {
        *ok = false;
    }

    QStringList ids;
    QSqlQuery query = prepare(QStringLiteral(
                "SELECT DISTINCT fbImageId "
                "FROM images WHERE fbAlbumId = :fbAlbumId"));
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
    QSqlQuery query = prepare(
                "SELECT fbImageId, fbAlbumId, fbUserId, createdTime, updatedTime, imageName, "
                "width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile "
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
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages.append(fbImageId);
}

void FacebookImagesDatabase::removeImages(const QStringList &fbImageIds)
{
    Q_D(FacebookImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages += fbImageIds;
}

void FacebookImagesDatabase::addImage(const QString &fbImageId, const QString &fbAlbumId,
                                      const QString &fbUserId, const QDateTime &createdTime,
                                      const QDateTime &updatedTime, const QString &imageName,
                                      int width, int height, const QString &thumbnailUrl,
                                      const QString &imageUrl, const QString &thumbnailFile,
                                      const QString &imageFile)
{
    Q_D(FacebookImagesDatabase);
    FacebookImage::Ptr image = FacebookImage::create(fbImageId, fbAlbumId, fbUserId, createdTime,
                                                     updatedTime, imageName, width, height,
                                                     thumbnailUrl, imageUrl, thumbnailFile,
                                                     imageFile);
    QMutexLocker locker(&d->mutex);

    d->queue.insertImages.insert(fbImageId, image);
}

void FacebookImagesDatabase::updateImageThumbnail(const QString &fbImageId,
                                                  const QString &thumbnailFile)
{
    Q_D(FacebookImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateThumbnailFiles.insert(fbImageId, thumbnailFile);
}

void FacebookImagesDatabase::updateImageFile(const QString &fbImageId, const QString &imageFile)
{
    Q_D(FacebookImagesDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.updateImageFiles.insert(fbImageId, imageFile);
}

void FacebookImagesDatabase::commit()
{
    executeWrite();
}

QList<FacebookUser::ConstPtr> FacebookImagesDatabase::users() const
{
    return d_func()->result.users;
}

QList<FacebookImage::ConstPtr> FacebookImagesDatabase::images() const
{
    return d_func()->result.images;
}

QList<FacebookAlbum::ConstPtr> FacebookImagesDatabase::albums() const
{
    return d_func()->result.albums;
}

void FacebookImagesDatabase::queryUsers()
{
    Q_D(FacebookImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = FacebookImagesDatabasePrivate::Users;
    }
    executeRead();
}

void FacebookImagesDatabase::queryAlbums(const QString &userId)
{
    Q_D(FacebookImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = FacebookImagesDatabasePrivate::Albums;
        d->query.id = userId;
    }
    executeRead();
}

void FacebookImagesDatabase::queryUserImages(const QString &userId)
{
    Q_D(FacebookImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = FacebookImagesDatabasePrivate::UserImages;
        d->query.id = userId;
    }
    executeRead();
}

void FacebookImagesDatabase::queryAlbumImages(const QString &albumId)
{
    Q_D(FacebookImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = FacebookImagesDatabasePrivate::AlbumImages;
        d->query.id = albumId;
    }
    executeRead();
}

bool FacebookImagesDatabase::read()
{
    Q_D(FacebookImagesDatabase);
    QMutexLocker locker(&d->mutex);

    switch (d->query.type) {
    case FacebookImagesDatabasePrivate::Users: {
        locker.unlock();
        QList<FacebookUser::ConstPtr> users = d->queryUsers();
        locker.relock();
        d->query.users = users;
        return true;
    }
    case FacebookImagesDatabasePrivate::Albums: {
        const QString userId = d->query.id;
        locker.unlock();
        QList<FacebookAlbum::ConstPtr> albums = d->queryAlbums(userId);
        locker.relock();
        d->query.albums = albums;
        return true;
    }
    case FacebookImagesDatabasePrivate::UserImages:
    case FacebookImagesDatabasePrivate::AlbumImages: {
        const QString userId = d->query.type == FacebookImagesDatabasePrivate::UserImages
                ? d->query.id
                : QString();
        const QString albumId = d->query.type == FacebookImagesDatabasePrivate::AlbumImages
                ? d->query.id
                : QString();
        locker.unlock();
        QList<FacebookImage::ConstPtr> images = d->queryImages(userId, albumId);
        locker.relock();
        d->query.images = images;
        return true;
    }
    default:
        return false;
    }
}

void FacebookImagesDatabase::readFinished()
{
    Q_D(FacebookImagesDatabase);
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

bool FacebookImagesDatabase::write()
{
    Q_D(FacebookImagesDatabase);
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

    const QMap<QString, FacebookUser::ConstPtr> insertUsers = d->queue.insertUsers;
    const QMap<QString, FacebookAlbum::ConstPtr> insertAlbums = d->queue.insertAlbums;
    const QMap<QString, FacebookImage::ConstPtr> insertImages = d->queue.insertImages;

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
                    "SELECT fbUserId "
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
                    "WHERE fbUserId = :fbUserId"));
        Q_FOREACH (const QString &userId, removeUsers) {
            userIds.append(userId);

            query.bindValue(QStringLiteral(":fbUserId"), userId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM accounts "
                    "WHERE fbUserId = :fbUserId"));
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM users "
                    "WHERE fbUserId = :fbUserId"));
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE fbUserId = :fbUserId"));
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE fbUserId = :fbUserId"));
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeAlbums.isEmpty()) {
        QVariantList albumIds;

        query = prepare(QStringLiteral(
                    "SELECT thumbnailFile, imageFile "
                    "FROM images "
                    "WHERE fbAlbumId = :fbAlbumId"));
        Q_FOREACH (const QString &albumId, removeAlbums) {
            albumIds.append(albumId);

            query.bindValue(QStringLiteral(":fbAlbumId"), albumId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE fbAlbumId = :fbAlbumId"));
        query.bindValue(QStringLiteral(":fbAlbumId"), albumIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE fbAlbumId = :fbAlbumId"));
        query.bindValue(QStringLiteral(":fbAlbumId"), albumIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeImages.isEmpty()) {
        QVariantList imageIds;

        query = prepare(QStringLiteral(
                    "SELECT thumbnailFile, imageFile "
                    "FROM images "
                    "WHERE fbImageId = :fbImageId"));
        Q_FOREACH (const QString &imageId, removeImages) {
            imageIds.append(imageId);

            query.bindValue(QStringLiteral(":fbImageId"), imageId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE fbImageId = :fbImageId"));
        query.bindValue(QStringLiteral(":fbImageId"), imageIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertUsers.isEmpty()) {
        QVariantList userIds;
        QVariantList updatedTimes;
        QVariantList usernames;

        Q_FOREACH (const FacebookUser::ConstPtr &user, insertUsers) {
            userIds.append(user->fbUserId());
            updatedTimes.append(user->updatedTime().toTime_t());
            usernames.append(user->userName());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO users ("
                    " fbUserId, updatedTime, userName) "
                    "VALUES ("
                    " :fbUserId, :updatedTime, :userName);"));
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":userName"), usernames);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertAlbums.isEmpty()) {
        QVariantList albumIds, userIds;
        QVariantList createdTimes, updatedTimes;
        QVariantList albumNames;
        QVariantList imageCounts;

        Q_FOREACH (const FacebookAlbum::ConstPtr &album, insertAlbums) {
            albumIds.append(album->fbAlbumId());
            userIds.append(album->fbUserId());
            createdTimes.append(album->createdTime().toTime_t());
            updatedTimes.append(album->updatedTime().toTime_t());
            albumNames.append(album->albumName());
            imageCounts.append(album->imageCount());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO albums("
                    " fbAlbumId, fbUserId, createdTime, updatedTime, albumName, imageCount) "
                    "VALUES ("
                    " :fbAlbumId, :fbUserId, :createdTime, :updatedTime, :albumName, :imageCount)"));
        query.bindValue(QStringLiteral(":fbAlbumId"), albumIds);
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
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

        Q_FOREACH (const FacebookImage::ConstPtr &image, insertImages) {
            imageIds.append(image->fbImageId());
            albumIds.append(image->fbAlbumId());
            userIds.append(image->fbUserId());
            createdTimes.append(image->createdTime().toTime_t());
            updatedTimes.append(image->updatedTime().toTime_t());
            imageNames.append(image->imageName());
            widths.append(image->width());
            heights.append(image->height());
            thumbnailUrls.append(image->thumbnailUrl());
            imageUrls.append(image->imageUrl());
            thumbnailFiles.append(image->thumbnailFile());
            imageFiles.append(image->imageFile());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO images ("
                    " fbImageId, fbAlbumId, fbUserId, createdTime, updatedTime, imageName,"
                    " width, height, thumbnailUrl, imageUrl, thumbnailFile, imageFile) "
                    "VALUES ("
                    " :fbImageId, :fbAlbumId, :fbUserId, :createdTime, :updatedTime, :imageName,"
                    " :width, :height, :thumbnailUrl, :imageUrl, :thumbnailFile, :imageFile)"));
        query.bindValue(QStringLiteral(":fbImageId"), imageIds);
        query.bindValue(QStringLiteral(":fbAlbumId"), albumIds);
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);
        query.bindValue(QStringLiteral(":updatedTime"), updatedTimes);
        query.bindValue(QStringLiteral(":imageName"), imageNames);
        query.bindValue(QStringLiteral(":width"), widths);
        query.bindValue(QStringLiteral(":height"), heights);
        query.bindValue(QStringLiteral(":thumbnailUrl"), thumbnailUrls);
        query.bindValue(QStringLiteral(":imageUrl"), imageUrls);
        query.bindValue(QStringLiteral(":thumbnailFile"), thumbnailFiles);
        query.bindValue(QStringLiteral(":imageFile"), imageFiles);
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
                    " accountId, fbUserId) "
                    "VALUES("
                    " :accountId, :fbUserId)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":fbUserId"), userIds);
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
                    "WHERE fbImageId = :fbImageId"));
        query.bindValue(QStringLiteral(":thumbnailFile"), thumbnailFiles);
        query.bindValue(QStringLiteral(":fbImageId"), imageIds);
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
                    "WHERE fbImageId = :fbImageId"));
        query.bindValue(QStringLiteral(":imageFile"), imageFiles);
        query.bindValue(QStringLiteral(":fbImageId"), imageIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool FacebookImagesDatabase::createTables(QSqlDatabase database) const
{
    // create the facebook image db tables
    // images = fbImageId, fbAlbumId, fbUserId, createdTime, updatedTime, imageName, width, height,
    //          thumbnailUrl, imageUrl, thumbnailFile, imageFile
    // albums = fbAlbumId, fbUserId, createdTime, updatedTime, albumName, imageCount
    // users = fbUserId, updatedTime, userName, thumbnailUrl, imageUrl, thumbnailFile, imageFile
    QSqlQuery query(database);
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
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS users ("
                   "fbUserId TEXT UNIQUE PRIMARY KEY,"
                   "updatedTime INTEGER,"
                   "userName TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create users table:" << query.lastError().text();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS accounts ("
                   "accountId INTEGER UNIQUE PRIMARY KEY,"
                   "fbUserId TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create accounts table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool FacebookImagesDatabase::dropTables(QSqlDatabase database) const
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
