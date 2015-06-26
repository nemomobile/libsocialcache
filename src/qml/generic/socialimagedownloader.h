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

#ifndef SOCIALIMAGEDOWNLOADER_H
#define SOCIALIMAGEDOWNLOADER_H

#include <QtCore/QObject>

#include "abstractimagedownloader.h"

class SocialImageCacheModel;
class SocialImageDownloaderPrivate;
class SocialImageDownloader : public AbstractImageDownloader
{
    Q_OBJECT

public:
    explicit SocialImageDownloader(QObject *parent = 0);
    virtual ~SocialImageDownloader();

    Q_INVOKABLE void imageFile(const QString &imageUrl, int accountId, QObject *caller);
    Q_INVOKABLE void removeFromRecentlyUsed(const QString &imageUrl);

protected:
    QString outputFile(const QString &url, const QVariantMap &data) const;

private Q_SLOTS:
    void notifyImageCached(const QString &url, const QString &path, const QVariantMap &metadata);
    void commitTimerTimeout();

private:
    Q_DECLARE_PRIVATE(SocialImageDownloader)
};

#endif // SOCIALIMAGEDOWNLOADER_H
