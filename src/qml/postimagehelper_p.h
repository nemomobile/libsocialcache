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

#ifndef POSTIMAGEHELPER_P_H
#define POSTIMAGEHELPER_P_H

#include <QtCore/QVariantMap>

static const char *URL_KEY = "url";
static const char *TYPE_KEY = "type";
static const char *TYPE_PHOTO = "photo";
static const char *TYPE_VIDEO = "video";

inline static QVariantMap createImageData(const SocialPostImage::ConstPtr &image)
{
    QVariantMap imageData;
    imageData.insert(QLatin1String(URL_KEY), image->url());
    switch (image->type()) {
    case SocialPostImage::Video:
        imageData.insert(QLatin1String(TYPE_KEY), QLatin1String(TYPE_VIDEO));
        break;
    default:
        imageData.insert(QLatin1String(TYPE_KEY), QLatin1String(TYPE_PHOTO));
        break;
    }
    return imageData;
}

#endif // POSTIMAGEHELPER_P_H
