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
#include "facebookimagesdatabase.h"
#include "socialsyncinterface.h"
#include "facebook/facebookimagecachemodel.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class FacebookImageTest: public QObject
{
    Q_OBJECT
private:
    FacebookImagesDatabase *fbDb;
    QSqlDatabase *checkDb;
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

        // Will create the database
        fbDb = new FacebookImagesDatabase();
        fbDb->initDatabase();

        QString dataType = SocialSyncInterface::dataType(SocialSyncInterface::Images);
        QString dbFile = QLatin1String("facebook.db"); // DB_NAME in facebookimagesdatabase.cpp


        checkDb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
        checkDb->setDatabaseName(QString("%1/%2/%3").arg(PRIVILEGED_DATA_DIR, dataType, dbFile));
        QVERIFY(checkDb->open());
    }

    void testAddUser()
    {
        QDateTime time1 (QDate(2013, 1, 2), QTime(12, 34, 56));

        fbDb->addUser("a", time1, "c");
        QVERIFY(fbDb->write());

        // Check if the user has been written
        QSqlQuery query (*checkDb);
        query.prepare("SELECT fbUserId, updatedTime, userName "\
                      "FROM users");
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QLatin1String("a"));
        QCOMPARE(query.value(1).toUInt(), time1.toTime_t());
        QCOMPARE(query.value(2).toString(), QLatin1String("c"));
        QVERIFY(!query.next());

        // Check if we can retrieve the user
        FacebookUser::ConstPtr user = fbDb->user(QLatin1String("a"));
        QCOMPARE(user->fbUserId(), QLatin1String("a"));
        QCOMPARE(user->updatedTime(), time1);
        QCOMPARE(user->userName(), QLatin1String("c"));
        QCOMPARE(user->count(), -1);

        QDateTime time2 (QDate(2012, 3, 4), QTime(10, 11, 12));

        // Let's write another user
        fbDb->addUser("d", time2, "f");
        QVERIFY(fbDb->write());

        // Check if we can retrieve everybody
        QList<FacebookUser::ConstPtr> users = fbDb->users();
        QCOMPARE(users.count(), 2);
        QCOMPARE(users[0]->fbUserId(), QLatin1String("a"));
        QCOMPARE(users[0]->updatedTime(), time1);
        QCOMPARE(users[0]->userName(), QLatin1String("c"));
        QCOMPARE(users[0]->count(), 0);

        QCOMPARE(users[1]->fbUserId(), QLatin1String("d"));
        QCOMPARE(users[1]->updatedTime(), time2);
        QCOMPARE(users[1]->userName(), QLatin1String("f"));
        QCOMPARE(users[1]->count(), 0);

        FacebookImageCacheModel model;
        model.setType(FacebookImageCacheModel::Users);
        model.refresh();
        QCOMPARE(model.count(), 0);
        QTest::qSleep(1000);
        QCOMPARE(model.count(), 2);
    }

    // TODO: more tests



    void cleanupTestCase()
    {
        delete fbDb;
        delete checkDb;

        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(FacebookImageTest)

#include "main.moc"
