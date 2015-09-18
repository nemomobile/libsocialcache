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

#ifndef SOCIALSYNCINTERFACE_H
#define SOCIALSYNCINTERFACE_H

#include <QtCore/QObject>

class SocialSyncInterface : public QObject
{
    Q_OBJECT
    Q_ENUMS(SocialNetwork)
    Q_ENUMS(DataType)
public:
    enum SocialNetwork {
        InvalidSocialNetwork,
        Facebook,
        Twitter,
        Google,
        VK,
        Diaspora,
        CalDAV,
        OneDrive,
        Dropbox
    };

    enum DataType {
        InvalidDataType,
        Contacts,
        Calendars,
        Notifications,
        Images,
        Videos,
        Posts,
        Messages,
        Emails
    };
    Q_INVOKABLE static QString socialNetwork(SocialNetwork sn);
    Q_INVOKABLE static QString dataType(DataType t);
    static QString profileName(SocialNetwork sn, DataType t);
};

#endif // SOCIALSYNCINTERFACE_H
