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

struct FacebookImageWorkerImageData;
class FacebookImageWorkerObject: public AbstractWorkerObject, private FacebookImagesDatabase
{
    Q_OBJECT

public:
    explicit FacebookImageWorkerObject();
    ~FacebookImageWorkerObject();

    void refresh();
    void finalCleanup();

public Q_SLOTS:
    void setType(int typeToSet);
    void queueImages();
    void queueImageThumbnail(int row, const FacebookImage::ConstPtr &image);
    void queueImageFull(int row, const FacebookImage::ConstPtr &image);

Q_SIGNALS:
    void requestQueue(const QString &url, const QVariantMap &metadata);

protected:
    FacebookImageCacheModel::ModelDataType type;

private:
    void queue(int row,
               FacebookImageDownloaderWorkerObject::ImageType imageType, const QString &identifier,
               const QString &url);

    bool m_enabled;
    QList<QPair<FacebookImage::ConstPtr, int> > m_fullImages;
};

class FacebookImageCacheModelPrivate: public AbstractSocialCacheModelPrivate
{
    Q_OBJECT

public:
    explicit FacebookImageCacheModelPrivate(FacebookImageCacheModel *q);
    FacebookImageCacheModel::ModelDataType type;
    FacebookImageDownloader *downloader;

public Q_SLOTS:
    void queue(const QString &url, const QVariantMap &metadata);
    void slotDataUpdated(const QString &url, const QString &path);

Q_SIGNALS:
    void typeChanged(int type);
    void queueImages();

protected:
    void initWorkerObject(AbstractWorkerObject *workerObject);

private:
    QHash<QString, QVariantMap> m_queuedImages;
    Q_DECLARE_PUBLIC(FacebookImageCacheModel)
};

FacebookImageWorkerObject::FacebookImageWorkerObject()
    : AbstractWorkerObject(), FacebookImagesDatabase()
    , type(FacebookImageCacheModel::None)
    , m_enabled(false)
{
}

FacebookImageWorkerObject::~FacebookImageWorkerObject()
{
}

void FacebookImageWorkerObject::finalCleanup()
{
    closeDatabase();
}

void FacebookImageWorkerObject::refresh()
{
    // We initialize the database when refresh is called
    // When it is called, we are sure that the object is already in a different thread
    if (!m_enabled) {
        initDatabase();
        m_enabled = true;
    }

    SocialCacheModelData data;
    switch (type) {
        case FacebookImageCacheModel::Users: {
            QList<FacebookUser::ConstPtr> usersData = users();
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
                foreach (const FacebookUser::ConstPtr &userData, usersData) {
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
        }
        break;
        case FacebookImageCacheModel::Albums: {
            QList<FacebookAlbum::ConstPtr> albumsData = albums(nodeIdentifier);
            foreach (const FacebookAlbum::ConstPtr & albumData, albumsData) {
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
                foreach (const FacebookAlbum::ConstPtr &albumData, albumsData) {
                    count += albumData->imageCount();
                }

                albumMap.insert(FacebookImageCacheModel::FacebookId, QString());
                // albumMap.insert(FacebookImageCacheModel::Icon, QString());
                //:  Label for the "show all photos from all albums (by this user or by all users, depending...)" option
                //% "All"
                albumMap.insert(FacebookImageCacheModel::Title, qtTrId("nemo_socialcache_facebook_images_model-all-albums"));
                albumMap.insert(FacebookImageCacheModel::Count, count);
                albumMap.insert(FacebookImageCacheModel::UserId, nodeIdentifier);
                data.prepend(albumMap);
            }
        }
        break;
        case FacebookImageCacheModel::Images: {
            QList<FacebookImage::ConstPtr> imagesData;
            QString userPrefix = QLatin1String(PHOTO_USER_PREFIX);
            QString albumPrefix = QLatin1String(PHOTO_ALBUM_PREFIX);
            if (nodeIdentifier.startsWith(userPrefix)) {
                QString userIdentifier = nodeIdentifier.mid(userPrefix.size());
                imagesData = userImages(userIdentifier);
            } else if (nodeIdentifier.startsWith(albumPrefix)) {
                QString albumIdentifier = nodeIdentifier.mid(albumPrefix.size());
                imagesData = albumImages(albumIdentifier);
            } else {
                imagesData = userImages();
            }

            for (int i = 0; i < imagesData.count(); i ++) {
                const FacebookImage::ConstPtr & imageData = imagesData.at(i);
                QMap<int, QVariant> imageMap;
                imageMap.insert(FacebookImageCacheModel::FacebookId, imageData->fbImageId());
                if (imageData->thumbnailFile().isEmpty()) {
                    queueImageThumbnail(i, imageData);
                }
                imageMap.insert(FacebookImageCacheModel::Thumbnail, imageData->thumbnailFile());
                if (imageData->imageFile().isEmpty()) {
                    m_fullImages.append(qMakePair<FacebookImage::ConstPtr, int>(imageData, i));
                }
                imageMap.insert(FacebookImageCacheModel::Image, imageData->imageFile());
                imageMap.insert(FacebookImageCacheModel::Title, imageData->imageName());
                imageMap.insert(FacebookImageCacheModel::DateTaken, imageData->createdTime());
                imageMap.insert(FacebookImageCacheModel::Width, imageData->width());
                imageMap.insert(FacebookImageCacheModel::Height, imageData->height());
                imageMap.insert(FacebookImageCacheModel::MimeType, QLatin1String("JPG"));
                imageMap.insert(FacebookImageCacheModel::AccountId, imageData->account());
                imageMap.insert(FacebookImageCacheModel::UserId, imageData->fbUserId());
                data.append(imageMap);
            }
        }
        break;
        default: return; break;
    }

    emit dataUpdated(data);
}

void FacebookImageWorkerObject::setType(int typeToSet)
{
    type = static_cast<FacebookImageCacheModel::ModelDataType>(typeToSet);
}

void FacebookImageWorkerObject::queueImages()
{
    for (QList<QPair<FacebookImage::ConstPtr, int> >::const_iterator i = m_fullImages.begin();
         i != m_fullImages.end(); i++) {
        queueImageFull((*i).second, (*i).first);
    }
}

void FacebookImageWorkerObject::queueImageThumbnail(int row, const FacebookImage::ConstPtr &image)
{
    queue(row, FacebookImageDownloaderWorkerObject::ThumbnailImage, image->fbImageId(),
          image->thumbnailUrl());
}

void FacebookImageWorkerObject::queueImageFull(int row, const FacebookImage::ConstPtr &image)
{
    queue(row, FacebookImageDownloaderWorkerObject::FullImage, image->fbImageId(),
          image->imageUrl());
}

void FacebookImageWorkerObject::queue(int row,
                                      FacebookImageDownloaderWorkerObject::ImageType imageType,
                                      const QString &identifier, const QString &url)
{
    QVariantMap metadata;
    metadata.insert(QLatin1String(TYPE_KEY), imageType);
    metadata.insert(QLatin1String(IDENTIFIER_KEY), identifier);
    metadata.insert(QLatin1String(URL_KEY), url);
    metadata.insert(QLatin1String(ROW_KEY), row);
    emit requestQueue(url, metadata);
}

FacebookImageCacheModelPrivate::FacebookImageCacheModelPrivate(FacebookImageCacheModel *q)
    : AbstractSocialCacheModelPrivate(q), downloader(0)
{
}

void FacebookImageCacheModelPrivate::queue(const QString &url, const QVariantMap &metadata)
{
    m_queuedImages.insert(url, metadata);
}

void FacebookImageCacheModelPrivate::slotDataUpdated(const QString &url, const QString &path)
{
    Q_Q(FacebookImageCacheModel);
    if (m_queuedImages.contains(url)) {
        QVariantMap imageData = m_queuedImages.value(url);
        int row = imageData.value(ROW_KEY).toInt();
        if (row < 0 || row >= m_data.count()) {
            qWarning() << Q_FUNC_INFO << "Incorrect number of rows" << imageData.count();
            return;
        }

        int type = imageData.value(TYPE_KEY).toInt();
        switch (type) {
        case FacebookImageDownloaderWorkerObject::ThumbnailImage:
            m_data[row].insert(FacebookImageCacheModel::Thumbnail, path);
            break;
        case FacebookImageDownloaderWorkerObject::FullImage:
            m_data[row].insert(FacebookImageCacheModel::Image, path);
            break;
        }

        emit q->dataChanged(q->index(row), q->index(row));
    }
}

void FacebookImageCacheModelPrivate::initWorkerObject(AbstractWorkerObject *workerObject)
{
    FacebookImageWorkerObject *facebookImageWorkerObject
            = qobject_cast<FacebookImageWorkerObject *>(workerObject);
    if (!facebookImageWorkerObject) {
        return;
    }

    connect(facebookImageWorkerObject, &FacebookImageWorkerObject::requestQueue,
            this, &FacebookImageCacheModelPrivate::queue);
    connect(this, &FacebookImageCacheModelPrivate::queueImages,
            facebookImageWorkerObject, &FacebookImageWorkerObject::queueImages);
    connect(this, &FacebookImageCacheModelPrivate::typeChanged,
            facebookImageWorkerObject, &FacebookImageWorkerObject::setType);

    AbstractSocialCacheModelPrivate::initWorkerObject(workerObject);
}

FacebookImageCacheModel::FacebookImageCacheModel(QObject *parent)
    : AbstractSocialCacheModel(*(new FacebookImageCacheModelPrivate(this)), parent)
{
    Q_D(FacebookImageCacheModel);
    d->initWorkerObject(new FacebookImageWorkerObject());
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
        emit d->typeChanged(type);
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
            d->m_workerObject->disconnect(d->downloader->workerObject());
            d->downloader->workerObject()->disconnect(d);
        }

        d->downloader = downloader;

        // Needed for the new Qt connection system
        FacebookImageWorkerObject *imageWorkerObject
                = qobject_cast<FacebookImageWorkerObject *>(d->m_workerObject);
        connect(imageWorkerObject, &FacebookImageWorkerObject::requestQueue,
                d->downloader->workerObject(), &AbstractImageDownloader::queue);
        connect(d->downloader->workerObject(), &AbstractImageDownloader::imageDownloaded,
                d, &FacebookImageCacheModelPrivate::slotDataUpdated);

        emit downloaderChanged();
    }
}

// Used to queue high-res photos to be loaded
void FacebookImageCacheModel::loadImages()
{
    Q_D(FacebookImageCacheModel);
    if (d->type != Images) {
        return;
    }

    emit d->queueImages();
}

#include "facebookimagecachemodel.moc"
