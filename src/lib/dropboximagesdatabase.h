/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Jonni Rainisto <jonni.rainito@jollamobile.com>
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

#ifndef DROPBOXIMAGESDATABASE_H
#define DROPBOXIMAGESDATABASE_H

#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

class DropboxUserPrivate;
class DropboxUser
{
public:
    typedef QSharedPointer<DropboxUser> Ptr;
    typedef QSharedPointer<const DropboxUser> ConstPtr;

    virtual ~DropboxUser();

    static DropboxUser::Ptr create(const QString &userId, const QDateTime &updatedTime,
                                    const QString &userName, int count = -1);

    QString userId() const;
    QDateTime updatedTime() const;
    QString userName() const;
    int count() const;

protected:
    QScopedPointer<DropboxUserPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(DropboxUser)
    explicit DropboxUser(const QString &userId, const QDateTime &updatedTime,
                          const QString &userName, int count = -1);
};

class DropboxAlbumPrivate;
class DropboxAlbum
{
public:
    typedef QSharedPointer<DropboxAlbum> Ptr;
    typedef QSharedPointer<const DropboxAlbum> ConstPtr;

    virtual ~DropboxAlbum();

    static DropboxAlbum::Ptr create(const QString &albumId, const QString &userId,
                                     const QDateTime &createdTime, const QDateTime &updatedTime,
                                     const QString &albumName, int imageCount, const QString &hash);

    QString albumId() const;
    QString userId() const;
    QDateTime createdTime() const;
    QDateTime updatedTime() const;
    QString albumName() const;
    QString hash() const;
    int imageCount() const;

protected:
    QScopedPointer<DropboxAlbumPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(DropboxAlbum)
    explicit DropboxAlbum(const QString &albumId, const QString &userId,
                           const QDateTime &createdTime, const QDateTime &updatedTime,
                           const QString &albumName, int imageCount, const QString &hash);
};

class DropboxImagePrivate;
class DropboxImage
{
public:
    typedef QSharedPointer<DropboxImage> Ptr;
    typedef QSharedPointer<const DropboxImage> ConstPtr;

    virtual ~DropboxImage();

    static DropboxImage::Ptr create(const QString & imageId, const QString & albumId,
                                     const QString & userId, const QDateTime & createdTime,
                                     const QDateTime &updatedTime, const QString &imageName,
                                     int width, int height, const QString & thumbnailUrl,
                                     const QString & imageUrl, const QString & thumbnailFile,
                                     const QString & imageFile, int account = -1,
                                     const QString & accessToken = QString());

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
    int account() const;
    QString accessToken() const;

protected:
    QScopedPointer<DropboxImagePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(DropboxImage)
    explicit DropboxImage(const QString & imageId, const QString & albumId,
                           const QString & userId, const QDateTime & createdTime,
                           const QDateTime & updatedTime, const QString & imageName,
                           int width, int height, const QString & thumbnailUrl,
                           const QString & imageUrl, const QString & thumbnailFile,
                           const QString & imageFile, int account = -1,
                           const QString & accessToken = QString());
};

bool operator==(const DropboxUser::ConstPtr &user1, const DropboxUser::ConstPtr &user2);
bool operator==(const DropboxAlbum::ConstPtr &album1, const DropboxAlbum::ConstPtr &album2);
bool operator==(const DropboxImage::ConstPtr &image1, const DropboxImage::ConstPtr &image2);

class DropboxImagesDatabasePrivate;
class DropboxImagesDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit DropboxImagesDatabase();
    ~DropboxImagesDatabase();

    // Account manipulation
    bool syncAccount(int accountId, const QString &userId);
    void purgeAccount(int accountId);
    QMap<int,QString> accounts(bool *ok = 0) const;

    // User cache manipulation
    DropboxUser::ConstPtr user(const QString &userId) const;
    void addUser(const QString &userId, const QDateTime &updatedTime,
                 const QString &userName);
    void removeUser(const QString &userId);

    // Album cache manipulation
    QStringList allAlbumIds(bool *ok = 0) const;
    DropboxAlbum::ConstPtr album(const QString &albumId) const;
    void addAlbum(const QString &albumId, const QString &userId, const QDateTime &createdTime,
                  const QDateTime &updatedTime, const QString &albumName, int imageCount, const QString &hash);
    void removeAlbum(const QString &albumId);
    void removeAlbums(const QStringList &albumIds);

    // Images cache manipulation
    QStringList allImageIds(bool *ok = 0) const;
    QStringList imageIds(const QString &albumId, bool *ok = 0) const;
    DropboxImage::ConstPtr image(const QString &imageId) const;
    void addImage(const QString & imageId, const QString & albumId,
                  const QString & userId, const QDateTime & createdTime,
                  const QDateTime & updatedTime, const QString & imageName,
                  int width, int height, const QString & thumbnailUrl,
                  const QString & imageUrl, const QString & accessToken);
    void updateImageThumbnail(const QString &imageId, const QString &thumbnailFile);
    void updateImageFile(const QString &imageId, const QString &imageFile);
    void removeImage(const QString &imageId);
    void removeImages(const QStringList &imageIds);

    void commit();

    QList<DropboxUser::ConstPtr> users() const;
    QList<DropboxImage::ConstPtr> images() const;
    QList<DropboxAlbum::ConstPtr> albums() const;

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
    Q_DECLARE_PRIVATE(DropboxImagesDatabase)
};

#endif // DROPBOXIMAGESDATABASE_H
