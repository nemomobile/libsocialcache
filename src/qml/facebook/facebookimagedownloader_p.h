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
#include <QtCore/QMetaType>
#include <QtCore/QThread>
#include <QtCore/QSet>
#include "facebookimagesdatabase.h"

// We try to minimize the use of signals and slots exposed to QML,
// so we FacebookImageDownloader only expose one method in C++.
// This method exposes the FacebookImageDownloaderWorkerObject.
//
// The FacebookImageDownloaderWorkerObject is used to perform
// download of Facebook images, and notify that images are downloaded,
// so that models can be notified and be changed.
//
// This object lives in a different thread, so it should communicates
// with other objects using signals and slots. The important signals
// and slots are FacebookImageDownloaderWorkerObject::queue and
// FacebookImageDownloaderWorkerObject::dataUpdated.
//
// The call path that is being done is
// FacebookImageWorkerObject::refresh calls FacebookImageWorkerObject::queue.
// This triggers an emission of requestQueue. This signal is connected to
// FacebookImageDownloaderWorkerObject::queue, so FacebookImageDownloaderWorkerObject
// starts downloading data. When data is downloaded,
// FacebookImageDownloaderWorkerObject::dataUpdated is sent, and triggers
// FacebookImageCacheModelPrivate::slotDataUpdated that changes the model.
//
// Basically we communicates between internal objects, and never touch the
// QML API.
struct FacebookImageDownloaderImageData
{
    enum Type {
        User,
        Album,
        Image
    };
    enum ImageType {
        ThumbnailImage,
        FullImage
    };
    Type type;
    ImageType imageType;
    QString identifier;
    QString url;
};

Q_DECLARE_METATYPE(FacebookImageDownloaderImageData)

bool operator==(const FacebookImageDownloaderImageData &data1,
                const FacebookImageDownloaderImageData &data2);

class QNetworkAccessManager;
class QNetworkReply;
class FacebookImageDownloaderWorkerObject: public QObject, private FacebookImagesDatabase
{
    Q_OBJECT
public:
    explicit FacebookImageDownloaderWorkerObject();
public Q_SLOTS:
    void queue(const FacebookImageDownloaderImageData &data);
Q_SIGNALS:
    void dataUpdated(const QString &url, const QString &path);
private:
    void manageStack();
    QNetworkAccessManager *m_networkAccessManager;
    QMap<QNetworkReply *, FacebookImageDownloaderImageData> m_runningReplies;
    QList<FacebookImageDownloaderImageData> m_stack;
    int m_loadedCount;
private slots:
    void slotImageDownloaded();
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
    FacebookImageDownloaderWorkerObject * workerObject;
    Q_DECLARE_PUBLIC(FacebookImageDownloader)
};


#endif // FACEBOOKIMAGEDOWNLOADER_P_H
