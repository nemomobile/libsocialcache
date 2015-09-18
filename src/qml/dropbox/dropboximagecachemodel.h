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

#ifndef DROPBOXIMAGECACHEMODEL_H
#define DROPBOXIMAGECACHEMODEL_H

#include "abstractsocialcachemodel.h"
#include "dropboximagedownloader.h"

class DropboxImageCacheModelPrivate;
class DropboxImageCacheModel: public AbstractSocialCacheModel
{
    Q_OBJECT
    Q_PROPERTY(DropboxImageCacheModel::ModelDataType type READ type WRITE setType
               NOTIFY typeChanged)
    Q_PROPERTY(DropboxImageDownloader * downloader READ downloader WRITE setDownloader
               NOTIFY downloaderChanged)

    Q_ENUMS(DropboxGalleryRole)
    Q_ENUMS(ModelDataType)

public:
    enum DropboxGalleryRole {
        DropboxId = 0,
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
        AccessToken
    };

    enum ModelDataType {
        None = 0, // used for resetting/refreshing the model.
        Users,
        Albums,
        Images
    };

    explicit DropboxImageCacheModel(QObject *parent = 0);
    ~DropboxImageCacheModel();

    QHash<int, QByteArray> roleNames() const;

    // properties
    DropboxImageCacheModel::ModelDataType type() const;
    void setType(DropboxImageCacheModel::ModelDataType type);

    DropboxImageDownloader *downloader() const;
    void setDownloader(DropboxImageDownloader *downloader);

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
    Q_DECLARE_PRIVATE(DropboxImageCacheModel)
    friend class DropboxImageDownloader;
};

#endif // DROPBOXIMAGECACHEMODEL_H
