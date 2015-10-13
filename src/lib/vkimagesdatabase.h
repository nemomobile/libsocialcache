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

class VKUserPrivate;
class VKUser
{
public:
    typedef QSharedPointer<VKUser> Ptr;
    typedef QSharedPointer<const VKUser> ConstPtr;

    virtual ~VKUser();

    static VKUser::Ptr create(const QString &id, const QString &first_name, const QString &last_name,
                              const QString &photo_src, const QString &photo_file, int accountId);

    QString id() const;
    QString firstName() const;
    QString lastName() const;
    QString photoSrc() const;
    QString photoFile() const;
    int accountId() const;
    int photosCount() const;
    void setPhotosCount(int photosCount);

    bool operator==(const VKUser &other) const;
    bool operator!=(const VKUser &other) const { return !(*this == other); }

protected:
    QScopedPointer<VKUserPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(VKUser)
    explicit VKUser(const QString &id, const QString &first_name, const QString &last_name,
                    const QString &photo_src, const QString &photo_file, int accountId);
};

class VKAlbumPrivate;
class VKAlbum
{
public:
    typedef QSharedPointer<VKAlbum> Ptr;
    typedef QSharedPointer<const VKAlbum> ConstPtr;

    static VKAlbum::Ptr create(const QString &id, const QString &owner_id, const QString &title,
                               const QString &description, const QString &thumb_src,
                               const QString &thumb_file, int size, int created, int updated,
                               int accountId);
    virtual ~VKAlbum();

    QString id() const;       // album id
    QString ownerId() const;  // user id
    QString title() const;
    QString description() const;
    QString thumbSrc() const;
    QString thumbFile() const;
    int size() const;
    int created() const;
    int updated() const;
    int accountId() const;

    bool operator==(const VKAlbum &other) const;
    bool operator!=(const VKAlbum &other) const { return !(*this == other); }

protected:
    QScopedPointer<VKAlbumPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(VKAlbum)
    explicit VKAlbum(const QString &id, const QString &owner_id, const QString &title,
                     const QString &description, const QString &thumb_src,
                     const QString &thumb_file, int size, int created, int updated,
                     int accountId);
};

class VKImagePrivate;
class VKImage
{
public:
    typedef QSharedPointer<VKImage> Ptr;
    typedef QSharedPointer<const VKImage> ConstPtr;

    static VKImage::Ptr create(const QString &id, const QString &album_id, const QString &owner_id,
                               const QString &text, const QString &thumb_src, const QString &photo_src,
                               const QString &thumb_file, const QString &photo_file,
                               int width, int height, int date, int accountId);
    virtual ~VKImage();

    QString id() const;         // photo id
    QString albumId() const;    // album id
    QString ownerId() const;    // user id
    QString text() const;
    QString thumbSrc() const;   // remote url
    QString photoSrc() const;   // remote url
    QString thumbFile() const;  // local file
    QString photoFile() const;  // local file
    int width() const;
    int height() const;
    int date() const;
    int accountId() const;

    bool operator==(const VKImage &other) const;
    bool operator!=(const VKImage &other) const { return !(*this == other); }

protected:
    QScopedPointer<VKImagePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(VKImage)
    explicit VKImage(const QString &id, const QString &album_id, const QString &owner_id,
                     const QString &text, const QString &thumb_src, const QString &photo_src,
                     const QString &thumb_file, const QString &photo_file,
                     int width, int height, int date, int accountId);
};

class VKImagesDatabasePrivate;
class VKImagesDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit VKImagesDatabase();
    ~VKImagesDatabase();

    // User data manipulation, not applied to db until commit()
    void addUser(const VKUser::ConstPtr &vkUser);
    void removeUser(const VKUser::ConstPtr &vkUser);

    // Album data manipulation, not applied to db until commit()
    void addAlbum(const VKAlbum::ConstPtr &vkAlbum);
    void addAlbums(const QList<VKAlbum::ConstPtr> &vkAlbums);
    void removeAlbum(const VKAlbum::ConstPtr &vkAlbum);
    void removeAlbums(const QList<VKAlbum::ConstPtr> &vkAlbums);

    // Images data manipulation, not applied to db until commit()
    void addImage(const VKImage::ConstPtr &vkImage);
    void addImages(const QList<VKImage::ConstPtr> &vkImages);
    void updateImageThumbnail(const VKImage::ConstPtr &vkImage, const QString &thumb_file);
    void updateImageFile(const VKImage::ConstPtr &vkImage, const QString &photo_file);
    void removeImage(const VKImage::ConstPtr &vkImage);
    void removeImages(const QList<VKImage::ConstPtr> &vkImages);

    // Purge all data associated with a given account, not applied to db until commit()
    void purgeAccount(int accountId);

    // commit changes in memory to disk.
    void commit();

    // methods to perform synchronous queries. For use by sync adapters only!
    VKUser::ConstPtr user(int accountId) const;
    VKAlbum::ConstPtr album(int accountId, const QString &vkUserId, const QString &vkAlbumId) const;
    VKImage::ConstPtr image(int accountId, const QString &vkUserId, const QString &vkAlbumId, const QString &vkImageId) const;
    QList<VKAlbum::ConstPtr> albums(int accountId, const QString &vkUserId) const;
    QList<VKImage::ConstPtr> images(int accountId, const QString &vkUserId, const QString &vkAlbumId) const;

    // methods to perform asynchronous queries and retrieve the results of those queries.
    void queryUsers();
    QList<VKUser::ConstPtr> users() const;
    void queryAlbums(int accountId = 0, const QString &vkUserId = QString());
    QList<VKAlbum::ConstPtr> albums() const;
    void queryUserImages(int accountId = 0, const QString &vkUserId = QString());
    void queryAlbumImages(int accountId, const QString &vkUserId, const QString &vkAlbumId);
    QList<VKImage::ConstPtr> images() const;

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
