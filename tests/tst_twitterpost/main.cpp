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
#include "twitterpostsdatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class TwitterPostsTest: public QObject
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
        QDateTime time1(QDate(2013, 1, 2), QTime(12, 34, 56));
        QDateTime time2(QDate(2012, 3, 4), QTime(10, 11, 12));

        const QString id1 = QLatin1String("id1");
        const QString id2 = QLatin1String("id2");
        const QString id3 = QLatin1String("id3");

        const QString name1 = QLatin1String("name1");
        const QString name2 = QLatin1String("name2");
        const QString name3 = QLatin1String("name3");

        const QString body1 = QLatin1String("body1");
        const QString body2 = QLatin1String("body2");
        const QString body3 = QLatin1String("body3");

        const QString icon1 = QLatin1String("/icon.jpg");
        const QString image1 = QLatin1String("http://example.com/image1.jpg");
        const QString image2 = QLatin1String("http://example.com/image2.jpg");
        const QString image3 = QLatin1String("http://example.com/image3.jpg");
        const QString screen1 = QLatin1String("screen1");
        const QString retweeter1 = QLatin1String("retweeter1");
        const QString key1 = QLatin1String("key1");
        const QString secret1 = QLatin1String("secret1");

        TwitterPostsDatabase database;

        database.addTwitterPost(
                    id1, name1, body1, time1, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >()
                            << qMakePair(image1, SocialPostImage::Photo)
                            << qMakePair(image2, SocialPostImage::Video),
                    screen1, retweeter1, key1, secret1, 1);
        database.addTwitterPost(
                    id1, name1, body1, time1, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >()
                            << qMakePair(image1, SocialPostImage::Photo)
                            << qMakePair(image2, SocialPostImage::Video),
                    screen1, retweeter1, key1, secret1, 2);
        database.addTwitterPost(
                    id2, name2, body2, time2, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >()
                            << qMakePair(image3, SocialPostImage::Photo),
                    screen1, retweeter1, key1, secret1, 1);
        database.addTwitterPost(
                    id3, name3, body3, time2, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >(),
                    screen1, retweeter1, key1, secret1, 2);

        database.commit();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);

        database.refresh();
        database.wait();
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);

        QList<SocialPost::ConstPtr> posts;

        posts = database.posts();
        QCOMPARE(posts.count(), 3);

        SocialPost::ConstPtr post;
        do {
            post = posts.takeFirst();
        } while (post->identifier() != id1 && posts.count() > 0);

        QCOMPARE(post->identifier(), id1);
        QCOMPARE(post->name(), name1);
        QCOMPARE(post->body(), body1);
        QCOMPARE(post->timestamp(), time1);
        QCOMPARE(post->icon(), icon1);
        QCOMPARE(post->images().count(), 2);
        QCOMPARE(TwitterPostsDatabase::screenName(post), screen1);
        QCOMPARE(TwitterPostsDatabase::retweeter(post), retweeter1);
        QCOMPARE(TwitterPostsDatabase::consumerKey(post), key1);
        QCOMPARE(TwitterPostsDatabase::consumerSecret(post), secret1);
        QCOMPARE(post->accounts().count(), 2);

        database.removePosts(2);
        database.commit();
        database.refresh();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);

        posts = database.posts();
        QCOMPARE(posts.count(), 2);

        database.removePosts(1);
        database.commit();
        database.refresh();
        database.wait();
        QCOMPARE(database.writeStatus(), AbstractSocialCacheDatabase::Finished);
        QCOMPARE(database.readStatus(), AbstractSocialCacheDatabase::Finished);

        posts = database.posts();
        QCOMPARE(posts.count(), 0);
    }

    void cleanupTestCase()
    {
        // Do the same cleanups
        QDir dir (PRIVILEGED_DATA_DIR);
        dir.removeRecursively();

    }
};

QTEST_MAIN(TwitterPostsTest)

#include "main.moc"
