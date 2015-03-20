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

#include "caldavcalendardatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>
#include <QSet>

static const char *DB_NAME = "caldav-sync.db";
static const int VERSION = 2;

class CalDavCalendarDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit CalDavCalendarDatabasePrivate(CalDavCalendarDatabase *q);
    ~CalDavCalendarDatabasePrivate();

private:
    Q_DECLARE_PUBLIC(CalDavCalendarDatabase);

    enum EntryRemovalMode {
        RemoveAllEntries,
        RemoveIncidenceChangeEntriesOnly
    };

    typedef QHash<KCalId, QString> IncidenceUpdate;
    typedef QHash<QString, QString> IncidenceETag; // etags are per URI

    bool writeAdditions(const QHash<QString, QSet<KCalId> > &addedIncidences);
    bool writeModifications(const QHash<QString, IncidenceUpdate> &modifiedIncidences);
    bool writeDeletions(const QHash<QString, QSet<KCalId> > &deletedIncidences);
    bool writeNotebookRemovals(const QString &table, const QStringList &notebookUids);

    bool writeETags(const QHash<QString, IncidenceETag> &eTags);

    struct {
        QHash<QString, bool> notebookUidsToDelete;
        QHash<QString, QSet<KCalId> > addedIncidences;
        QHash<QString, IncidenceUpdate> modifiedIncidences;
        QHash<QString, QSet<KCalId> > deletedIncidences;
        QHash<QString, IncidenceETag> eTags;
    } queue;
};

CalDavCalendarDatabasePrivate::CalDavCalendarDatabasePrivate(CalDavCalendarDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::CalDAV),
            SocialSyncInterface::dataType(SocialSyncInterface::Calendars),
            QLatin1String(DB_NAME),
            VERSION)
{
}

CalDavCalendarDatabasePrivate::~CalDavCalendarDatabasePrivate()
{
}


CalDavCalendarDatabase::CalDavCalendarDatabase()
    : AbstractSocialCacheDatabase(*(new CalDavCalendarDatabasePrivate(this)))
{
}

CalDavCalendarDatabase::~CalDavCalendarDatabase()
{
}

QSet<KCalId> CalDavCalendarDatabase::additions(const QString &notebookUid, bool *ok)
{
    static const QString queryString = QStringLiteral("SELECT incidenceUid FROM Additions WHERE notebookUid = '%1'");
    QSqlQuery query = prepare(queryString.arg(notebookUid));
    if (!query.exec()) {
        qWarning() << "SQL query failed:" << query.executedQuery() << "Error:" << query.lastError().text();
        *ok = false;
        return QSet<KCalId>();
    }
    QSet<KCalId> ret;
    while (query.next()) {
        KCalId kcalid = KCalId::fromString(query.value(0).toString());
        if (!kcalid.uid.isEmpty()) {
            ret.insert(kcalid);
        } else {
            qWarning() << "Could not convert stored value to KCalId:" << query.value(0).toString();
        }
    }
    *ok = true;
    return ret;
}

QHash<KCalId, QString> CalDavCalendarDatabase::modifications(const QString &notebookUid, bool *ok)
{
    static const QString queryString = QStringLiteral("SELECT incidenceUid,iCalData FROM Modifications WHERE notebookUid = '%1'");
    QSqlQuery query = prepare(queryString.arg(notebookUid));
    if (!query.exec()) {
        qWarning() << "SQL query failed:" << query.executedQuery() << "Error:" << query.lastError().text();
        return QHash<KCalId, QString>();
    }
    QHash<KCalId, QString> ret;
    while (query.next()) {
        KCalId kcalid = KCalId::fromString(query.value(0).toString());
        if (!kcalid.uid.isEmpty()) {
            ret.insert(kcalid, query.value(1).toString());
        } else {
            qWarning() << "Could not convert stored value to KCalId:" << query.value(0).toString();
        }
    }
    *ok = true;
    return ret;
}

QSet<KCalId> CalDavCalendarDatabase::deletions(const QString &notebookUid, bool *ok)
{
    static const QString queryString = QStringLiteral("SELECT incidenceUid FROM Deletions WHERE notebookUid = '%1'");
    QSqlQuery query = prepare(queryString.arg(notebookUid));
    if (!query.exec()) {
        qWarning() << "SQL query failed:" << query.executedQuery() << "Error:" << query.lastError().text();
        *ok = false;
        return QSet<KCalId>();
    }
    QSet<KCalId> ret;
    while (query.next()) {
        KCalId kcalid = KCalId::fromString(query.value(0).toString());
        if (!kcalid.uid.isEmpty()) {
            ret.insert(kcalid);
        } else {
            qWarning() << "Could not convert stored value to KCalId:" << query.value(0).toString();
        }
    }
    *ok = true;
    return ret;
}

void CalDavCalendarDatabase::insertAdditions(const QString &notebookUid, const QSet<KCalId> &incidenceUids)
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.addedIncidences[notebookUid].unite(incidenceUids);
}

void CalDavCalendarDatabase::insertModifications(const QString &notebookUid, const QHash<KCalId, QString> &incidenceDetails)
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.modifiedIncidences[notebookUid].unite(incidenceDetails);
}

void CalDavCalendarDatabase::insertDeletions(const QString &notebookUid, const QSet<KCalId> &incidenceUids)
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.deletedIncidences[notebookUid].unite(incidenceUids);
}

// Removes addition, modification and deletion entries for this notebook
void CalDavCalendarDatabase::removeIncidenceChangeEntriesOnly(const QString &notebookUid)
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);
    d->queue.notebookUidsToDelete.insert(notebookUid, CalDavCalendarDatabasePrivate::RemoveIncidenceChangeEntriesOnly);
}

// Removes all types of entries for this notebook
void CalDavCalendarDatabase::removeEntries(const QString &notebookUid)
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);
    d->queue.notebookUidsToDelete.insert(notebookUid, CalDavCalendarDatabasePrivate::RemoveAllEntries);
}

QHash<QString, QString> CalDavCalendarDatabase::eTags(const QString &notebookUid, bool *ok)
{
    static const QString queryString = QStringLiteral("SELECT incidenceUid,eTag FROM ETags WHERE notebookUid = '%1'");
    QSqlQuery query = prepare(queryString.arg(notebookUid));
    if (!query.exec()) {
        qWarning() << "SQL query failed:" << query.executedQuery() << "Error:" << query.lastError().text();
        return QHash<QString, QString>();
    }
    QHash<QString, QString> ret;
    while (query.next()) {
        ret.insert(query.value(0).toString(), query.value(1).toString());
    }
    *ok = true;
    return ret;
}

void CalDavCalendarDatabase::insertETags(const QString &notebookUid, QHash<QString, QString> &eTags)
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    d->queue.eTags[notebookUid].unite(eTags);
}

bool CalDavCalendarDatabase::hasChanges()
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);
    return !d->queue.addedIncidences.isEmpty()
            || !d->queue.modifiedIncidences.isEmpty()
            || !d->queue.deletedIncidences.isEmpty()
            || !d->queue.notebookUidsToDelete.isEmpty()
            || !d->queue.eTags.isEmpty();
}

void CalDavCalendarDatabase::commit()
{
    executeWrite();
}

bool CalDavCalendarDatabase::write()
{
    Q_D(CalDavCalendarDatabase);

    QMutexLocker locker(&d->mutex);

    QHash<QString, bool> notebookUidsToDelete = d->queue.notebookUidsToDelete;
    QHash<QString, QSet<KCalId> > addedIncidences = d->queue.addedIncidences;
    QHash<QString, CalDavCalendarDatabasePrivate::IncidenceUpdate> modifiedIncidences = d->queue.modifiedIncidences;
    QHash<QString, QSet<KCalId> > deletedIncidences = d->queue.deletedIncidences;
    QHash<QString, CalDavCalendarDatabasePrivate::IncidenceETag> eTags = d->queue.eTags;

    d->queue.notebookUidsToDelete.clear();
    d->queue.addedIncidences.clear();
    d->queue.modifiedIncidences.clear();
    d->queue.deletedIncidences.clear();
    d->queue.eTags.clear();

    locker.unlock();

    QStringList notebookUids = notebookUidsToDelete.keys();
    if (notebookUidsToDelete.count()) {
        if (!d->writeNotebookRemovals(QStringLiteral("Additions"), notebookUids)) {
            return false;
        }
        if (!d->writeNotebookRemovals(QStringLiteral("Modifications"), notebookUids)) {
            return false;
        }
        if (!d->writeNotebookRemovals(QStringLiteral("Deletions"), notebookUids)) {
            return false;
        }
        QStringList removeETagsWithNotebookUids;
        Q_FOREACH (const QString &notebookUid, notebookUids) {
            if (notebookUidsToDelete[notebookUid] == CalDavCalendarDatabasePrivate::RemoveAllEntries) {
                removeETagsWithNotebookUids.append(notebookUid);
            }
        }
        if (removeETagsWithNotebookUids.count() &&
                !d->writeNotebookRemovals(QStringLiteral("ETags"), removeETagsWithNotebookUids)) {
            return false;
        }
    }

    if (!addedIncidences.isEmpty() && !d->writeAdditions(addedIncidences)) {
        return false;
    }
    if (!modifiedIncidences.isEmpty() && !d->writeModifications(modifiedIncidences)) {
        return false;
    }
    if (!deletedIncidences.isEmpty() && !d->writeDeletions(deletedIncidences)) {
        return false;
    }
    if (!eTags.isEmpty() && !d->writeETags(eTags)) {
        return false;
    }
    return true;
}

bool CalDavCalendarDatabasePrivate::writeNotebookRemovals(const QString &table, const QStringList &notebookUids)
{
    Q_Q(CalDavCalendarDatabase);

    if (notebookUids.isEmpty()) {
        return true;
    }
    QStringList uids;
    Q_FOREACH (const QString &notebookUid, notebookUids) {
        uids << ("'" + notebookUid + "'");
    }
    bool success = true;
    QString queryString = QStringLiteral("DELETE FROM %1 WHERE notebookUid IN (%2)");
    queryString = queryString.arg(table).arg(uids.join(","));
    QSqlQuery query = q->prepare(queryString);
    executeSocialCacheQuery(query);

    return success;
}

bool CalDavCalendarDatabasePrivate::writeAdditions(const QHash<QString, QSet<KCalId> > &addedIncidences)
{
    Q_Q(CalDavCalendarDatabase);

    QVariantList incidenceUidsVariants;
    QVariantList notebookUidsVariants;
    Q_FOREACH (const QString &notebookUid, addedIncidences.keys()) {
        const QSet<KCalId> &additions = addedIncidences[notebookUid];
        Q_FOREACH (const KCalId &kcalid, additions) {
            incidenceUidsVariants << kcalid.toString();
            notebookUidsVariants << notebookUid;
        }
    }
    if (incidenceUidsVariants.count()) {
        bool success = true;
        QSqlQuery query = q->prepare(QStringLiteral("INSERT INTO Additions ("
                                                    "incidenceUid, notebookUid) "
                                                    "VALUES (:"
                                                    "incidenceUid, :notebookUid)"));
        query.addBindValue(incidenceUidsVariants);
        query.addBindValue(notebookUidsVariants);
        executeBatchSocialCacheQuery(query);
        return success;
    } else {
        return true;
    }
}

bool CalDavCalendarDatabasePrivate::writeModifications(const QHash<QString, IncidenceUpdate> &modifiedIncidences)
{
    Q_Q(CalDavCalendarDatabase);

    QVariantList incidenceUidsVariants;
    QVariantList notebookUidsVariants;
    QVariantList incidenceICalVariants;
    Q_FOREACH (const QString &notebookUid, modifiedIncidences.keys()) {
        const IncidenceUpdate &modifications = modifiedIncidences[notebookUid];
        Q_FOREACH (const KCalId &kcalid, modifications.uniqueKeys()) {
            incidenceUidsVariants << kcalid.toString();
            notebookUidsVariants << notebookUid;
            incidenceICalVariants << modifications[kcalid];
        }
    }
    if (incidenceUidsVariants.count()) {
        bool success = true;
        QSqlQuery query = q->prepare(QStringLiteral("INSERT INTO Modifications ("
                                                    "incidenceUid, notebookUid, iCalData) "
                                                    "VALUES ("
                                                    ":incidenceUid, :notebookUid, :iCalData)"));
        query.addBindValue(incidenceUidsVariants);
        query.addBindValue(notebookUidsVariants);
        query.addBindValue(incidenceICalVariants);
        executeBatchSocialCacheQuery(query);
        return success;
    } else {
        return true;
    }
}

bool CalDavCalendarDatabasePrivate::writeDeletions(const QHash<QString, QSet<KCalId> > &deletedIncidences)
{
    Q_Q(CalDavCalendarDatabase);

    QVariantList incidenceUidsVariants;
    QVariantList notebookUidsVariants;
    Q_FOREACH (const QString &notebookUid, deletedIncidences.keys()) {
        const QSet<KCalId> &additions = deletedIncidences[notebookUid];
        Q_FOREACH (const KCalId &kcalid, additions) {
            incidenceUidsVariants << kcalid.toString();
            notebookUidsVariants << notebookUid;
        }
    }
    if (incidenceUidsVariants.count()) {
        bool success = true;
        QSqlQuery query = q->prepare(QStringLiteral("INSERT INTO Deletions ("
                                                    "incidenceUid, notebookUid) "
                                                    "VALUES (:"
                                                    "incidenceUid, :notebookUid)"));
        query.addBindValue(incidenceUidsVariants);
        query.addBindValue(notebookUidsVariants);
        executeBatchSocialCacheQuery(query);
        return success;
    } else {
        return true;
    }
}

bool CalDavCalendarDatabasePrivate::writeETags(const QHash<QString, IncidenceETag> &eTags)
{
    Q_Q(CalDavCalendarDatabase);

    QVariantList incidenceUidsVariants;
    QVariantList notebookUidsVariants;
    QVariantList incidenceETagVariants;
    Q_FOREACH (const QString &notebookUid, eTags.uniqueKeys()) {
        const IncidenceETag &incidencesAndETags = eTags[notebookUid];
        Q_FOREACH (const QString &uri, incidencesAndETags.uniqueKeys()) {
            incidenceUidsVariants << uri;
            notebookUidsVariants << notebookUid;
            incidenceETagVariants << incidencesAndETags[uri];
        }
    }
    if (incidenceUidsVariants.count()) {
        bool success = true;
        QSqlQuery query = q->prepare(QStringLiteral("INSERT OR REPLACE INTO ETags ("
                                                    "incidenceUid, notebookUid, eTag) "
                                                    "VALUES ("
                                                    ":incidenceUid, :notebookUid, :eTag)"));
        query.addBindValue(incidenceUidsVariants);
        query.addBindValue(notebookUidsVariants);
        query.addBindValue(incidenceETagVariants);
        executeBatchSocialCacheQuery(query);
        return success;
    } else {
        return true;
    }
}

bool CalDavCalendarDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS Additions ("
                                 "incidenceUid TEXT PRIMARY KEY,"
                                 "notebookUid TEXT NOT NULL)"));
    if (!query.exec()) {
        qWarning() << "Cannot create additions table:" << query.lastError().text();
        return false;
    }

    query.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS Modifications ("
                                 "incidenceUid TEXT PRIMARY KEY,"
                                 "notebookUid TEXT NOT NULL,"
                                 "iCalData TEXT)"));
    if (!query.exec()) {
        qWarning() << "Cannot create modifications table:" << query.lastError().text();
        return false;
    }

    query.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS Deletions ("
                                 "incidenceUid TEXT PRIMARY KEY,"
                                 "notebookUid TEXT NOT NULL)"));
    if (!query.exec()) {
        qWarning() << "Cannot create deletions table:" << query.lastError().text();
        return false;
    }

    query.prepare(QStringLiteral("CREATE TABLE IF NOT EXISTS ETags ("
                                 "incidenceUid TEXT PRIMARY KEY,"
                                 "notebookUid TEXT NOT NULL,"
                                 "eTag TEXT)"));
    if (!query.exec()) {
        qWarning() << "Cannot create etags table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool CalDavCalendarDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("DROP TABLE IF EXISTS Additions"));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete additions table:" << query.lastError().text();
        return false;
    }

    query.prepare(QStringLiteral("DROP TABLE IF EXISTS Modifications"));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete modifications table:" << query.lastError().text();
        return false;
    }

    query.prepare(QStringLiteral("DROP TABLE IF EXISTS Deletions"));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete deletions table:" << query.lastError().text();
        return false;
    }

    query.prepare(QStringLiteral("DROP TABLE IF EXISTS ETags"));
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete etags table:" << query.lastError().text();
        return false;
    }

    return true;
}
