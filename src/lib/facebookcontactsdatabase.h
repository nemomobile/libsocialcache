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

#ifndef FACEBOOKCONTACTSDATABASE_H
#define FACEBOOKCONTACTSDATABASE_H

#include "abstractsocialcachedatabase.h"
#include <QtCore/QSharedPointer>

class FacebookContactPrivate;
class FacebookContact
{
public:
    typedef QSharedPointer<FacebookContact> Ptr;
    typedef QSharedPointer<const FacebookContact> ConstPtr;
    virtual ~FacebookContact();
    static FacebookContact::Ptr create(const QString &fbFriendId, int accountId,
                                       const QString &pictureUrl, const QString &coverUrl,
                                       const QString &pictureFile, const QString &coverFile);
    QString fbFriendId() const;
    int accountId() const;
    QString pictureUrl() const;
    QString coverUrl() const;
    QString pictureFile() const;
    QString coverFile() const;
protected:
    QScopedPointer<FacebookContactPrivate> d_ptr;
private:
    Q_DECLARE_PRIVATE(FacebookContact)
    explicit FacebookContact(const QString &fbFriendId, int accountId,
                             const QString &pictureUrl, const QString &coverUrl,
                             const QString &pictureFile, const QString &coverFile);
};

class FacebookContactsDatabasePrivate;
class FacebookContactsDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
public:
    explicit FacebookContactsDatabase();
    ~FacebookContactsDatabase();

    bool removeContacts(int accountId);
    bool removeContacts(const QStringList &fbFriendIds);

    FacebookContact::ConstPtr contact(const QString &fbFriendId, int accountId) const;
    QList<FacebookContact::ConstPtr> contacts(int accountId) const;
    QStringList contactIds(int accountId) const;
    void addSyncedContact(const QString &fbFriendId, int accountId, const QString &pictureUrl,
                          const QString &coverUrl);
    void updatePictureFile(const QString &fbFriendId, const QString &pictureFile);
    void updateCoverFile(const QString &fbFriendId, const QString &coverFile);

    void commit();

protected:
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

private:
    Q_DECLARE_PRIVATE(FacebookContactsDatabase)
};

#endif // FACEBOOKCONTACTSDATABASE_H
