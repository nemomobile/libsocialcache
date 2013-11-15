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

#include "facebookimagecachemodel.h"
#include "abstractsocialcachemodel_p.h"
#include "facebookimagesdatabase.h"

#include "facebookimagedownloader_p.h"
#include "facebookimagedownloaderconstants_p.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>

#include <QtDebug>

// Note:
//
// When querying photos, the nodeIdentifier should be either
// - nothing: query all photos
// - user-USER_ID: query all photos for the given user
// - album-ALBUM_ID: query all photos for the given album

static const char *PHOTO_USER_PREFIX = "user-";
static const char *PHOTO_ALBUM_PREFIX = "album-";

static const char *URL_KEY = "url";
static const char *ROW_KEY = "row";

#define SOCIALCACHE_FACEBOOK_IMAGE_DIR   PRIVILEGED_DATA_DIR + QLatin1String("/Images/")

class FacebookImageCacheModelPrivate : public AbstractSocialCacheModelPrivate
{
public:
    FacebookImageCacheModelPrivate(FacebookImageCacheModel *q);

    void queue(
            int row,
            FacebookImageDownloader::ImageType imageType,
            const QString &identifier,
            const QString &url);

    FacebookImageDownloader *downloader;
    FacebookImagesDatabase database;
    FacebookImageCacheModel::ModelDataType type;

};

FacebookImageCacheModelPrivate::FacebookImageCacheModelPrivate(FacebookImageCacheModel *q)
    : AbstractSocialCacheModelPrivate(q), downloader(0), type(FacebookImageCacheModel::Images)
{
}

void FacebookImageCacheModelPrivate::queue(
        int row,
        FacebookImageDownloader::ImageType imageType,
        const QString &identifier,
        const QString &url)
{
    if (downloader) {
        QVariantMap metadata;
        metadata.insert(QLatin1String(TYPE_KEY), imageType);
        metadata.insert(QLatin1String(IDENTIFIER_KEY), identifier);
        metadata.insert(QLatin1String(URL_KEY), url);
        metadata.insert(QLatin1String(ROW_KEY), row);

        downloader->queue(url, metadata);
    }
}

FacebookImageCacheModel::FacebookImageCacheModel(QObject *parent)
    : AbstractSocialCacheModel(*(new FacebookImageCacheModelPrivate(this)), parent)
{
    Q_D(const FacebookImageCacheModel);
    connect(&d->database, &FacebookImagesDatabase::queryFinished,
            this, &FacebookImageCacheModel::queryFinished);
}

QHash<int, QByteArray> FacebookImageCacheModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(FacebookId, "facebookId");
    roleNames.insert(Thumbnail, "thumbnail");
    roleNames.insert(Image, "image");
    roleNames.insert(Title, "title");
    roleNames.insert(DateTaken, "dateTaken");
    roleNames.insert(Width, "photoWidth");
    roleNames.insert(Height, "photoHeight");
    roleNames.insert(Count, "dataCount");
    roleNames.insert(MimeType, "mimeType");
    roleNames.insert(AccountId, "accountId");
    roleNames.insert(UserId, "userId");
    return roleNames;
}

FacebookImageCacheModel::ModelDataType FacebookImageCacheModel::type() const
{
    Q_D(const FacebookImageCacheModel);
    return d->type;
}

void FacebookImageCacheModel::setType(FacebookImageCacheModel::ModelDataType type)
{
    Q_D(FacebookImageCacheModel);
    if (d->type != type) {
        d->type = type;
        emit typeChanged();
    }
}

FacebookImageDownloader * FacebookImageCacheModel::downloader() const
{
    Q_D(const FacebookImageCacheModel);
    return d->downloader;
}

void FacebookImageCacheModel::setDownloader(FacebookImageDownloader *downloader)
{
    Q_D(FacebookImageCacheModel);
    if (d->downloader != downloader) {
        if (d->downloader) {
            // Disconnect worker object
            disconnect(d->downloader);
        }

        d->downloader = downloader;

        // Needed for the new Qt connection system
        connect(d->downloader, &AbstractImageDownloader::imageDownloaded,
                this, &FacebookImageCacheModel::imageDownloaded);

        emit downloaderChanged();
    }
}

void FacebookImageCacheModel::loadImages()
{
    refresh();
}

void FacebookImageCacheModel::refresh()
{
    Q_D(FacebookImageCacheModel);

    const QString userPrefix = QLatin1String(PHOTO_USER_PREFIX);
    const QString albumPrefix = QLatin1String(PHOTO_ALBUM_PREFIX);

    switch (d->type) {
    case FacebookImageCacheModel::Users:
        d->database.queryUsers();
        break;
    case FacebookImageCacheModel::Albums:
        d->database.queryAlbums(d->nodeIdentifier);
        break;
    case FacebookImageCacheModel::Images:
        if (d->nodeIdentifier.startsWith(userPrefix)) {
            d->database.queryUserImages(d->nodeIdentifier.mid(userPrefix.size()));
        } else if (d->nodeIdentifier.startsWith(albumPrefix)) {
            d->database.queryAlbumImages(d->nodeIdentifier.mid(albumPrefix.size()));
        } else {
            d->database.queryUserImages();
        }
        break;
    default:
        break;
    }
}

void FacebookImageCacheModel::imageDownloaded(
        const QString &, const QString &path, const QVariantMap &imageData)
{
    Q_D(FacebookImageCacheModel);

    int row = imageData.value(ROW_KEY).toInt();
    if (row < 0 || row >= d->m_data.count()) {
        qWarning() << Q_FUNC_INFO << "Incorrect number of rows" << imageData.count();
        return;
    }

    int type = imageData.value(TYPE_KEY).toInt();
    switch (type) {
    case FacebookImageDownloader::ThumbnailImage:
        d->m_data[row].insert(FacebookImageCacheModel::Thumbnail, path);
        break;
    case FacebookImageDownloader::FullImage:
        d->m_data[row].insert(FacebookImageCacheModel::Image, path);
        break;
    }

    emit dataChanged(index(row), index(row));
}

void FacebookImageCacheModel::queryFinished()
{
    Q_D(FacebookImageCacheModel);

    SocialCacheModelData data;
    switch (d->type) {
    case Users: {
        QList<FacebookUser::ConstPtr> usersData = d->database.users();
        for (int i = 0; i < usersData.count(); i++) {
            const FacebookUser::ConstPtr & userData = usersData.at(i);
            QMap<int, QVariant> userMap;
            userMap.insert(FacebookImageCacheModel::FacebookId, userData->fbUserId());
            userMap.insert(FacebookImageCacheModel::Title, userData->userName());
            userMap.insert(FacebookImageCacheModel::Count, userData->count());
            data.append(userMap);
        }

        if (data.count() > 1) {
            QMap<int, QVariant> userMap;
            int count = 0;
            Q_FOREACH (const FacebookUser::ConstPtr &userData, usersData) {
                count += userData->count();
            }

            userMap.insert(FacebookImageCacheModel::FacebookId, QString());
            userMap.insert(FacebookImageCacheModel::Thumbnail, QString());
            //: Label for the "show all users from all Facebook accounts" option
            //% "All"
            userMap.insert(FacebookImageCacheModel::Title, qtTrId("nemo_socialcache_facebook_images_model-all-users"));
            userMap.insert(FacebookImageCacheModel::Count, count);
            data.prepend(userMap);
        }
        break;
    }
    case Albums: {
        QList<FacebookAlbum::ConstPtr> albumsData = d->database.albums();
        Q_FOREACH (const FacebookAlbum::ConstPtr & albumData, albumsData) {
            QMap<int, QVariant> albumMap;
            albumMap.insert(FacebookImageCacheModel::FacebookId, albumData->fbAlbumId());
            albumMap.insert(FacebookImageCacheModel::Title, albumData->albumName());
            albumMap.insert(FacebookImageCacheModel::Count, albumData->imageCount());
            albumMap.insert(FacebookImageCacheModel::UserId, albumData->fbUserId());
            data.append(albumMap);
        }

        if (data.count() > 1) {
            QMap<int, QVariant> albumMap;
            int count = 0;
            Q_FOREACH (const FacebookAlbum::ConstPtr &albumData, albumsData) {
                count += albumData->imageCount();
            }

            albumMap.insert(FacebookImageCacheModel::FacebookId, QString());
            // albumMap.insert(FacebookImageCacheModel::Icon, QString());
            //:  Label for the "show all photos from all albums (by this user or by all users, depending...)" option
            //% "All"
            albumMap.insert(FacebookImageCacheModel::Title, qtTrId("nemo_socialcache_facebook_images_model-all-albums"));
            albumMap.insert(FacebookImageCacheModel::Count, count);
            albumMap.insert(FacebookImageCacheModel::UserId, QString());
            data.prepend(albumMap);
        }
        break;
    }
    case Images: {
        QList<FacebookImage::ConstPtr> imagesData = d->database.images();

        for (int i = 0; i < imagesData.count(); i ++) {
            const FacebookImage::ConstPtr & imageData = imagesData.at(i);
            QMap<int, QVariant> imageMap;
            imageMap.insert(FacebookImageCacheModel::FacebookId, imageData->fbImageId());
            if (imageData->thumbnailFile().isEmpty()) {
                d->queue(i, FacebookImageDownloader::ThumbnailImage,
                            imageData->fbImageId(),
                            imageData->thumbnailUrl());
            }
            imageMap.insert(FacebookImageCacheModel::Thumbnail, imageData->thumbnailFile());
            if (imageData->imageFile().isEmpty()) {
                d->queue(i, FacebookImageDownloader::FullImage,
                            imageData->fbImageId(),
                            imageData->imageUrl());
            }
            imageMap.insert(FacebookImageCacheModel::Image, imageData->imageFile());
            imageMap.insert(FacebookImageCacheModel::Title, imageData->imageName());
            imageMap.insert(FacebookImageCacheModel::DateTaken, imageData->createdTime());
            imageMap.insert(FacebookImageCacheModel::Width, imageData->width());
            imageMap.insert(FacebookImageCacheModel::Height, imageData->height());
            imageMap.insert(FacebookImageCacheModel::MimeType, QLatin1String("image/jpeg"));
            imageMap.insert(FacebookImageCacheModel::AccountId, imageData->account());
            imageMap.insert(FacebookImageCacheModel::UserId, imageData->fbUserId());
            data.append(imageMap);
        }
        break;
    }
    default:
        return;
    }

    updateData(data);
}
