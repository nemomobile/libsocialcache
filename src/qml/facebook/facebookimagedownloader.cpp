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
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

static int MAX_SIMULTANEOUS_DOWNLOAD = 10;
#define SOCIALCACHE_FACEBOOK_IMAGE_DIR   PRIVILEGED_DATA_DIR + QLatin1String("/Images/")

bool operator==(const FacebookImageDownloaderImageData &data1,
                const FacebookImageDownloaderImageData &data2)
{
    if (data1.url != data2.url) {
        return false;
    }

    if (data1.identifier != data2.identifier) {
        return false;
    }

    return data1.type == data2.type && data1.imageType == data2.imageType;
}

FacebookImageDownloaderWorkerObject::FacebookImageDownloaderWorkerObject()
    : QObject(), FacebookImagesDatabase(), m_networkAccessManager(new QNetworkAccessManager(this))
    , m_loadedCount(0)
{
    initDatabase();
}

void FacebookImageDownloaderWorkerObject::queue(const FacebookImageDownloaderImageData &data)
{
    if (m_stack.contains(data)) {
        m_stack.removeAll(data);
    }

    m_stack.prepend(data);
    manageStack();
}

void FacebookImageDownloaderWorkerObject::manageStack()
{
    while (m_runningReplies.count() < MAX_SIMULTANEOUS_DOWNLOAD && !m_stack.isEmpty()) {
        // Create a reply to download the image
        FacebookImageDownloaderImageData data = m_stack.takeFirst();

        QNetworkRequest request (data.url);
        QNetworkReply *reply = m_networkAccessManager->get(request);
        connect(reply, &QNetworkReply::finished,
                this, &FacebookImageDownloaderWorkerObject::slotImageDownloaded);
        m_runningReplies.insert(reply, data);
    }
}

void FacebookImageDownloaderWorkerObject::slotImageDownloaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    const FacebookImageDownloaderImageData &data = m_runningReplies.value(reply);
    m_runningReplies.remove(reply);

    if (data.identifier.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Empty id";
        return;
    }

    QString imageType;
    switch (data.imageType) {
    case FacebookImageDownloaderImageData::ThumbnailImage:
        imageType = QLatin1String("thumb");
        break;
    case FacebookImageDownloaderImageData::FullImage:
        imageType = QLatin1String("image");
        break;
    }

    QImage image;
    bool loadedOk = image.loadFromData(reply->readAll());
    if (!loadedOk || image.isNull()) {
        qWarning() << Q_FUNC_INFO << "Downloaded" << imageType << "for" << data.identifier
                   << "but could not load image";
        return;
    }

    // Save the new image (eg fbphotoid-thumb.jpg or fbphotoid-image.jpg)
    QString newName = QString(QLatin1String("%1%2-%3.jpg")).arg(SOCIALCACHE_FACEBOOK_IMAGE_DIR,
                                                                data.identifier, imageType);

    bool saveOk = image.save(newName);
    if (!saveOk) {
        qWarning() << Q_FUNC_INFO << "Cannot save image";
        return;
    }

    // Update database
    switch (data.type) {
        case FacebookImageDownloaderImageData::User:
            updateUserThumbnail(data.identifier, newName);
            break;
        case FacebookImageDownloaderImageData::Image:
            switch (data.imageType) {
            case FacebookImageDownloaderImageData::ThumbnailImage:
                updateImageThumbnail(data.identifier, newName);
                break;
            case FacebookImageDownloaderImageData::FullImage:
                updateImageFile(data.identifier, newName);
                break;
            }
            break;
        default:
            break;
    }

    // Emit signal
    emit dataUpdated(data.url, newName);

    m_loadedCount ++;
    manageStack();

    if (m_loadedCount > MAX_SIMULTANEOUS_DOWNLOAD * 2
        || (m_runningReplies.isEmpty() && m_stack.isEmpty())) {
        write();
        m_loadedCount = 0;
    }
}

FacebookImageDownloaderPrivate::FacebookImageDownloaderPrivate(FacebookImageDownloader *q)
    : QObject(), q_ptr(q), workerObject(new FacebookImageDownloaderWorkerObject())
{
    m_workerThread.start(QThread::LowestPriority);
    workerObject->moveToThread(&m_workerThread);
}

FacebookImageDownloaderPrivate::~FacebookImageDownloaderPrivate()
{
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait();
    }
    workerObject->deleteLater();
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
    return d->workerObject;
}
