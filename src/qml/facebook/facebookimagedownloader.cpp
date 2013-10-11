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

FacebookImageDownloaderWorkerObject::FacebookImageDownloaderWorkerObject()
    : AbstractImageDownloader(), m_initialized(false)
{
}

QString FacebookImageDownloaderWorkerObject::outputFile(const QString &url,
                                                        const QVariantMap &data) const
{
    Q_UNUSED(url)

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

bool FacebookImageDownloaderWorkerObject::dbInit()
{
    if (!m_initialized) {
        m_db.initDatabase();
        m_initialized = true;
    }

    return m_db.isValid();
}

void FacebookImageDownloaderWorkerObject::dbQueueImage(const QString &url, const QVariantMap &data,
                                                       const QString &file)
{
    Q_UNUSED(url)
    QString identifier = data.value(QLatin1String(IDENTIFIER_KEY)).toString();
    if (identifier.isEmpty()) {
        return;
    }
    int type = data.value(QLatin1String(TYPE_KEY)).toInt();

    switch (type) {
    case ThumbnailImage:
        m_db.updateImageThumbnail(identifier, file);
        break;
    case FullImage:
        m_db.updateImageFile(identifier, file);
        break;
    }
}

void FacebookImageDownloaderWorkerObject::dbWrite()
{
    m_db.write();
}

FacebookImageDownloaderPrivate::FacebookImageDownloaderPrivate(FacebookImageDownloader *q)
    : QObject(), q_ptr(q), m_workerObject(new FacebookImageDownloaderWorkerObject())
{
    m_workerThread.start(QThread::IdlePriority);
    m_workerObject->moveToThread(&m_workerThread);
}

FacebookImageDownloaderPrivate::~FacebookImageDownloaderPrivate()
{
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait();
    }
    m_workerObject->deleteLater();
}

FacebookImageDownloader::FacebookImageDownloader(QObject *parent) :
    QObject(parent), d_ptr(new FacebookImageDownloaderPrivate(this))
{
}

FacebookImageDownloader::~FacebookImageDownloader()
{
}

FacebookImageDownloaderWorkerObject * FacebookImageDownloader::workerObject() const
{
    Q_D(const FacebookImageDownloader);
    return d->m_workerObject;
}
