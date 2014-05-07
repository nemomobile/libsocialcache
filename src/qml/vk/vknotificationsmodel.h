/*
 * Copyright (C) 2014 Jolla Ltd.
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

#ifndef VKNOTIFICATIONSMODEL_H
#define VKNOTIFICATIONSMODEL_H

#include "abstractsocialcachemodel.h"

class VKNotificationsModelPrivate;
class VKNotificationsModel: public AbstractSocialCacheModel
{
    Q_OBJECT
    Q_ENUMS(VKNotificationsRole)
public:
    enum VKNotificationsRole {
        VkId = 0,
        Type,
        From,
        FromName,
        FromIcon,
        To,
        Timestamp,
        Accounts
    };
    explicit VKNotificationsModel(QObject *parent = 0);
    QHash<int, QByteArray> roleNames() const;

    void refresh();

private Q_SLOTS:
    void notificationsChanged();

private:
    Q_DECLARE_PRIVATE(VKNotificationsModel)
};

#endif // VKNOTIFICATIONSMODEL_H
