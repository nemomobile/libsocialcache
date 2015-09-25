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

#include "onedriveimagedownloader.h"
#include "onedriveimagedownloader_p.h"
#include "onedriveimagedownloaderconstants_p.h"

#include "onedriveimagecachemodel.h"

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <QtDebug>

static const char *MODEL_KEY = "model";
static const char *URL_KEY = "url";
static const char *TYPE_PHOTO = "photo";

OneDriveImageDownloader::UncachedImage::UncachedImage()
{}

OneDriveImageDownloader::UncachedImage::UncachedImage(const QString &imageId,
                                                      const QString &albumId,
                                                      int accountId,
                                                      QVariantList connectedModels)
    : imageId(imageId)
    , albumId(albumId)
    , accountId(accountId)
    , connectedModels(connectedModels)
{}

OneDriveImageDownloader::UncachedImage::UncachedImage(const UncachedImage &other)
    : imageId(other.imageId)
    , albumId(other.albumId)
    , accountId(other.accountId)
    , connectedModels(other.connectedModels)
{}

OneDriveImageDownloaderPrivate::OneDriveImageDownloaderPrivate(OneDriveImageDownloader *q)
    : AbstractImageDownloaderPrivate(q)
    , m_optimalThumbnailSize(180)
{
}

OneDriveImageDownloaderPrivate::~OneDriveImageDownloaderPrivate()
{
}

OneDriveImageDownloader::OneDriveImageDownloader(QObject *parent) :
    AbstractImageDownloader(*new OneDriveImageDownloaderPrivate(this), parent)
{
    connect(this, &AbstractImageDownloader::imageDownloaded,
            this, &OneDriveImageDownloader::invokeSpecificModelCallback);
}

OneDriveImageDownloader::~OneDriveImageDownloader()
{
}

void OneDriveImageDownloader::addModelToHash(OneDriveImageCacheModel *model)
{
    Q_D(OneDriveImageDownloader);
    d->m_connectedModels.insert(model);
}

void OneDriveImageDownloader::removeModelFromHash(OneDriveImageCacheModel *model)
{
    Q_D(OneDriveImageDownloader);
    d->m_connectedModels.remove(model);
}

int OneDriveImageDownloader::optimalThumbnailSize() const
{
    Q_D(const OneDriveImageDownloader);
    return d->m_optimalThumbnailSize;
}

void OneDriveImageDownloader::setOptimalThumbnailSize(int optimalThumbnailSize)
{
    Q_D(OneDriveImageDownloader);

    if (d->m_optimalThumbnailSize != optimalThumbnailSize) {
        d->m_optimalThumbnailSize = optimalThumbnailSize;
        emit optimalThumbnailSizeChanged();
    }
}

/*
 * A OneDriveImageDownloader can be connected to multiple models.
 * Instead of connecting the imageDownloaded signal directly to the
 * model, we connect it to this slot, which retrieves the target model
 * from the metadata map and invokes its callback directly.
 * This avoids a possibly large number of signal connections + invocations.
 */
void OneDriveImageDownloader::invokeSpecificModelCallback(const QString &url, const QString &path, const QVariantMap &metadata)
{
    Q_D(OneDriveImageDownloader);
    OneDriveImageCacheModel *model = static_cast<OneDriveImageCacheModel*>(metadata.value(MODEL_KEY).value<void*>());

    // check to see if the model was destroyed in the meantime.
    // If not, we can directly invoke the callback.
    if (d->m_connectedModels.contains(model)) {
        model->imageDownloaded(url, path, metadata);
    }
}

QString OneDriveImageDownloader::outputFile(const QString &url,
                                            const QVariantMap &data) const
{
    Q_UNUSED(url);

    // We create the identifier by appending the type to the real identifier
    QString identifier = data.value(QLatin1String(IDENTIFIER_KEY)).toString();
    if (identifier.isEmpty()) {
        return QString();
    }

    QString typeString = data.value(QLatin1String(TYPE_KEY)).toString();
    if (typeString.isEmpty()) {
        return QString();
    }

    identifier.append(typeString);

    return makeOutputFile(SocialSyncInterface::OneDrive, SocialSyncInterface::Images, identifier);
}

void OneDriveImageDownloader::dbQueueImage(const QString &url, const QVariantMap &data,
                                                       const QString &file)
{
    Q_D(OneDriveImageDownloader);
    Q_UNUSED(url);

    QString identifier = data.value(QLatin1String(IDENTIFIER_KEY)).toString();
    if (identifier.isEmpty()) {
        return;
    }
    int type = data.value(QLatin1String(TYPE_KEY)).toInt();

    switch (type) {
    case ThumbnailImage:
        d->database.updateImageThumbnail(identifier, file);
        break;
    }
}

void OneDriveImageDownloader::dbWrite()
{
    Q_D(OneDriveImageDownloader);

    d->database.commit();
}

void OneDriveImageDownloader::cacheImages(QList<OneDriveImageDownloader::UncachedImage> images)
{
    Q_D(OneDriveImageDownloader);

    QList<int> accessTokenRequests;

    d->m_cacheMutex.lock();

    foreach (const UncachedImage& image, images) {
        if (d->m_uncachedImages.contains(image.imageId)) {
            // already being processes, update connected models
            UncachedImage &uncached = d->m_uncachedImages[image.imageId];
            foreach (QVariant v, image.connectedModels) {
                if (!uncached.connectedModels.contains(v)) {
                    uncached.connectedModels.append(v);
                }
            }
            d->m_uncachedImages[image.imageId].connectedModels.append(image.connectedModels);
        } else {
            d->m_uncachedImages.insert(image.imageId, image);
        }
        if (!d->m_ongoingAlbumrequests.contains(image.albumId)) {
            if (!d->m_accountsWaitingForAccessToken.contains(image.accountId)
                   && !accessTokenRequests.contains(image.accountId)) {
                accessTokenRequests.append(image.accountId);
                d->m_accountsWaitingForAccessToken.append(image.accountId);
            }
            d->m_ongoingAlbumrequests.insert(image.albumId, image.accountId);
        }
    }

    d->m_cacheMutex.unlock();

    foreach (int accountId, accessTokenRequests) {
        emit accessTokenRequested(accountId);
    }
}

void OneDriveImageDownloader::accessTokenRetrived(const QString &accessToken, int accountId)
{
    Q_D(OneDriveImageDownloader);

    QMutexLocker locker(&d->m_cacheMutex);

    if (d->m_accountsWaitingForAccessToken.contains(accountId)) {
        d->m_accountsWaitingForAccessToken.removeAll(accountId);
        QMap<QString, int>::const_iterator i = d->m_ongoingAlbumrequests.constBegin();
        while (i != d->m_ongoingAlbumrequests.constEnd()) {
            if (i.value() == accountId) {
                requestImages(accountId, accessToken, i.key());
            }
            ++i;
        }
    }
}

void OneDriveImageDownloader::accessTokenFailed(int accountId)
{
    Q_D(OneDriveImageDownloader);

    d->m_cacheMutex.lock();
    d->m_accountsWaitingForAccessToken.removeAll(accountId);

    QStringList albums;

    QMap<QString, int>::const_iterator i = d->m_ongoingAlbumrequests.constBegin();
    while (i != d->m_ongoingAlbumrequests.constEnd()) {
        if (i.value() == accountId) {
            if (!albums.contains(i.key())) {
                albums.append(i.key());
            }
        }
        ++i;
    }

    d->m_cacheMutex.unlock();

    foreach (const QString album, albums) {
        clearRequests(album);
    }
}

void OneDriveImageDownloader::requestImages(int accountId, const QString &accessToken,
                                            const QString &albumId, const QString &nextRound)
{
    Q_D(OneDriveImageDownloader);

    QString path = nextRound.isEmpty() ? QStringLiteral("%1/files/?filter=photos&limit=100").arg(albumId)
                                       : nextRound;

    QUrl url(QStringLiteral("https://apis.live.net/v5.0/%1&access_token=%2&").arg(path).arg(accessToken));
    QNetworkReply *reply = d->networkAccessManager->get(QNetworkRequest(url));
    if (reply) {
        reply->setProperty("accountId", accountId);
        reply->setProperty("accessToken", accessToken);
        reply->setProperty("albumId", albumId);
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(errorHandler(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorsHandler(QList<QSslError>)));
        connect(reply, SIGNAL(finished()), this, SLOT(imagesFinishedHandler()));

        setupReplyTimeout(albumId, reply);
    } else {
        qWarning() << "unable to request data from OneDrive account with id" << accountId;
        clearRequests(albumId);
    }
}

void OneDriveImageDownloader::imagesFinishedHandler()
{
    Q_D(OneDriveImageDownloader);

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    bool isError = reply->property("isError").toBool();
    int accountId = reply->property("accountId").toInt();
    QString accessToken = reply->property("accessToken").toString();
    QString albumId = reply->property("albumId").toString();
    QByteArray replyData = reply->readAll();
    disconnect(reply);
    reply->deleteLater();
    removeReplyTimeout(albumId, reply);

    QList<QPair<QString, QString> > results;

    bool ok = false;
    QJsonObject parsed;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(replyData);
    ok = !jsonDocument.isEmpty();
    if (ok && jsonDocument.isObject()) {
        parsed =  jsonDocument.object();
    }

    if (isError || !ok || !parsed.contains(QLatin1String("data"))) {
        qWarning() << "Unable to read photos response for OneDrive account with id" << accountId;
        clearRequests(albumId);
        return;
    }

    QJsonArray data = parsed.value(QLatin1String("data")).toArray();
    if (data.size() == 0) {
        clearRequests(albumId);
        return;
    }

    // read the photos information
    foreach (const QJsonValue imageValue, data) {
        QJsonObject imageObject = imageValue.toObject();
        if (imageObject.isEmpty()) {
            continue;
        }

        QString type = imageObject.value(QLatin1String("type")).toString();
        if (type != QStringLiteral("photo")) {
            // should not happen as we have a filter in request, but just to be sure
            continue;
        }

        QString photoId = imageObject.value(QLatin1String("id")).toString();
        QString thumbnailUrl = imageObject.value(QLatin1String("picture")).toString();
        // Find optimal thumbnail and image source urls based on dimensions.

        QList<ImageSource> imageSources;
        QJsonArray images = imageObject.value(QLatin1String("images")).toArray();
        foreach (const QJsonValue &imageValue, images) {
            QJsonObject image = imageValue.toObject();
            imageSources << ImageSource(static_cast<int>(image.value(QLatin1String("width")).toDouble()),
                                        static_cast<int>(image.value(QLatin1String("height")).toDouble()),
                                        image.value(QLatin1String("source")).toString());
        }

        bool foundOptimalThumbnail = false;
        std::sort(imageSources.begin(), imageSources.end());
        Q_FOREACH (const ImageSource &img, imageSources) {
            if (!foundOptimalThumbnail && qMin(img.width, img.height) >= d->m_optimalThumbnailSize) {
                foundOptimalThumbnail = true;
                thumbnailUrl = img.sourceUrl;
            }
        }
        if (!foundOptimalThumbnail) {
            // just choose the largest one.
            thumbnailUrl = imageSources.last().sourceUrl;
        }

        results.append(qMakePair<QString,QString>(photoId, thumbnailUrl));
    }

    d->m_cacheMutex.lock();
    for (int i = 0; i < results.count(); ++i) {
        QPair<QString,QString> pair = results.at(i);
        if (d->m_uncachedImages.contains(pair.first)) {
            UncachedImage &uncached = d->m_uncachedImages[pair.first];

            foreach (QVariant modelPtr, uncached.connectedModels) {
                QVariantMap metadata;
                metadata.insert(QLatin1String(TYPE_KEY), TYPE_PHOTO);
                metadata.insert(QLatin1String(IDENTIFIER_KEY), pair.first);
                metadata.insert(QLatin1String(URL_KEY), pair.second);
                metadata.insert(QLatin1String(MODEL_KEY), modelPtr);
                queue(pair.second, metadata);
            }
        }
    }
    d->m_cacheMutex.unlock();

    // perform a continuation request if required.
    QJsonObject paging = parsed.value(QLatin1String("paging")).toObject();
    if (!paging.isEmpty()) {
        QString next = paging.value(QLatin1String("next")).toString();
        if (!next.isEmpty()) {
            requestImages(accountId, accessToken, albumId, next);
            return;
        }
    }

    // done, clear request queues
    clearRequests(albumId);
}

void OneDriveImageDownloader::sslErrorsHandler(const QList<QSslError> &errs)
{
    QString sslerrs;
    foreach (const QSslError &e, errs) {
        sslerrs += e.errorString() + "; ";
    }
    if (errs.size() > 0) {
        sslerrs.chop(2);
    }
    qWarning() << "OneDriveImageDownloader: request with account"
               << sender()->property("accountId").toInt()
               << "experienced ssl errors:" << sslerrs;
    // set "isError" on the reply so that finished() handler knows to ignore the result
    sender()->setProperty("isError", QVariant::fromValue<bool>(true));
    // Note: not all errors are "unrecoverable" errors, so we don't change the status here.
}

void OneDriveImageDownloader::errorHandler(QNetworkReply::NetworkError err)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (err == QNetworkReply::AuthenticationRequiredError) {
        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray jsonBody = reply->readAll();
        qWarning() << "OneDrive: would normally set CredentialsNeedUpdate for account"
                   << reply->property("accountId").toInt() << "but could be spurious\n"
                   << "    Http code:" << httpCode << "\n"
                   << "    Json body:\n" << jsonBody << "\n";
    }

    qWarning() << "OneDriveImageDownloader request with account"
               << sender()->property("accountId").toInt()
               << "experienced error:" << err << "\n"
               << QString::fromUtf8(reply->readAll());
    // set "isError" on the reply so that adapters know to ignore the result in the finished() handler
    reply->setProperty("isError", QVariant::fromValue<bool>(true));
    // Note: not all errors are "unrecoverable" errors, so we don't change the status here.
}

void OneDriveImageDownloader::clearRequests(const QString &albumId)
{
    Q_D(OneDriveImageDownloader);

    QMutexLocker locker(&d->m_cacheMutex);

    d->m_ongoingAlbumrequests.remove(albumId);

    QStringList removeImages;
    QMap<QString, OneDriveImageDownloader::UncachedImage>::const_iterator i = d->m_uncachedImages.constBegin();
    while (i != d->m_uncachedImages.constEnd()) {
        if (i.value().albumId == albumId) {
            removeImages.append(i.key());
        }
        ++i;
    }

    foreach (const QString imageId, removeImages) {
        d->m_uncachedImages.remove(imageId);
    }
}

void OneDriveImageDownloader::timeoutReply()
{
    Q_D(OneDriveImageDownloader);

    QTimer *timer = qobject_cast<QTimer*>(sender());
    QNetworkReply *reply = timer->property("networkReply").value<QNetworkReply*>();
    QString albumId = timer->property("albumId").toString();

    qWarning() << "OneDriveImageDownloader: network request timed out while performing sync for albumId" << albumId;

    d->m_networkReplyTimeouts[albumId].remove(reply);
    reply->setProperty("isError", QVariant::fromValue<bool>(true));
    reply->finished(); // invoke finished, so that the error handling there decrements the semaphore etc.
    reply->disconnect();
}

void OneDriveImageDownloader::setupReplyTimeout(const QString &albumId, QNetworkReply *reply)
{
    Q_D(OneDriveImageDownloader);

    // this function should be called whenever a new network request is performed.
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(60000);
    timer->setProperty("albumId", albumId);
    timer->setProperty("networkReply", QVariant::fromValue<QNetworkReply*>(reply));
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutReply()));
    timer->start();
    d->m_networkReplyTimeouts[albumId].insert(reply, timer);
}

void OneDriveImageDownloader::removeReplyTimeout(const QString &albumId, QNetworkReply *reply)
{
    Q_D(OneDriveImageDownloader);

    // this function should be called by the finished() handler for the reply.
    QTimer *timer = d->m_networkReplyTimeouts[albumId].value(reply);
    if (!reply) {
        return;
    }

    delete timer;
    d->m_networkReplyTimeouts[albumId].remove(reply);
}
