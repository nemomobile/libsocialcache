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

        QString url = info->url;
        if (!info->redirectUrl.isEmpty()) {
            url = info->redirectUrl;
        }

        info->file.setFileName(q->outputFile(url, info->requestsData.first()));
        QDir parentDir = QFileInfo(info->file.fileName()).dir();
        if (!parentDir.exists()) {
            parentDir.mkpath(".");
        }

        if (QNetworkReply *reply = q->createReply(url, info->requestsData.first())) {
            QTimer *timer = new QTimer(q);
            timer->setInterval(60000);
            timer->setSingleShot(true);
            QObject::connect(timer, &QTimer::timeout,
                    q, &AbstractImageDownloader::timedOut);
            timer->start();
            replyTimeouts.insert(timer, reply);
            reply->setProperty("timeoutTimer", QVariant::fromValue<QTimer*>(timer));
            QObject::connect(reply, SIGNAL(finished()), q, SLOT(slotFinished())); // For some reason, this fixes an issue with oopp sync plugins
            runningReplies.insert(reply, info);
        } else {
            // emit signal.  Empty file signifies error.
            Q_FOREACH (const QVariantMap &metadata, info->requestsData) {
                emit q->imageDownloaded(info->url, QString(), metadata);
            }
            delete info;
        }
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
        qWarning() << Q_FUNC_INFO << "finished signal received with null reply";
        d->manageStack();
        return;
    }

    ImageInfo *info = d->runningReplies.take(reply);
    QTimer *timer = reply->property("timeoutTimer").value<QTimer*>();
    if (timer) {
        d->replyTimeouts.remove(timer);
        timer->stop();
        timer->deleteLater();
    }
    reply->deleteLater();
    if (!info) {
        qWarning() << Q_FUNC_INFO << "No image info associated with reply";
        d->manageStack();
        return;
    }

    QByteArray redirectedUrl = reply->rawHeader("Location");
    if (redirectedUrl.length() > 0) {
        // this is URL redirection
        info->redirectUrl = QString(redirectedUrl);
        d->stack.append(info);
        d->manageStack();
    } else {
        if (!info->file.open(QIODevice::ReadWrite)) {
            qWarning() << Q_FUNC_INFO << "Failed to open file for write" << info->file.errorString();
            Q_FOREACH (const QVariantMap &metadata, info->requestsData) {
                emit imageDownloaded(info->url, QString(), metadata);
            }
            d->manageStack();
            return;
        }

        const QString fileName = info->file.fileName();
        readData(info, reply);
        info->file.close();

        QImageReader reader(fileName);
        if (reader.canRead()) {
            dbQueueImage(info->url, info->requestsData.first(), fileName);
            Q_FOREACH (const QVariantMap &metadata, info->requestsData) {
                emit imageDownloaded(info->url, fileName, metadata);
            }
        } else {
            // the file is not in image format.
            info->file.remove(fileName); // remove artifacts.
            Q_FOREACH (const QVariantMap &metadata, info->requestsData) {
                emit imageDownloaded(info->url, QString(), metadata);
            }
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
}

void AbstractImageDownloader::timedOut()
{
    Q_D(AbstractImageDownloader);

    QTimer *timer = qobject_cast<QTimer*>(sender());
    if (timer) {
        QNetworkReply *reply = d->replyTimeouts.take(timer);
        if (reply) {
            reply->deleteLater();
            timer->deleteLater();
            ImageInfo *info = d->runningReplies.take(reply);
            qWarning() << Q_FUNC_INFO << "Image download request timed out";
            Q_FOREACH (const QVariantMap &metadata, info->requestsData) {
                emit imageDownloaded(info->url, QString(), metadata);
            }
        }
    }

    d->manageStack();
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
        qWarning() << Q_FUNC_INFO << "Cannot perform operation, database is not initialized";
        emit imageDownloaded(url, QString(), metadata); // empty file signifies error.
        return;
    }


    Q_FOREACH (ImageInfo *info, d->runningReplies) {
        if (info->url == url) {
            qWarning() << Q_FUNC_INFO << "duplicate running request, appending metadata.";
            info->requestsData.append(metadata);
            return;
        }
    }

    ImageInfo *info = 0;
    for (int i = 0; i < d->stack.count(); ++i) {
        if (d->stack.at(i)->url == url) {
            qWarning() << Q_FUNC_INFO << "duplicate queued request, appending metadata.";
            info = d->stack.takeAt(i);
            info->requestsData.append(metadata);
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
    Q_D(AbstractImageDownloader);
    QNetworkRequest request (url);

    for (QVariantMap::const_iterator iter = metadata.begin(); iter != metadata.end(); ++iter) {
        if (iter.key().startsWith("accessToken")) {
            request.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                                 QString(QLatin1String("Bearer ")).toUtf8() + iter.value().toString().toUtf8());
            break;
        }
    }

    qWarning() << "AbstractImageDownloader::about to fetch image:" << url;
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
