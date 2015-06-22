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

#ifndef FACEBOOKNOTIFICATIONSMODEL_H
#define FACEBOOKNOTIFICATIONSMODEL_H

#include "abstractsocialcachemodel.h"

class FacebookNotificationsModelPrivate;
class FacebookNotificationsModel : public AbstractSocialCacheModel
{
    Q_OBJECT

    Q_ENUMS(FacebookNotificationsRole)
public:
    enum FacebookNotificationsRole {
        NotificationId = 0,
        From,
        To,
        Timestamp,
        Title,
        Link,
        AppId,
        Object,
        Accounts,
        ClientId,
        Unread
    };
    explicit FacebookNotificationsModel(QObject *parent = 0);
    QHash<int, QByteArray> roleNames() const;

    void refresh();
    void clear();

private Q_SLOTS:
    void notificationsChanged();

private:
    Q_DECLARE_PRIVATE(FacebookNotificationsModel)
};

#endif // FACEBOOKNOTIFICATIONSMODEL_H
