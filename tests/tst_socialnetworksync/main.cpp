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
#include "socialnetworksyncdatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class SocialNetworkSyncTest: public QObject
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

    void posts()
    {
        const QDateTime time1(QDate(2013, 1, 2), QTime(12, 34, 56));
        const QDateTime time2(QDate(2012, 3, 4), QTime(10, 11, 12));

        const QString service1 = QLatin1String("service1");
        const QString service2 = QLatin1String("service2");
        const QString data1 = QLatin1String("data1");
        const QString data2 = QLatin1String("data2");

        SocialNetworkSyncDatabase database;

        database.addSyncTimestamp(service1, data1, 1, time1);
        database.addSyncTimestamp(service1, data1, 1, time2);
        database.addSyncTimestamp(service1, data1, 2, time1);
        database.addSyncTimestamp(service1, data2, 1, time2);
        database.addSyncTimestamp(service2, data1, 1, time1);
        database.addSyncTimestamp(service2, data2, 2, time2);

        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        QList<int> accounts;

        accounts = database.syncedAccounts(service1, data1);
        QCOMPARE(accounts.contains(1), true);
        QCOMPARE(accounts.contains(2), true);

        accounts = database.syncedAccounts(service1, data2);
        QCOMPARE(accounts.contains(1), true);
        QCOMPARE(accounts.contains(2), false);

        accounts = database.syncedAccounts(service2, data2);
        QCOMPARE(accounts.contains(1), false);
        QCOMPARE(accounts.contains(2), true);

        QCOMPARE(database.lastSyncTimestamp(service1, data1, 1), time2);
        QCOMPARE(database.lastSyncTimestamp(service1, data1, 2), time1);
        QCOMPARE(database.lastSyncTimestamp(service1, data2, 1), time2);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(SocialNetworkSyncTest)

#include "main.moc"
