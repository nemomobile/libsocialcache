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

#include "abstractimagedownloader.h"

class OneDriveImageCacheModel;
class OneDriveImageDownloaderWorkerObject;
class OneDriveImageDownloaderPrivate;
class OneDriveImageDownloader : public AbstractImageDownloader
{
    Q_OBJECT
public:
    enum ImageType {
        // We cache only thumnail images, full size images are cached by UI code via SocialImageCache
        // which purges them after given number of days has passed.
        ThumbnailImage
    };

    explicit OneDriveImageDownloader(QObject *parent = 0);
    virtual ~OneDriveImageDownloader();

    // tracking object lifetime of models connected to this downloader.
    void addModelToHash(OneDriveImageCacheModel *model);
    void removeModelFromHash(OneDriveImageCacheModel *model);

protected:
    QString outputFile(const QString &url, const QVariantMap &data) const;

    void dbQueueImage(const QString &url, const QVariantMap &data, const QString &file);
    void dbWrite();

private Q_SLOTS:
    void invokeSpecificModelCallback(const QString &url, const QString &path, const QVariantMap &metadata);

private:
    Q_DECLARE_PRIVATE(OneDriveImageDownloader)
};

#endif // ONEDRIVEIMAGEDOWNLOADER_H
