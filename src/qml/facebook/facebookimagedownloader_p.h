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

#ifndef FACEBOOKIMAGEDOWNLOADER_P_H
#define FACEBOOKIMAGEDOWNLOADER_P_H

#include <QtCore/QObject>
#include <QtCore/QThread>
#include "abstractimagedownloader.h"
#include "facebookimagesdatabase.h"

// We try to minimize the use of signals and slots exposed to QML,
// so we FacebookImageDownloader only expose one method in C++.
// This method exposes the FacebookImageDownloaderWorkerObject.
//
// FacebookImageDownloaderWorkerObject is a subclass of AbstractImagesDownloader,
// and is an image downloader object. It lives in it's own low priority thread.
//
// The call path that is being done is
// FacebookImageWorkerObject::refresh calls FacebookImageWorkerObject::queue.
// This triggers an emission of requestQueue. This signal is connected to
// FacebookImageDownloaderWorkerObject::queue, so FacebookImageDownloaderWorkerObject
// starts downloading data. When data is downloaded,
// FacebookImageDownloaderWorkerObject::imageDownloaded is sent, and triggers
// FacebookImageCacheModelPrivate::slotDataUpdated that changes the model.
//
// Basically we communicates between internal objects, and never touch the
// QML API.

class FacebookImageDownloaderWorkerObject: public AbstractImageDownloader
{
    Q_OBJECT
public:
    enum ImageType {
        InvalidImage,
        ThumbnailImage,
        FullImage
    };

    explicit FacebookImageDownloaderWorkerObject();
protected:
    QString outputFile(const QString &url, const QVariantMap &data) const;
    bool dbInit();
    void dbQueueImage(const QString &url, const QVariantMap &data, const QString &file);
    void dbWrite();
private:
    bool m_initialized;
    FacebookImagesDatabase m_db;
};

class AbstractSocialCacheModel;
class FacebookImageDownloader;
class FacebookImageDownloaderPrivate: public QObject
{
    Q_OBJECT
public:
    explicit FacebookImageDownloaderPrivate(FacebookImageDownloader *q);
    virtual ~FacebookImageDownloaderPrivate();
protected:
    FacebookImageDownloader * const q_ptr;
private:
    QThread m_workerThread;
    FacebookImageDownloaderWorkerObject *m_workerObject;
    Q_DECLARE_PUBLIC(FacebookImageDownloader)
};


#endif // FACEBOOKIMAGEDOWNLOADER_P_H
