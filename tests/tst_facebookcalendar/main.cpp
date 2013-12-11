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
#include "facebookcalendardatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class FacebookCalendarTest: public QObject
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
        FacebookCalendarDatabase database;

        const QString event1 = QLatin1String("event1");
        const QString event2 = QLatin1String("event2");
        const QString event3 = QLatin1String("event3");
        const QString event4 = QLatin1String("event4");
        const QString event5 = QLatin1String("event5");

        const QString incidence1 = QLatin1String("incidence1");
        const QString incidence2 = QLatin1String("incidence2");
        const QString incidence3 = QLatin1String("incidence3");

        database.addSyncedEvent(event1, 1, incidence1);
        database.addSyncedEvent(event2, 1, incidence1);
        database.addSyncedEvent(event3, 1, incidence2);
        database.addSyncedEvent(event4, 2, incidence1);
        database.addSyncedEvent(event5, 2, incidence3);

        database.sync(1);
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        QList<FacebookEvent::ConstPtr> events;

        events = database.events(1);
        QCOMPARE(events.count(), 3);

        events = database.events(2);
        QCOMPARE(events.count(), 0);

        database.sync(2);
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        events = database.events(2);
        QCOMPARE(events.count(), 2);

        if (events.first()->fbEventId() == event5) {
            events.prepend(events.takeLast());
        }

        QCOMPARE(events.at(0)->fbEventId(), event4);
        QCOMPARE(events.at(0)->accountId(), 2);
        QCOMPARE(events.at(0)->incidenceId(), incidence1);

        QCOMPARE(events.at(1)->fbEventId(), event5);
        QCOMPARE(events.at(1)->accountId(), 2);
        QCOMPARE(events.at(1)->incidenceId(), incidence3);

        database.removeEvents(1);
        database.sync(1);
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        events = database.events(1);
        QCOMPARE(events.count(), 0);

        events = database.events(2);
        QCOMPARE(events.count(), 2);

        database.removeEvents(2);
        database.sync(2);
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        events = database.events(1);
        QCOMPARE(events.count(), 0);

        events = database.events(2);
        QCOMPARE(events.count(), 0);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(FacebookCalendarTest)

#include "main.moc"
