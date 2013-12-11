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
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

class DummyDatabase: public AbstractSocialCacheDatabase
{
public:
    enum Test {
        None,
        Insert,
        Update,
        Delete,
        Clean,
        BenchmarkInsertBatch,
        BenchmarkInsertNaive,
        BenchmarkPrepareDeletion,
        BenchmarkDeleteAlbum,
        BenchmarkDeletePhotos
    };


    explicit DummyDatabase()
        : AbstractSocialCacheDatabase(*(new AbstractSocialCacheDatabasePrivate(
                this, QLatin1String("Test"), QLatin1String("Test"), QLatin1String("test.db"), 1)))
        , currentTest(None)
    {
    }

    using AbstractSocialCacheDatabase::executeRead;
    using AbstractSocialCacheDatabase::executeWrite;

    Test currentTest;

private:

    bool testInsert() {

        QVariantList ids;
        ids.append(QVariant(1));
        ids.append(QVariant(2));
        ids.append(QVariant(3));

        QVariantList values;
        values.append(QLatin1String("a"));
        values.append(QLatin1String("b"));
        values.append(QLatin1String("c"));

        bool success = true;

        QSqlQuery query = prepare(QStringLiteral(
                    "INSERT INTO tests ("
                    " id, value) "
                    "VALUES ("
                    " :id, :value)"));
        query.bindValue(QStringLiteral(":id"), ids);
        query.bindValue(QStringLiteral(":value"), values);
        executeBatchSocialCacheQuery(query);

        return success;
    }
    bool checkInsert() {
        QSqlQuery query = prepare(QStringLiteral("SELECT id, value FROM tests"));
        if (!query.exec()) {
            return false;
        }

        QList<int> expectedIds;
        expectedIds << 1 << 2 << 3;
        QList<QString> expectedValues;
        expectedValues << QLatin1String("a") << QLatin1String("b") << QLatin1String("c");

        int i = 0;
        while (query.next()) {
            if (i == expectedValues.count()) {
                return false;
            }
            if (query.value(0) != expectedIds.at(i) || query.value(1) != expectedValues.at(i))  {
                return false;
            }
            i++;
        }
        return true;
    }
    bool testUpdate() {
        QVariantList ids;
        ids.append(QVariant(1));
        ids.append(QVariant(2));

        QVariantList values;
        values.append(QLatin1String("aa"));
        values.append(QLatin1String("bb"));

        bool success = true;
        QSqlQuery query = prepare(QStringLiteral(
                    "UPDATE tests "
                    "SET value = :value "
                    "WHERE id = :id"));
        query.bindValue(QStringLiteral(":value"), values);
        query.bindValue(QStringLiteral(":id"), ids);
        executeBatchSocialCacheQuery(query);

        return success;
    }
    bool checkUpdate() {
        QSqlQuery query = prepare(QStringLiteral("SELECT id, value FROM tests"));
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
        QVariantList ids;
        ids.append(QVariant(3));

        bool success = true;
        QSqlQuery query = prepare(QStringLiteral(
                    "DELETE FROM tests "
                    "WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), ids);
        executeBatchSocialCacheQuery(query);

        return success;
    }
    bool checkDelete() {
        QSqlQuery query = prepare(QStringLiteral("SELECT id, value FROM tests"));
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
        QSqlQuery query = prepare(QStringLiteral("DELETE FROM tests"));
        query.exec();
    }

    void benchmarkInsertBatch() {
        QVariantList ids;
        QVariantList values;

        for (int i = 0; i < 100; i ++) {
            ids.append(QVariant());
            values.append(QLatin1String("a"));
        }

        QSqlQuery query = prepare("INSERT INTO tests(value) VALUES(:value)");
        query.bindValue(QLatin1String(":id"), ids);
        query.bindValue(QLatin1String(":value"), values);
        query.execBatch();
    }

    void benchmarkInsertNaive() {
        QSqlQuery query = prepare("INSERT INTO tests(value) VALUES(:value)");
        for (int i = 0; i < 100; i ++) {
            query.bindValue(QLatin1String(":value"), QVariant(QLatin1String("a")));
            query.exec();
        }
    }

    void benchmarkPrepareDeletion() {
        // We will fill the data with 500 albums
        // And 500 photos per album
        QSqlQuery query = prepare(QStringLiteral("DELETE FROM albums"));
        query.exec();
        query = prepare(QStringLiteral("DELETE FROM SQLITE_SEQUENCE WHERE NAME = :name"));
        query.bindValue(":name", QLatin1String("albums"));
        query.exec();
        query = prepare(QStringLiteral("DELETE FROM photos"));
        query.exec();
        query = prepare(QStringLiteral("DELETE FROM SQLITE_SEQUENCE WHERE NAME = :name"));
        query.bindValue(":name", QLatin1String("photos"));
        query.exec();

        query = prepare(QStringLiteral(
                    "INSERT INTO albums (value) "
                    "VALUES(:value)"));

        for (int i = 0; i < 500; i ++) {
            query.bindValue(QStringLiteral(":value"), QString(QStringLiteral("Album %1")).arg(i + 1));
            query.exec();
        }

        for (int i = 0; i < 500; i ++) {
            for (int j = 0; j < 500; j++) {

                query = prepare(QStringLiteral(
                            "INSERT INTO photos (albumId, value) "
                            "VALUES (:albumId, :value)"));

                query.bindValue(QStringLiteral(":albumId"), QVariant(i + 1));
                query.bindValue(QStringLiteral(":value"), QString(QStringLiteral("Photo %1 in album %2")).arg(j + 1).arg(i + 1));
                query.exec();
            }
        }
    }

    void benchmarkDeleteAlbum() {
        QSqlQuery query = prepare(QStringLiteral("DELETE FROM albums WHERE id = :id"));
        query.bindValue(":id", QVariant(1));
        query.exec();
    }

    void benchmarkDeletePhotos() {
        QSqlQuery query = prepare(QStringLiteral("DELETE FROM photos WHERE albumId = :id"));
        query.bindValue(":id", QVariant(1));
        query.exec();
    }

protected:
    bool read()
    {
        switch (currentTest) {
        case Insert:
            return checkInsert();
        case Update:
            return checkUpdate();
        case Delete:
            return checkDelete();
        case Clean:
            clean();
            return true;
        case BenchmarkInsertBatch:
            benchmarkInsertBatch();
            return true;
        case BenchmarkInsertNaive:
            benchmarkInsertNaive();
            return true;
        case BenchmarkPrepareDeletion:
            benchmarkPrepareDeletion();
            return true;
        case BenchmarkDeleteAlbum:
            benchmarkDeleteAlbum();
            return true;
        case BenchmarkDeletePhotos:
            benchmarkDeletePhotos();
            return true;
        default:
            return false;
        }
    }

    bool write()
    {
        switch (currentTest) {
        case Insert:
            return testInsert();
        case Update:
            return testUpdate();
        case Delete:
            return testDelete();
        case Clean:
            clean();
            return true;
        case BenchmarkInsertBatch:
            benchmarkInsertBatch();
            return true;
        case BenchmarkInsertNaive:
            benchmarkInsertNaive();
            return true;
        case BenchmarkPrepareDeletion:
            benchmarkPrepareDeletion();
            return true;
        case BenchmarkDeleteAlbum:
            benchmarkDeleteAlbum();
            return true;
        case BenchmarkDeletePhotos:
            benchmarkDeletePhotos();
            return true;
        default:
            return false;
        }
    }


    bool createTables(QSqlDatabase database) const
    {
        QSqlQuery query(database);
        query.prepare( "CREATE TABLE IF NOT EXISTS tests ("
                       "id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT,"
                       "value TEXT)");
        if (!query.exec()) {
            return false;
        }

        query.prepare( "CREATE TABLE IF NOT EXISTS albums ("
                       "id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT,"
                       "value TEXT)");
        if (!query.exec()) {
            return false;
        }

        query.prepare( "CREATE TABLE IF NOT EXISTS photos ("
                       "id INTEGER UNIQUE PRIMARY KEY AUTOINCREMENT,"
                       "albumId INTEGER,"
                       "value TEXT)");
        if (!query.exec()) {
            return false;
        }

        return true;
    }

    bool dropTables(QSqlDatabase database) const
    {
        QSqlQuery query(database);
        query.prepare("DROP TABLE IF EXISTS tests");
        if (!query.exec()) {
            return false;
        }

        query.prepare("DROP TABLE IF EXISTS albums");
        if (!query.exec()) {
            return false;
        }

        query.prepare("DROP TABLE IF EXISTS photos");
        if (!query.exec()) {
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

        db = new DummyDatabase;
    }

    void testCommits()
    {
        db->currentTest = DummyDatabase::Insert;
        db->executeWrite();
        db->wait();
        QCOMPARE(db->writeStatus(), AbstractSocialCacheDatabase::Finished);
        db->executeRead();
        db->wait();
        QCOMPARE(db->readStatus(), AbstractSocialCacheDatabase::Finished);

        db->currentTest = DummyDatabase::Update;
        db->executeWrite();
        db->wait();
        QCOMPARE(db->writeStatus(), AbstractSocialCacheDatabase::Finished);
        db->executeRead();
        db->wait();
        QCOMPARE(db->readStatus(), AbstractSocialCacheDatabase::Finished);

        db->currentTest = DummyDatabase::Delete;
        db->executeWrite();
        db->wait();
        QCOMPARE(db->writeStatus(), AbstractSocialCacheDatabase::Finished);
        db->executeRead();
        db->wait();
        QCOMPARE(db->readStatus(), AbstractSocialCacheDatabase::Finished);
    }

//private:

    void insertionBenchmarkBatch()
    {
        clean();
        // Beware this takes time
        QBENCHMARK(benchmarkInsertBatch());
    }

    void insertionBenchmarkNaive()
    {
        clean();
        // Beware this takes time
        QBENCHMARK(benchmarkInsertNaive());
    }

    void insertionBenchmarkNaiveTransaction()
    {
        clean();
        QBENCHMARK(benchmarkInsertNaiveWithTransaction());
    }

    void insertionBenchmarkBatchTransaction()
    {
        clean();
        QBENCHMARK(benchmarkInsertBatchWithTransaction());
    }

    void heavyInsertionBenchmark()
    {
        QBENCHMARK(benchmarkPrepareDeletion());
    }

    void primaryKeyDeletion()
    {
        benchmarkPrepareDeletion();
        QBENCHMARK_ONCE(benchmarkDeleteAlbum());
    }

    void nonPrimaryKeyDeletion()
    {
        benchmarkPrepareDeletion();
        QBENCHMARK_ONCE(benchmarkDeletePhotos());
    }

    void cleanupTestCase()
    {
        delete db;

        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();
    }

private:
    void clean()
    {
        db->currentTest = DummyDatabase::Clean;
        db->executeWrite();
        db->wait();
    }

    void benchmarkInsertBatch()
    {
        db->currentTest = DummyDatabase::BenchmarkInsertBatch;
        db->executeRead();
        db->wait();
    }

    void benchmarkInsertNaive()
    {
        db->currentTest = DummyDatabase::BenchmarkInsertNaive;
        db->executeRead();
        db->wait();
    }

    void benchmarkInsertBatchWithTransaction()
    {
        db->currentTest = DummyDatabase::BenchmarkInsertBatch;
        db->executeWrite();
        db->wait();
    }

    void benchmarkInsertNaiveWithTransaction()
    {
        db->currentTest = DummyDatabase::BenchmarkInsertNaive;
        db->executeWrite();
        db->wait();
    }

    void benchmarkPrepareDeletion()
    {
        db->currentTest = DummyDatabase::BenchmarkPrepareDeletion;
        db->executeWrite();
        db->wait();
    }

    void benchmarkDeleteAlbum()
    {
        db->currentTest = DummyDatabase::BenchmarkDeleteAlbum;
        db->executeWrite();
        db->wait();
    }

    void benchmarkDeletePhotos()
    {
        db->currentTest = DummyDatabase::BenchmarkDeletePhotos;
        db->executeWrite();
        db->wait();
    }

};

QTEST_MAIN(AbstractSocialCacheDatabaseTest)

#include "main.moc"

