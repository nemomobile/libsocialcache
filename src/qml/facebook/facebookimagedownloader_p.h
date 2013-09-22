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

#ifndef FACEBOOKIMAGEDOWNLOADER_P_H
#define FACEBOOKIMAGEDOWNLOADER_P_H

#include <QtCore/QObject>
#include <QtCore/QMetaType>
#include <QtCore/QThread>
#include <QtCore/QSet>
#include "facebookimagesdatabase.h"

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
    QSet<AbstractSocialCacheModel *> models;
    Q_DECLARE_PUBLIC(FacebookImageDownloader)
};


#endif // FACEBOOKIMAGEDOWNLOADER_P_H
