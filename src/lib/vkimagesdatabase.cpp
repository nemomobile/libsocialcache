/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jolla.com>
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

#include "vkimagesdatabase.h"
#include "abstractsocialcachedatabase.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QFile>

#include <QtDebug>

static const char *DB_NAME = "vk.db";
static const int VERSION = 1;

VKUser::VKUser(const QString &id, const QString &first_name, const QString &last_name,
               const QString &photo_src, const QString &photo_file, int accountId)
    : id(id), first_name(first_name), last_name(last_name), photo_src(photo_src)
    , photo_file(photo_file), accountId(accountId), photos_count(0)
{
}

bool VKUser::operator==(const VKUser &other) const
{
    return id == other.id
        && first_name == other.first_name
        && last_name == other.last_name
        && photo_src == other.photo_src
        && photo_file == other.photo_file
        && accountId == other.accountId;
}

VKAlbum::VKAlbum(const QString &id, const QString &owner_id, const QString &title,
                 const QString &description, const QString &thumb_src,
                 const QString &thumb_file, int size, int created, int updated,
                 int accountId)
    : id(id), owner_id(owner_id), title(title), description(description)
    , thumb_src(thumb_src), thumb_file(thumb_file), size(size)
    , created(created), updated(updated), accountId(accountId)
{
}

bool VKAlbum::operator==(const VKAlbum &other) const
{
    return id == other.id
        && owner_id == other.owner_id
        && title == other.title
        && description == other.description
        && thumb_src == other.thumb_src
        && thumb_file == other.thumb_file
        && size == other.size
        && created == other.created
        && updated == other.updated
        && accountId == other.accountId;
}

VKImage::VKImage(const QString &id, const QString &album_id, const QString &owner_id,
        const QString &text, const QString &thumb_src, const QString &photo_src,
        const QString &thumb_file, const QString &photo_file,
        int width, int height, int date, int accountId)
    : id(id), album_id(album_id), owner_id(owner_id), text(text)
    , thumb_src(thumb_src), photo_src(photo_src), thumb_file(thumb_file),
      photo_file(photo_file), width(width), height(height)
    , date(date), accountId(accountId)
{
}

bool VKImage::operator==(const VKImage &other) const
{
    return id == other.id
        && album_id == other.album_id
        && owner_id == other.owner_id
        && text == other.text
        && thumb_src == other.thumb_src
        && photo_src == other.photo_src
        && thumb_file == other.thumb_file
        && photo_file == other.photo_file;
}

class VKImagesDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    enum QueryType {
        Users,
        Albums,
        Images
    };

    explicit VKImagesDatabasePrivate(VKImagesDatabase *q);
    ~VKImagesDatabasePrivate();

private:
    Q_DECLARE_PUBLIC(VKImagesDatabase)

    static void clearCachedImages(QSqlQuery &query);

    QList<VKUser> queryUsers(int accountId) const;
    QList<VKAlbum> queryAlbums(int accountId, const QString &vkUserId, const QString &vkAlbumId) const;
    QList<VKImage> queryImages(int accountId, const QString &vkUserId, const QString &vkAlbumId, const QString &vkImageId) const;

    struct {
        QList<int> purgeAccounts;

        QList<VKUser> removeUsers;
        QList<VKAlbum> removeAlbums;
        QList<VKImage> removeImages;

        QList<VKUser> insertUsers;
        QList<VKAlbum> insertAlbums;
        QList<VKImage> insertImages;

        QList<QPair<VKImage, QString> > updateThumbnailFiles;
        QList<QPair<VKImage, QString> > updateImageFiles;
    } queue;

    struct {
        QueryType type;
        int accountId;
        QString ownerId;
        QString albumId;
        QList<VKUser> users;
        QList<VKAlbum> albums;
        QList<VKImage> images;
    } query;

    struct {
        QList<VKUser> users;
        QList<VKAlbum> albums;
        QList<VKImage> images;
    } result;
};

VKImagesDatabasePrivate::VKImagesDatabasePrivate(VKImagesDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::VK),
            SocialSyncInterface::dataType(SocialSyncInterface::Images),
            QLatin1String(DB_NAME),
            VERSION)
{
}

VKImagesDatabasePrivate::~VKImagesDatabasePrivate()
{
}

void VKImagesDatabasePrivate::clearCachedImages(QSqlQuery &query)
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

QList<VKUser> VKImagesDatabasePrivate::queryUsers(int accountId) const
{
    QList<VKUser> retn;
    QString queryString = QStringLiteral(
                "SELECT accountId, vkUserId, first_name, last_name, photo_src, photo_file "
                "FROM users ");
    if (accountId != 0) {
        queryString.append(QStringLiteral("WHERE accountId = :accountId "));
    }
    queryString.append(QStringLiteral("ORDER BY first_name ASC"));

    QSqlQuery query = q_func()->prepare(queryString);
    if (accountId != 0) {
        query.bindValue(QStringLiteral(":accountId"), accountId);
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query users:" << query.lastError().text();
        return retn;
    }

    while (query.next()) {
        VKUser user(query.value(1).toString(), query.value(2).toString(),
                    query.value(3).toString(), query.value(4).toString(),
                    query.value(5).toString(), query.value(0).toInt());

        // now determine the (transient) photos count for the user.
        QString countQueryString = QStringLiteral(
                "SELECT count(*) FROM images WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId;");
        QSqlQuery countQuery = q_func()->prepare(countQueryString);
        countQuery.bindValue(QStringLiteral(":accountId"), user.accountId);
        countQuery.bindValue(QStringLiteral(":vkOwnerId"), user.id);
        if (!countQuery.exec()) {
            qWarning() << Q_FUNC_INFO << "Failed to query user photos count:" << query.lastError().text();
            return retn;
        }
        if (query.next()) {
            user.photos_count = query.value(0).toInt();
        }

        retn.append(user);
    }

    query.finish();
    return retn;
}

QList<VKAlbum> VKImagesDatabasePrivate::queryAlbums(int accountId, const QString &vkOwnerId, const QString &vkAlbumId) const
{
    QList<VKAlbum> retn;
    QString queryString = QStringLiteral("SELECT accountId,"
                                               " vkOwnerId,"
                                               " vkAlbumId,"
                                               " title,"
                                               " description,"
                                               " thumb_src,"
                                               " thumb_file,"
                                               " size,"
                                               " created,"
                                               " updated "
                                        "FROM albums ");
    if (accountId != 0) {
        queryString.append(QStringLiteral("WHERE accountId = :accountId "));
        if (!vkOwnerId.isEmpty()) {
            queryString.append(QStringLiteral("AND vkOwnerId = :vkOwnerId "));
            if (!vkAlbumId.isEmpty()) {
                queryString.append(QStringLiteral("AND vkAlbumId = :vkAlbumId "));
            }
        }
    }
    queryString.append(QStringLiteral("ORDER BY vkOwnerId DESC, created ASC"));

    QSqlQuery query = q_func()->prepare(queryString);
    if (accountId != 0) {
        query.bindValue(":accountId", accountId);
        if (!vkOwnerId.isEmpty()) {
            query.bindValue(":vkOwnerId", vkOwnerId);
            if (!vkAlbumId.isEmpty()) {
                query.bindValue(":vkAlbumId", vkAlbumId);
            }
        }
    }
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query albums:" << query.lastError().text();
        return retn;
    }

    while (query.next()) {
        retn.append(VKAlbum(query.value(2).toString(), query.value(1).toString(),
                            query.value(3).toString(), query.value(4).toString(),
                            query.value(5).toString(), query.value(6).toString(),
                            query.value(7).toInt(), query.value(8).toInt(),
                            query.value(9).toInt(), query.value(0).toInt()));
    }

    query.finish();
    return retn;
}

QList<VKImage> VKImagesDatabasePrivate::queryImages(int accountId,
                                                    const QString &vkOwnerId,
                                                    const QString &vkAlbumId,
                                                    const QString &vkImageId) const
{
    QList<VKImage> retn;
    QString queryString = QLatin1String("SELECT vkImageId,"
                                              " vkAlbumId,"
                                              " vkOwnerId,"
                                              " text,"
                                              " thumb_src,"
                                              " photo_src,"
                                              " thumb_file,"
                                              " photo_file,"
                                              " width,"
                                              " height,"
                                              " date,"
                                              " accountId "
                                        "FROM images ");
    if (accountId > 0) {
        queryString.append(QLatin1String("WHERE accountId = :accountId "));
        if (!vkOwnerId.isEmpty()) {
            queryString.append(QLatin1String("AND vkOwnerId = :vkOwnerId "));
            if (!vkAlbumId.isEmpty()) {
                queryString.append(QLatin1String("AND vkAlbumId = :vkAlbumId "));
                if (!vkImageId.isEmpty()) {
                    queryString.append(QLatin1String("AND vkImageId = :vkImageId "));
                }
            }
        }
    }
    queryString.append(QLatin1String("ORDER BY date ASC"));
    QSqlQuery query = q_func()->prepare(queryString);
    if (accountId > 0) {
        query.bindValue(":accountId", accountId);
        if (!vkOwnerId.isEmpty()) {
            query.bindValue(":vkOwnerId", vkOwnerId);
            if (!vkAlbumId.isEmpty()) {
                query.bindValue(":vkAlbumId", vkAlbumId);
                if (!vkImageId.isEmpty()) {
                    query.bindValue(":vkImageId", vkImageId);
                }
            }
        }
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query images:" << query.lastError().text();
        return retn;
    }
    while (query.next()) {
        retn.append(VKImage(query.value(0).toString(), query.value(1).toString(),
                            query.value(2).toString(), query.value(3).toString(),
                            query.value(4).toString(), query.value(5).toString(),
                            query.value(6).toString(), query.value(7).toString(),
                            query.value(8).toInt(), query.value(9).toInt(),
                            query.value(10).toInt(), query.value(11).toInt()));
    }

    query.finish();
    return retn;
}

//---------------------------------------------------------------------

VKImagesDatabase::VKImagesDatabase()
    : AbstractSocialCacheDatabase(*(new VKImagesDatabasePrivate(this)))
{
}

VKImagesDatabase::~VKImagesDatabase()
{
    wait();
}

void VKImagesDatabase::addUser(const VKUser &vkUser)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.insertUsers.append(vkUser);
}

void VKImagesDatabase::removeUser(const VKUser &vkUser)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.removeUsers.append(vkUser);
}

void VKImagesDatabase::addAlbum(const VKAlbum &vkAlbum)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.insertAlbums.append(vkAlbum);
}

void VKImagesDatabase::addAlbums(const QList<VKAlbum> &vkAlbums)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.insertAlbums.append(vkAlbums);
}

void VKImagesDatabase::removeAlbum(const VKAlbum &vkAlbum)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.removeAlbums.append(vkAlbum);
}

void VKImagesDatabase::removeAlbums(const QList<VKAlbum> &vkAlbums)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.removeAlbums += vkAlbums;
}

void VKImagesDatabase::addImage(const VKImage &vkImage)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.insertImages.append(vkImage);
}

void VKImagesDatabase::addImages(const QList<VKImage> &vkImages)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.insertImages.append(vkImages);
}

void VKImagesDatabase::updateImageThumbnail(const VKImage &vkImage, const QString &thumb_file)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.updateThumbnailFiles.append(qMakePair(vkImage, thumb_file));
}

void VKImagesDatabase::updateImageFile(const VKImage &vkImage, const QString &photo_file)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.updateImageFiles.append(qMakePair(vkImage, photo_file));
}

void VKImagesDatabase::removeImage(const VKImage &vkImage)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.removeImages.append(vkImage);
}

void VKImagesDatabase::removeImages(const QList<VKImage> &vkImages)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.removeImages += vkImages;
}

void VKImagesDatabase::purgeAccount(int accountId)
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);
    d->queue.purgeAccounts.append(accountId);
}

void VKImagesDatabase::commit()
{
    executeWrite();
}

VKUser VKImagesDatabase::user(int accountId) const
{
    Q_D(const VKImagesDatabase);
    QList<VKUser> users = d->queryUsers(accountId);
    if (users.size() == 0) {
        qWarning() << Q_FUNC_INFO << "No user in database for account:" << accountId;
        return VKUser();
    } else if (users.size() > 1) {
        qWarning() << Q_FUNC_INFO << "Multiple users in database for account:" << accountId;
        // shouldn't happen, but return the first one anyway.
    }
    return users[0];
}

VKAlbum VKImagesDatabase::album(int accountId, const QString &vkUserId, const QString &vkAlbumId) const
{
    Q_D(const VKImagesDatabase);
    QList<VKAlbum> albums = d->queryAlbums(accountId, vkUserId, vkAlbumId);
    if (albums.size() == 0) {
        qWarning() << Q_FUNC_INFO << "No album in database for account:" << accountId << "user:" << vkUserId << "album:" << vkAlbumId;
        return VKAlbum();
    } else if (albums.size() > 1) {
        qWarning() << Q_FUNC_INFO << "Multiple albums in database for account:" << accountId << "user:" << vkUserId << "album:" << vkAlbumId;
        // shouldn't happen, but return the first one anyway.
    }
    return albums[0];
}

VKImage VKImagesDatabase::image(int accountId, const QString &vkUserId, const QString &vkAlbumId, const QString &vkImageId) const
{
    Q_D(const VKImagesDatabase);
    QList<VKImage> images = d->queryImages(accountId, vkUserId, vkAlbumId, vkImageId);
    if (images.size() == 0) {
        qWarning() << Q_FUNC_INFO << "No image in database for account:" << accountId << "user:" << vkUserId << "album:" << vkAlbumId << "image:" << vkImageId;
        return VKImage();
    } else if (images.size() > 1) {
        qWarning() << Q_FUNC_INFO << "Multiple images in database for account:" << accountId << "user:" << vkUserId << "album:" << vkAlbumId << "image:" << vkImageId;
        // shouldn't happen, but return the first one anyway.
    }
    return images[0];
}

QList<VKAlbum> VKImagesDatabase::albums(int accountId, const QString &vkUserId) const
{
    Q_D(const VKImagesDatabase);
    return d->queryAlbums(accountId, vkUserId, QString());
}

QList<VKImage> VKImagesDatabase::images(int accountId, const QString &vkUserId, const QString &vkAlbumId) const
{
    Q_D(const VKImagesDatabase);
    return d->queryImages(accountId, vkUserId, vkAlbumId, QString());
}

QList<VKUser> VKImagesDatabase::users() const
{
    return d_func()->result.users;
}

QList<VKImage> VKImagesDatabase::images() const
{
    return d_func()->result.images;
}

QList<VKAlbum> VKImagesDatabase::albums() const
{
    return d_func()->result.albums;
}

void VKImagesDatabase::queryUsers()
{
    Q_D(VKImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = VKImagesDatabasePrivate::Users;
        d->query.accountId = 0;
        d->query.ownerId = QString();
        d->query.albumId = QString();
    }
    executeRead();
}

void VKImagesDatabase::queryAlbums(int accountId, const QString &vkUserId)
{
    Q_D(VKImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = VKImagesDatabasePrivate::Albums;
        d->query.accountId = accountId;
        d->query.ownerId = vkUserId;
        d->query.albumId = QString();
    }
    executeRead();
}

void VKImagesDatabase::queryUserImages(int accountId, const QString &vkUserId)
{
    Q_D(VKImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = VKImagesDatabasePrivate::Images;
        d->query.accountId = accountId;
        d->query.ownerId = vkUserId;
        d->query.albumId = QString();
    }
    executeRead();
}

void VKImagesDatabase::queryAlbumImages(int accountId, const QString &vkUserId, const QString &vkAlbumId)
{
    Q_D(VKImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.type = VKImagesDatabasePrivate::Images;
        d->query.accountId = accountId;
        d->query.ownerId = vkUserId;
        d->query.albumId = vkAlbumId;
    }
    executeRead();
}

bool VKImagesDatabase::read()
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);

    int queryAccountId = d->query.accountId;
    QString queryOwnerId = d->query.ownerId;
    QString queryAlbumId = d->query.albumId;

    switch (d->query.type) {
    case VKImagesDatabasePrivate::Users: {
        locker.unlock();
        QList<VKUser> users = d->queryUsers(queryAccountId);
        locker.relock();
        d->query.users = users;
        return true;
    }
    case VKImagesDatabasePrivate::Albums: {
        locker.unlock();
        QList<VKAlbum> albums = d->queryAlbums(queryAccountId, queryOwnerId, queryAlbumId);
        locker.relock();
        d->query.albums = albums;
        return true;
    }
    case VKImagesDatabasePrivate::Images: {
        locker.unlock();
        QList<VKImage> images = d->queryImages(queryAccountId, queryOwnerId, queryAlbumId, QString());
        locker.relock();
        d->query.images = images;
        return true;
    }
    default:
        return false;
    }
}

void VKImagesDatabase::readFinished()
{
    Q_D(VKImagesDatabase);
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

bool VKImagesDatabase::write()
{
    Q_D(VKImagesDatabase);
    QMutexLocker locker(&d->mutex);

    qWarning() << "Number of queued users being saved:" << d->queue.insertUsers.count();
    qWarning() << "Number of queued albums being saved:" << d->queue.insertAlbums.count();
    qWarning() << "Number of queued images being saved:" << d->queue.insertImages.count();
    qWarning() << "Number of queued thumbnail files being updated:" << d->queue.updateThumbnailFiles.count();
    qWarning() << "Number of queued image files being updated:" << d->queue.updateImageFiles.count();

    const QList<int> purgeAccounts = d->queue.purgeAccounts;

    const QList<VKUser> removeUsers = d->queue.removeUsers;
    const QList<VKAlbum> removeAlbums = d->queue.removeAlbums;
    const QList<VKImage> removeImages = d->queue.removeImages;

    const QList<VKUser> insertUsers = d->queue.insertUsers;
    const QList<VKAlbum> insertAlbums = d->queue.insertAlbums;
    const QList<VKImage> insertImages = d->queue.insertImages;

    const QList<QPair<VKImage, QString> > updateThumbnailFiles = d->queue.updateThumbnailFiles;
    const QList<QPair<VKImage, QString> > updateImageFiles = d->queue.updateImageFiles;

    d->queue.purgeAccounts.clear();

    d->queue.removeUsers.clear();
    d->queue.removeAlbums.clear();
    d->queue.removeImages.clear();

    d->queue.insertUsers.clear();
    d->queue.insertAlbums.clear();
    d->queue.insertImages.clear();

    d->queue.updateThumbnailFiles.clear();
    d->queue.updateImageFiles.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!purgeAccounts.isEmpty()) {
        // delete image files and build bindable accountIds list
        QVariantList accountIds;
        Q_FOREACH (int accountId, purgeAccounts) {
            accountIds << accountId;
            query = prepare(QStringLiteral(
                        "SELECT DISTINCT thumb_file, photo_file "
                        "FROM images "
                        "WHERE accountId = :accountId"));
            query.bindValue(QStringLiteral(":accountId"), accountId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query while purging accounts:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }

        // delete the data from the database.
        query = prepare(QStringLiteral(
                    "DELETE FROM users "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    Q_FOREACH (const VKUser &user, removeUsers) {
        // delete image files
        query = prepare(QStringLiteral(
                    "SELECT DISTINCT thumb_file, photo_file "
                    "FROM images "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId"));
        query.bindValue(QStringLiteral(":accountId"), user.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), user.id);
        if (!query.exec()) {
            qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query while removing albums:"
                       << query.lastError().text();
        } else {
            d->clearCachedImages(query);
        }

        // delete the data from the database.
        query = prepare(QStringLiteral(
                    "DELETE FROM users "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId"));
        query.bindValue(QStringLiteral(":accountId"), user.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), user.id);
        executeSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId"));
        query.bindValue(QStringLiteral(":accountId"), user.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), user.id);
        executeSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId"));
        query.bindValue(QStringLiteral(":accountId"), user.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), user.id);
        executeSocialCacheQuery(query);
    }

    Q_FOREACH (const VKAlbum &album, removeAlbums) {
        // delete image files
        query = prepare(QStringLiteral(
                    "SELECT DISTINCT thumb_file, photo_file "
                    "FROM images "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId"));
        query.bindValue(QStringLiteral(":accountId"), album.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), album.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), album.id);
        if (!query.exec()) {
            qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query while removing albums:"
                       << query.lastError().text();
        } else {
            d->clearCachedImages(query);
        }

        // delete the data from the database.
        query = prepare(QStringLiteral(
                    "DELETE FROM albums "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId"));
        query.bindValue(QStringLiteral(":accountId"), album.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), album.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), album.id);
        executeSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId"));
        query.bindValue(QStringLiteral(":accountId"), album.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), album.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), album.id);
        executeSocialCacheQuery(query);
    }

    Q_FOREACH (const VKImage &image, removeImages) {
        // delete image files
        query = prepare(QStringLiteral(
                    "SELECT DISTINCT thumb_file, photo_file "
                    "FROM images "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId AND vkImageId = :vkImageId"));
        query.bindValue(QStringLiteral(":accountId"), image.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), image.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), image.album_id);
        query.bindValue(QStringLiteral(":vkImageId"), image.id);
        if (!query.exec()) {
            qWarning() << Q_FUNC_INFO << "Failed to exec cached images selection query while removing images:"
                       << query.lastError().text();
        } else {
            d->clearCachedImages(query);
        }

        // delete the data from the database.
        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId AND vkImageId = :vkImageId"));
        query.bindValue(QStringLiteral(":accountId"), image.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), image.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), image.album_id);
        query.bindValue(QStringLiteral(":vkImageId"), image.id);
        executeSocialCacheQuery(query);
    }

    if (!insertUsers.isEmpty()) {
        QVariantList userIds, firstNames, lastNames, photoSrcs, photoFiles, accountIds;
        Q_FOREACH (const VKUser &user, insertUsers) {
            userIds.append(user.id);
            firstNames.append(user.first_name);
            lastNames.append(user.last_name);
            photoSrcs.append(user.photo_src);
            photoFiles.append(user.photo_file);
            accountIds.append(user.accountId);
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO users ("
                    " accountId, vkUserId, first_name, last_name, photo_src, photo_file) "
                    "VALUES ("
                    " :accountId, :vkUserId, :first_name, :last_name, :photo_src, :photo_file);"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":vkUserId"), userIds);
        query.bindValue(QStringLiteral(":first_name"), firstNames);
        query.bindValue(QStringLiteral(":last_name"), lastNames);
        query.bindValue(QStringLiteral(":photo_src"), photoSrcs);
        query.bindValue(QStringLiteral(":photo_file"), photoFiles);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertAlbums.isEmpty()) {
        QVariantList accountIds, vkOwnerIds, vkAlbumIds, titles, descriptions,
                     thumbSrcs, sizes, createds, updateds, thumbFiles;

        Q_FOREACH (const VKAlbum &album, insertAlbums) {
            accountIds.append(album.accountId);
            vkOwnerIds.append(album.owner_id);
            vkAlbumIds.append(album.id);
            titles.append(album.title);
            descriptions.append(album.description);
            thumbSrcs.append(album.thumb_src);
            sizes.append(album.size);
            createds.append(album.created);
            updateds.append(album.updated);
            thumbFiles.append(album.thumb_file);
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO albums("
                    " accountId, vkOwnerId, vkAlbumId, title, description, thumb_src, size, created, updated, thumb_file) "
                    "VALUES ("
                    " :accountId, :vkOwnerId, :vkAlbumId, :title, :description, :thumb_src, :size, :created, :updated, :thumb_file)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":vkOwnerId"), vkOwnerIds);
        query.bindValue(QStringLiteral(":vkAlbumId"), vkAlbumIds);
        query.bindValue(QStringLiteral(":title"), titles);
        query.bindValue(QStringLiteral(":description"), descriptions);
        query.bindValue(QStringLiteral(":thumb_src"), thumbSrcs);
        query.bindValue(QStringLiteral(":size"), sizes);
        query.bindValue(QStringLiteral(":created"), createds);
        query.bindValue(QStringLiteral(":updated"), updateds);
        query.bindValue(QStringLiteral(":thumb_file"), thumbFiles);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertImages.isEmpty()) {
        QVariantList accountIds, vkOwnerIds, vkAlbumIds, vkImageIds,
                     texts, thumbSrcs, photoSrcs, widths, heights,
                     dates, thumbFiles, photoFiles;
        Q_FOREACH (const VKImage &image, insertImages) {
            accountIds.append(image.accountId);
            vkOwnerIds.append(image.owner_id);
            vkAlbumIds.append(image.album_id);
            vkImageIds.append(image.id);
            texts.append(image.text);
            thumbSrcs.append(image.thumb_src);
            photoSrcs.append(image.photo_src);
            widths.append(image.width);
            heights.append(image.height);
            dates.append(image.date);
            thumbFiles.append(image.thumb_file);
            photoFiles.append(image.photo_file);
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO images ("
                    " accountId, vkOwnerId, vkAlbumId, vkImageId, text, thumb_src, photo_src, width, height, date, thumb_file, photo_file) "
                    "VALUES ("
                    " :accountId, :vkOwnerId, :vkAlbumId, :vkImageId, :text, :thumb_src, :photo_src, :width, :height, :date, :thumb_file, :photo_file);"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":vkOwnerId"), vkOwnerIds);
        query.bindValue(QStringLiteral(":vkAlbumId"), vkAlbumIds);
        query.bindValue(QStringLiteral(":vkImageId"), vkImageIds);
        query.bindValue(QStringLiteral(":text"), texts);
        query.bindValue(QStringLiteral(":thumb_src"), thumbSrcs);
        query.bindValue(QStringLiteral(":photo_src"), photoSrcs);
        query.bindValue(QStringLiteral(":width"), widths);
        query.bindValue(QStringLiteral(":height"), heights);
        query.bindValue(QStringLiteral(":date"), dates);
        query.bindValue(QStringLiteral(":thumb_file"), thumbFiles);
        query.bindValue(QStringLiteral(":photo_file"), photoFiles);
        executeBatchSocialCacheQuery(query);
    }

    for (int i = 0; i < updateThumbnailFiles.size(); ++i) {
        query = prepare(QStringLiteral(
                    "UPDATE images "
                    "SET thumb_file = :thumb_file "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId AND vkImageId = :vkImageId"));
        query.bindValue(QStringLiteral(":thumb_file"), updateThumbnailFiles[i].second);
        query.bindValue(QStringLiteral(":accountId"), updateThumbnailFiles[i].first.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), updateThumbnailFiles[i].first.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), updateThumbnailFiles[i].first.album_id);
        query.bindValue(QStringLiteral(":vkImageId"), updateThumbnailFiles[i].first.id);
        executeSocialCacheQuery(query);
    }

    for (int i = 0; i < updateImageFiles.size(); ++i) {
        query = prepare(QStringLiteral(
                    "UPDATE images "
                    "SET photo_file = :photo_file "
                    "WHERE accountId = :accountId AND vkOwnerId = :vkOwnerId AND vkAlbumId = :vkAlbumId AND vkImageId = :vkImageId"));
        query.bindValue(QStringLiteral(":photo_file"), updateImageFiles[i].second);
        query.bindValue(QStringLiteral(":accountId"), updateImageFiles[i].first.accountId);
        query.bindValue(QStringLiteral(":vkOwnerId"), updateImageFiles[i].first.owner_id);
        query.bindValue(QStringLiteral(":vkAlbumId"), updateImageFiles[i].first.album_id);
        query.bindValue(QStringLiteral(":vkImageId"), updateImageFiles[i].first.id);
        executeSocialCacheQuery(query);
    }

    return success;
}

bool VKImagesDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare( "CREATE TABLE IF NOT EXISTS images ("
                   "accountId INTEGER NOT NULL,"
                   "vkOwnerId TEXT NOT NULL,"
                   "vkAlbumId TEXT NOT NULL,"
                   "vkImageId TEXT NOT NULL,"
                   "text TEXT,"
                   "thumb_src TEXT,"
                   "photo_src TEXT,"
                   "width INTEGER,"
                   "height INTEGER,"
                   "date INTEGER,"
                   "thumb_file TEXT,"
                   "photo_file TEXT,"
                   "PRIMARY KEY (accountId, vkOwnerId, vkAlbumId, vkImageId) )");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table:" << query.lastError().text();
        return false;
    }

    query.prepare( "CREATE TABLE IF NOT EXISTS albums ("
                   "accountId INTEGER NOT NULL,"
                   "vkOwnerId TEXT NOT NULL,"
                   "vkAlbumId TEXT NOT NULL,"
                   "title TEXT,"
                   "description TEXT,"
                   "thumb_src TEXT,"
                   "size INTEGER,"
                   "created INTEGER,"
                   "updated INTEGER,"
                   "thumb_file TEXT,"
                   "PRIMARY KEY (accountId, vkOwnerId, vkAlbumId) )");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create albums table:" << query.lastError().text();
        return false;
    }
    struct VKUser
    {
        QString id; // user id
        QString first_name;
        QString last_name;
        QString photo_src;
        int accountId;
    };

    query.prepare( "CREATE TABLE IF NOT EXISTS users ("
                   "accountId INTEGER NOT NULL,"
                   "vkUserId TEXT NOT NULL,"
                   "first_name TEXT,"
                   "last_name TEXT,"
                   "photo_src TEXT,"
                   "photo_file TEXT,"
                   "PRIMARY KEY (accountId, vkUserId) )");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create users table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool VKImagesDatabase::dropTables(QSqlDatabase database) const
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

    return true;
}
