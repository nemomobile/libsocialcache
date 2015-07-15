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

#ifndef SOCIALIMAGEDOWNLOADER_P_H
#define SOCIALIMAGEDOWNLOADER_P_H

#include "abstractimagedownloader_p.h"
#include "socialimagedownloader.h"
#include "socialimagesdatabase.h"

#include <QPointer>

class SocialImageDownloaderPrivate : public AbstractImageDownloaderPrivate
{
public:
    explicit SocialImageDownloaderPrivate(SocialImageDownloader *q);
    virtual ~SocialImageDownloaderPrivate();

    SocialImagesDatabase m_db;
    QTimer m_commitTimer;
    QMap<QString, QString> m_recentItems;
    QMultiMap<QString, QPointer<QObject> > m_ongoingCalls;
    QMutex m_mutex;

private:
    Q_DECLARE_PUBLIC(SocialImageDownloader)
};

#endif // SOCIALIMAGEDOWNLOADER_P_H
