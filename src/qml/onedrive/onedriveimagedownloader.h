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

#ifndef ONEDRIVEIMAGEDOWNLOADER_H
#define ONEDRIVEIMAGEDOWNLOADER_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>

#include "abstractimagedownloader.h"

class OneDriveImageCacheModel;
class OneDriveImageDownloaderWorkerObject;
class OneDriveImageDownloaderPrivate;
class OneDriveImageDownloader : public AbstractImageDownloader
{
    Q_OBJECT
    Q_PROPERTY(int optimalThumbnailSize READ optimalThumbnailSize WRITE setOptimalThumbnailSize NOTIFY optimalThumbnailSizeChanged)
public:
    enum ImageType {
        // We cache only thumnail images, full size images are cached by UI code via SocialImageCache
        // which purges them after given number of days has passed.
        ThumbnailImage
    };

    struct UncachedImage {
        UncachedImage();
        UncachedImage(const QString &imageId,
                      const QString &albumId,
                      int accountId,
                      QVariantList connectedModels);
        UncachedImage(const UncachedImage &other);

        QString imageId;
        QString albumId;
        int accountId;
        QVariantList connectedModels;
    };

    explicit OneDriveImageDownloader(QObject *parent = 0);
    virtual ~OneDriveImageDownloader();

    // tracking object lifetime of models connected to this downloader.
    void addModelToHash(OneDriveImageCacheModel *model);
    void removeModelFromHash(OneDriveImageCacheModel *model);

    void cacheImages(QList<UncachedImage> images);

    int optimalThumbnailSize() const;
    void setOptimalThumbnailSize(int optimalThumbnailSize);

    Q_INVOKABLE void accessTokenRetrived(const QString &accessToken, int accountId);
    Q_INVOKABLE void accessTokenFailed(int accountId);

signals:
    void optimalThumbnailSizeChanged();
    void accessTokenRequested(int accountId);

protected:
    QString outputFile(const QString &url, const QVariantMap &data) const;

    void dbQueueImage(const QString &url, const QVariantMap &data, const QString &file);
    void dbWrite();

private:
    void requestImages(int accountId, const QString &accessToken,
                       const QString &albumId, const QString &nextRound = QString());
    void clearRequests(const QString &albumId);
    void setupReplyTimeout(const QString &albumId, QNetworkReply *reply);
    void removeReplyTimeout(const QString &albumId, QNetworkReply *reply);

    struct ImageSource {
        ImageSource(int width, int height, const QString &sourceUrl) : width(width), height(height), sourceUrl(sourceUrl) {}
        bool operator<(const ImageSource &other) const { return this->width < other.width; }
        int width;
        int height;
        QString sourceUrl;
    };

private Q_SLOTS:
    void invokeSpecificModelCallback(const QString &url, const QString &path, const QVariantMap &metadata);
    void imagesFinishedHandler();
    void errorHandler(QNetworkReply::NetworkError err);
    void sslErrorsHandler(const QList<QSslError> &errs);
    void timeoutReply();

private:
    Q_DECLARE_PRIVATE(OneDriveImageDownloader)
};

#endif // ONEDRIVEIMAGEDOWNLOADER_H
