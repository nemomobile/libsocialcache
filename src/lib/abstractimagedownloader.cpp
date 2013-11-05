/*
 * Copyright (C) 2013 Lucien Xu <sfietkonstantin@free.fr>
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

#include "abstractimagedownloader.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QCryptographicHash>
#include <QtCore/QStandardPaths>
#include <QtGui/QImageReader>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QtDebug>

#include "abstractimagedownloader_p.h"

// The AbstractImageDownloader is a class used to build image downloader objects
//
// An image downloader object is a QObject based object that lives in
// a lower priority thread, downloads images from social networks
// and updates a database.
//
// This object do not expose many methods. Instead, since it lives
// in it's own thread, communications should be done using signals
// and slots.
//
// To download an image, the AbstractImagesDownloader::queue slot
// should be used, and when the download is completed, the
// AbstractImagesDownloaderPrivate::imageDownloaded will be emitted.

static int MAX_SIMULTANEOUS_DOWNLOAD = 5;
static int MAX_BATCH_SAVE = 50;

AbstractImageDownloaderPrivate::AbstractImageDownloaderPrivate(AbstractImageDownloader *q)
    : QObject(q), networkAccessManager(0), q_ptr(q), loadedCount(0)
{
}

AbstractImageDownloaderPrivate::~AbstractImageDownloaderPrivate()
{
}

void AbstractImageDownloaderPrivate::manageStack()
{
    Q_Q(AbstractImageDownloader);
    while (runningReplies.count() < MAX_SIMULTANEOUS_DOWNLOAD && !stack.isEmpty()) {
        // Create a reply to download the image
        ImageInfo *info = stack.takeLast();

        info->file.setFileName(q->outputFile(info->url, info->data));

        QDir parentDir = QFileInfo(info->file.fileName()).dir();
        if (!parentDir.exists()) {
            parentDir.mkpath(".");
        }

        if (!info->file.open(QIODevice::ReadWrite)) {
        } else if (QNetworkReply *reply = q->createReply(info->url, info->data)) {
            reply->setReadBufferSize(250000);

            connect(reply, &QIODevice::readyRead, this, &AbstractImageDownloaderPrivate::readyRead);
            connect(reply, &QNetworkReply::finished,
                    this, &AbstractImageDownloaderPrivate::slotFinished);

            runningReplies.insert(reply, info);

            return;
        }
        qWarning() << Q_FUNC_INFO << "Failed to open file for write" << info->file.errorString();

        // emit signal.  Empty file signifies error.
        emit q->imageDownloaded(info->url, QString(), info->data);

        delete info;
    }
}

static void readData(ImageInfo *info, QNetworkReply *reply)
{
    qint64 size = info->file.size();
    qint64 bytesAvailable = reply->bytesAvailable();
    if (bytesAvailable == 0)
        return;

    info->file.resize(size + bytesAvailable);
    if (uchar *fileData = info->file.map(size, bytesAvailable)) {
        char *buffer = reinterpret_cast<char *>(fileData);
        while (bytesAvailable > 0) {
            qint64 bytesRead = reply->read(buffer, bytesAvailable);
            bytesAvailable -= bytesRead;
            buffer += bytesRead;

            if (bytesRead > 0) {
                break;
            }
        }
        info->file.unmap(fileData);
    }
}

void AbstractImageDownloaderPrivate::readyRead()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    ImageInfo *info = runningReplies.value(reply);
    if (info) {
        readData(info, reply);
    }
}

void AbstractImageDownloaderPrivate::slotFinished()
{
    Q_Q(AbstractImageDownloader);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    ImageInfo *info = runningReplies.take(reply);
    if (!info) {
        return;
    }

    readData(info, reply);

    const QString fileName = info->file.fileName();


    info->file.close();


    QImageReader reader(fileName);
    if (reader.canRead()) {
        q->dbQueueImage(info->url, info->data, fileName);

        // Emit signal
        emit q->imageDownloaded(info->url, fileName, info->data);
    } else {
        emit q->imageDownloaded(info->url, QString(), info->data);
    }

    delete info;

    loadedCount ++;
    manageStack();

    if (loadedCount > MAX_BATCH_SAVE
            || (runningReplies.isEmpty() && stack.isEmpty())) {
        q->dbWrite();
        loadedCount = 0;
    }
}

AbstractImageDownloader::AbstractImageDownloader() :
    QObject(), d_ptr(new AbstractImageDownloaderPrivate(this))
{
    Q_D(AbstractImageDownloader);
    d->networkAccessManager = new QNetworkAccessManager(this);
}

AbstractImageDownloader::~AbstractImageDownloader()
{
}

void AbstractImageDownloader::queue(const QString &url, const QVariantMap &metadata)
{
    Q_D(AbstractImageDownloader);
    if (!dbInit()) {
        qWarning() << "Cannot perform operation, database is not initialized";
        emit imageDownloaded(url, QString(), metadata); // empty file signifies error.
        return;
    }


    foreach (ImageInfo *info, d->runningReplies) {
        if (info->url == url) {
            return;
        }
    }

    ImageInfo *info = 0;
    for (int i = 0; i < d->stack.count(); ++i) {
        if (d->stack.at(i)->url == url) {
            info = d->stack.takeAt(i);
            break;
        }
    }

    if (!info) {
        info = new ImageInfo(url, metadata);
    }

    d->stack.append(info);
    d->manageStack();
}

QNetworkReply *AbstractImageDownloader::createReply(const QString &url, const QVariantMap &metadata)
{
    Q_UNUSED(metadata)
    Q_D(AbstractImageDownloader);
    QNetworkRequest request (url);
    return d->networkAccessManager->get(request);
}

QString AbstractImageDownloader::makeOutputFile(SocialSyncInterface::SocialNetwork socialNetwork,
                                                 SocialSyncInterface::DataType dataType,
                                                 const QString &identifier)
{
    if (identifier.isEmpty()) {
        return QString();
    }

    QCryptographicHash hash (QCryptographicHash::Md5);
    hash.addData(identifier.toLocal8Bit());
    QByteArray hashedIdentifier = hash.result().toHex();
    QString firstLetter = QString::fromLatin1(hashedIdentifier.left(1));

    QString path = QString("%1/%2/%3/%4/%5.jpg").arg(PRIVILEGED_DATA_DIR,
                                                        SocialSyncInterface::dataType(dataType),
                                                        SocialSyncInterface::socialNetwork(socialNetwork),
                                                        firstLetter, identifier);
    return path;
}

bool AbstractImageDownloader::dbInit()
{
    return true;
}

void AbstractImageDownloader::dbQueueImage(const QString &url, const QVariantMap &metadata,
                                           const QString &file)
{
    Q_UNUSED(url)
    Q_UNUSED(metadata)
    Q_UNUSED(file)
}

void AbstractImageDownloader::dbWrite()
{
}

bool AbstractImageDownloader::dbClose()
{
    return true;
}
