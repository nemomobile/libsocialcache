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
#include "abstractsocialcachedatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QStandardPaths>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtSql/QSqlQuery>

class DummyDatabase: public AbstractSocialCacheDatabase
{
public:
    explicit DummyDatabase():
        AbstractSocialCacheDatabase()
    {
        dbInit(QLatin1String("Test"),
                     QLatin1String("Test"),
                     QLatin1String("test.db"), 1);
    }

    void initDatabase() {}

    bool testInsert() {
        QMap<QString, QVariantList> entries;

        QStringList keys;
        keys << "id" << "value";

        QVariantList ids;
        ids.append(QVariant());
        ids.append(QVariant());
        ids.append(QVariant());

        QVariantList values;
        values.append(QLatin1String("a"));
        values.append(QLatin1String("b"));
        values.append(QLatin1String("c"));

        entries.insert(QLatin1String("id"), ids);
        entries.insert(QLatin1String("value"), values);

        return dbWrite(QLatin1String("tests"), keys, entries);
    }
    bool checkInsert() {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        query.prepare("SELECT id, value FROM tests");
        if (!query.exec()) {
            return false;
        }

        QList<int> expectedIds;
        expectedIds << 1 << 2 << 3;
        QList<QString> expectedValues;
        expectedValues << QLatin1String("a") << QLatin1String("b") << QLatin1String("c");

        int i = 0;
        while (query.next()) {
            if (query.value(0) != expectedIds.at(i) || query.value(1) != expectedValues.at(i))  {
                return false;
            }
            i++;
        }
        return true;
    }
    bool testUpdate() {
        QMap<QString, QVariantList> entries;

        QStringList keys;
        keys << "id" << "value";

        QVariantList ids;
        ids.append(QVariant(1));
        ids.append(QVariant(2));

        QVariantList values;
        values.append(QLatin1String("aa"));
        values.append(QLatin1String("bb"));

        entries.insert(QLatin1String("id"), ids);
        entries.insert(QLatin1String("value"), values);

        return dbWrite(QLatin1String("tests"), keys, entries, Update, QLatin1String("id"));
    }
    bool checkUpdate() {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        query.prepare("SELECT id, value FROM tests");
        if (!query.exec()) {
            return false;
        }

        QList<int> expectedIds;
        expectedIds << 1 << 2 << 3;
        QList<QString> expectedValues;
        expectedValues << QLatin1String("aa") << QLatin1String("bb") << QLatin1String("c");

        int i = 0;
        while (query.next()) {
            if (query.value(0) != expectedIds.at(i) || query.value(1) != expectedValues.at(i))  {
                return false;
            }
            i++;
        }
        return true;
    }
    bool testDelete() {
        QMap<QString, QVariantList> entries;

        QStringList keys;
        keys << "id";

        QVariantList ids;
        ids.append(QVariant(3));

        entries.insert(QLatin1String("id"), ids);

        return dbWrite(QLatin1String("tests"), keys, entries, Delete);
    }
    bool checkDelete() {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        query.prepare("SELECT id, value FROM tests");
        if (!query.exec()) {
            return false;
        }

        QList<int> expectedIds;
        expectedIds << 1 << 2;
        QList<QString> expectedValues;
        expectedValues << QLatin1String("aa") << QLatin1String("bb");

        int i = 0;
        while (query.next()) {
            if (query.value(0) != expectedIds.at(i) || query.value(1) != expectedValues.at(i))  {
                return false;
            }
            i++;
        }
        return true;
    }

    void clean() {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        query.prepare("DELETE FROM tests");
        query.exec();
    }

    void benchmarkInsertBatch() {
        QMap<QString, QVariantList> entries;

        QStringList keys;
        keys << "id" << "value";

        QVariantList ids;
        QVariantList values;

        for (int i = 0; i < 100; i ++) {
            ids.append(QVariant());
            values.append(QLatin1String("a"));
        }

        entries.insert(QLatin1String("id"), ids);
        entries.insert(QLatin1String("value"), values);

        dbWrite(QLatin1String("tests"), keys, entries, Insert);
    }

    void benchmarkInsertNaive() {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        for (int i = 0; i < 100; i ++) {
            query.prepare("INSERT INTO tests VALUES(:id, :value)");
            query.bindValue(QLatin1String(":id"), QVariant());
            query.bindValue(QLatin1String(":value"), QVariant(QLatin1String("a")));
            query.exec();
        }
    }

    void benchmarkInsertBatchWithTransaction() {
        dbBeginTransaction();

        QMap<QString, QVariantList> entries;

        QStringList keys;
        keys << "id" << "value";

        QVariantList ids;
        QVariantList values;

        for (int i = 0; i < 100; i ++) {
            ids.append(QVariant());
            values.append(QLatin1String("a"));
        }

        entries.insert(QLatin1String("id"), ids);
        entries.insert(QLatin1String("value"), values);

        dbWrite(QLatin1String("tests"), keys, entries, Insert);

        dbCommitTransaction();
    }

    void benchmarkPrepareDeletion() {
        Q_D(AbstractSocialCacheDatabase);
        dbBeginTransaction();

        // We will fill the data with 500 albums
        // And 500 photos per album
        QSqlQuery query(d->db);
        query.prepare("DELETE FROM albums");
        query.exec();
        query.prepare("DELETE FROM SQLITE_SEQUENCE WHERE NAME = :name");
        query.bindValue(":name", QLatin1String("albums"));
        query.exec();
        query.prepare("DELETE FROM photos");
        query.exec();
        query.prepare("DELETE FROM SQLITE_SEQUENCE WHERE NAME = :name");
        query.bindValue(":name", QLatin1String("photos"));
        query.exec();

        QMap<QString, QVariantList> entries;

        QStringList keys;
        keys << "id" << "value";

        QVariantList ids;
        QVariantList albumIds;
        QVariantList values;

        for (int i = 0; i < 500; i ++) {
            ids.append(QVariant());
            values.append(QString(QLatin1String("Album %1")).arg(i + 1));
        }

        entries.insert(QLatin1String("id"), ids);
        entries.insert(QLatin1String("value"), values);

        dbWrite(QLatin1String("albums"), keys, entries, Insert);

        entries.clear();
        keys.clear();
        ids.clear();
        values.clear();

        keys << "id" << "albumId" << "value";

        for (int i = 0; i < 500; i ++) {
            for (int j = 0; j < 500; j++) {
                ids.append(QVariant());
                albumIds.append(QVariant(i + 1));
                values.append(QString(QLatin1String("Photo %1 in album %2")).arg(j + 1).arg(i + 1));
            }
        }

        entries.insert(QLatin1String("id"), ids);
        entries.insert(QLatin1String("albumId"), albumIds);
        entries.insert(QLatin1String("value"), values);

        dbWrite(QLatin1String("photos"), keys, entries, Insert);


        dbCommitTransaction();
    }

    void benchmarkDeleteAlbum() {
        Q_D(AbstractSocialCacheDatabase);
        dbBeginTransaction();

        QSqlQuery query(d->db);
        query.prepare("DELETE FROM albums WHERE id = :id");
        query.bindValue(":id", QVariant(1));
        query.exec();

        dbCommitTransaction();
    }

    void benchmarkDeletePhotos() {
        Q_D(AbstractSocialCacheDatabase);
        dbBeginTransaction();

        QSqlQuery query(d->db);
        query.prepare("DELETE FROM photos WHERE albumId = :id");
        query.bindValue(":id", QVariant(1));
        query.exec();

        dbCommitTransaction();
    }

protected:
    bool dbCreateTables()
    {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        query.prepare( "CREATE TABLE IF NOT EXISTS tests ("
                       "id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT,"
                       "value TEXT)");
        if (!query.exec()) {
            d->db.close();
            return false;
        }

        query.prepare( "CREATE TABLE IF NOT EXISTS albums ("
                       "id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT,"
                       "value TEXT)");
        if (!query.exec()) {
            d->db.close();
            return false;
        }

        query.prepare( "CREATE TABLE IF NOT EXISTS photos ("
                       "id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT,"
                       "albumId INTEGER,"
                       "value TEXT)");
        if (!query.exec()) {
            d->db.close();
            return false;
        }

        if (!dbCreatePragmaVersion(1)) {
            return false;
        }

        return true;
    }

    bool dbDropTables()
    {
        Q_D(AbstractSocialCacheDatabase);
        QSqlQuery query(d->db);
        query.prepare("DROP TABLE IF EXISTS tests");
        if (!query.exec()) {
            d->db.close();
            return false;
        }

        query.prepare("DROP TABLE IF EXISTS albums");
        if (!query.exec()) {
            d->db.close();
            return false;
        }

        query.prepare("DROP TABLE IF EXISTS photos");
        if (!query.exec()) {
            d->db.close();
            return false;
        }

        return true;
    }
private:
    Q_DECLARE_PRIVATE(AbstractSocialCacheDatabase)
};


class AbstractSocialCacheDatabaseTest: public QObject
{
    Q_OBJECT
private:
    DummyDatabase *db;
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

    void testCommits()
    {
        db = new DummyDatabase();
        QVERIFY(db->isValid());
        QVERIFY(db->testInsert());
        QVERIFY(db->checkInsert());
        QVERIFY(db->testUpdate());
        QVERIFY(db->checkUpdate());
        QVERIFY(db->testDelete());
        QVERIFY(db->checkDelete());
    }

    void insertionBenchmarkBatch()
    {
        db->clean();
        // Beware this takes time
//        QBENCHMARK(db->benchmarkInsertBatch());
    }

    void insertionBenchmarkNaive()
    {
        db->clean();
        // Beware this takes time
//        QBENCHMARK(db->benchmarkInsertNaive());
    }

    void insertionBenchmarkTransaction()
    {
        db->clean();
        QBENCHMARK(db->benchmarkInsertBatchWithTransaction());
    }

    void heavyInsertionBenchmark()
    {
        QBENCHMARK(db->benchmarkPrepareDeletion());
    }

    void primaryKeyDeletion()
    {
        db->benchmarkPrepareDeletion();
        QBENCHMARK_ONCE(db->benchmarkDeleteAlbum());
    }

    void nonPrimaryKeyDeletion()
    {
        db->benchmarkPrepareDeletion();
        QBENCHMARK_ONCE(db->benchmarkDeletePhotos());
    }

    void cleanupTestCase()
    {
        delete db;
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();
    }
};

QTEST_MAIN(AbstractSocialCacheDatabaseTest)

#include "main.moc"
