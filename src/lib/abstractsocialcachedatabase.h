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

QT_BEGIN_NAMESPACE
class QSqlDatabase;
class QSqlQuery;
QT_END_NAMESPACE

class AbstractSocialCacheDatabasePrivate;
class AbstractSocialCacheDatabase : public QObject
{
    Q_OBJECT
public:
    enum Status
    {
        Null,
        Executing,
        Finished,
        Error
    };

    explicit AbstractSocialCacheDatabase(
            const QString &serviceName,
            const QString &dataType,
            const QString &databaseFile,
            int version);
    virtual ~AbstractSocialCacheDatabase();

    bool isValid() const;

    Status readStatus() const;
    Status writeStatus() const;

    bool event(QEvent *event);

    void wait();

Q_SIGNALS:
    void readStatusChanged();
    void writeStatusChanged();

protected:
    void executeRead();
    void cancelRead();

    void executeWrite();
    void cancelWrite();

    virtual bool read();
    virtual bool write();
    virtual bool createTables(QSqlDatabase database) const = 0;
    virtual bool dropTables(QSqlDatabase database) const = 0;

    virtual void readFinished();
    virtual void writeFinished();


    QSqlQuery prepare(const QString &query) const;

    explicit AbstractSocialCacheDatabase(AbstractSocialCacheDatabasePrivate &dd);

    QScopedPointer<AbstractSocialCacheDatabasePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(AbstractSocialCacheDatabase)
};

#endif // ABSTRACTSOCIALCACHEDATABASE_H
