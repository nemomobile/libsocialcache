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

#ifndef SOCIALNETWORKSYNCDATABASE_H
#define SOCIALNETWORKSYNCDATABASE_H

#include "abstractsocialcachedatabase.h"
#include <QtCore/QDateTime>

class SocialNetworkSyncDatabasePrivate;
class SocialNetworkSyncDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit SocialNetworkSyncDatabase();
    ~SocialNetworkSyncDatabase();

    QList<int> syncedAccounts(const QString &serviceName, const QString &dataType) const;
    QDateTime lastSyncTimestamp(const QString &serviceName, const QString &dataType,
                                int accountId) const;
    void addSyncTimestamp(const QString &serviceName, const QString &dataType,
                          int accountId, const QDateTime &timestamp);
    void commit();

protected:
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(SocialNetworkSyncDatabase)
};

#endif // SOCIALNETWORKSYNCDATABASE_H
