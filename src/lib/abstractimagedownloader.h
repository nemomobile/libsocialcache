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

#ifndef ABSTRACTIMAGEDOWNLOADER_H
#define ABSTRACTIMAGEDOWNLOADER_H

#include "socialsyncinterface.h"

#include <QtCore/QObject>
#include <QtCore/QVariantMap>

class QNetworkReply;
class AbstractImageDownloaderPrivate;
class AbstractImageDownloader : public QObject
{
    Q_OBJECT
public:
    AbstractImageDownloader(QObject *parent = 0);
    virtual ~AbstractImageDownloader();

public Q_SLOTS:
    void queue(const QString &url, const QVariantMap &data);

Q_SIGNALS:
    void imageDownloaded(const QString &url, const QString &path, const QVariantMap &metadata);

protected:
    explicit AbstractImageDownloader(AbstractImageDownloaderPrivate &dd, QObject *parent);

    static QString makeOutputFile(SocialSyncInterface::SocialNetwork socialNetwork,
                                  SocialSyncInterface::DataType dataType,
                                  const QString &identifier);
    static QString makeOutputFile(SocialSyncInterface::SocialNetwork socialNetwork,
                                  SocialSyncInterface::DataType dataType,
                                  const QString &identifier,
                                  const QString &remoteUrl); // added to retain BC.

    virtual QNetworkReply * createReply(const QString &url, const QVariantMap &metadata);

    // Output file based on passed data
    virtual QString outputFile(const QString &url, const QVariantMap &metadata) const = 0;

    // Init the database if not initialized
    // used to delay initialization of the database
    virtual bool dbInit();

    // Queue an image in the database
    virtual void dbQueueImage(const QString &url, const QVariantMap &metadata,
                              const QString &file);

    // Write in the database
    virtual void dbWrite();

    QScopedPointer<AbstractImageDownloaderPrivate> d_ptr;

private Q_SLOTS:
    void readyRead();
    void slotFinished();
    void timedOut();

private:
    Q_DECLARE_PRIVATE(AbstractImageDownloader)
};

#endif // ABSTRACTIMAGEDOWNLOADER_H
