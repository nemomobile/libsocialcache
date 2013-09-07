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
#include "abstractfacebookcachemodel_p.h"
#include "facebookimagesdatabase.h"
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QStandardPaths>
#include <QtCore/QMultiMap>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

// Note:
//
// When querying photos, the nodeIdentifier should be either
// - nothing: query all photos
// - user-USER_ID: query all photos for the given user
// - album-ALBUM_ID: query all photos for the given album

static const char *PHOTO_USER_PREFIX = "user-";
static const char *PHOTO_ALBUM_PREFIX = "album-";
static int MAX_SIMULTANEOUS_DOWNLOAD = 10;

#define SOCIALCACHE_FACEBOOK_IMAGE_DIR   PRIVILEGED_DATA_DIR + QLatin1String("/Images/")

struct FacebookImageWorkerImageData;
class FacebookImageWorkerObject: public AbstractWorkerObject, private FacebookImagesDatabase
{
    Q_OBJECT
public:
    enum Type {
        User,
        Album,
        Image
    };
    enum ImageType {
        ThumbnailImage,
        FullImage
    };
    explicit FacebookImageWorkerObject();
    virtual ~FacebookImageWorkerObject();
    void refresh();
public Q_SLOTS:
    void setType(int typeToSet);
    void queueImages();
    void queueUser(int row, const FacebookUser::ConstPtr &user);
    void queueImageThumbnail(int row, const FacebookImage::ConstPtr &image);
    void queueImageFull(int row, const FacebookImage::ConstPtr &image);
protected:
    FacebookImageCacheModel::ModelDataType type;
private:
    void queue(int row, Type type, ImageType imageType, const QString &identifier,
               const QString &url);
    void manageStack();
    QMap<QNetworkReply *, FacebookImageWorkerImageData> m_runningReplies;
    QList<FacebookImageWorkerImageData> m_stack;
    bool m_haveLoaded;

    QNetworkAccessManager *m_networkAccessManager;
    QTimer *m_saveTimer;
    bool m_enabled;
    QList<QPair<int, FacebookImage::ConstPtr> > m_fullImages;
private Q_SLOTS:
    void slotImageDownloaded();
    void saveUpdates();
};

struct FacebookImageWorkerImageData
{
    int row;
    FacebookImageWorkerObject::Type type;
    FacebookImageWorkerObject::ImageType imageType;
    QString identifier;
    QString url;
};

bool operator==(const FacebookImageWorkerImageData &data1,
                const FacebookImageWorkerImageData &data2);

class FacebookImageCacheModelPrivate: public AbstractFacebookCacheModelPrivate
{
    Q_OBJECT
public:
    explicit FacebookImageCacheModelPrivate(FacebookImageCacheModel *q);
    FacebookImageCacheModel::ModelDataType type;
Q_SIGNALS:
    void typeChanged(int type);
    void queueImages();
protected:
    void initWorkerObject(AbstractWorkerObject *m_workerObject);
private:
    Q_DECLARE_PUBLIC(FacebookImageCacheModel)
};

FacebookImageWorkerObject::FacebookImageWorkerObject()
    : AbstractWorkerObject(), FacebookImagesDatabase(), type(FacebookImageCacheModel::None)
    , m_haveLoaded(false) , m_networkAccessManager(0), m_saveTimer(0), m_enabled(false)
{
    m_networkAccessManager = new QNetworkAccessManager(this);
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(false);
    m_saveTimer->setInterval(5000);
    connect(m_saveTimer, &QTimer::timeout, this, &FacebookImageWorkerObject::saveUpdates);
}

FacebookImageWorkerObject::~FacebookImageWorkerObject()
{
    saveUpdates();
}

void FacebookImageWorkerObject::refresh()
{
    // We initialize the database when refresh is called
    // When it is called, we are sure that the object is already in a different thread
    if (!m_enabled) {
        initDatabase();
        m_enabled = true;
    }

    QList<QMap<int, QVariant> > data;
    switch (type) {
        case FacebookImageCacheModel::Users: {
            QList<FacebookUser::ConstPtr> usersData = users();
            for (int i = 0; i < usersData.count(); i++) {
                const FacebookUser::ConstPtr & userData = usersData.at(i);
                QMap<int, QVariant> userMap;
                userMap.insert(FacebookImageCacheModel::FacebookId, userData->fbUserId());
                if (userData->thumbnailFile().isEmpty()) {
                    queueUser(i, userData);
                }
                userMap.insert(FacebookImageCacheModel::Thumbnail, userData->thumbnailFile());
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
                    m_fullImages.append(qMakePair<int, FacebookImage::ConstPtr>(i, imageData));
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

    qWarning() << "Loaded" << data.count() << "entries";
    emit dataRead(data);
}

void FacebookImageWorkerObject::setType(int typeToSet)
{
    type = static_cast<FacebookImageCacheModel::ModelDataType>(typeToSet);
}

void FacebookImageWorkerObject::queueImages()
{
    for (QList<QPair<int, FacebookImage::ConstPtr> >::const_iterator i = m_fullImages.begin();
         i != m_fullImages.end(); i++) {
        queueImageFull((*i).first, (*i).second);
    }
}

void FacebookImageWorkerObject::queueUser(int row, const FacebookUser::ConstPtr &user)
{
    queue(row, User, ThumbnailImage, user->fbUserId(), user->thumbnailUrl());
}

void FacebookImageWorkerObject::queueImageThumbnail(int row, const FacebookImage::ConstPtr &image)
{
    queue(row, Image, ThumbnailImage, image->fbImageId(), image->thumbnailUrl());
}

void FacebookImageWorkerObject::queueImageFull(int row, const FacebookImage::ConstPtr &image)
{
    queue(row, Image, FullImage, image->fbImageId(), image->imageUrl());
}

void FacebookImageWorkerObject::queue(int row, Type type, ImageType imageType,
                                      const QString &identifier, const QString &url)
{
    FacebookImageWorkerImageData data;
    data.row = row;
    data.type = type;
    data.imageType = imageType;
    data.identifier = identifier;
    data.url = url;

    if (m_stack.contains(data)) {
        m_stack.removeAll(data);
    }
    m_stack.prepend(data);
    manageStack();
}

void FacebookImageWorkerObject::manageStack()
{
    qWarning() << m_stack.count();
    while (m_runningReplies.count() < MAX_SIMULTANEOUS_DOWNLOAD && !m_stack.isEmpty()) {
        // Create a reply to download the image
        FacebookImageWorkerImageData data = m_stack.takeFirst();

        QNetworkRequest request (data.url);
        QNetworkReply *reply = m_networkAccessManager->get(request);
        connect(reply, &QNetworkReply::finished,
                this, &FacebookImageWorkerObject::slotImageDownloaded);
        m_runningReplies.insert(reply, data);
    }
}

void FacebookImageWorkerObject::slotImageDownloaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    const FacebookImageWorkerImageData &data = m_runningReplies.value(reply);
    m_runningReplies.remove(reply);

    if (data.identifier.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Empty id";
        return;
    }

    QString imageType;
    switch (data.imageType) {
    case ThumbnailImage:
        imageType = QLatin1String("thumb");
        break;
    case FullImage:
        imageType = QLatin1String("image");
        break;
    }

    QImage image;
    bool loadedOk = image.loadFromData(reply->readAll());
    if (!loadedOk || image.isNull()) {
        qWarning() << Q_FUNC_INFO << "Downloaded" << imageType << "for" << data.identifier
                   << "but could not load image";
        return;
    }

    // TODO: remove existing file if needed

    // Save the new image (eg fbphotoid-thumb.jpg or fbphotoid-image.jpg)
    QString newName = QString(QLatin1String("%1%2-%3.jpg")).arg(SOCIALCACHE_FACEBOOK_IMAGE_DIR,
                                                                data.identifier, imageType);

    qWarning() << "Downloaded" << newName;
    bool saveOk = image.save(newName);
    if (!saveOk) {
        qWarning() << Q_FUNC_INFO << "Cannot save image";
        return;
    }

    QMap<int, QVariant> updatedRow;
    switch (data.type) {
        case User:
            updateUserThumbnail(data.identifier, newName);
            updatedRow.insert(FacebookImageCacheModel::Thumbnail, newName);
            break;
        case Image:
            switch (data.imageType) {
            case ThumbnailImage:
                updateImageThumbnail(data.identifier, newName);
                updatedRow.insert(FacebookImageCacheModel::Thumbnail, newName);
                break;
            case FullImage:
                updateImageFile(data.identifier, newName);
                updatedRow.insert(FacebookImageCacheModel::Image, newName);
                break;
            }
            break;
        default:
            break;
    }

    // Update row
    qWarning() << "Performing row update";
    emit rowUpdated(data.row, updatedRow);

    m_haveLoaded = true;
    if (!m_saveTimer->isActive()) {
        qWarning() << "Started timer";
        m_saveTimer->start();
    }
    manageStack();
}

void FacebookImageWorkerObject::saveUpdates()
{
    if (m_haveLoaded) {
        qWarning() << "Writing updates";
        write();
        m_haveLoaded = false;
    } else {
        qWarning() << "Stopping timer";
        m_saveTimer->stop();
    }

}

bool operator==(const FacebookImageWorkerImageData &data1,
                const FacebookImageWorkerImageData &data2)
{
    return data1.identifier == data2.identifier
           && data1.imageType == data2.imageType
           && data1.url == data2.url
           && data1.type == data2.type;
}

FacebookImageCacheModelPrivate::FacebookImageCacheModelPrivate(FacebookImageCacheModel *q)
    : AbstractFacebookCacheModelPrivate(q)
{
}

void FacebookImageCacheModelPrivate::initWorkerObject(AbstractWorkerObject *workerObject)
{
    FacebookImageWorkerObject *facebookImageWorkerObject
            = qobject_cast<FacebookImageWorkerObject *>(workerObject);
    if (!facebookImageWorkerObject) {
        qWarning() << "No worker object" << facebookImageWorkerObject;
        return;
    }

    connect(this, &FacebookImageCacheModelPrivate::queueImages,
            facebookImageWorkerObject, &FacebookImageWorkerObject::queueImages);
    connect(this, &FacebookImageCacheModelPrivate::typeChanged,
            facebookImageWorkerObject, &FacebookImageWorkerObject::setType);

    AbstractFacebookCacheModelPrivate::initWorkerObject(workerObject);
}

FacebookImageCacheModel::FacebookImageCacheModel(QObject *parent)
    : AbstractFacebookCacheModel(*(new FacebookImageCacheModelPrivate(this)), parent)
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
