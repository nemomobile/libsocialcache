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

#include "vkimagedownloader.h"
#include "vkimagedownloader_p.h"
#include "vkimagecachemodel.h"

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>

#include <QtDebug>

static const char *MODEL_KEY = "model";
static const char *TYPE_KEY = "type";
static const char *PHOTOID_KEY = "photo_id";
static const char *ALBUMID_KEY = "album_id";
static const char *OWNERID_KEY = "owner_id";
static const char *ACCOUNTID_KEY = "account_id";

VKImageDownloaderPrivate::VKImageDownloaderPrivate(VKImageDownloader *q)
    : AbstractImageDownloaderPrivate(q)
{
}

VKImageDownloaderPrivate::~VKImageDownloaderPrivate()
{
}

VKImageDownloader::VKImageDownloader(QObject *parent) :
    AbstractImageDownloader(*new VKImageDownloaderPrivate(this), parent)
{
    connect(this, &AbstractImageDownloader::imageDownloaded,
            this, &VKImageDownloader::invokeSpecificModelCallback);
}

VKImageDownloader::~VKImageDownloader()
{
}

void VKImageDownloader::addModelToHash(VKImageCacheModel *model)
{
    Q_D(VKImageDownloader);
    d->m_connectedModels.insert(model);
}

void VKImageDownloader::removeModelFromHash(VKImageCacheModel *model)
{
    Q_D(VKImageDownloader);
    d->m_connectedModels.remove(model);
}

/*
 * A VKImageDownloader can be connected to multiple models.
 * Instead of connecting the imageDownloaded signal directly to the
 * model, we connect it to this slot, which retrieves the target model
 * from the metadata map and invokes its callback directly.
 * This avoids a possibly large number of signal connections + invocations.
 */
void VKImageDownloader::invokeSpecificModelCallback(const QString &url, const QString &path, const QVariantMap &metadata)
{
    Q_D(VKImageDownloader);
    VKImageCacheModel *model = static_cast<VKImageCacheModel*>(metadata.value(MODEL_KEY).value<void*>());

    // check to see if the model was destroyed in the meantime.
    // If not, we can directly invoke the callback.
    if (d->m_connectedModels.contains(model)) {
        model->imageDownloaded(url, path, metadata);
    }
}

QString VKImageDownloader::outputFile(const QString &url, const QVariantMap &data) const
{
    Q_UNUSED(url);
    QString identifier = QStringLiteral("%1-%2-%3-%4")
                         .arg(data.value(QLatin1String(OWNERID_KEY)).toString())
                         .arg(data.value(QLatin1String(ALBUMID_KEY)).toString())
                         .arg(data.value(QLatin1String(PHOTOID_KEY)).toString())
                         .arg(data.value(QLatin1String(TYPE_KEY)).toString());
    return makeOutputFile(SocialSyncInterface::VK, SocialSyncInterface::Images, identifier);
}

void VKImageDownloader::dbQueueImage(const QString &url, const QVariantMap &data, const QString &file)
{
    Q_D(VKImageDownloader);
    QString photo_id = data.value(QLatin1String(PHOTOID_KEY)).toString();
    QString album_id = data.value(QLatin1String(ALBUMID_KEY)).toString();
    QString owner_id = data.value(QLatin1String(OWNERID_KEY)).toString();
    int accountId = data.value(QLatin1String(ACCOUNTID_KEY)).toInt();
    if (photo_id.isEmpty() || album_id.isEmpty() || owner_id.isEmpty() || accountId == 0) {
        qWarning() << "invalid photo metadata specified for downloaded image:" << photo_id << album_id << owner_id << accountId;
        return;
    }

    VKImage::Ptr image = VKImage::create(photo_id, album_id, owner_id,
                                         QString(), QString(), QString(),
                                         QString(), QString(),
                                         0, 0, 0,
                                         accountId);
    int type = data.value(QLatin1String(TYPE_KEY)).toInt();
    switch (type) {
    case ThumbnailImage:
        d->database.updateImageThumbnail(image, file);
        break;
    case FullImage:
        d->database.updateImageFile(image, file);
        break;
    }
}

void VKImageDownloader::dbWrite()
{
    Q_D(VKImageDownloader);
    d->database.commit();
}
