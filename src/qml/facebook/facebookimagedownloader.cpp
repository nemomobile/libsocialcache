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

#include "facebookimagedownloader.h"
#include "facebookimagedownloader_p.h"
#include "facebookimagedownloaderconstants_p.h"

#include "facebookimagecachemodel.h"

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>

#include <QtDebug>

static const char *MODEL_KEY = "model";

FacebookImageDownloaderPrivate::FacebookImageDownloaderPrivate(FacebookImageDownloader *q)
    : AbstractImageDownloaderPrivate(q)
{
}

FacebookImageDownloaderPrivate::~FacebookImageDownloaderPrivate()
{
}

FacebookImageDownloader::FacebookImageDownloader(QObject *parent) :
    AbstractImageDownloader(*new FacebookImageDownloaderPrivate(this), parent)
{
    connect(this, &AbstractImageDownloader::imageDownloaded,
            this, &FacebookImageDownloader::invokeSpecificModelCallback);
}

FacebookImageDownloader::~FacebookImageDownloader()
{
}

void FacebookImageDownloader::addModelToHash(FacebookImageCacheModel *model)
{
    Q_D(FacebookImageDownloader);
    d->m_connectedModels.insert(model);
}

void FacebookImageDownloader::removeModelFromHash(FacebookImageCacheModel *model)
{
    Q_D(FacebookImageDownloader);
    d->m_connectedModels.remove(model);
}

/*
 * A FacebookImageDownloader can be connected to multiple models.
 * Instead of connecting the imageDownloaded signal directly to the
 * model, we connect it to this slot, which retrieves the target model
 * from the metadata map and invokes its callback directly.
 * This avoids a possibly large number of signal connections + invocations.
 */
void FacebookImageDownloader::invokeSpecificModelCallback(const QString &url, const QString &path, const QVariantMap &metadata)
{
    Q_D(FacebookImageDownloader);
    FacebookImageCacheModel *model = static_cast<FacebookImageCacheModel*>(metadata.value(MODEL_KEY).value<void*>());

    // check to see if the model was destroyed in the meantime.
    // If not, we can directly invoke the callback.
    if (d->m_connectedModels.contains(model)) {
        model->imageDownloaded(url, path, metadata);
    }
}

QString FacebookImageDownloader::outputFile(const QString &url,
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

    return makeOutputFile(SocialSyncInterface::Facebook, SocialSyncInterface::Images, identifier);
}

void FacebookImageDownloader::dbQueueImage(const QString &url, const QVariantMap &data,
                                                       const QString &file)
{
    Q_D(FacebookImageDownloader);
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
    case FullImage:
        d->database.updateImageFile(identifier, file);
        break;
    }
}

void FacebookImageDownloader::dbWrite()
{
    Q_D(FacebookImageDownloader);

    d->database.commit();
}
