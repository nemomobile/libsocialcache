/*
 * Copyright (C) 2014 Jolla Ltd. <antti.seppala@jollamobile.com>
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
#include "facebooknotificationsdatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class FacebookNotificationsTest: public QObject
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

    void notifications()
    {
        QDateTime time1(QDate(2013, 1, 2), QTime(12, 34, 56));
        QDateTime time2(QDate(2012, 3, 4), QTime(10, 11, 12));
        QDateTime time3(QDate(2014, 1, 2), QTime(12, 34, 56));

        const QString id1 = QLatin1String("id1");
        const QString id2 = QLatin1String("id2");
        const QString id3 = QLatin1String("id3");

        const QString from1 = QLatin1String("from1");
        const QString from2 = QLatin1String("from2");
        const QString from3 = QLatin1String("from3");

        const QString to1 = QLatin1String("to1");
        const QString to2 = QLatin1String("to2");
        const QString to3 = QLatin1String("to3");

        const QString title1 = QLatin1String("title1");
        const QString title2 = QLatin1String("title2");
        const QString title3 = QLatin1String("title3");
        
        const QString link1 = QLatin1String("link1");
        const QString link2 = QLatin1String("link2");
        const QString link3 = QLatin1String("link3");

        const QString app1 = QLatin1String("app1");
        const QString app2 = QLatin1String("app2");
        const QString app3 = QLatin1String("app3");

        const QString object1 = QLatin1String("object1");
        const QString object2 = QLatin1String("object2");
        const QString object3 = QLatin1String("object3");

        const bool unread1 = true;
        const bool unread2 = false;
        const bool unread3 = false;

        int account1 = 1; 
        int account2 = 2; 

        QString clientId = QLatin1String("clientId");

        FacebookNotificationsDatabase database;

        database.addFacebookNotification(id1, from1, to1, time1, time1,
                                         title1, link1, app1, object1, unread1, account1, clientId);

        database.addFacebookNotification(id2, from2, to2, time2, time2,
                                         title2, link2, app2, object2, unread2, account1, clientId);

        database.addFacebookNotification(id3, from3, to3, time3, time3,
                                         title3, link3, app3, object3, unread3, account2, clientId);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        QList<FacebookNotification::ConstPtr> notifications;

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 3);

        FacebookNotification::ConstPtr notification;
        do {
            notification = notifications.takeFirst();
        } while (notification->facebookId() != id1 && notifications.count() > 0);

        QCOMPARE(notification->facebookId(), id1);
        QCOMPARE(notification->from(), from1);
        QCOMPARE(notification->to(), to1);
        QCOMPARE(notification->createdTime(), time1);
        QCOMPARE(notification->updatedTime(), time1);
        QCOMPARE(notification->title(), title1);
        QCOMPARE(notification->link(), link1);
        QCOMPARE(notification->application(), app1);
        QCOMPARE(notification->object(), object1);
        QCOMPARE(notification->unread(), unread1);
        QCOMPARE(notification->accountId(), account1);
        QCOMPARE(notification->clientId(), clientId);

        QStringList toBeDeleted;
        toBeDeleted.append(id1);
        toBeDeleted.append(id2);
        database.removeNotifications(toBeDeleted);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 1);

        notification = notifications.takeFirst();
        QCOMPARE(notification->facebookId(), id3);
        QCOMPARE(notification->from(), from3);
        QCOMPARE(notification->to(), to3);
        QCOMPARE(notification->createdTime(), time3);
        QCOMPARE(notification->updatedTime(), time3);
        QCOMPARE(notification->title(), title3);
        QCOMPARE(notification->link(), link3);
        QCOMPARE(notification->application(), app3);
        QCOMPARE(notification->object(), object3);
        QCOMPARE(notification->unread(), unread3);
        QCOMPARE(notification->accountId(), account2);
        QCOMPARE(notification->clientId(), clientId);

        database.removeNotification(id3);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);
        
        notifications = database.notifications();
        QCOMPARE(notifications.count(), 0);

        database.addFacebookNotification(id1, from1, to1, time1, time1,
                                         title1, link1, app1, object1, unread1, account1, clientId);

        database.addFacebookNotification(id2, from2, to2, time2, time2,
                                         title2, link2, app2, object2, unread2, account1, clientId);

        database.addFacebookNotification(id3, from3, to3, time3, time3,
                                         title3, link3, app3, object3, unread3, account2, clientId);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 3);

        database.removeNotifications(account1);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 1);

        database.removeNotifications(account2);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 0);
          
        database.addFacebookNotification(id1, from1, to1, time1, time1,
                                         title1, link1, app1, object1, unread1, account1, clientId);
        database.removeNotification(id1);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 0);
    }

    void purgeOldNotifications()
    {
        QDateTime time1 = QDateTime::currentDateTime();
        QDateTime time2;
        time2.setTime_t(time1.toTime_t() - 10 * 24 * 60 * 60);

        const QString id1 = QLatin1String("id1");
        const QString id2 = QLatin1String("id2");

        const QString from1 = QLatin1String("from1");
        const QString from2 = QLatin1String("from2");

        const QString to1 = QLatin1String("to1");
        const QString to2 = QLatin1String("to2");

        const QString title1 = QLatin1String("title1");
        const QString title2 = QLatin1String("title2");

        const QString link1 = QLatin1String("link1");
        const QString link2 = QLatin1String("link2");

        const QString app1 = QLatin1String("app1");
        const QString app2 = QLatin1String("app2");

        const QString object1 = QLatin1String("object1");
        const QString object2 = QLatin1String("object2");

        const bool unread1 = true;
        const bool unread2 = false;

        int account1 = 1;

        QString clientId = QLatin1String("clientId");

        FacebookNotificationsDatabase database;

        database.addFacebookNotification(id1, from1, to1, time1, time1,
                                         title1, link1, app1, object1, unread1, account1, clientId);

        database.addFacebookNotification(id2, from2, to2, time2, time2,
                                         title2, link2, app2, object2, unread2, account1, clientId);

        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        QList<FacebookNotification::ConstPtr> notifications;

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 2);

        database.purgeOldNotifications(9);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        notifications = database.notifications();
        QCOMPARE(notifications.count(), 1);

        FacebookNotification::ConstPtr notification = notifications.takeFirst();
        QCOMPARE(notification->facebookId(), id1);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();
    }
};

QTEST_MAIN(FacebookNotificationsTest)

#include "main.moc"
