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
#include "facebookpostsdatabase.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

class FacebookPostsTest: public QObject
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
        const QString attachment1 = QLatin1String("attachment1");
        const QString caption1 = QLatin1String("caption1");
        const QString description1 = QLatin1String("description1");
        const QString url1 = QLatin1String("http://example.com/attachment.png");
        const QString client1 = QLatin1String("client1");

        FacebookPostsDatabase database;

        database.addFacebookPost(
                    id1, name1, body1, time1, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >()
                            << qMakePair(image1, SocialPostImage::Photo)
                            << qMakePair(image2, SocialPostImage::Video),
                    attachment1, caption1, description1, url1,
                    true, true, client1, 1);
        database.addFacebookPost(
                    id1, name1, body1, time1, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >()
                            << qMakePair(image1, SocialPostImage::Photo)
                            << qMakePair(image2, SocialPostImage::Video),
                    attachment1, caption1, description1, url1,
                    true, true, client1, 2);
        database.addFacebookPost(
                    id2, name2, body2, time2, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >()
                            << qMakePair(image3, SocialPostImage::Photo),
                    QString(), QString(), QString(), QString(),
                    false, false, client1, 1);
        database.addFacebookPost(
                    id3, name3, body3, time2, icon1,
                    QList<QPair<QString, SocialPostImage::ImageType> >(),
                    QString(), QString(), QString(), QString(),
                    true, true, client1, 2);

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
        QCOMPARE(FacebookPostsDatabase::attachmentName(post), attachment1);
        QCOMPARE(FacebookPostsDatabase::attachmentCaption(post), caption1);
        QCOMPARE(FacebookPostsDatabase::attachmentDescription(post), description1);
        QCOMPARE(FacebookPostsDatabase::attachmentUrl(post), url1);
        QCOMPARE(FacebookPostsDatabase::allowLike(post), true);
        QCOMPARE(FacebookPostsDatabase::allowComment(post), true);
        QCOMPARE(FacebookPostsDatabase::clientId(post), client1);
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

QTEST_MAIN(FacebookPostsTest)

#include "main.moc"
