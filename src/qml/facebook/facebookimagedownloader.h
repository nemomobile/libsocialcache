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

#ifndef FACEBOOKIMAGEDOWNLOADER_H
#define FACEBOOKIMAGEDOWNLOADER_H

#include <QtCore/QObject>
#include "facebookimagesdatabase.h"
#include "abstractsocialcachemodel.h"

class FacebookImageDownloaderWorkerObject;
class FacebookImageDownloaderPrivate;
class FacebookImageDownloader : public QObject
{
    Q_OBJECT
public:
    explicit FacebookImageDownloader(QObject *parent = 0);
    virtual ~FacebookImageDownloader();
    FacebookImageDownloaderWorkerObject * workerObject() const;
protected:
    QScopedPointer<FacebookImageDownloaderPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(FacebookImageDownloader)
};

#endif // FACEBOOKIMAGEDOWNLOADER_H
