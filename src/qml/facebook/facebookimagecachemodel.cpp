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

// libaccounts-qt
#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>

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
static const char *MODEL_KEY = "model";

#define SOCIALCACHE_FACEBOOK_IMAGE_DIR   PRIVILEGED_DATA_DIR + QLatin1String("/Images/")

class FacebookImageCacheModelPrivate : public AbstractSocialCacheModelPrivate
{
public:
    FacebookImageCacheModelPrivate(FacebookImageCacheModel *q);

    bool anyFacebookAccountsEnabled() const;
    bool accountIsEnabled(int accountId);


    void queue(
            int row,
            FacebookImageDownloader::ImageType imageType,
            const QString &identifier,
            const QString &url);

    FacebookImageDownloader *downloader;
    FacebookImagesDatabase database;
    FacebookImageCacheModel::ModelDataType type;

    Accounts::Manager manager;
    QHash<int, bool> accountsEnabled;
    bool anyFbAccountsEnabled;
};

FacebookImageCacheModelPrivate::FacebookImageCacheModelPrivate(FacebookImageCacheModel *q)
    : AbstractSocialCacheModelPrivate(q), downloader(0), type(FacebookImageCacheModel::Images)
{
    // Ugly hack: check on startup whether we have any enabled Facebook accounts.
    // This is because we don't store accountId in the FacebookUser or FacebookAlbum
    // data types, which means that we cannot immediately determine whether we have
    // any visible data or not.
    // TODO: fix facebookimagesdatabase.h and buteo-sync-plugins-social, and then
    // remove this code.
    Accounts::AccountIdList accountIds = manager.accountList();
    Q_FOREACH (Accounts::AccountId accountId, accountIds) {
        Accounts::Account *account = manager.account(accountId);
        if (account->providerName() == QStringLiteral("facebook")) {
            if (!account->enabled()) {
                accountsEnabled.insert(accountId, false);
            } else {
                Accounts::Service srv(manager.service("facebook-images"));
                if (!srv.isValid()) {
                    qWarning() << Q_FUNC_INFO << "no images service defined for Facebook account:" << accountId;
                    accountsEnabled.insert(accountId, false);
                } else {
                    account->selectService(srv);
                    if (account->enabled()) {
                        accountsEnabled.insert(accountId, true);
                        anyFbAccountsEnabled = true;
                    } else {
                        accountsEnabled.insert(accountId, false);
                    }
                }
            }
        }
    }
}

void FacebookImageCacheModelPrivate::queue(
        int row,
        FacebookImageDownloader::ImageType imageType,
        const QString &identifier,
        const QString &url)
{
    FacebookImageCacheModel *modelPtr = qobject_cast<FacebookImageCacheModel*>(q_ptr);
    if (downloader) {
        QVariantMap metadata;
        metadata.insert(QLatin1String(TYPE_KEY), imageType);
        metadata.insert(QLatin1String(IDENTIFIER_KEY), identifier);
        metadata.insert(QLatin1String(URL_KEY), url);
        metadata.insert(QLatin1String(ROW_KEY), row);
        metadata.insert(QLatin1String(MODEL_KEY), QVariant::fromValue<void*>((void*)modelPtr));

        downloader->queue(url, metadata);
    }
}

bool FacebookImageCacheModelPrivate::accountIsEnabled(int accountId)
{
    // If we've already cached the enabled-status of the account, just return it.
    // Note that we don't react to account-enabled-changed signals currently.
    if (!accountsEnabled.contains(accountId)) {
        // otherwise, find the enabled status of the account.
        if (!accountId) {
            qWarning() << Q_FUNC_INFO << "invalid account id";
            return false;
        }

        Accounts::Account *account = manager.account(accountId);
        if (!account) {
            qWarning() << Q_FUNC_INFO << "account is invalid, probably previously deleted - image data is orphaned!";
            return false;
        }

        if (!account->enabled()) {
            accountsEnabled.insert(accountId, false);
        } else {
            Accounts::Service srv(manager.service("facebook-images"));
            if (!srv.isValid()) {
                qWarning() << Q_FUNC_INFO << "no images service defined for Facebook account:" << accountId;
                accountsEnabled.insert(accountId, false);
            } else {
                account->selectService(srv);
                accountsEnabled.insert(accountId, account->enabled());
            }
        }
    }

    return accountsEnabled.value(accountId);
}

bool FacebookImageCacheModelPrivate::anyFacebookAccountsEnabled() const
{
    return anyFbAccountsEnabled;
}

FacebookImageCacheModel::FacebookImageCacheModel(QObject *parent)
    : AbstractSocialCacheModel(*(new FacebookImageCacheModelPrivate(this)), parent)
{
    Q_D(const FacebookImageCacheModel);
    connect(&d->database, &FacebookImagesDatabase::queryFinished,
            this, &FacebookImageCacheModel::queryFinished);
}

FacebookImageCacheModel::~FacebookImageCacheModel()
{
    Q_D(FacebookImageCacheModel);
    if (d->downloader) {
        d->downloader->removeModelFromHash(this);
    }
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
            d->downloader->removeModelFromHash(this);
        }

        d->downloader = downloader;
        d->downloader->addModelToHash(this);
        emit downloaderChanged();
    }
}

QVariant FacebookImageCacheModel::data(const QModelIndex &index, int role) const
{
    Q_D(const FacebookImageCacheModel);
    int row = index.row();
    if (row < 0 || row >= d->m_data.count()) {
        return QVariant();
    }

    if (role == FacebookImageCacheModel::Image) {
        if (d->m_data.at(row).value(role).toString().isEmpty()) {
            // haven't downloaded the image yet.  Download it.
            if (d->database.images().size() > row) {
                FacebookImage::ConstPtr imageData = d->database.images().at(row);
                FacebookImageCacheModelPrivate *nonconstD = const_cast<FacebookImageCacheModelPrivate*>(d);
                nonconstD->queue(row, FacebookImageDownloader::FullImage,
                                 imageData->fbImageId(),
                                 imageData->imageUrl());
            }
        }
    }

    return d->m_data.at(row).value(role);
}

void FacebookImageCacheModel::loadImages()
{
    refresh();
}

void FacebookImageCacheModel::refresh()
{
    Q_D(FacebookImageCacheModel);

    if (!d->anyFbAccountsEnabled) {
        return;
    }

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

// NOTE: this is now called directly by FacebookImageDownloader
// rather than connected to the imageDownloaded signal, for
// performance reasons.
void FacebookImageCacheModel::imageDownloaded(
        const QString &, const QString &path, const QVariantMap &imageData)
{
    Q_D(FacebookImageCacheModel);

    int row = imageData.value(ROW_KEY).toInt();
    if (row < 0 || row >= d->m_data.count()) {
        qWarning() << Q_FUNC_INFO
                   << "Invalid row:" << row
                   << "max row:" << d->m_data.count();
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

    QList<QVariantMap> thumbQueue;
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

        QString fbUserId;
        Q_FOREACH (const FacebookAlbum::ConstPtr & albumData, albumsData) {
            QMap<int, QVariant> albumMap;
            fbUserId = albumData->fbUserId();  // remember user id for 'All' album
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
            //:  Label for the "show all photos from all albums by this user" option
            //% "All"
            albumMap.insert(FacebookImageCacheModel::Title, qtTrId("nemo_socialcache_facebook_images_model-all-albums"));
            albumMap.insert(FacebookImageCacheModel::Count, count);
            albumMap.insert(FacebookImageCacheModel::UserId, fbUserId);
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
                QVariantMap thumbQueueData;
                thumbQueueData.insert("row", QVariant::fromValue<int>(i));
                thumbQueueData.insert("imageType", QVariant::fromValue<int>(FacebookImageDownloader::ThumbnailImage));
                thumbQueueData.insert("identifier", imageData->fbImageId());
                thumbQueueData.insert("url", imageData->thumbnailUrl());
                thumbQueue.append(thumbQueueData);
            }
            // note: we don't queue the image file until the user explicitly opens that in fullscreen.
            imageMap.insert(FacebookImageCacheModel::Thumbnail, imageData->thumbnailFile());
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

    // now download the queued thumbnails.
    foreach (const QVariantMap &thumbQueueData, thumbQueue) {
        d->queue(thumbQueueData["row"].toInt(),
                 static_cast<FacebookImageDownloader::ImageType>(thumbQueueData["imageType"].toInt()),
                 thumbQueueData["identifier"].toString(),
                 thumbQueueData["url"].toString());
    }
}
