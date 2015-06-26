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
#include "socialimagesdatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class SocialImageTest: public QObject
{
    Q_OBJECT

private:
    SocialImage::ConstPtr imageFromList(QList<SocialImage::ConstPtr> &images,
                                         const QString &imageUrl)
    {
        foreach (SocialImage::ConstPtr image, images) {
            if (image->imageUrl() == imageUrl) {
                return image;
            }
        }

        return SocialImage::ConstPtr();
    }

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

    void images()
    {
        const int account1 = 1;
        const int account2 = 2;

        QDateTime time1 (QDate(2013, 1, 2), QTime(12, 34, 56));
        QDateTime time2 (QDate(2015, 1, 2), QTime(12, 43, 56));

        SocialImagesDatabase database;

        // test insert images
        database.addImage(
                    account1,
                    QLatin1String("file:///t1.jpg"), QLatin1String("file:///1.jpg"),
                    time1);
        database.addImage(
                    account1,
                    QLatin1String("file:///t2.jpg"), QLatin1String("file:///2.jpg"),
                    time2);
        database.addImage(
                    account1,
                    QLatin1String("file:///t3.jpg"), QLatin1String("file:///3.jpg"),
                    time2);
        database.addImage(
                    account2,
                    QLatin1String("file:///t4.jpg"), QLatin1String("file:///4.jpg"),
                    time2);

        // check that the images are availbale from insert queue
        // already before commit
        SocialImage::ConstPtr image = database.image(QLatin1String("file:///t1.jpg"));
        QVERIFY(image != 0);
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account1);
        QCOMPARE(image->imageUrl(), QString("file:///t1.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///1.jpg"));
        QCOMPARE(image->createdTime(), time1);

        // then commit and test the rest
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        database.queryImages(account1);
        database.wait();
        QList<SocialImage::ConstPtr> images = database.images();
        QCOMPARE(images.count(), 3);

        image = imageFromList(images, "file:///t1.jpg");
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account1);
        QCOMPARE(image->imageUrl(), QString("file:///t1.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///1.jpg"));
        QCOMPARE(image->createdTime(), time1);

        image = imageFromList(images, "file:///t2.jpg");
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account1);
        QCOMPARE(image->imageUrl(), QString("file:///t2.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///2.jpg"));
        QCOMPARE(image->createdTime(), time2);

        image = imageFromList(images, "file:///t3.jpg");
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account1);
        QCOMPARE(image->imageUrl(), QString("file:///t3.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///3.jpg"));
        QCOMPARE(image->createdTime(), time2);

        database.queryImages(account2);
        database.wait();
        images = database.images();
        QCOMPARE(images.count(), 1);

        image = imageFromList(images, "file:///t4.jpg");
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account2);
        QCOMPARE(image->imageUrl(), QString("file:///t4.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///4.jpg"));
        QCOMPARE(image->createdTime(), time2);

        // test olderThan
        database.queryImages(account1, QDateTime(QDate(2014, 1, 2), QTime(12, 43, 56)));
        database.wait();
        images = database.images();
        QCOMPARE(images.count(), 1);

        image = imageFromList(images, "file:///t1.jpg");
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account1);
        QCOMPARE(image->imageUrl(), QString("file:///t1.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///1.jpg"));
        QCOMPARE(image->createdTime(), time1);

        // test remove image
        database.removeImage("file:///t1.jpg");
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        database.queryImages(account1);
        database.wait();
        images = database.images();
        QCOMPARE(images.count(), 2);
        image = imageFromList(images, "file:///t1.jpg");
        QVERIFY(image == 0);

        image = imageFromList(images, "file:///t2.jpg");
        QVERIFY(image != 0);
        QList<SocialImage::ConstPtr> removeImages;
        removeImages.append(image);
        database.removeImages(removeImages);
        database.commit();
        database.wait();

        database.queryImages(account1);
        database.wait();
        images = database.images();
        QCOMPARE(images.count(), 1);
        image = imageFromList(images, "file:///t2.jpg");
        QVERIFY(image == 0);

        // test purge accounts
        database.purgeAccount(1);
        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        database.queryImages(account1);
        database.wait();
        images = database.images();
        QCOMPARE(images.count(), 0);

        database.queryImages(account2);
        database.wait();
        images = database.images();
        QCOMPARE(images.count(), 1);

        image = imageFromList(images, "file:///t4.jpg");
        QVERIFY(image != 0);
        QCOMPARE(image->accountId(), account2);
        QCOMPARE(image->imageUrl(), QString("file:///t4.jpg"));
        QCOMPARE(image->imageFile(), QString("file:///4.jpg"));
        QCOMPARE(image->createdTime(), time2);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(SocialImageTest)

#include "main.moc"
