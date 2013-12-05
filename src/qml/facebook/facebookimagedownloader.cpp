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

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>

#include <QtDebug>

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
}

FacebookImageDownloader::~FacebookImageDownloader()
{
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
