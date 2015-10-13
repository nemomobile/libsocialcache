/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Chris Adams <chris.adams@jollamobile.com>
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

#ifndef VKIMAGEDOWNLOADER_P_H
#define VKIMAGEDOWNLOADER_P_H

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QSet>

#include "abstractimagedownloader_p.h"
#include "vkimagedownloader.h"
#include "vkimagesdatabase.h"

class VKImageCacheModel;
class VKImageDownloaderPrivate: public AbstractImageDownloaderPrivate
{
public:
    explicit VKImageDownloaderPrivate(VKImageDownloader *q);
    virtual ~VKImageDownloaderPrivate();

    VKImagesDatabase database;
    QSet<VKImageCacheModel*> m_connectedModels;

private:
    Q_DECLARE_PUBLIC(VKImageDownloader)
};

#endif // VKIMAGEDOWNLOADER_P_H
