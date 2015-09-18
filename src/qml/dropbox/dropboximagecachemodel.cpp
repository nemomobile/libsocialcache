/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Jonni Rainisto <jonni.rainisto@jollamobile.com>
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

#include "dropboximagecachemodel.h"
#include "abstractsocialcachemodel_p.h"
#include "dropboximagesdatabase.h"

#include "dropboximagedownloader_p.h"
#include "dropboximagedownloaderconstants_p.h"

#include <QtCore/QThread>
#include <QtCore/QStandardPaths>

#include <QtDebug>

// Note:
//
// When querying photos, the nodeIdentifier should be either
// - nothing: query all photos
// - user-USER_ID: query all photos for the given user
// - album-ALBUM_ID: query all photos for the given album
//
// When querying albums, the nodeIdentifier should be either
// - nothing: query all albums for all users
// - USER_ID: query all albums for the given user

static const char *PHOTO_USER_PREFIX = "user-";
static const char *PHOTO_ALBUM_PREFIX = "album-";

static const char *URL_KEY = "url";
static const char *ROW_KEY = "row";
static const char *MODEL_KEY = "model";
static const char *ACCESSTOKEN = "accessToken";

class DropboxImageCacheModelPrivate : public AbstractSocialCacheModelPrivate
{
public:
    DropboxImageCacheModelPrivate(DropboxImageCacheModel *q);

    void queue(
            int row,
            DropboxImageDownloader::ImageType imageType,
            const QString &identifier,
            const QString &url, 
            const QString &accessToken);

    DropboxImageDownloader *downloader;
    DropboxImagesDatabase database;
    DropboxImageCacheModel::ModelDataType type;
};

DropboxImageCacheModelPrivate::DropboxImageCacheModelPrivate(DropboxImageCacheModel *q)
    : AbstractSocialCacheModelPrivate(q), downloader(0), type(DropboxImageCacheModel::Images)
{
}

void DropboxImageCacheModelPrivate::queue(
        int row,
        DropboxImageDownloader::ImageType imageType,
        const QString &identifier,
        const QString &url,
        const QString &accessToken)
{
    DropboxImageCacheModel *modelPtr = qobject_cast<DropboxImageCacheModel*>(q_ptr);
    if (downloader) {
        QVariantMap metadata;
        metadata.insert(QLatin1String(TYPE_KEY), imageType);
        metadata.insert(QLatin1String(IDENTIFIER_KEY), identifier);
        metadata.insert(QLatin1String(URL_KEY), url);
        metadata.insert(QLatin1String(ROW_KEY), row);
        metadata.insert(QLatin1String(MODEL_KEY), QVariant::fromValue<void*>((void*)modelPtr));
        metadata.insert(QLatin1String(ACCESSTOKEN), accessToken);

        // no use to download if there is no accessToken
        if (accessToken.length()) {
            downloader->queue(url, metadata);
        } else {
            qWarning() << Q_FUNC_INFO << "fail accesstoken is missing" << url;
        }
    }
}

DropboxImageCacheModel::DropboxImageCacheModel(QObject *parent)
    : AbstractSocialCacheModel(*(new DropboxImageCacheModelPrivate(this)), parent)
{
    Q_D(const DropboxImageCacheModel);
    connect(&d->database, &DropboxImagesDatabase::queryFinished,
            this, &DropboxImageCacheModel::queryFinished);
}

DropboxImageCacheModel::~DropboxImageCacheModel()
{
    Q_D(DropboxImageCacheModel);
    if (d->downloader) {
        d->downloader->removeModelFromHash(this);
    }
}

QHash<int, QByteArray> DropboxImageCacheModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(DropboxId, "id");
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
    roleNames.insert(AccessToken, "accessToken");
    return roleNames;
}

DropboxImageCacheModel::ModelDataType DropboxImageCacheModel::type() const
{
    Q_D(const DropboxImageCacheModel);
    return d->type;
}

void DropboxImageCacheModel::setType(DropboxImageCacheModel::ModelDataType type)
{
    Q_D(DropboxImageCacheModel);
    if (d->type != type) {
        d->type = type;
        emit typeChanged();
    }
}

DropboxImageDownloader * DropboxImageCacheModel::downloader() const
{
    Q_D(const DropboxImageCacheModel);
    return d->downloader;
}

void DropboxImageCacheModel::setDownloader(DropboxImageDownloader *downloader)
{
    Q_D(DropboxImageCacheModel);
    if (d->downloader != downloader) {
        if (d->downloader) {
            // Disconnect worker object
            disconnect(d->downloader);
            d->downloader->removeModelFromHash(this);
        }

        d->downloader = downloader;
        d->downloader->addModelToHash(this);
        emit downloaderChanged();
    }
}

QVariant DropboxImageCacheModel::data(const QModelIndex &index, int role) const
{
    Q_D(const DropboxImageCacheModel);
    int row = index.row();
    if (row < 0 || row >= d->m_data.count()) {
        return QVariant();
    }

    return d->m_data.at(row).value(role);
}

void DropboxImageCacheModel::loadImages()
{
    refresh();
}

void DropboxImageCacheModel::refresh()
{
    Q_D(DropboxImageCacheModel);

    const QString userPrefix = QLatin1String(PHOTO_USER_PREFIX);
    const QString albumPrefix = QLatin1String(PHOTO_ALBUM_PREFIX);

    switch (d->type) {
    case DropboxImageCacheModel::Users:
        d->database.queryUsers();
        break;
    case DropboxImageCacheModel::Albums:
        d->database.queryAlbums(d->nodeIdentifier);
        break;
    case DropboxImageCacheModel::Images:
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

// NOTE: this is now called directly by DropboxImageDownloader
// rather than connected to the imageDownloaded signal, for
// performance reasons.
void DropboxImageCacheModel::imageDownloaded(
        const QString &, const QString &path, const QVariantMap &imageData)
{
    Q_D(DropboxImageCacheModel);

    int row = imageData.value(ROW_KEY).toInt();
    if (row < 0 || row >= d->m_data.count()) {
        qWarning() << Q_FUNC_INFO
                   << "Invalid row:" << row
                   << "max row:" << d->m_data.count();
        return;
    }

    int type = imageData.value(TYPE_KEY).toInt();
    switch (type) {
    case DropboxImageDownloader::ThumbnailImage:
        d->m_data[row].insert(DropboxImageCacheModel::Thumbnail, path);
        break;
    }

    emit dataChanged(index(row), index(row));
}

void DropboxImageCacheModel::queryFinished()
{
    Q_D(DropboxImageCacheModel);

    QList<QVariantMap> thumbQueue;
    SocialCacheModelData data;
    switch (d->type) {
    case Users: {
        QList<DropboxUser::ConstPtr> usersData = d->database.users();
        int count = 0;
        Q_FOREACH (const DropboxUser::ConstPtr &userData, usersData) {
            QMap<int, QVariant> userMap;
            userMap.insert(DropboxImageCacheModel::DropboxId, userData->userId());
            userMap.insert(DropboxImageCacheModel::Title, userData->userName());
            userMap.insert(DropboxImageCacheModel::Count, userData->count());
            count += userData->count();
            data.append(userMap);
        }

        if (data.count() > 1) {
            QMap<int, QVariant> userMap;
            userMap.insert(DropboxImageCacheModel::DropboxId, QString());
            userMap.insert(DropboxImageCacheModel::Thumbnail, QString());
            //: Label for the "show all users from all Dropbox accounts" option
            //% "All"
            userMap.insert(DropboxImageCacheModel::Title, qtTrId("nemo_socialcache_dropbox_images_model-all-users"));
            userMap.insert(DropboxImageCacheModel::Count, count);
            data.prepend(userMap);
        }
        break;
    }
    case Albums: {
        QList<DropboxAlbum::ConstPtr> albumsData = d->database.albums();

        int count = 0;
        Q_FOREACH (const DropboxAlbum::ConstPtr &albumData, albumsData) {
            QMap<int, QVariant> albumMap;
            albumMap.insert(DropboxImageCacheModel::DropboxId, albumData->albumId());
            albumMap.insert(DropboxImageCacheModel::Title, albumData->albumName());
            albumMap.insert(DropboxImageCacheModel::Count, albumData->imageCount());
            albumMap.insert(DropboxImageCacheModel::UserId, albumData->userId());
            count += albumData->imageCount();
            data.append(albumMap);
        }

        if (data.count() > 1) {
            QMap<int, QVariant> albumMap;
            albumMap.insert(DropboxImageCacheModel::DropboxId, QString());
            // albumMap.insert(DropboxImageCacheModel::Icon, QString());
            //:  Label for the "show all photos from all albums by this user" option
            //% "All"
            albumMap.insert(DropboxImageCacheModel::Title, qtTrId("nemo_socialcache_dropbox_images_model-all-albums"));
            albumMap.insert(DropboxImageCacheModel::Count, count);
            if (d->nodeIdentifier.isEmpty()) {
                albumMap.insert(DropboxImageCacheModel::UserId, QString());
            } else {
                albumMap.insert(DropboxImageCacheModel::UserId, data.first().value(DropboxImageCacheModel::UserId));
            }
            data.prepend(albumMap);
        }
        break;
    }
    case Images: {
        QList<DropboxImage::ConstPtr> imagesData = d->database.images();

        for (int i = 0; i < imagesData.count(); i ++) {
            const DropboxImage::ConstPtr & imageData = imagesData.at(i);
            QMap<int, QVariant> imageMap;
            imageMap.insert(DropboxImageCacheModel::DropboxId, imageData->imageId());
            if (imageData->thumbnailFile().isEmpty()) {
                QVariantMap thumbQueueData;
                thumbQueueData.insert("row", QVariant::fromValue<int>(i));
                thumbQueueData.insert("imageType", QVariant::fromValue<int>(DropboxImageDownloader::ThumbnailImage));
                thumbQueueData.insert("identifier", imageData->imageId());
                thumbQueueData.insert("url", imageData->thumbnailUrl());
                thumbQueueData.insert("accessToken", imageData->accessToken());
                thumbQueue.append(thumbQueueData);
            }
            // note: we don't queue the image file until the user explicitly opens that in fullscreen.
            imageMap.insert(DropboxImageCacheModel::Thumbnail, imageData->thumbnailFile());
            imageMap.insert(DropboxImageCacheModel::Image, imageData->imageUrl());
            imageMap.insert(DropboxImageCacheModel::Title, imageData->imageName());
            imageMap.insert(DropboxImageCacheModel::DateTaken, imageData->createdTime());
            imageMap.insert(DropboxImageCacheModel::Width, imageData->width());
            imageMap.insert(DropboxImageCacheModel::Height, imageData->height());
            imageMap.insert(DropboxImageCacheModel::MimeType, QLatin1String("image/jpeg"));
            imageMap.insert(DropboxImageCacheModel::AccountId, imageData->account());
            imageMap.insert(DropboxImageCacheModel::UserId, imageData->userId());
            imageMap.insert(DropboxImageCacheModel::AccessToken, imageData->accessToken());
            data.append(imageMap);
        }
        break;
    }
    default:
        return;
    }

    updateData(data);

    // now download the queued thumbnails.
    Q_FOREACH (const QVariantMap &thumbQueueData, thumbQueue) {
        if (thumbQueueData["accessToken"].toString().length()) {
        d->queue(thumbQueueData["row"].toInt(),
                 static_cast<DropboxImageDownloader::ImageType>(thumbQueueData["imageType"].toInt()),
                 thumbQueueData["identifier"].toString(),
                 thumbQueueData["url"].toString(), thumbQueueData["accessToken"].toString());
        } else {
            qWarning() << "Error: cannot queue without accessToken";
        }
    }
}
