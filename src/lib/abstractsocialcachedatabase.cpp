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

#include "abstractsocialcachedatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QtDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStandardPaths>
#include <QtCore/QUuid>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

// AbstractSocialCacheDatabase
// This class is the base class for all classes
// that deals with database access.
//
// It provides a set of useful methods that should
// be used to initialize the database, and write into
// it. dbBeginTransaction, dbWrite and dbCommitTransaction
// are particularly handy, and can make write operations
// into db very fast.

AbstractSocialCacheDatabasePrivate::AbstractSocialCacheDatabasePrivate(AbstractSocialCacheDatabase *q):
    q_ptr(q), valid(false), mutex(0)
{
}

AbstractSocialCacheDatabasePrivate::~AbstractSocialCacheDatabasePrivate()
{
    db.close();
}

// Get the user_version of the currently opened database
int AbstractSocialCacheDatabasePrivate::dbUserVersion(const QString &serviceName,
                                                        const QString &dataType) const
{
    const QString queryStr = QString("PRAGMA user_version");

    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        qWarning() << Q_FUNC_INFO << "Failed to query pragma_user version. Service"
                   << serviceName << "with data type" << dataType << "will be inactive. Error"
                   << query.lastError().text();
        return -1;
    }
    QSqlRecord record = query.record();
    if (query.isActive() && query.isSelect()) {
        query.first();
        QString value = query.value(record.indexOf("user_version")).toString();
        if (value.isEmpty()) {
            return -1;
        }
        return value.toInt();
    }
    return -1;
}

// Perform a batch insert
bool AbstractSocialCacheDatabasePrivate::doInsert(const QString &table,
                                                    const QStringList &keys,
                                                    const QMap<QString, QVariantList> &entries,
                                                    bool replace)
{
    QString queryString = QLatin1String("INSERT ");
    if (replace) {
        queryString.append(QLatin1String("OR REPLACE "));
    }

    queryString.append(QLatin1String("INTO "));
    queryString.append(table);
    queryString.append(QLatin1String(" ("));
    foreach (const QString &key, keys) {
        queryString.append(key);
        queryString.append(QLatin1String(", "));
    }
    queryString.chop(2);
    queryString.append(QLatin1String(") VALUES ("));
    queryString.append(QString(QLatin1String("? ,")).repeated(entries.keys().count()));
    queryString.chop(2);
    queryString.append(QLatin1String(")"));

    QSqlQuery query (db);
    query.prepare(queryString);
    foreach (const QString &key, keys) {
        query.addBindValue(entries.value(key));
    }

    bool ok = query.execBatch();
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "Failed to execute query. Request:" << queryString
                   << "Error:" << query.lastError().text();
    }
    return ok;
}

// Perform a batch update
bool AbstractSocialCacheDatabasePrivate::doUpdate(const QString &table,
                                                    const QMap<QString, QVariantList> &entries,
                                                    const QString &primary)
{
    QVariantList primaryEntries = entries.value(primary);
    QMap<QString, QVariantList> otherEntries;
    for (QMap<QString, QVariantList>::const_iterator i = entries.constBegin();
         i != entries.constEnd(); ++i) {
        if (i.key() != primary) {
            otherEntries.insert(i.key(), i.value());
        }
    }

    QString queryString = QLatin1String("UPDATE ");
    queryString.append(table);
    queryString.append(QLatin1String(" SET "));
    foreach (const QString &key, otherEntries.keys()) {
        queryString.append(key);
        queryString.append(QLatin1String("= ? , "));
    }
    queryString.chop(2);
    queryString.append(QLatin1String("WHERE "));
    queryString.append(primary);
    queryString.append(QLatin1String(" = ?"));

    QSqlQuery query (db);
    query.prepare(queryString);
    for (QMap<QString, QVariantList>::const_iterator i = otherEntries.constBegin();
         i != otherEntries.constEnd(); ++i) {
        query.addBindValue(i.value());
    }
    query.addBindValue(primaryEntries);

    bool ok = query.execBatch();
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "Failed to execute query. Request:" << queryString
                   << "Error:" << query.lastError().text();
    }
    return ok;
}

// Perform a batch delete
bool AbstractSocialCacheDatabasePrivate::doDelete(const QString &table, const QString &key,
                                                        const QVariantList &entries)
{
    QString queryString = QLatin1String("DELETE FROM ");
    queryString.append(table);
    queryString.append(QLatin1String(" WHERE "));
    queryString.append(key);
    queryString.append(QLatin1String(" = ?"));

    QSqlQuery query (db);
    query.prepare(queryString);
    query.addBindValue(entries);

    bool ok = query.execBatch();
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "Failed to execute query. Request:" << queryString
                   << "Error:" << query.lastError().text();
    }
    return ok;
}

AbstractSocialCacheDatabase::AbstractSocialCacheDatabase()
    : d_ptr(new AbstractSocialCacheDatabasePrivate(this))
{
}

AbstractSocialCacheDatabase::AbstractSocialCacheDatabase(AbstractSocialCacheDatabasePrivate &dd)
    : d_ptr(&dd)
{
}

AbstractSocialCacheDatabase::~AbstractSocialCacheDatabase()
{
}

bool AbstractSocialCacheDatabase::isValid() const
{
    Q_D(const AbstractSocialCacheDatabase);
    return d->valid;
}

// Initialize the database in PRIVILEGED_DATA_DIR/dataType, with name dbFile
// serviceName is used by debugging to indicate the service that is using this db.
// Creates the dir structure if needed, then create the database if needed.
// Compare the user_version stored in the database to userVersion, and recreate
// the database if it is lower.
//
// This method uses dbCreateTables that should be used to create the structure
// of the database and dbDropTables that should be used to drop the different tables.
void AbstractSocialCacheDatabase::dbInit(const QString &serviceName, const QString &dataType,
                                           const QString &dbFile, int userVersion)
{
    Q_D(AbstractSocialCacheDatabase);
    if (!QFile::exists(QString(QLatin1String("%1/%2/%3")).arg(PRIVILEGED_DATA_DIR, dataType,
                                                              dbFile))) {
        QDir dir(QString(QLatin1String("%1/%2")).arg(PRIVILEGED_DATA_DIR, dataType));
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QString absolutePath = dir.absoluteFilePath(dbFile);
        QFile dbfile(absolutePath);
        if (!dbfile.open(QIODevice::ReadWrite)) {
            qWarning() << Q_FUNC_INFO << "Unable to create database" << dbFile << "Service"
                       << serviceName << "with data type" << dataType << "will be inactive";
            return;
        }
        dbfile.close();
    }

    QString connectionName
            = QString(QLatin1String("socialcache/%1/%2/%3")).arg(serviceName, dataType,
                                                                 QUuid::createUuid().toString());

    // open the database in which we store our synced image information
    d->db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    d->db.setDatabaseName(QString("%1/%2/%3").arg(PRIVILEGED_DATA_DIR, dataType, dbFile));

    d->mutex = new ProcessMutex(d->db.databaseName());

    if (!d->db.open()) {
        qWarning() << Q_FUNC_INFO << "Unable to open database" << dbFile << "Service"
                   << serviceName << "with data type" << dataType << "will be inactive";
        return;
    }

    int dbUserVersion = d->dbUserVersion(serviceName, dataType);
    if (dbUserVersion < userVersion) {
        qWarning() << Q_FUNC_INFO << "Version required is" << userVersion
                   << "while database is using" << dbUserVersion;

        // DB needs to be recreated
        if (!dbDropTables()) {
            qWarning() << Q_FUNC_INFO << "Failed to update database" << dbFile
                       << "It is probably broken and need to be removed manually";
            d->db.close();
            return;
        }
    }

    if (!dbCreateTables()) {
        qWarning() << Q_FUNC_INFO << "Failed to update database" << dbFile
                   << "It is probably broken and need to be removed manually";
        d->db.close();
        return;
    }

    d->valid = true;
}

// Set a user_version to the currently opened database
// Usually used when implementing dbCreateTable.
bool AbstractSocialCacheDatabase::dbCreatePragmaVersion(int version)
{
    Q_D(AbstractSocialCacheDatabase);
    QSqlQuery query (d->db);
    query.prepare(QString(QLatin1String("PRAGMA user_version=%1")).arg(version));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to create pragma_user version. Error:"
                   << query.lastError().text();
        return false;
    }
    return true;
}

// Begin a transaction
//
// Begin a transaction, so that insertion
// queries can be executed quickly.
// Transactions uses a process mutex and
// use IMMEDIATE TRANSACTION.
bool AbstractSocialCacheDatabase::dbBeginTransaction()
{
    Q_D(AbstractSocialCacheDatabase);

    // Acquire lock
    if (!d->mutex->lock()) {
        return false;
    }

    QSqlQuery query(d->db);
    query.prepare(QLatin1String("BEGIN IMMEDIATE TRANSACTION"));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to begin immediate transaction. Error:"
                   << query.lastError().text();

        d->mutex->unlock();
        return false;
    }

    return true;
}

// Perform a batch query
//
// This method is used to perform a batch query, that is usually an insertion
// an update or a deletion.
// Batch query might speeds these operations drastically, and should be done when
// possible. Used with exclusive transactions (dbBeginWrite, dbEndWrite), they help
// inserting or updating a lot of entries.
//
// Entries are a hash of list. In case of insertion, the keys of the hash are
// the orderedlist of keys of the table, and each list contains the value for
// a column to be inserted. In update, we use the optionnal argument primary to
// get the key that will be used as the id for update, and use the other keys
// as new values to be inserted. In case of deletion, the entries should only
// contain one key and list, that is the list of keys that are used to perform
// the deletion.
bool AbstractSocialCacheDatabase::dbWrite(const QString &table, const QStringList &keys,
                                            const QMap<QString, QVariantList> &entries,
                                            QueryMode mode, const QString &primary)
{
    Q_D(AbstractSocialCacheDatabase);
    // When we have empty entries, we simply return true
    // since there is no need to do any db write to
    // write nothing
    if (entries.isEmpty()) {
        return true;
    }

    if (mode == Delete && entries.count() != 1) {
        qWarning() << Q_FUNC_INFO << "Error: When deleting, entries should only contain one key.";
        return false;
    }

    if (mode == Update && !entries.contains(primary)) {
        qWarning() << Q_FUNC_INFO << "Error: When updating, primary should be in the entries.";
        return false;
    }


    foreach (const QString &key, keys) {
        if (!entries.contains(key)) {
            qWarning() << Q_FUNC_INFO << "Entries should contain the keys.";
            return false;
        }
    }

    int count = entries.constBegin().value().count();
    for (QMap<QString, QVariantList>::const_iterator i = entries.constBegin();
         i != entries.constEnd(); ++i) {
        if (i.value().count() != count) {
            qWarning() << Q_FUNC_INFO << "Entries should be of the same size.";
            return false;
        }
    }

    switch (mode) {
        case Insert:
            return d->doInsert(table, keys, entries);
        break;
        case InsertOrReplace:
            return d->doInsert(table, keys, entries, true);
        break;
        case Update:
            return d->doUpdate(table, entries, primary);
        break;
        case Delete:
            return d->doDelete(table, entries.begin().key(), entries.begin().value());
        break;
        default: break;
    }

    return false;
}

// Commit the changes
//
// End a transaction, by commiting the changes.
bool AbstractSocialCacheDatabase::dbCommitTransaction()
{
    Q_D(AbstractSocialCacheDatabase);

    QSqlQuery query(d->db);
    query.prepare(QLatin1String("COMMIT TRANSACTION"));
    bool ok = query.exec();
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "Failed to commit transaction. Error:"
                   << query.lastError().text();
    }

    if (d->mutex->isLocked()) {
        d->mutex->unlock();
    }

    return ok;
}

// Rollback a transaction
bool AbstractSocialCacheDatabase::dbRollbackTransaction()
{
    Q_D(AbstractSocialCacheDatabase);
    QSqlQuery query(d->db);
    query.prepare(QLatin1String("ROLLBACK TRANSACTION"));
    bool ok = query.exec();
    if (!ok) {
        qWarning() << Q_FUNC_INFO << "Failed to rollback transaction. Error:"
                   << query.lastError().text();
    }

    if (d->mutex->isLocked()) {
        d->mutex->unlock();
    }

    return ok;
}
