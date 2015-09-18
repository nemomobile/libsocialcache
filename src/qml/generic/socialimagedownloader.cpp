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

#include "socialimagedownloader.h"
#include "socialimagedownloader_p.h"

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>

#include <QtDebug>
#include <QUrl>
#include <QFileInfo>

SocialImageDownloaderPrivate::SocialImageDownloaderPrivate(SocialImageDownloader *q)
    : AbstractImageDownloaderPrivate(q)
{
}

SocialImageDownloaderPrivate::~SocialImageDownloaderPrivate()
{
    if (m_commitTimer.isActive()) {
        m_db.commit();
    }
    m_db.wait();
}

SocialImageDownloader::SocialImageDownloader(QObject *parent) :
    AbstractImageDownloader(*new SocialImageDownloaderPrivate(this), parent)
{
    Q_D(SocialImageDownloader);

    connect(this, &AbstractImageDownloader::imageDownloaded,
            this, &SocialImageDownloader::notifyImageCached);

    // Commit timer will wait 30 seconds for additional addImage database calls to avoid
    // unnecessary commits.
    d->m_commitTimer.setInterval(30000);
    d->m_commitTimer.setSingleShot(true);
    connect(&d->m_commitTimer, SIGNAL(timeout()), this, SLOT(commitTimerTimeout()));
}

SocialImageDownloader::~SocialImageDownloader()
{
}

QString SocialImageDownloader::cached(const QString &imageId)
{
    Q_D(SocialImageDownloader);

    QString recentById = d->m_recentItemsById.value(imageId);
    if (!recentById.isEmpty()) {
        return recentById;
    }

    SocialImage::ConstPtr image = d->m_db.imageById(imageId);
    if (image != 0) {
        d->m_recentItemsById.insert(imageId, image->imageFile());
        return image->imageFile();
    }

    return QString();
}

void SocialImageDownloader::imageFile(const QString &imageUrl,
                                      int accountId,
                                      QObject *caller,
                                      int expiresInDays,
                                      const QString &imageId,
                                      const QString &accessToken )
{
    Q_D(SocialImageDownloader);

    if (imageUrl.isEmpty() || caller == 0) {
        return;
    }

    QMutexLocker locker(&d->m_mutex);
    if (!imageId.isEmpty()) {
        // check if an image with same id was cached recently
        QString recentById = d->m_recentItemsById.value(imageId);
        if (!recentById.isEmpty()) {
            QMetaObject::invokeMethod(caller, "imageCached", Q_ARG(QVariant, recentById));
            return;
        }  else {
            SocialImage::ConstPtr image = d->m_db.imageById(imageId);
            if (image != 0) {
                d->m_recentItemsById.insert(imageId, image->imageFile());
                QMetaObject::invokeMethod(caller, "imageCached", Q_ARG(QVariant, image->imageFile()));
                return;
            }
        }
    } else {
        QString recent = d->m_recentItems.value(imageUrl);
        if (!recent.isEmpty()) {
            QMetaObject::invokeMethod(caller, "imageCached", Q_ARG(QVariant, recent));
            return;
        } else {
            SocialImage::ConstPtr image = d->m_db.image(imageUrl);
            if (image != 0) {
                d->m_recentItems.insert(imageUrl, image->imageFile());
                QMetaObject::invokeMethod(caller, "imageCached", Q_ARG(QVariant, image->imageFile()));
                return;
            }
        }
    }

    d->m_ongoingCalls.insert(imageUrl, QPointer<QObject>(caller));

    QVariantMap data;
    data.insert(QStringLiteral("accountId"), accountId);
    data.insert(QStringLiteral("expiresInDays"), expiresInDays);
    data.insert(QStringLiteral("imageId"), imageId);
    if (accessToken.length()) data.insert(QStringLiteral("accessToken"), accessToken);
    queue(imageUrl, data);
    return;
}

void SocialImageDownloader::removeFromRecentlyUsed(const QString &imageUrl)
{
    Q_D(SocialImageDownloader);

    QMutexLocker locker(&d->m_mutex);
    d->m_recentItems.remove(imageUrl);
}

void SocialImageDownloader::removeFromRecentlyUsedById(const QString &imageId)
{
    Q_D(SocialImageDownloader);

    QMutexLocker locker(&d->m_mutex);
    d->m_recentItemsById.remove(imageId);
}

void SocialImageDownloader::notifyImageCached(const QString &imageUrl,
                                              const QString &imageFile,
                                              const QVariantMap &metadata)
{
    Q_D(SocialImageDownloader);

    QMutexLocker locker(&d->m_mutex);

    QList< QPointer<QObject> > ongoingCalls = d->m_ongoingCalls.values(imageUrl);

    if (imageFile.isEmpty()) {
        qWarning() << "SocialImageDownloader: failed to download " << imageFile;
        for (int i = 0; i < ongoingCalls.count(); ++i) {
           if (ongoingCalls.at(i) != 0) {
               QMetaObject::invokeMethod(ongoingCalls.at(i).data(), "downloadError");
           }
        }
        d->m_ongoingCalls.remove(imageUrl);
        return;
    }

    d->m_recentItems.insert(imageUrl, imageFile);
    int accountId = metadata.value(QStringLiteral("accountId")).toInt();
    int expiresInDays = metadata.value(QStringLiteral("expiresInDays")).toInt();
    QString imageId = metadata.value(QStringLiteral("imageId")).toString();
    if (!imageId.isEmpty()) {
        d->m_recentItemsById.insert(imageId, imageFile);
    }
    QDateTime currentTime(QDateTime::currentDateTime());
    d->m_db.addImage(accountId, imageUrl, imageFile, currentTime,
                     currentTime.addDays(expiresInDays), imageId); // TODO Accesstoken?
    // We assume that there will consecutive addImage calls. Wait suitable
    // time before commiting.
    if (d->m_commitTimer.isActive()) {
        d->m_commitTimer.stop();
    }
    d->m_commitTimer.start();

    for (int i = 0; i < ongoingCalls.count(); ++i) {
       if (ongoingCalls.at(i) != 0) {
           QMetaObject::invokeMethod(ongoingCalls.at(i).data(), "imageCached", Q_ARG(QVariant, imageFile));
       }
    }
    d->m_ongoingCalls.remove(imageUrl);
}

QString SocialImageDownloader::outputFile(const QString &url,
                                           const QVariantMap &data) const
{
    Q_UNUSED(data);

    if (url.isEmpty()) {
        return QString();
    }

    QUrl name(url);
    QString ending;
    QStringList parts = name.fileName().split(".");
    if (parts.count() > 1) {
        ending = parts.last();
    }
    if (ending.isEmpty()) {
        // assume jpg
        ending = "jpg";
    }

    QCryptographicHash hash (QCryptographicHash::Md5);
    hash.addData(url.toUtf8());
    QByteArray hashedIdentifier = hash.result().toHex();

    QString path = QStringLiteral("%1%2/%3/%4/%5.%6").arg(PRIVILEGED_DATA_DIR,
                                                  SocialSyncInterface::dataType(SocialSyncInterface::Images),
                                                  "cache",
                                                  QChar(hashedIdentifier.at(0)),
                                                  hashedIdentifier,
                                                  ending);
    return path;
}

void SocialImageDownloader::commitTimerTimeout()
{
    Q_D(SocialImageDownloader);
    d->m_db.commit();
}
