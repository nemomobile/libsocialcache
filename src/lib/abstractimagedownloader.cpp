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
    : networkAccessManager(0), q_ptr(q), loadedCount(0)
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

        if (QNetworkReply *reply = q->createReply(info->url, info->data)) {
            QObject::connect(reply, &QNetworkReply::finished,
                    q, &AbstractImageDownloader::slotFinished);
            runningReplies.insert(reply, info);
            return;
        }

        // emit signal.  Empty file signifies error.
        emit q->imageDownloaded(info->url, QString(), info->data);
        delete info;
    }
}

static void readData(ImageInfo *info, QNetworkReply *reply)
{
    qint64 bytesAvailable = reply->bytesAvailable();
    if (bytesAvailable == 0) {
        qWarning() << Q_FUNC_INFO << "No image data available";
        return;
    }

    QByteArray buf = reply->readAll();
    info->file.write(buf);
}

void AbstractImageDownloader::readyRead()
{
    Q_D(AbstractImageDownloader);

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    ImageInfo *info = d->runningReplies.value(reply);
    if (info) {
        readData(info, reply);
    }
}

void AbstractImageDownloader::slotFinished()
{
    Q_D(AbstractImageDownloader);

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    ImageInfo *info = d->runningReplies.take(reply);
    if (!info) {
        return;
    }

    if (!info->file.open(QIODevice::ReadWrite)) {
        qWarning() << Q_FUNC_INFO << "Failed to open file for write" << info->file.errorString();
        emit imageDownloaded(info->url, QString(), info->data);
        return;
    }

    const QString fileName = info->file.fileName();
    readData(info, reply);
    info->file.close();

    QImageReader reader(fileName);
    if (reader.canRead()) {
        dbQueueImage(info->url, info->data, fileName);
        emit imageDownloaded(info->url, fileName, info->data);
    } else {
        emit imageDownloaded(info->url, QString(), info->data);
    }

    delete info;

    d->loadedCount ++;
    d->manageStack();

    if (d->loadedCount > MAX_BATCH_SAVE
        || (d->runningReplies.isEmpty() && d->stack.isEmpty())) {
        dbWrite();
        d->loadedCount = 0;
    }
}

AbstractImageDownloader::AbstractImageDownloader(QObject *parent)
    : QObject(parent)
    , d_ptr(new AbstractImageDownloaderPrivate(this))
{
    Q_D(AbstractImageDownloader);
    d->networkAccessManager = new QNetworkAccessManager(this);
}

AbstractImageDownloader::AbstractImageDownloader(AbstractImageDownloaderPrivate &dd, QObject *parent)
    : QObject(parent), d_ptr(&dd)
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


    Q_FOREACH (ImageInfo *info, d->runningReplies) {
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
    hash.addData(identifier.toUtf8());
    QByteArray hashedIdentifier = hash.result().toHex();

    QString path;
    if (dataType == SocialSyncInterface::Contacts) {
        path = QStringLiteral("%1/%2/%3/%4/%5/%6.jpg").arg(PRIVILEGED_DATA_DIR,
                                                 SocialSyncInterface::dataType(dataType),
                                                 QStringLiteral("avatars"),
                                                 SocialSyncInterface::socialNetwork(socialNetwork),
                                                 QChar(hashedIdentifier.at(0)),
                                                 identifier);
    } else {
        path = QStringLiteral("%1/%2/%3/%4/%5.jpg").arg(PRIVILEGED_DATA_DIR,
                                                 SocialSyncInterface::dataType(dataType),
                                                 SocialSyncInterface::socialNetwork(socialNetwork),
                                                 QChar(hashedIdentifier.at(0)),
                                                 identifier);
    }

    return path;
}

QString AbstractImageDownloader::makeOutputFile(SocialSyncInterface::SocialNetwork socialNetwork,
                                                SocialSyncInterface::DataType dataType,
                                                const QString &identifier,
                                                const QString &remoteUrl)
{
    // this function hashes the remote URL in order to increase the
    // chance that a changed remote url will result in resynchronisation
    // of the image, due to output file path mismatch.

    if (identifier.isEmpty() || remoteUrl.isEmpty()) {
        return QString();
    }

    QCryptographicHash urlHash(QCryptographicHash::Md5);
    urlHash.addData(remoteUrl.toUtf8());
    QString hashedUrl = QString::fromUtf8(urlHash.result().toHex());

    QCryptographicHash idHash(QCryptographicHash::Md5);
    idHash.addData(identifier.toUtf8());
    QByteArray hashedId = idHash.result().toHex();

    QString path;
    if (dataType == SocialSyncInterface::Contacts) {
        path = QStringLiteral("%1/%2/%3/%4/%5/%6.jpg").arg(PRIVILEGED_DATA_DIR,
                                                 SocialSyncInterface::dataType(dataType),
                                                 QStringLiteral("avatars"),
                                                 SocialSyncInterface::socialNetwork(socialNetwork),
                                                 QChar(hashedId.at(0)),
                                                 hashedUrl);
    } else {
        path = QStringLiteral("%1/%2/%3/%4/%5.jpg").arg(PRIVILEGED_DATA_DIR,
                                                 SocialSyncInterface::dataType(dataType),
                                                 SocialSyncInterface::socialNetwork(socialNetwork),
                                                 QChar(hashedId.at(0)),
                                                 hashedUrl);
    }

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
