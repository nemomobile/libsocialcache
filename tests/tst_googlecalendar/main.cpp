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
#include "googlecalendardatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class GoogleCalendarTest: public QObject
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

    void events()
    {
        int accountId1 = 1;
        int accountId2 = 2;

        QString gcalEventId1 = "gcal1";
        QString gcalEventId2 = "gcal2";
        QString gcalEventId3 = "gcal3";

        QString localCalendarId1 = "localCal1";
        QString localCalendarId2 = "localCal2";
        QString localCalendarId3 = "localCal3";

        QString localEventId1 = "localEvent1";
        QString localEventId2 = "localEvent2";
        QString localEventId3 = "localEvent3";

        GoogleCalendarDatabase database;

        // insert 3 events and see that they are correctly inserted
        database.insertEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.insertEvent(accountId1, gcalEventId2, localCalendarId2, localEventId2, QString());
        database.insertEvent(accountId2, gcalEventId3, localCalendarId3, localEventId3, QString());
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        QList<GoogleEvent::ConstPtr> events;

        events = database.events(accountId1);
        QCOMPARE(events.count(), 2);

        GoogleEvent::ConstPtr event;
        do {
            event = events.takeFirst();
        } while (event->gcalEventId() != gcalEventId1 && events.count() > 0);

        QCOMPARE(event->accountId(), accountId1);
        QCOMPARE(event->gcalEventId(), gcalEventId1);
        QCOMPARE(event->localCalendarId(), localCalendarId1);
        QCOMPARE(event->localEventId(), localEventId1);

        events = database.events(accountId2);
        QCOMPARE(events.count(), 1);

        event = events.takeFirst();
        QCOMPARE(event->accountId(), accountId2);
        QCOMPARE(event->gcalEventId(), gcalEventId3);
        QCOMPARE(event->localCalendarId(), localCalendarId3);
        QCOMPARE(event->localEventId(), localEventId3);

        // remove events for accountId1, one event should be left behind
        database.removeEvents(accountId1);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        events = database.events(accountId1);
        QCOMPARE(events.count(), 0);

        events = database.events(accountId2);
        QCOMPARE(events.count(), 1);

        // verify that correct one is left in the db
        event = events.takeFirst();
        QCOMPARE(event->accountId(), accountId2);
        QCOMPARE(event->gcalEventId(), gcalEventId3);
        QCOMPARE(event->localCalendarId(), localCalendarId3);
        QCOMPARE(event->localEventId(), localEventId3);

        // remove events for accountId2
        database.removeEvents(accountId2);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // see that db is empty
        events = database.events(accountId2);
        QCOMPARE(events.count(), 0);

        // re-insert items
        database.insertEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.insertEvent(accountId1, gcalEventId2, localCalendarId2, localEventId2, QString());
        database.insertEvent(accountId2, gcalEventId3, localCalendarId3, localEventId3, QString());
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        events = database.events(accountId1);
        QCOMPARE(events.count(), 2);

        events = database.events(accountId2);
        QCOMPARE(events.count(), 1);

        // remove events based on accountId and localCalendarId
        database.removeEvents(accountId2, localCalendarId3);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // 2 events should remain
        events = database.events(accountId2);
        QCOMPARE(events.count(), 0);

        events = database.events(accountId1);
        QCOMPARE(events.count(), 2);

        // remove single event based on accountid and gcalEventId
        database.removeEvent(accountId1, gcalEventId1);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // see that the correct one was left behind.
        events = database.events(accountId1);
        QCOMPARE(events.count(), 1);

        event = events.takeFirst();
        QCOMPARE(event->accountId(), accountId1);
        QCOMPARE(event->gcalEventId(), gcalEventId2);
        QCOMPARE(event->localCalendarId(), localCalendarId2);
        QCOMPARE(event->localEventId(), localEventId2);

        // now supply all the parameters
        database.removeEvent(accountId1, gcalEventId2, localCalendarId2, localEventId2, QString());
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // confirm that db is now empty.
        events = database.events(accountId1);
        QCOMPARE(events.count(), 0);

        // try to insert same event twice
        database.insertEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.insertEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // confirm that only one instance ends up to db.
        events = database.events(accountId1);
        QCOMPARE(events.count(), 1);

        // clean up db
        database.removeEvents(accountId1);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // see that db is empty
        events = database.events(accountId1);
        QCOMPARE(events.count(), 0);
        events = database.events(accountId2);
        QCOMPARE(events.count(), 0);

        // now insert event and remove it without syncing in between, then sync.
        database.insertEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.removeEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // confirm that database remains empty
        events = database.events(accountId1);
        QCOMPARE(events.count(), 0);

        // insert event and persistent occurrence of that event (simulating RRULE + RECURRENCE-ID)
        QString gcalEventId1R1(QStringLiteral("%1-R1").arg(gcalEventId1));
        QString localEventRecurrenceId1(QStringLiteral("recurrence-id-1"));
        database.insertEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.insertEvent(accountId1, gcalEventId1R1, localCalendarId1, localEventId1, localEventRecurrenceId1);
        database.sync();
        database.wait();
        events = database.events(accountId1);
        QCOMPARE(events.count(), 2);

        // ensure that removal of the persistent occurrence doesn't cause removal of the recurring event
        database.removeEvent(accountId1, gcalEventId1R1, localCalendarId1, localEventId1, localEventRecurrenceId1);
        database.sync();
        database.wait();
        events = database.events(accountId1);
        QCOMPARE(events.count(), 1);
        event = events.takeFirst();
        QCOMPARE(event->accountId(), accountId1);
        QCOMPARE(event->gcalEventId(), gcalEventId1);
        QCOMPARE(event->localCalendarId(), localCalendarId1);
        QCOMPARE(event->localEventId(), localEventId1);
        QCOMPARE(event->localEventRecurrenceId(), QString());

        // ensure that removal of the recurring series doesn't cause removal of all occurrences
        database.insertEvent(accountId1, gcalEventId1R1, localCalendarId1, localEventId1, localEventRecurrenceId1);
        database.sync();
        database.wait();
        database.removeEvent(accountId1, gcalEventId1, localCalendarId1, localEventId1, QString());
        database.sync();
        database.wait();
        events = database.events(accountId1);
        QCOMPARE(events.count(), 1);
        event = events.takeFirst();
        QCOMPARE(event->accountId(), accountId1);
        QCOMPARE(event->gcalEventId(), gcalEventId1R1);
        QCOMPARE(event->localCalendarId(), localCalendarId1);
        QCOMPARE(event->localEventId(), localEventId1);
        QCOMPARE(event->localEventRecurrenceId(), localEventRecurrenceId1);

        // clean up db
        database.removeEvents(accountId1);
        database.sync();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);
    }

    void lastUpdateTimes()
    {
        QString calendarId1 = "localCal1";
        QString calendarId2 = "localCal2";
        QString time1 = "testing";
        QString time2 = "testing2";
        int account1 = 1;
        int account2 = 2;

        GoogleCalendarDatabase database;

        QVERIFY(database.lastUpdateTime("", 0) == "");
        QVERIFY(database.lastUpdateTime(calendarId1, 1) == "");
        QVERIFY(database.lastUpdateTime(calendarId2, 2) == "");

        database.setLastUpdateTime(calendarId1, account1, time1);
        database.setLastUpdateTime(calendarId2, account2, time2);
        database.sync();
        database.wait();

        QCOMPARE(database.lastUpdateTime(calendarId1, account1), time1);
        QCOMPARE(database.lastUpdateTime(calendarId2, account2), time2);

        database.removeLastUpdateTimes(account1);
        database.sync();
        database.wait();

        QVERIFY(database.lastUpdateTime(calendarId1, account1) == "");
        QCOMPARE(database.lastUpdateTime(calendarId2, account2), time2);

        database.removeLastUpdateTimes(account2);
        database.sync();
        database.wait();

        QVERIFY(database.lastUpdateTime(calendarId1, account1) == "");
        QVERIFY(database.lastUpdateTime(calendarId2, account2) == "");

        // set twice for same account, last one needs to be valid
        database.setLastUpdateTime(calendarId1, account1, time1);
        database.setLastUpdateTime(calendarId1, account1, time2);
        database.sync();
        database.wait();

        QCOMPARE(database.lastUpdateTime(calendarId1, account1), time2);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();
    }
};

QTEST_MAIN(GoogleCalendarTest)

#include "main.moc"
