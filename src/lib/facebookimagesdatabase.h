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

#include "databasemanipulationinterface.h"
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

class FacebookUserPrivate;
class FacebookUser
{
public:
    typedef QSharedPointer<FacebookUser> Ptr;
    typedef QSharedPointer<const FacebookUser> ConstPtr;
    virtual ~FacebookUser();
    static FacebookUser::Ptr create(const QString &fbUserId, const QString &updatedTime,
                                    const QString &userName, const QString &thumbnailUrl,
                                    const QString &thumbnailFile, int count = -1);
    QString fbUserId() const;
    QString updatedTime() const;
    QString userName() const;
    QString thumbnailUrl() const;
    QString thumbnailFile() const;
    int count() const;
protected:
    QScopedPointer<FacebookUserPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(FacebookUser)
    explicit FacebookUser(const QString &fbUserId, const QString &updatedTime,
                          const QString &userName, const QString &thumbnailUrl,
                          const QString &thumbnailFile, int count = -1);
};

class FacebookAlbumPrivate;
class FacebookAlbum
{
public:
    typedef QSharedPointer<FacebookAlbum> Ptr;
    typedef QSharedPointer<const FacebookAlbum> ConstPtr;
    virtual ~FacebookAlbum();
    static FacebookAlbum::Ptr create(const QString &fbAlbumId, const QString &fbUserId,
                                     const QString &createdTime, const QString &updatedTime,
                                     const QString &albumName, int imageCount,
                                     const QString &coverImageId, const QString &thumbnailFile);
    QString fbAlbumId() const;
    QString fbUserId() const;
    QString createdTime() const;
    QString updatedTime() const;
    QString albumName() const;
    int imageCount() const;
    QString coverImageId() const;
    QString thumbnailFile() const;
protected:
    QScopedPointer<FacebookAlbumPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(FacebookAlbum)
    explicit FacebookAlbum(const QString &fbAlbumId, const QString &fbUserId,
                           const QString &createdTime, const QString &updatedTime,
                           const QString &albumName, int imageCount,
                           const QString &coverImageId, const QString &thumbnailFile);
};

class FacebookImagePrivate;
class FacebookImage
{
public:
    typedef QSharedPointer<FacebookImage> Ptr;
    typedef QSharedPointer<const FacebookImage> ConstPtr;
    virtual ~FacebookImage();
    static FacebookImage::Ptr create(const QString & fbImageId, const QString & fbAlbumId,
                                     const QString & fbUserId, const QString & createdTime,
                                     const QString & updatedTime, const QString & imageName,
                                     int width, int height, const QString & thumbnailUrl,
                                     const QString & imageUrl, const QString & thumbnailFile,
                                     const QString & imageFile, int account = -1);
    QString fbImageId() const;
    QString fbAlbumId() const;
    QString fbUserId() const;
    QString createdTime() const;
    QString updatedTime() const;
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
                           const QString & fbUserId, const QString & createdTime,
                           const QString & updatedTime, const QString & imageName,
                           int width, int height, const QString & thumbnailUrl,
                           const QString & imageUrl, const QString & thumbnailFile,
                           const QString & imageFile, int account = -1);
};

bool operator==(const FacebookUser::ConstPtr &user1, const FacebookUser::ConstPtr &user2);
bool operator==(const FacebookAlbum::ConstPtr &album1, const FacebookAlbum::ConstPtr &album2);
bool operator==(const FacebookImage::ConstPtr &image1, const FacebookImage::ConstPtr &image2);

class FacebookImagesDatabasePrivate;
class FacebookImagesDatabase: public DatabaseManipulationInterface
{
public:
    explicit FacebookImagesDatabase();
    ~FacebookImagesDatabase();

    void initDatabase();

    // Account manipulation
    void syncAccount(int accountId, const QString &fbUserId);
    void purgeAccount(int accountId);

    // User cache manipulation
    FacebookUser::ConstPtr user(const QString &fbUserId) const;
    void addUser(const QString &fbUserId, const QString &updatedTime,
                 const QString &userName, const QString &thumbnailUrl);
    void updateUserThumbnail(const QString &fbUserId, const QString &thumbnailFile);
    void removeUser(const QString &fbUserId);
    QList<FacebookUser::ConstPtr> users() const;

    // Album cache manipulation
    QStringList allAlbumIds(bool *ok = 0) const;
    FacebookAlbum::ConstPtr album(const QString &fbAlbumId) const;
    void addAlbum(const QString &fbAlbumId, const QString &fbUserId, const QString &createdTime,
                  const QString &updatedTime, const QString &albumName, int imageCount,
                  const QString &coverImageId);
    void removeAlbum(const QString &fbAlbumId);
    QList<FacebookAlbum::ConstPtr> albums(const QString &fbUserId = QString());

    // Images cache manipulation
    QStringList allImageIds(bool *ok = 0) const;
    QStringList imagesId(const QString &fbAlbumId, bool *ok = 0) const;
    FacebookImage::ConstPtr image(const QString &fbImageId) const;
    void addImage(const QString & fbImageId, const QString & fbAlbumId,
                  const QString & fbUserId, const QString & createdTime,
                  const QString & updatedTime, const QString & imageName,
                  int width, int height, const QString & thumbnailUrl,
                  const QString & imageUrl);
    void updateImageThumbnail(const QString &fbImageId, const QString &thumbnailFile);
    void updateImageFile(const QString &fbImageId, const QString &imageFile);
    void removeImage(const QString &fbImageId);
    QList<FacebookImage::ConstPtr> userImages(const QString &fbUserId = QString());
    QList<FacebookImage::ConstPtr> albumImages(const QString &fbAlbumId);

    bool write();

protected:
    bool dbCreateTables();
    bool dbDropTables();
private:
    Q_DECLARE_PRIVATE(FacebookImagesDatabase)
};

#endif // FACEBOOKIMAGESDATABASE_H
