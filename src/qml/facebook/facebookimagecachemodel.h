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

#ifndef FACEBOOKIMAGECACHEMODEL_H
#define FACEBOOKIMAGECACHEMODEL_H

#include "abstractsocialcachemodel.h"
#include "facebookimagedownloader.h"

class FacebookImageCacheModelPrivate;
class FacebookImageCacheModel: public AbstractSocialCacheModel
{
    Q_OBJECT
    Q_PROPERTY(FacebookImageCacheModel::ModelDataType type READ type WRITE setType
               NOTIFY typeChanged)
    Q_PROPERTY(FacebookImageDownloader * downloader READ downloader WRITE setDownloader
               NOTIFY downloaderChanged)

    Q_ENUMS(FacebookGalleryRole)
    Q_ENUMS(ModelDataType)

public:
    enum FacebookGalleryRole {
        FacebookId = 0,
        Thumbnail,
        Image,
        Title,
        DateTaken,
        Width,
        Height,
        Count,
        MimeType,
        AccountId,
        UserId
    };

    enum ModelDataType {
        None = 0, // used for resetting/refreshing the model.
        Users,
        Albums,
        Images
    };

    explicit FacebookImageCacheModel(QObject *parent = 0);
    QHash<int, QByteArray> roleNames() const;

    // properties
    FacebookImageCacheModel::ModelDataType type() const;
    void setType(FacebookImageCacheModel::ModelDataType type);

    FacebookImageDownloader *downloader() const;
    void setDownloader(FacebookImageDownloader *downloader);

public Q_SLOTS:
    void loadImages();

Q_SIGNALS:
    void typeChanged();
    void downloaderChanged();

private:
    Q_DECLARE_PRIVATE(FacebookImageCacheModel)
};

#endif // FACEBOOKIMAGECACHEMODEL_H
