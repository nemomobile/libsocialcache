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

#ifndef ONEDRIVEIMAGESDATABASE_H
#define ONEDRIVEIMAGESDATABASE_H

#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

class OneDriveUserPrivate;
class OneDriveUser
{
public:
    typedef QSharedPointer<OneDriveUser> Ptr;
    typedef QSharedPointer<const OneDriveUser> ConstPtr;

    virtual ~OneDriveUser();

    static OneDriveUser::Ptr create(const QString &userId, const QDateTime &updatedTime,
                                    const QString &userName, int accountId, int count = -1);

    QString userId() const;
    QDateTime updatedTime() const;
    QString userName() const;
    int accountId() const;
    int count() const;

protected:
    QScopedPointer<OneDriveUserPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(OneDriveUser)
    explicit OneDriveUser(const QString &userId, const QDateTime &updatedTime,
                          const QString &userName, int accountId, int count = -1);
};

class OneDriveAlbumPrivate;
class OneDriveAlbum
{
public:
    typedef QSharedPointer<OneDriveAlbum> Ptr;
    typedef QSharedPointer<const OneDriveAlbum> ConstPtr;

    virtual ~OneDriveAlbum();

    static OneDriveAlbum::Ptr create(const QString &albumId, const QString &userId,
                                     const QDateTime &createdTime, const QDateTime &updatedTime,
                                     const QString &albumName, int imageCount);

    QString albumId() const;
    QString userId() const;
    QDateTime createdTime() const;
    QDateTime updatedTime() const;
    QString albumName() const;
    int imageCount() const;

protected:
    QScopedPointer<OneDriveAlbumPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(OneDriveAlbum)
    explicit OneDriveAlbum(const QString &albumId, const QString &userId,
                           const QDateTime &createdTime, const QDateTime &updatedTime,
                           const QString &albumName, int imageCount);
};

class OneDriveImagePrivate;
class OneDriveImage
{
public:
    typedef QSharedPointer<OneDriveImage> Ptr;
    typedef QSharedPointer<const OneDriveImage> ConstPtr;

    virtual ~OneDriveImage();

    static OneDriveImage::Ptr create(const QString & imageId, const QString & albumId,
                                     const QString & userId, const QDateTime & createdTime,
                                     const QDateTime &updatedTime, const QString &imageName,
                                     int width, int height, const QString & thumbnailUrl,
                                     const QString & imageUrl, const QString & thumbnailFile,
                                     const QString & imageFile, const QString & description,
                                     int accountId);

    QString imageId() const;
    QString albumId() const;
    QString userId() const;
    QDateTime createdTime() const;
    QDateTime updatedTime() const;
    QString imageName() const;
    int width() const;
    int height() const;
    QString thumbnailUrl() const;
    QString imageUrl() const;
    QString thumbnailFile() const;
    QString imageFile() const;
    QString description() const;
    int accountId() const;

protected:
    QScopedPointer<OneDriveImagePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(OneDriveImage)
    explicit OneDriveImage(const QString & imageId, const QString & albumId,
                           const QString & userId, const QDateTime & createdTime,
                           const QDateTime & updatedTime, const QString & imageName,
                           int width, int height, const QString & thumbnailUrl,
                           const QString & imageUrl, const QString & thumbnailFile,
                           const QString & imageFile, const QString & description,
                           int accountId);
};

bool operator==(const OneDriveUser::ConstPtr &user1, const OneDriveUser::ConstPtr &user2);
bool operator==(const OneDriveAlbum::ConstPtr &album1, const OneDriveAlbum::ConstPtr &album2);
bool operator==(const OneDriveImage::ConstPtr &image1, const OneDriveImage::ConstPtr &image2);

class OneDriveImagesDatabasePrivate;
class OneDriveImagesDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit OneDriveImagesDatabase();
    ~OneDriveImagesDatabase();

    // Account manipulation
    bool syncAccount(int accountId, const QString &userId);
    void purgeAccount(int accountId);
    QMap<int,QString> accounts(bool *ok = 0) const;

    // User cache manipulation
    OneDriveUser::ConstPtr user(const QString &userId) const;
    void addUser(const QString &userId, const QDateTime &updatedTime,
                 const QString &userName, int accountId);
    void removeUser(const QString &userId);

    // Album cache manipulation
    QStringList allAlbumIds(bool *ok = 0) const;
    OneDriveAlbum::ConstPtr album(const QString &albumId) const;
    void addAlbum(const QString &albumId, const QString &userId, const QDateTime &createdTime,
                  const QDateTime &updatedTime, const QString &albumName, int imageCount);
    void removeAlbum(const QString &albumId);
    void removeAlbums(const QStringList &albumIds);

    // Images cache manipulation
    QStringList allImageIds(bool *ok = 0) const;
    QStringList imageIds(const QString &albumId, bool *ok = 0) const;
    OneDriveImage::ConstPtr image(const QString &imageId) const;
    void addImage(const QString & imageId, const QString & albumId,
                  const QString & userId, const QDateTime & createdTime,
                  const QDateTime & updatedTime, const QString & imageName,
                  int width, int height, const QString & thumbnailUrl,
                  const QString & imageUrl, const QString & description,
                  int accountId);
    void updateImageThumbnail(const QString &imageId, const QString &thumbnailFile);
    void updateImageFile(const QString &imageId, const QString &imageFile);
    void removeImage(const QString &imageId);
    void removeImages(const QStringList &imageIds);

    void commit();

    QList<OneDriveUser::ConstPtr> users() const;
    QList<OneDriveImage::ConstPtr> images() const;
    QList<OneDriveAlbum::ConstPtr> albums() const;

    void queryUsers();
    void queryAlbums(const QString &userId = QString());
    void queryUserImages(const QString &userId = QString());
    void queryAlbumImages(const QString &albumId);

Q_SIGNALS:
    void queryFinished();

protected:
    bool read();
    void readFinished();

    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;


private:
    Q_DECLARE_PRIVATE(OneDriveImagesDatabase)
};

#endif // ONEDRIVEIMAGESDATABASE_H
