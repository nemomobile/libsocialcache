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

#ifndef ONEDRIVEIMAGECACHEMODEL_H
#define ONEDRIVEIMAGECACHEMODEL_H

#include "abstractsocialcachemodel.h"
#include "onedriveimagedownloader.h"

class OneDriveImageCacheModelPrivate;
class OneDriveImageCacheModel: public AbstractSocialCacheModel
{
    Q_OBJECT
    Q_PROPERTY(OneDriveImageCacheModel::ModelDataType type READ type WRITE setType
               NOTIFY typeChanged)
    Q_PROPERTY(OneDriveImageDownloader * downloader READ downloader WRITE setDownloader
               NOTIFY downloaderChanged)

    Q_ENUMS(OneDriveGalleryRole)
    Q_ENUMS(ModelDataType)

public:
    enum OneDriveGalleryRole {
        OneDriveId = 0,
        Thumbnail,
        Image,
        Title,
        DateTaken,
        Width,
        Height,
        Count,
        MimeType,
        AccountId,
        UserId,
        Description
    };

    enum ModelDataType {
        None = 0, // used for resetting/refreshing the model.
        Users,
        Albums,
        Images
    };

    explicit OneDriveImageCacheModel(QObject *parent = 0);
    ~OneDriveImageCacheModel();

    QHash<int, QByteArray> roleNames() const;

    // properties
    OneDriveImageCacheModel::ModelDataType type() const;
    void setType(OneDriveImageCacheModel::ModelDataType type);

    OneDriveImageDownloader *downloader() const;
    void setDownloader(OneDriveImageDownloader *downloader);

    // from AbstractListModel
    QVariant data(const QModelIndex &index, int role) const;

public Q_SLOTS:
    void loadImages();
    void refresh();

Q_SIGNALS:
    void typeChanged();
    void downloaderChanged();

private Q_SLOTS:
    void queryFinished();
    void imageDownloaded(const QString &url, const QString &path, const QVariantMap &imageData);

private:
    Q_DECLARE_PRIVATE(OneDriveImageCacheModel)
    friend class OneDriveImageDownloader;
};

#endif // ONEDRIVEIMAGECACHEMODEL_H
