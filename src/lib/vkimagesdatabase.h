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

#ifndef VKIMAGESDATABASE_H
#define VKIMAGESDATABASE_H

#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

struct VKUser
{
    VKUser(const QString &id, const QString &first_name, const QString &last_name,
           const QString &photo_src, const QString &photo_file, int accountId);
    VKUser() : accountId(0), photos_count(0) {}
    QString id; // user id
    QString first_name;
    QString last_name;
    QString photo_src;
    QString photo_file;
    int accountId;

    int photos_count; // transient, not database-stored value! Not used in comparisons etc.

    bool operator==(const VKUser &other) const;
    bool operator!=(const VKUser &other) const { return !(*this == other); }
};

struct VKAlbum
{
    VKAlbum(const QString &id, const QString &owner_id, const QString &title,
            const QString &description, const QString &thumb_src,
            const QString &thumb_file, int size, int created, int updated,
            int accountId);
    VKAlbum() : size(0), created(0), updated(0), accountId(0) {}
    QString id;       // album id
    QString owner_id; // user id
    QString title;
    QString description;
    QString thumb_src;
    QString thumb_file;
    int size;
    int created;
    int updated;
    int accountId;

    bool operator==(const VKAlbum &other) const;
    bool operator!=(const VKAlbum &other) const { return !(*this == other); }
};

struct VKImage
{
    VKImage(const QString &id, const QString &album_id, const QString &owner_id,
            const QString &text, const QString &thumb_src, const QString &photo_src,
            const QString &thumb_file, const QString &photo_file,
            int width, int height, int date, int accountId);
    VKImage() : width(0), height(0), date(0), accountId(0) {}
    QString id;         // photo id
    QString album_id;   // album id
    QString owner_id;   // user id
    QString text;
    QString thumb_src;  // remote url
    QString photo_src;  // remote url
    QString thumb_file; // local file
    QString photo_file; // local file
    int width;
    int height;
    int date;
    int accountId;

    bool operator==(const VKImage &other) const;
    bool operator!=(const VKImage &other) const { return !(*this == other); }
};


class VKImagesDatabasePrivate;
class VKImagesDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit VKImagesDatabase();
    ~VKImagesDatabase();

    // User data manipulation, not applied to db until commit()
    void addUser(const VKUser &vkUser);
    void removeUser(const VKUser &vkUser);

    // Album data manipulation, not applied to db until commit()
    void addAlbum(const VKAlbum &vkAlbum);
    void addAlbums(const QList<VKAlbum> &vkAlbums);
    void removeAlbum(const VKAlbum &vkAlbum);
    void removeAlbums(const QList<VKAlbum> &vkAlbums);

    // Images data manipulation, not applied to db until commit()
    void addImage(const VKImage &vkImage);
    void addImages(const QList<VKImage> &vkImages);
    void updateImageThumbnail(const VKImage &vkImage, const QString &thumb_file);
    void updateImageFile(const VKImage &vkImage, const QString &photo_file);
    void removeImage(const VKImage &vkImage);
    void removeImages(const QList<VKImage> &vkImages);

    // Purge all data associated with a given account, not applied to db until commit()
    void purgeAccount(int accountId);

    // commit changes in memory to disk.
    void commit();

    // methods to perform synchronous queries. For use by sync adapters only!
    VKUser user(int accountId) const;
    VKAlbum album(int accountId, const QString &vkUserId, const QString &vkAlbumId) const;
    VKImage image(int accountId, const QString &vkUserId, const QString &vkAlbumId, const QString &vkImageId) const;
    QList<VKAlbum> albums(int accountId, const QString &vkUserId) const;
    QList<VKImage> images(int accountId, const QString &vkUserId, const QString &vkAlbumId) const;

    // methods to perform asynchronous queries and retrieve the results of those queries.
    void queryUsers();
    QList<VKUser> users() const;
    void queryAlbums(int accountId = 0, const QString &vkUserId = QString());
    QList<VKAlbum> albums() const;
    void queryUserImages(int accountId = 0, const QString &vkUserId = QString());
    void queryAlbumImages(int accountId, const QString &vkUserId, const QString &vkAlbumId);
    QList<VKImage> images() const;

Q_SIGNALS:
    void queryFinished();

protected:
    bool read();
    void readFinished();

    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(VKImagesDatabase)
};

#endif // VKIMAGESDATABASE_H
