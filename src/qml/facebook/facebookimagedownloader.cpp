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

#include "facebookimagedownloader.h"
#include "facebookimagedownloader_p.h"
#include "facebookimagedownloaderconstants_p.h"

#include <QtCore/QStandardPaths>
#include <QtGui/QGuiApplication>

#include <QtDebug>

FacebookImageDownloaderWorkerObject::FacebookImageDownloaderWorkerObject()
    : AbstractImageDownloader(), m_initialized(false), m_killed(false)
{
}

QString FacebookImageDownloaderWorkerObject::outputFile(const QString &url,
                                                        const QVariantMap &data) const
{
    Q_UNUSED(url)
    if (m_killed) {
        return QString(); // we are in the process of being terminated.
    }

    // We create the identifier by appending the type to the real identifier
    QString identifier = data.value(QLatin1String(IDENTIFIER_KEY)).toString();
    if (identifier.isEmpty()) {
        return QString();
    }

    QString typeString = data.value(QLatin1String(TYPE_KEY)).toString();
    if (typeString.isEmpty()) {
        return QString();
    }

    identifier.append(typeString);

    return makeOutputFile(SocialSyncInterface::Facebook, SocialSyncInterface::Images, identifier);
}

bool FacebookImageDownloaderWorkerObject::dbInit()
{
    if (m_killed) {
        return false; // we are in the process of being terminated.
    }

    if (!m_initialized) {
        m_db.initDatabase();
        m_initialized = true;
    }

    return m_db.isValid();
}

bool FacebookImageDownloaderWorkerObject::dbClose()
{
    if (m_killed) {
        return false; // we are in the process of being terminated.
    }

    m_initialized = false;
    return m_db.closeDatabase();
}

void FacebookImageDownloaderWorkerObject::dbQueueImage(const QString &url, const QVariantMap &data,
                                                       const QString &file)
{
    Q_UNUSED(url)
    if (m_killed) {
        return; // we are in the process of being terminated.
    }

    QString identifier = data.value(QLatin1String(IDENTIFIER_KEY)).toString();
    if (identifier.isEmpty()) {
        return;
    }
    int type = data.value(QLatin1String(TYPE_KEY)).toInt();

    switch (type) {
    case ThumbnailImage:
        m_db.updateImageThumbnail(identifier, file);
        break;
    case FullImage:
        m_db.updateImageFile(identifier, file);
        break;
    }
}

void FacebookImageDownloaderWorkerObject::dbWrite()
{
    if (m_killed) {
        return; // we are in the process of being terminated.
    }

    m_db.write();
}

void FacebookImageDownloaderWorkerObject::quitGracefully()
{
    m_quitMutex.lock();
    // Note: we cannot dbWrite() / dbClose() here.
    // Most likely, the database has already been closed
    // by the time this function is called.
    // So any attempt to lock, will result in a crash.
    // Thus, set m_killed to avoid possibility of doing that.
    m_killed = true;
    // We also need to push this object to the null
    // thread to stop event processing (so that we
    // can delete the object).  XXX TODO: why is this needed?
    this->moveToThread(0);
    // now we can die.
    m_quitWC.wakeAll();
    m_quitMutex.unlock();
}

FacebookImageDownloaderPrivate::FacebookImageDownloaderPrivate(FacebookImageDownloader *q)
    : QObject(), q_ptr(q), m_workerObject(new FacebookImageDownloaderWorkerObject())
{
    m_workerThread.start(QThread::IdlePriority);
    m_workerObject->moveToThread(&m_workerThread);
}

FacebookImageDownloaderPrivate::~FacebookImageDownloaderPrivate()
{
    if (m_workerThread.isRunning()) {
        // tell worker object to quit gracefully.
        m_workerObject->m_quitMutex.lock();
        QMetaObject::invokeMethod(m_workerObject, "quitGracefully", Qt::QueuedConnection);
        m_workerObject->m_quitWC.wait(&m_workerObject->m_quitMutex);
        m_workerObject->m_quitMutex.unlock();

        // now terminate the thread
        m_workerThread.quit();
        m_workerThread.wait();
    }

    // and delete the worker object - this will ensure the database gets closed.
    delete m_workerObject;
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
    return d->m_workerObject;
}
