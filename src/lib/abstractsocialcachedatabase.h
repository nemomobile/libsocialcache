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

#ifndef ABSTRACTSOCIALCACHEDATABASE_H
#define ABSTRACTSOCIALCACHEDATABASE_H

#include <QtCore/QMap>
#include <QtCore/QVariantList>

class AbstractSocialCacheDatabasePrivate;
class AbstractSocialCacheDatabase
{
public:
    explicit AbstractSocialCacheDatabase();
    virtual ~AbstractSocialCacheDatabase();

    // Whoever calls initDatabase() must also call closeDatabase().
    virtual void initDatabase() = 0;
    bool closeDatabase();
    bool isValid() const;

protected:
    enum QueryMode {
        Insert,
        InsertOrReplace,
        Update,
        Delete
    };

    explicit AbstractSocialCacheDatabase(AbstractSocialCacheDatabasePrivate &dd);
    void dbInit(const QString &serviceName, const QString &dataType,
                const QString &dbFile, int userVersion);
    bool dbClose();

    virtual bool dbCreateTables() = 0;
    virtual bool dbDropTables() = 0;
    bool dbCreatePragmaVersion(int version);

    bool dbBeginTransaction();
    bool dbWrite(const QString &table, const QStringList &keys,
                 const QMap<QString, QVariantList> &entries,
                 QueryMode mode = Insert, const QString &primary = QString());
    bool dbCommitTransaction();
    bool dbRollbackTransaction();

    QScopedPointer<AbstractSocialCacheDatabasePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(AbstractSocialCacheDatabase)
};

#endif // ABSTRACTSOCIALCACHEDATABASE_H
