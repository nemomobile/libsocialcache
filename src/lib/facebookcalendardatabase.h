/*
 * Copyright (C) 2013 Lucien Xu <sfietkonstantin@free.fr>
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

#ifndef FACEBOOKCALENDARDATABASE_H
#define FACEBOOKCALENDARDATABASE_H

#include "abstractsocialcachedatabase.h"
#include <QtCore/QSharedPointer>

class FacebookEventPrivate;
class FacebookEvent
{
public:
    typedef QSharedPointer<FacebookEvent> Ptr;
    typedef QSharedPointer<const FacebookEvent> ConstPtr;
    virtual ~FacebookEvent();
    static FacebookEvent::Ptr create(const QString &fbEventId, int accountId,
                                     const QString &incidenceId);
    QString fbEventId() const;
    int accountId() const;
    QString incidenceId() const;
protected:
    QScopedPointer<FacebookEventPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(FacebookEvent)
    explicit FacebookEvent(const QString &fbEventId, int accountId, const QString &incidenceId);
};

class FacebookCalendarDatabasePrivate;
class FacebookCalendarDatabase: public AbstractSocialCacheDatabase
{
public:
    explicit FacebookCalendarDatabase();
    ~FacebookCalendarDatabase();

    void initDatabase();

    QList<FacebookEvent::ConstPtr> events(int accountId);
    void addSyncedEvent(const QString &fbEventId, int accountId, const QString &incidenceId);

    bool sync(int accountId);
protected:
    bool dbCreateTables();
    bool dbDropTables();
private:
    Q_DECLARE_PRIVATE(FacebookCalendarDatabase)
};

#endif // FACEBOOKCALENDARDATABASE_H
