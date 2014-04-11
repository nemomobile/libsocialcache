/*
 * Copyright (C) 2014 Jolla Ltd.
 * Contact: Bea Lam <bea.lam@jollamobile.com>
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

#ifndef CALDAVCALENDARDATABASE_H
#define CALDAVCALENDARDATABASE_H

#include "abstractsocialcachedatabase.h"

#include <QtCore/QStringList>
#include <QtCore/QHash>

class CalDavCalendarDatabasePrivate;
class CalDavCalendarDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit CalDavCalendarDatabase();
    ~CalDavCalendarDatabase();

    QStringList additions(const QString &notebookUid, bool *ok);
    QHash<QString, QString> modifications(const QString &notebookUid, bool *ok);
    QStringList deletions(const QString &notebookUid, bool *ok);

    void insertAdditions(const QString &notebookUid, const QStringList &incidenceUids);
    void insertModifications(const QString &notebookUid, const QHash<QString, QString> &incidenceDetails);
    void insertDeletions(const QString &notebookUid, const QStringList &incidenceUids);
    void removeEntries(const QString &notebookUid);

    bool hasChanges();

    void commit();

protected:
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(CalDavCalendarDatabase)
};

#endif // CALDAVCALENDARDATABASE_H
