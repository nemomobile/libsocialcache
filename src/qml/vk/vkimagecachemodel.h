/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
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

#ifndef VKIMAGECACHEMODEL_H
#define VKIMAGECACHEMODEL_H

#include "abstractsocialcachemodel.h"
#include "vkimagedownloader.h"

class VKImageCacheModelPrivate;
class VKImageCacheModel: public AbstractSocialCacheModel
{
    Q_OBJECT
    Q_PROPERTY(VKImageCacheModel::ModelDataType type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(VKImageDownloader * downloader READ downloader WRITE setDownloader NOTIFY downloaderChanged)
    Q_ENUMS(VKGalleryRole)
    Q_ENUMS(ModelDataType)

public:
    enum VKGalleryRole {
        PhotoId = 0,
        AlbumId,
        UserId,
        AccountId,
        Text,
        Date,
        Width,
        Height,
        Thumbnail,
        Image,
        Count,
        MimeType
    };

    enum ModelDataType {
        None = 0, // used for resetting/refreshing the model.
        Users,
        Albums,
        Images
    };

    explicit VKImageCacheModel(QObject *parent = 0);
    ~VKImageCacheModel();

    QHash<int, QByteArray> roleNames() const;

    // properties
    VKImageCacheModel::ModelDataType type() const;
    void setType(VKImageCacheModel::ModelDataType type);

    VKImageDownloader *downloader() const;
    void setDownloader(VKImageDownloader *downloader);

    // from AbstractListModel
    QVariant data(const QModelIndex &index, int role) const;

    // since VK doens't use globally-unique identifiers, we need to encode breadcrumb information into the node identifier.
    Q_INVOKABLE QString constructNodeIdentifier(int accountId, const QString &user_id, const QString &album_id, const QString &photo_id);
    Q_INVOKABLE QVariantMap parseNodeIdentifier(const QString &nid) const; // { "accountId"=int, "user_id"=string, "album_id"=string, "photo_id"=string }

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
    Q_DECLARE_PRIVATE(VKImageCacheModel)
    friend class VKImageDownloader;
};

#endif // VKIMAGECACHEMODEL_H
