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

#include <QtDebug>

static const char *MODEL_KEY = "model";

OneDriveImageDownloaderPrivate::OneDriveImageDownloaderPrivate(OneDriveImageDownloader *q)
    : AbstractImageDownloaderPrivate(q)
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
