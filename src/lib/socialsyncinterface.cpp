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

#include "socialsyncinterface.h"
#include <QtCore/QMetaObject>
#include <QtCore/QMetaEnum>

QString SocialSyncInterface::socialNetwork(SocialNetwork sn)
{
    QMetaEnum metaEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("SocialNetwork"));
    return metaEnum.valueToKey(sn);
}

QString SocialSyncInterface::dataType(DataType t)
{
    QMetaEnum metaEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("DataType"));
    return metaEnum.valueToKey(t);
}

QString SocialSyncInterface::profileName(SocialNetwork sn, DataType t)
{
    return QString("%1.%2").arg(socialNetwork(sn).toLower(), dataType(t));
}
