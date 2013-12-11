/*
 * Copyright (C) 2013 Jolla Ltd. <lucien.xu@jollamobile.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include <QtTest/QTest>
#include "facebookcontactsdatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class FacebookContactsTest: public QObject
{
    Q_OBJECT
private:

private slots:
    // Perform some cleanups
    // we basically remove the whole ~/.local/share/system/privileged. While it is
    // damaging on a device, on a desktop system it should not be much
    // damaging.
    void initTestCase()
    {
        QStandardPaths::enableTestMode(true);

        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();
    }

    void contacts()
    {
        FacebookContactsDatabase database;

        const QString friend1 = QLatin1String("friend1");
        const QString friend2 = QLatin1String("friend2");
        const QString friend3 = QLatin1String("friend3");
        const QString friend4 = QLatin1String("friend4");
        const QString friend5 = QLatin1String("friend5");

        const QString pictureUrl1 = QLatin1String("http://example.com/friend1.jpg");
        const QString pictureUrl2 = QLatin1String("http://example.com/friend1.jpg");
        const QString coverUrl1 = QLatin1String("http://example.com/cover1.jpg");
        const QString coverUrl2 = QLatin1String("http://example.com/cover2.jpg");

        const QString pictureFile1 = QLatin1String("/temp/friend1.jpg");
        const QString coverFile1 = QLatin1String("/temp/cover1.jpg");

        database.addSyncedContact(friend1, 1, pictureUrl1, coverUrl1);
        database.addSyncedContact(friend2, 1, pictureUrl2, coverUrl2);
        database.addSyncedContact(friend3, 1, pictureUrl1, coverUrl1);
        database.addSyncedContact(friend4, 1, pictureUrl1, coverUrl1);
        database.addSyncedContact(friend1, 2, pictureUrl1, coverUrl1);
        database.addSyncedContact(friend4, 2, pictureUrl1, coverUrl1);
        database.addSyncedContact(friend5, 2, pictureUrl1, coverUrl1);

        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        FacebookContact::ConstPtr contact;

        contact = database.contact(friend1, 1);
        QCOMPARE(contact->fbFriendId(), friend1);
        QCOMPARE(contact->accountId(), 1);
        QCOMPARE(contact->pictureUrl(), pictureUrl1);
        QCOMPARE(contact->coverUrl(), coverUrl1);
        QCOMPARE(contact->pictureFile(), QString());
        QCOMPARE(contact->coverFile(), QString());

        contact = database.contact(friend1, 2);
        QCOMPARE(contact->fbFriendId(), friend1);
        QCOMPARE(contact->accountId(), 2);
        QCOMPARE(contact->pictureUrl(), pictureUrl1);
        QCOMPARE(contact->coverUrl(), coverUrl1);
        QCOMPARE(contact->pictureFile(), QString());
        QCOMPARE(contact->coverFile(), QString());

        database.updatePictureFile(friend1, pictureFile1);
        database.updateCoverFile(friend1, coverFile1);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        contact = database.contact(friend1, 1);
        QCOMPARE(contact->pictureFile(), pictureFile1);
        QCOMPARE(contact->coverFile(), coverFile1);

        contact = database.contact(friend1, 2);
        QCOMPARE(contact->pictureFile(), pictureFile1);
        QCOMPARE(contact->coverFile(), coverFile1);

        QList<FacebookContact::ConstPtr> contacts;

        contacts = database.contacts(1);
        QCOMPARE(contacts.count(), 4);

        contacts = database.contacts(2);
        QCOMPARE(contacts.count(), 3);

        database.removeContacts(QStringList() << friend1 << friend2);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        contacts = database.contacts(1);
        QCOMPARE(contacts.count(), 2);

        contacts = database.contacts(2);
        QCOMPARE(contacts.count(), 2);

        database.removeContacts(1);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        contacts = database.contacts(1);
        QCOMPARE(contacts.count(), 0);

        contacts = database.contacts(2);
        QCOMPARE(contacts.count(), 2);

        database.removeContacts(2);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        contacts = database.contacts(2);
        QCOMPARE(contacts.count(), 0);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(FacebookContactsTest)

#include "main.moc"
