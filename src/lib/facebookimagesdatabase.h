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

#ifndef FACEBOOKIMAGESDATABASE_H
#define FACEBOOKIMAGESDATABASE_H

#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

class FacebookUserPrivate;
class FacebookUser
{
public:
    typedef QSharedPointer<FacebookUser> Ptr;
    typedef QSharedPointer<const FacebookUser> ConstPtr;

    virtual ~FacebookUser();

    static FacebookUser::Ptr create(const QString &fbUserId, const QDateTime &updatedTime,
                                    const QString &userName, int count = -1);

    QString fbUserId() const;
    QDateTime updatedTime() const;
    QString userName() const;
    int count() const;

protected:
    QScopedPointer<FacebookUserPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(FacebookUser)
    explicit FacebookUser(const QString &fbUserId, const QDateTime &updatedTime,
                          const QString &userName, int count = -1);
};

class FacebookAlbumPrivate;
class FacebookAlbum
{
public:
    typedef QSharedPointer<FacebookAlbum> Ptr;
    typedef QSharedPointer<const FacebookAlbum> ConstPtr;

    virtual ~FacebookAlbum();

    static FacebookAlbum::Ptr create(const QString &fbAlbumId, const QString &fbUserId,
                                     const QDateTime &createdTime, const QDateTime &updatedTime,
                                     const QString &albumName, int imageCount);

    QString fbAlbumId() const;
    QString fbUserId() const;
    QDateTime createdTime() const;
    QDateTime updatedTime() const;
    QString albumName() const;
    int imageCount() const;

protected:
    QScopedPointer<FacebookAlbumPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(FacebookAlbum)
    explicit FacebookAlbum(const QString &fbAlbumId, const QString &fbUserId,
                           const QDateTime &createdTime, const QDateTime &updatedTime,
                           const QString &albumName, int imageCount);
};

class FacebookImagePrivate;
class FacebookImage
{
public:
    typedef QSharedPointer<FacebookImage> Ptr;
    typedef QSharedPointer<const FacebookImage> ConstPtr;

    virtual ~FacebookImage();

    static FacebookImage::Ptr create(const QString & fbImageId, const QString & fbAlbumId,
                                     const QString & fbUserId, const QDateTime & createdTime,
                                     const QDateTime &updatedTime, const QString &imageName,
                                     int width, int height, const QString & thumbnailUrl,
                                     const QString & imageUrl, const QString & thumbnailFile,
                                     const QString & imageFile, int account = -1);

    QString fbImageId() const;
    QString fbAlbumId() const;
    QString fbUserId() const;
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

protected:
    QScopedPointer<FacebookImagePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(FacebookImage)
    explicit FacebookImage(const QString & fbImageId, const QString & fbAlbumId,
                           const QString & fbUserId, const QDateTime & createdTime,
                           const QDateTime & updatedTime, const QString & imageName,
                           int width, int height, const QString & thumbnailUrl,
                           const QString & imageUrl, const QString & thumbnailFile,
                           const QString & imageFile, int account = -1);
};

bool operator==(const FacebookUser::ConstPtr &user1, const FacebookUser::ConstPtr &user2);
bool operator==(const FacebookAlbum::ConstPtr &album1, const FacebookAlbum::ConstPtr &album2);
bool operator==(const FacebookImage::ConstPtr &image1, const FacebookImage::ConstPtr &image2);

class FacebookImagesDatabasePrivate;
class FacebookImagesDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit FacebookImagesDatabase();
    ~FacebookImagesDatabase();

    // Account manipulation
    bool syncAccount(int accountId, const QString &fbUserId);
    void purgeAccount(int accountId);
    QMap<int,QString> accounts(bool *ok = 0) const;

    // User cache manipulation
    FacebookUser::ConstPtr user(const QString &fbUserId) const;
    void addUser(const QString &fbUserId, const QDateTime &updatedTime,
                 const QString &userName);
    void removeUser(const QString &fbUserId);

    // Album cache manipulation
    QStringList allAlbumIds(bool *ok = 0) const;
    FacebookAlbum::ConstPtr album(const QString &fbAlbumId) const;
    void addAlbum(const QString &fbAlbumId, const QString &fbUserId, const QDateTime &createdTime,
                  const QDateTime &updatedTime, const QString &albumName, int imageCount);
    void removeAlbum(const QString &fbAlbumId);
    void removeAlbums(const QStringList &fbAlbumIds);

    // Images cache manipulation
    QStringList allImageIds(bool *ok = 0) const;
    QStringList imageIds(const QString &fbAlbumId, bool *ok = 0) const;
    FacebookImage::ConstPtr image(const QString &fbImageId) const;
    void addImage(const QString & fbImageId, const QString & fbAlbumId,
                  const QString & fbUserId, const QDateTime & createdTime,
                  const QDateTime & updatedTime, const QString & imageName,
                  int width, int height, const QString & thumbnailUrl,
                  const QString & imageUrl,
                  const QString &thumbnailFile = QString(), const QString &imageFile = QString());
    void updateImageThumbnail(const QString &fbImageId, const QString &thumbnailFile);
    void updateImageFile(const QString &fbImageId, const QString &imageFile);
    void removeImage(const QString &fbImageId);
    void removeImages(const QStringList &fbImageIds);

    void commit();

    QList<FacebookUser::ConstPtr> users() const;
    QList<FacebookImage::ConstPtr> images() const;
    QList<FacebookAlbum::ConstPtr> albums() const;

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
    Q_DECLARE_PRIVATE(FacebookImagesDatabase)
};

#endif // FACEBOOKIMAGESDATABASE_H
