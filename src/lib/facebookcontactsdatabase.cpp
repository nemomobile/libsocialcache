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

#include "facebookcontactsdatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"

#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

#include <QtDebug>

static const char *DB_NAME = "facebook.db";
static const int VERSION = 3;

static const char *PICTURE_FILE_KEY = "pictureFile";
static const char *COVER_FILE_KEY = "coverFile";

struct FacebookContactPrivate
{
    explicit FacebookContactPrivate(const QString &fbFriendId, int accountId,
                                    const QString &pictureUrl, const QString &coverUrl,
                                    const QString &pictureFile, const QString &coverFile);
    QString fbFriendId;
    int accountId;
    QString pictureUrl;
    QString coverUrl;
    QString pictureFile;
    QString coverFile;
};

FacebookContactPrivate::FacebookContactPrivate(const QString &fbFriendId, int accountId,
                                               const QString &pictureUrl, const QString &coverUrl,
                                               const QString &pictureFile, const QString &coverFile)
    : fbFriendId(fbFriendId), accountId(accountId), pictureUrl(pictureUrl)
    , coverUrl(coverUrl), pictureFile(pictureFile), coverFile(coverFile)
{
}

FacebookContact::FacebookContact(const QString &fbFriendId, int accountId,
                                 const QString &pictureUrl, const QString &coverUrl,
                                 const QString &pictureFile, const QString &coverFile)
    : d_ptr(new FacebookContactPrivate(fbFriendId, accountId, pictureUrl, coverUrl,
                                       pictureFile, coverFile))
{
}

FacebookContact::~FacebookContact()
{
}

FacebookContact::Ptr FacebookContact::create(const QString &fbFriendId, int accountId,
                                             const QString &pictureUrl, const QString &coverUrl,
                                             const QString &pictureFile, const QString &coverFile)
{
    return FacebookContact::Ptr(new FacebookContact(fbFriendId, accountId, pictureUrl, coverUrl,
                                                    pictureFile, coverFile));
}

QString FacebookContact::fbFriendId() const
{
    Q_D(const FacebookContact);
    return d->fbFriendId;
}

int FacebookContact::accountId() const
{
    Q_D(const FacebookContact);
    return d->accountId;
}

QString FacebookContact::pictureUrl() const
{
    Q_D(const FacebookContact);
    return d->pictureUrl;
}

QString FacebookContact::coverUrl() const
{
    Q_D(const FacebookContact);
    return d->coverUrl;
}

QString FacebookContact::pictureFile() const
{
    Q_D(const FacebookContact);
    return d->pictureFile;
}

QString FacebookContact::coverFile() const
{
    Q_D(const FacebookContact);
    return d->coverFile;
}

class FacebookContactsDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit FacebookContactsDatabasePrivate(FacebookContactsDatabase *q);
    void createUpdatedEntries(const QMap<QString, QMap<QString, QVariant> > &input,
                              const QString &primary, QMap<QString, QVariantList> &entries);
    void clearCachedImages(QSqlQuery &query);
    QList<FacebookContact::ConstPtr> queuedContacts;
    QMap<QString, QMap<QString, QVariant> > queuedUpdatedContacts;
};

FacebookContactsDatabasePrivate::FacebookContactsDatabasePrivate(FacebookContactsDatabase *q)
    : AbstractSocialCacheDatabasePrivate(q)
{
}

void FacebookContactsDatabasePrivate::createUpdatedEntries(const QMap<QString, QMap<QString, QVariant> > &input,
                                                           const QString &primary,
                                                           QMap<QString, QVariantList> &entries)
{
    entries.clear();

    // Check if we have the same data
    QStringList keys;
    foreach (const QVariantMap &inputEntry, input) {
        // TODO: instead of return, we should only insert entries that "works" to
        // the list.
        if (inputEntry.isEmpty()) {
            return;
        }

        if (keys.isEmpty()) {
            keys = inputEntry.keys();
        } else if (inputEntry.keys() != keys) {
            return;
        }
    }

    foreach (const QString &key, keys) {
        entries.insert(key, QVariantList());
    }
    entries.insert(primary, QVariantList());

    for (QMap<QString, QMap<QString, QVariant> >::const_iterator i = input.begin();
         i != input.end(); i++) {
        entries[primary].append(i.key());

        const QMap<QString, QVariant> &inputEntry = i.value();
        foreach (const QString &key, inputEntry.keys()) {
            entries[key].append(inputEntry.value(key));
        }
    }
}

void FacebookContactsDatabasePrivate::clearCachedImages(QSqlQuery &query)
{
    while (query.next()) {
        QString picture = query.value(0).toString();
        QString cover = query.value(1).toString();

        if (!picture.isEmpty()) {
            QFile pictureFile (picture);
            if (pictureFile.exists()) {
                pictureFile.remove();
            }
        }

        if (!cover.isEmpty()) {
            QFile coverFile (cover);
            if (coverFile.exists()) {
                coverFile.remove();
            }
        }
    }

}

FacebookContactsDatabase::FacebookContactsDatabase()
    : AbstractSocialCacheDatabase(*(new FacebookContactsDatabasePrivate(this)))
{
}

FacebookContactsDatabase::~FacebookContactsDatabase()
{
}

void FacebookContactsDatabase::initDatabase()
{
    dbInit(SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
           SocialSyncInterface::dataType(SocialSyncInterface::Contacts),
           QLatin1String(DB_NAME), VERSION);
}

bool FacebookContactsDatabase::removeContacts(int accountId)
{
    Q_D(FacebookContactsDatabase);
    if (!dbBeginTransaction()) {
        return false;
    }

    // Clean images
    QSqlQuery query (d->db);
    if (!query.prepare("SELECT pictureFile, coverFile FROM friends WHERE accountId = :accountId")) {
        qWarning() << Q_FUNC_INFO << "Failed to prepare cached images selection query:"
                   << query.lastError().text();
    } else {
        query.bindValue(":accountId", accountId);
        if (!query.exec()) {
            qWarning() << Q_FUNC_INFO << "Failed to exec cached contacts selection query:"
                       << query.lastError().text();
        } else {
            d->clearCachedImages(query);
        }
    }

    query.prepare(QLatin1String("DELETE FROM friends WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete contacts" << query.lastError().text();
        dbRollbackTransaction();
        return false;
    }

    return dbCommitTransaction();
}

bool FacebookContactsDatabase::removeContacts(const QStringList &fbFriendIds)
{
    Q_D(FacebookContactsDatabase);
    if (!dbBeginTransaction()) {
        return false;
    }

    // Clean images
    QSqlQuery query (d->db);
    foreach (const QString &fbFriendId, fbFriendIds) {
        if (!query.prepare("SELECT pictureFile, coverFile FROM friends WHERE fbFriendId = :fbFriendId")) {
            qWarning() << Q_FUNC_INFO << "Failed to prepare cached contacts selection query:"
                       << query.lastError().text();
        } else {
            query.bindValue(":fbFriendId", fbFriendId);
            if (!query.exec()) {
                qWarning() << Q_FUNC_INFO << "Failed to exec cached contacts selection query:"
                           << query.lastError().text();
            } else {
                d->clearCachedImages(query);
            }
        }
    }

    QVariantList fbFriendIdList;
    foreach (const QString &fbFriendId, fbFriendIds) {
        fbFriendIdList.append(fbFriendId);
    }
    QMap<QString, QVariantList> entries;
    entries.insert(QLatin1String("fbFriendId"), fbFriendIdList);
    QStringList keys;
    keys.append(QLatin1String("fbFriendId"));
    if (!dbWrite(QLatin1String("friends"), keys, entries, Delete)) {
        qWarning() << Q_FUNC_INFO << "Failed to clean contacts" << fbFriendIds;
        dbRollbackTransaction();
        return false;
    }

    if (!dbCommitTransaction()) {
        qWarning() << Q_FUNC_INFO << "Failed to commit transaction";
        dbRollbackTransaction();
        return false;
    }

    return true;
}

FacebookContact::ConstPtr FacebookContactsDatabase::contact(const QString &fbFriendId,
                                                            int accountId) const
{
    Q_D(const FacebookContactsDatabase);

    QSqlQuery query (d->db);
    query.prepare(QLatin1String("SELECT fbFriendId, accountId, pictureUrl, coverUrl, "\
                                "pictureFile, coverFile "\
                                "FROM friends "\
                                "WHERE fbFriendId = :fbFriendId "\
                                "AND accountId = :accountId"));
    query.bindValue(":fbFriendId", fbFriendId);
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query contacts" << query.lastError().text();
        return FacebookContact::ConstPtr();
    }

    if (!query.next()) {
        return FacebookContact::ConstPtr();
    }

    return FacebookContact::create(query.value(0).toString(), query.value(1).toInt(),
                                   query.value(2).toString(), query.value(3).toString(),
                                   query.value(4).toString(), query.value(5).toString());
}

QList<FacebookContact::ConstPtr> FacebookContactsDatabase::contacts(int accountId) const
{
    Q_D(const FacebookContactsDatabase);
    QList<FacebookContact::ConstPtr> data;

    QSqlQuery query (d->db);
    query.prepare(QLatin1String("SELECT fbFriendId, accountId, pictureUrl, coverUrl, "\
                                "pictureFile, coverFile "\
                                "FROM friends "\
                                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query contacts" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(FacebookContact::create(query.value(0).toString(), query.value(1).toInt(),
                                            query.value(2).toString(), query.value(3).toString(),
                                            query.value(4).toString(), query.value(5).toString()));
    }

    return data;
}

QStringList FacebookContactsDatabase::contactIds(int accountId) const
{
    Q_D(const FacebookContactsDatabase);
    QStringList data;

    QSqlQuery query (d->db);
    query.prepare(QLatin1String("SELECT fbFriendId FROM friends "\
                                "WHERE accountId = :accountId"));
    query.bindValue(":accountId", accountId);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query contacts" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(query.value(0).toString());
    }

    return data;
}

void FacebookContactsDatabase::addSyncedContact(const QString &fbFriendId, int accountId,
                                                const QString &pictureUrl, const QString &coverUrl)
{
    Q_D(FacebookContactsDatabase);
    d->queuedContacts.append(FacebookContact::create(fbFriendId, accountId, pictureUrl, coverUrl,
                                                     QString(), QString()));
}

void FacebookContactsDatabase::updatePictureFile(const QString &fbFriendId,
                                                 const QString &pictureFile)
{
    Q_D(FacebookContactsDatabase);
    if (!d->queuedUpdatedContacts.contains(fbFriendId)) {
        QMap<QString, QVariant> data;
        data.insert(QLatin1String(PICTURE_FILE_KEY), pictureFile);
        data.insert(QLatin1String(COVER_FILE_KEY), QString());
        d->queuedUpdatedContacts.insert(fbFriendId, data);
    } else {
        d->queuedUpdatedContacts[fbFriendId].insert(QLatin1String(PICTURE_FILE_KEY), pictureFile);
    }
}

void FacebookContactsDatabase::updateCoverFile(const QString &fbFriendId, const QString &coverFile)
{
    Q_D(FacebookContactsDatabase);
    if (!d->queuedUpdatedContacts.contains(fbFriendId)) {
        QMap<QString, QVariant> data;
        data.insert(QLatin1String(PICTURE_FILE_KEY), QString());
        data.insert(QLatin1String(COVER_FILE_KEY), coverFile);
        d->queuedUpdatedContacts.insert(fbFriendId, data);
    } else {
        d->queuedUpdatedContacts[fbFriendId].insert(QLatin1String(COVER_FILE_KEY), coverFile);
    }
}

bool FacebookContactsDatabase::write()
{
    Q_D(FacebookContactsDatabase);

    // We sync for one given account. We should have all the interesting
    // events queued. So we will wipe all events from the account id
    // and only add the events that are queued, and match the account id.
    if (!dbBeginTransaction()) {
        return false;
    }

    QStringList keys;
    keys << QLatin1String("fbFriendId") << QLatin1String("accountId")
         << QLatin1String("pictureUrl") << QLatin1String("coverUrl")
         << QLatin1String("pictureFile") << QLatin1String("coverFile");
    QMap<QString, QVariantList> data;
    foreach (const FacebookContact::ConstPtr &contact,d->queuedContacts) {
        data[QLatin1String("fbFriendId")].append(contact->fbFriendId());
        data[QLatin1String("accountId")].append(contact->accountId());
        data[QLatin1String("pictureUrl")].append(contact->pictureUrl());
        data[QLatin1String("coverUrl")].append(contact->coverUrl());
        data[QLatin1String("pictureFile")].append(contact->pictureFile());
        data[QLatin1String("coverFile")].append(contact->coverFile());
    }

    if (!dbWrite(QLatin1String("friends"), keys, data, InsertOrReplace)) {
        dbRollbackTransaction();
        return false;
    }

    QMap<QString, QVariantList> entries;
    d->createUpdatedEntries(d->queuedUpdatedContacts, QLatin1String("fbFriendId"),
                            entries);


    if (!dbWrite(QLatin1String("friends"), QStringList(), entries, Update,
                 QLatin1String("fbFriendId"))) {
        dbRollbackTransaction();
        return false;
    }

    qWarning() << "Updated" << d->queuedUpdatedContacts.count() << "contacts";
    qWarning() << "Inserted" << d->queuedContacts.count() << "contacts";

    d->queuedUpdatedContacts.clear();
    d->queuedContacts.clear();

    return dbCommitTransaction();
}

bool FacebookContactsDatabase::dbCreateTables()
{
    Q_D(FacebookContactsDatabase);
    QSqlQuery query(d->db);
    query.prepare( "CREATE TABLE IF NOT EXISTS friends ("
                   "fbFriendId TEXT,"
                   "accountId INTEGER,"
                   "pictureUrl TEXT,"
                   "coverUrl TEXT,"
                   "pictureFile TEXT,"
                   "coverFile TEXT,"
                   "CONSTRAINT id PRIMARY KEY (fbFriendId, accountId))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create friends table:" << query.lastError().text();
        return false;
    }

    if (!dbCreatePragmaVersion(VERSION)) {
        return false;
    }
    return true;
}

bool FacebookContactsDatabase::dbDropTables()
{
    Q_D(FacebookContactsDatabase);
    QSqlQuery query(d->db);
    query.prepare("DROP TABLE IF EXISTS friends");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete friends table:" << query.lastError().text();
        return false;
    }
    return true;
}
