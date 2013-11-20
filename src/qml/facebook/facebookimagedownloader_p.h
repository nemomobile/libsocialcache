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
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "abstractimagedownloader_p.h"
#include "facebookimagedownloader.h"
#include "facebookimagesdatabase.h"


class FacebookImageDownloaderPrivate: public AbstractImageDownloaderPrivate
{
public:
    explicit FacebookImageDownloaderPrivate(FacebookImageDownloader *q);
    virtual ~FacebookImageDownloaderPrivate();

    FacebookImagesDatabase database;

private:
    Q_DECLARE_PUBLIC(FacebookImageDownloader)
};

#endif // FACEBOOKIMAGEDOWNLOADER_P_H
