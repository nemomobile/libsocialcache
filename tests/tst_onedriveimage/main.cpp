/*
 * Copyright (C) 2015 Jolla Ltd. <antti.seppala@jollamobile.com>
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
#include "onedriveimagesdatabase.h"
#include "onedriveimagecachemodel.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class OneDriveImageTest: public QObject
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

    void testAddUser()
    {
        OneDriveImagesDatabase database;

        QDateTime time1 (QDate(2013, 1, 2), QTime(12, 34, 56));

        database.addUser("a", time1, "c", 1);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        QString dataType = SocialSyncInterface::dataType(SocialSyncInterface::Images);
        QString dbFile = QLatin1String("onedrive.db"); // DB_NAME in onedriveimagesdatabase.cpp

        QSqlDatabase checkDb = QSqlDatabase::addDatabase("QSQLITE");
        checkDb.setDatabaseName(QString("%1/%2/%3").arg(PRIVILEGED_DATA_DIR, dataType, dbFile));
        QVERIFY(checkDb.open());

        // Check if the user has been written
        QSqlQuery query (checkDb);
        query.prepare("SELECT userId, updatedTime, userName, accountId "\
                      "FROM users");
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toString(), QLatin1String("a"));
        QCOMPARE(query.value(1).toUInt(), time1.toTime_t());
        QCOMPARE(query.value(2).toString(), QLatin1String("c"));
        QCOMPARE(query.value(3).toInt(), 1);
        QVERIFY(!query.next());

        // Check if we can retrieve the user
        OneDriveUser::ConstPtr user = database.user(QLatin1String("a"));
        QCOMPARE(user->userId(), QLatin1String("a"));
        QCOMPARE(user->updatedTime(), time1);
        QCOMPARE(user->userName(), QLatin1String("c"));
        QCOMPARE(user->accountId(), 1);
        QCOMPARE(user->count(), -1);

        QDateTime time2 (QDate(2012, 3, 4), QTime(10, 11, 12));

        // Let's write another user
        database.addUser("d", time2, "f", 5);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        // Check if we can retrieve everybody
        database.queryUsers();
        database.wait();
        QList<OneDriveUser::ConstPtr> users = database.users();
        QCOMPARE(users.count(), 2);
        QCOMPARE(users[0]->userId(), QLatin1String("a"));
        QCOMPARE(users[0]->updatedTime(), time1);
        QCOMPARE(users[0]->userName(), QLatin1String("c"));
        QCOMPARE(users[0]->accountId(), 1);
        QCOMPARE(users[0]->count(), 0);

        QCOMPARE(users[1]->userId(), QLatin1String("d"));
        QCOMPARE(users[1]->updatedTime(), time2);
        QCOMPARE(users[1]->userName(), QLatin1String("f"));
        QCOMPARE(users[1]->accountId(), 5);
        QCOMPARE(users[1]->count(), 0);

        OneDriveImageCacheModel model;
        model.setType(OneDriveImageCacheModel::Users);
        model.refresh();
        QCOMPARE(model.count(), 0);
        QTRY_COMPARE(model.count(), 3);

        database.removeUser(QLatin1String("a"));
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        model.refresh();
        QTRY_COMPARE(model.count(), 1);

        database.removeUser(QLatin1String("d"));
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        model.refresh();
        QTRY_COMPARE(model.count(), 0);
    }

    void albums()
    {
        QDateTime time1(QDate(2013, 1, 2), QTime(12, 34, 56));
        QDateTime time2(QDate(2012, 3, 4), QTime(10, 11, 12));

        const QString user1 = QLatin1String("user1");
        const QString user2 = QLatin1String("user2");
        const QString album1 = QLatin1String("album1");
        const QString album2 = QLatin1String("album2");
        const QString album3 = QLatin1String("album3");

        OneDriveImagesDatabase database;

        database.addUser(user1, time1, QLatin1String("joe"), 2);
        database.addUser(user2, time2, QLatin1String("dave"), 3);

        database.addAlbum(album1, user1, time1, time2, QLatin1String("holidays"), 3);
        database.addAlbum(album2, user1, time2, time1, QLatin1String("work"), 2);
        database.addAlbum(album3, user2, time1, time2, QLatin1String("holidays"), 4);

        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        bool ok = false;
        QStringList albumIds = database.allAlbumIds(&ok);
        QCOMPARE(ok, true);
        QCOMPARE(albumIds.contains(album1), true);
        QCOMPARE(albumIds.contains(album2), true);
        QCOMPARE(albumIds.contains(album3), true);

        OneDriveAlbum::ConstPtr album;

        album = database.album(album1);
        QCOMPARE(album->albumId(), album1);
        QCOMPARE(album->userId(), user1);
        QCOMPARE(album->createdTime(), time1);
        QCOMPARE(album->updatedTime(), time2);
        QCOMPARE(album->albumName(), QLatin1String("holidays"));
        QCOMPARE(album->imageCount(), 3);

        album = database.album(album2);
        QCOMPARE(album->albumId(), album2);
        QCOMPARE(album->userId(), user1);
        QCOMPARE(album->createdTime(), time2);
        QCOMPARE(album->updatedTime(), time1);
        QCOMPARE(album->albumName(), QLatin1String("work"));
        QCOMPARE(album->imageCount(), 2);

        album = database.album(album3);
        QCOMPARE(album->albumId(), album3);
        QCOMPARE(album->userId(), user2);
        QCOMPARE(album->createdTime(), time1);
        QCOMPARE(album->updatedTime(), time2);
        QCOMPARE(album->albumName(), QLatin1String("holidays"));
        QCOMPARE(album->imageCount(), 4);

        QList<OneDriveAlbum::ConstPtr> albums;

        database.queryAlbums();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 3);

        database.queryAlbums(user1);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 2);

        database.queryAlbums(user2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 1);

        database.removeAlbum(album2);
        database.commit();

        database.queryAlbums();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 2);

        database.queryAlbums(user1);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 1);

        database.queryAlbums(user2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 1);

        database.removeUser(user2);
        database.commit();

        database.queryAlbums();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 1);

        database.queryAlbums(user1);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 1);

        database.queryAlbums(user2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        albums = database.albums();
        QCOMPARE(albums.count(), 0);
    }

    void images()
    {
        QDateTime time1(QDate(2013, 1, 2), QTime(12, 34, 56));
        QDateTime time2(QDate(2012, 3, 4), QTime(10, 11, 12));

        const QString user1 = QLatin1String("user1");
        const QString user2 = QLatin1String("user2");
        const QString album1 = QLatin1String("album1");
        const QString album2 = QLatin1String("album2");
        const QString album3 = QLatin1String("album3");
        const QString image1 = QLatin1String("image1");
        const QString image2 = QLatin1String("image2");
        const QString image3 = QLatin1String("image3");
        const QString image4 = QLatin1String("image4");
        const QString image5 = QLatin1String("image5");
        const QString image6 = QLatin1String("image6");
        const QString image7 = QLatin1String("image7");
        const QString image8 = QLatin1String("image8");
        const QString image9 = QLatin1String("image9");

        OneDriveImagesDatabase database;

        database.addUser(user1, time1, QLatin1String("joe"), 3);
        database.addUser(user2, time2, QLatin1String("dave"), 4);

        database.syncAccount(1, user1);
        database.syncAccount(2, user2);

        database.addAlbum(album1, user1, time1, time2, QLatin1String("holidays"), 3);
        database.addAlbum(album2, user1, time2, time1, QLatin1String("work"), 2);
        database.addAlbum(album3, user2, time1, time2, QLatin1String("holidays"), 4);

        database.addImage(
                    image1, album1, user1,
                    time1, time2,
                    QLatin1String("1"),
                    640, 480,
                    QLatin1String("file:///t1.jpg"), QLatin1String("file:///1.jpg"), "desc1", 101);
        database.addImage(
                    image2, album1, user1,
                    time1, time1,
                    QLatin1String("2"),
                    480, 640,
                    QLatin1String("file:///t2.jpg"), QLatin1String("file:///2.jpg"), "desc2", 102);
        database.addImage(
                    image3, album1, user1,
                    time2, time1,
                    QLatin1String("3"),
                    640, 480,
                    QLatin1String("file:///t3.jpg"), QLatin1String("file:///3.jpg"), "desc2", 103);
        database.addImage(
                    image4, album2, user1,
                    time1, time1,
                    QLatin1String("4"),
                    640, 480,
                    QLatin1String("file:///t4.jpg"), QLatin1String("file:///4.jpg"), "desc4", 104);
        database.addImage(
                    image5, album2, user1,
                    time1, time2,
                    QLatin1String("5"),
                    480, 640,
                    QLatin1String("file:///t5.jpg"), QLatin1String("file:///5.jpg"), "desc5", 105);

        database.addImage(
                    image6, album3, user2,
                    time1, time2,
                    QLatin1String("6"),
                    640, 480,
                    QLatin1String("file:///t6.jpg"), QLatin1String("file:///6.jpg"), "desc6", 106);

        database.addImage(
                    image7, album3, user2,
                    time1, time2,
                    QLatin1String("7"),
                    640, 480,
                    QLatin1String("file:///t7.jpg"), QLatin1String("file:///7.jpg"), "desc7", 107);
        database.addImage(
                    image8, album3, user2,
                    time2, time2,
                    QLatin1String("8"),
                    640, 480,
                    QLatin1String("file:///t8.jpg"), QLatin1String("file:///8.jpg"), "desc8", 108);
        database.addImage(
                    image9, album3, user2,
                    time1, time2,
                    QLatin1String("9"),
                    640, 480,
                    QLatin1String("file:///t9.jpg"), QLatin1String("file:///9.jpg"), "desc9", 109);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        bool ok = false;
        QStringList imageIds;
        imageIds = database.allImageIds(&ok);
        QCOMPARE(ok, true);
        QCOMPARE(imageIds.contains(image1), true);
        QCOMPARE(imageIds.contains(image2), true);
        QCOMPARE(imageIds.contains(image3), true);
        QCOMPARE(imageIds.contains(image4), true);
        QCOMPARE(imageIds.contains(image5), true);
        QCOMPARE(imageIds.contains(image6), true);
        QCOMPARE(imageIds.contains(image7), true);
        QCOMPARE(imageIds.contains(image8), true);
        QCOMPARE(imageIds.contains(image9), true);

        imageIds = database.imageIds(album1, &ok);
        QCOMPARE(ok, true);
        QCOMPARE(imageIds.contains(image1), true);
        QCOMPARE(imageIds.contains(image2), true);
        QCOMPARE(imageIds.contains(image3), true);

        imageIds = database.imageIds(album2, &ok);
        QCOMPARE(ok, true);
        QCOMPARE(imageIds.contains(image4), true);
        QCOMPARE(imageIds.contains(image5), true);

        imageIds = database.imageIds(album3, &ok);
        QCOMPARE(ok, true);
        QCOMPARE(imageIds.contains(image6), true);
        QCOMPARE(imageIds.contains(image7), true);
        QCOMPARE(imageIds.contains(image8), true);
        QCOMPARE(imageIds.contains(image9), true);

        OneDriveImage::ConstPtr image;

        image = database.image(image1);
        QCOMPARE(image->imageId(), image1);
        QCOMPARE(image->albumId(), album1);
        QCOMPARE(image->userId(), user1);
        QCOMPARE(image->createdTime(), time1);
        QCOMPARE(image->updatedTime(), time2);
        QCOMPARE(image->imageName(), QLatin1String("1"));
        QCOMPARE(image->width(), 640);
        QCOMPARE(image->height(), 480);
        QCOMPARE(image->thumbnailUrl(), QLatin1String("file:///t1.jpg"));
        QCOMPARE(image->imageUrl(), QLatin1String("file:///1.jpg"));
        QCOMPARE(image->imageFile(), QString());
        QCOMPARE(image->thumbnailFile(), QString());
        QCOMPARE(image->description(), QLatin1String("desc1"));
        QCOMPARE(image->accountId(), 101);

        database.updateImageThumbnail(image1, QLatin1String("/t1.jpg"));
        database.updateImageFile(image1, QLatin1String("/1.jpg"));
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        image = database.image(image1);
        QCOMPARE(image->thumbnailFile(), QLatin1String("/t1.jpg"));
        QCOMPARE(image->imageFile(), QLatin1String("/1.jpg"));

        QList<OneDriveImage::ConstPtr> images;

        database.queryUserImages();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 9);

        database.queryUserImages(user1);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 5);

        database.queryUserImages(user2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 4);

        database.queryAlbumImages(album1);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 3);

        database.queryAlbumImages(album2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 2);

        database.queryAlbumImages(album3);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 4);

        database.removeImage(image7);
        database.commit();

        database.queryUserImages();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 8);

        database.queryUserImages(user2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 3);

        database.queryAlbumImages(album3);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 3);

        database.removeAlbum(album2);
        database.commit();

        database.queryUserImages();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 6);

        database.queryUserImages(user1);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 3);

        database.queryAlbumImages(album2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 0);

        ok = false;
        QMap<int, QString> accounts = database.accounts(&ok);
        QCOMPARE(ok, true);
        QCOMPARE(accounts.count(), 2);
        QCOMPARE(accounts.value(1), user1);
        QCOMPARE(accounts.value(2), user2);

        database.removeUser(user2);
        database.commit();

        database.queryUserImages();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 3);

        database.queryUserImages(user2);
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 0);

        database.purgeAccount(1);
        database.purgeAccount(2);
        database.commit();

        database.queryUserImages();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);
        images = database.images();
        QCOMPARE(images.count(), 0);
    }


    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(OneDriveImageTest)

#include "main.moc"
