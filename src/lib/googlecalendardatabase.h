/*
 * Copyright (C) 2014 Jolla Ltd.
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

#ifndef GOOGLECALENDARDATABASE_H
#define GOOGLECALENDARDATABASE_H

#include "abstractsocialcachedatabase.h"
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QList>

class GoogleEventPrivate;
class GoogleEvent
{
public:
    typedef QSharedPointer<GoogleEvent> Ptr;
    typedef QSharedPointer<const GoogleEvent> ConstPtr;
    virtual ~GoogleEvent();
    static GoogleEvent::Ptr create(int accountId,
                                   const QString &gcalEventId,
                                   const QString &localCalendarId,
                                   const QString &localEventId);
    int accountId() const;
    QString gcalEventId() const;
    QString localCalendarId() const;
    QString localEventId() const;
protected:
    QScopedPointer<GoogleEventPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(GoogleEvent)
    explicit GoogleEvent(int accountId, const QString &gcalEventId,
                         const QString &localCalendarId, const QString &localEventId);
};

class GoogleCalendarDatabasePrivate;
class GoogleCalendarDatabase: public AbstractSocialCacheDatabase
{
public:
    explicit GoogleCalendarDatabase();
    ~GoogleCalendarDatabase();

    QList<GoogleEvent::ConstPtr> events(int accountId, const QString &localCalendarId = QString());
    QString gcalEventId(int accountId, const QString &localCalendarId, const QString &localEventId);

    // the following three won't be committed to the db until sync()+wait() is called.
    void insertEvent(int accountId, const QString &gcalEventId, const QString &localCalendarId, const QString &localEventId);
    void removeEvent(int accountId, const QString &gcalEventId, const QString &localCalendarId = QString(), const QString &localEventId = QString());
    void removeEvents(int accountId, const QString &localCalendarId = QString());

    void sync();

protected:
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(GoogleCalendarDatabase)
};

#endif // GOOGLECALENDARDATABASE_H
