/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote 
 *     products derived from this software without specific prior written 
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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

    // TODO: remove existing file if needed

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

void FacebookImageDownloader::registerModel(AbstractSocialCacheModel *model)
{
    Q_D(FacebookImageDownloader);
    d->models.insert(model);
}

void FacebookImageDownloader::unregisterModel(AbstractSocialCacheModel *model)
{
    Q_D(FacebookImageDownloader);
    d->models.remove(model);
}
