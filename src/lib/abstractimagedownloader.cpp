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
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QCryptographicHash>
#include <QtCore/QStandardPaths>
#include <QtGui/QImage>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

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
typedef QPair<QString, QVariantMap> ImageInfo;

class AbstractImageDownloaderPrivate: public QObject
{
    Q_OBJECT
public:
    explicit AbstractImageDownloaderPrivate(AbstractImageDownloader *q);
    virtual ~AbstractImageDownloaderPrivate();
    void manageStack();
    QNetworkAccessManager *networkAccessManager;
    QMap<QNetworkReply *, ImageInfo> runningReplies;
    QList<ImageInfo> stack;
    int loadedCount;
protected:
    AbstractImageDownloader * const q_ptr;
private:
    Q_DECLARE_PUBLIC(AbstractImageDownloader)
private Q_SLOTS:
    void slotFinished();
};

AbstractImageDownloaderPrivate::AbstractImageDownloaderPrivate(AbstractImageDownloader *q)
    : QObject(q), networkAccessManager(0), loadedCount(0), q_ptr(q)
{
}

AbstractImageDownloaderPrivate::~AbstractImageDownloaderPrivate()
{
}

void AbstractImageDownloaderPrivate::manageStack()
{
    while (runningReplies.count() < MAX_SIMULTANEOUS_DOWNLOAD && !stack.isEmpty()) {
        // Create a reply to download the image
        ImageInfo data = stack.takeFirst();

        QNetworkRequest request (data.first);
        QNetworkReply *reply = networkAccessManager->get(request);
        connect(reply, &QNetworkReply::finished,
                this, &AbstractImageDownloaderPrivate::slotFinished);
        runningReplies.insert(reply, data);
    }
}

void AbstractImageDownloaderPrivate::slotFinished()
{
    Q_Q(AbstractImageDownloader);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) {
        return;
    }

    const ImageInfo &data = runningReplies.value(reply);
    runningReplies.remove(reply);

    QImage image;
    bool loadedOk = image.loadFromData(reply->readAll());
    reply->deleteLater();
    if (!loadedOk || image.isNull()) {
        qWarning() << Q_FUNC_INFO << "Data downloaded from" << data.first
                   << "is not an image";
        return;
    }

    // Save the new image (eg fbphotoid-thumb.jpg or fbphotoid-image.jpg)
    QString file = q->outputFile(data.first, data.second);
    if (file.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Output file is not valid";
        return;
    }

    QDir parentDir = QFileInfo(file).dir();
    if (!parentDir.exists()) {
        QDir::root().mkpath(parentDir.absolutePath());
    }

    bool saveOk = image.save(file);
    if (!saveOk) {
        qWarning() << Q_FUNC_INFO << "Cannot save image downloaded from" << data.first;
        return;
    }

    q->dbQueueImage(data.first, data.second, file);

    // Emit signal
    emit q->imageDownloaded(data.first, file);

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

void AbstractImageDownloader::queue(const QString &url, const QVariantMap &data)
{
    Q_D(AbstractImageDownloader);
    if (!dbInit()) {
        qWarning() << "Cannot perform operation, database is not initialized";
        return;
    }

    ImageInfo info = ImageInfo(url, data);
    if (d->stack.contains(info)) {
        d->stack.removeAll(info);
    }

    d->stack.prepend(info);
    d->manageStack();
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

#include "abstractimagedownloader.moc"
