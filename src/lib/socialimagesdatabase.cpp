/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Antti Seppälä <antti.seppala@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "socialimagesdatabase.h"
#include "abstractsocialcachedatabase.h"
#include "socialsyncinterface.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QFile>

#include <QtDebug>

static const char *DB_NAME = "socialimagecache.db";
static const int VERSION = 4;

struct SocialImagePrivate
{
    explicit SocialImagePrivate(int accountId,
                                const QString &imageUrl,
                                const QString &imageFile,
                                const QDateTime &createdTime,
                                const QDateTime &expires,
                                const QString &imageId);
    int accountId;
    QString imageUrl;
    QString imageFile;
    QDateTime createdTime;
    QDateTime expires;
    QString imageId;
};

SocialImagePrivate::SocialImagePrivate(int accountId,
                                       const QString &imageUrl,
                                       const QString &imageFile,
                                       const QDateTime &createdTime,
                                       const QDateTime &expires,
                                       const QString &imageId)
    : accountId(accountId)
    , imageUrl(imageUrl)
    , imageFile(imageFile)
    , createdTime(createdTime)
    , expires(expires)
    , imageId(imageId)
{
}

SocialImage::SocialImage(int accountId,
                         const QString &imageUrl,
                         const QString &imageFile,
                         const QDateTime &createdTime,
                         const QDateTime &expires,
                         const QString &imageId)
    : d_ptr(new SocialImagePrivate(accountId, imageUrl,
                                   imageFile, createdTime,
                                   expires, imageId))
{
}

SocialImage::~SocialImage()
{
}

SocialImage::Ptr SocialImage::create(int accountId,
                                     const QString & imageUrl,
                                     const QString & imageFile,
                                     const QDateTime &createdTime,
                                     const QDateTime &expires,
                                     const QString &imageId)
{
    return SocialImage::Ptr(new SocialImage(accountId, imageUrl,
                                            imageFile, createdTime,
                                            expires, imageId));
}

int SocialImage::accountId() const
{
    Q_D(const SocialImage);
    return d->accountId;
}

QString SocialImage::imageUrl() const
{
    Q_D(const SocialImage);
    return d->imageUrl;
}

QString SocialImage::imageFile() const
{
    Q_D(const SocialImage);
    return d->imageFile;
}

QDateTime SocialImage::createdTime() const
{
    Q_D(const SocialImage);
    return d->createdTime;
}

QDateTime SocialImage::expires() const
{
    Q_D(const SocialImage);
    return d->expires;
}

QString SocialImage::imageId() const
{
    Q_D(const SocialImage);
    return d->imageId;
}

class SocialImagesDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    explicit SocialImagesDatabasePrivate(SocialImagesDatabase *q);
    ~SocialImagesDatabasePrivate();

private:
    Q_DECLARE_PUBLIC(SocialImagesDatabase)

    static void clearCachedImages(QSqlQuery &query);

    QList<SocialImage::ConstPtr> queryImages(int accountId,
                                             const QDateTime &olderThan);
    QList<SocialImage::ConstPtr> queryExpired(int accountId);

    struct {
        QList<int> purgeAccounts;
        QStringList removeImages;
        QMap<QString, SocialImage::ConstPtr> insertImages;
    } queue;

    struct {
        bool queryExpired;
        int accountId;
        QDateTime olderThan;
        QList<SocialImage::ConstPtr> images;
    } query;

    struct {
        QList<SocialImage::ConstPtr> images;
    } result;

    QSqlQuery m_singleImageQuery;
};

SocialImagesDatabasePrivate::SocialImagesDatabasePrivate(SocialImagesDatabase *q)
    : AbstractSocialCacheDatabasePrivate(
            q,
            SocialSyncInterface::socialNetwork(SocialSyncInterface::Facebook),
            SocialSyncInterface::dataType(SocialSyncInterface::Images),
            QLatin1String(DB_NAME),
            VERSION)
{
}

SocialImagesDatabasePrivate::~SocialImagesDatabasePrivate()
{
}

void SocialImagesDatabasePrivate::clearCachedImages(QSqlQuery &query)
{
    while (query.next()) {
        QString image = query.value(0).toString();
        if (!image.isEmpty()) {
            QFile imageFile(image);
            if (imageFile.exists()) {
                imageFile.remove();
            }
        }
    }
}

QList<SocialImage::ConstPtr> SocialImagesDatabasePrivate::queryImages(int accountId,
                                                                      const QDateTime &olderThan)
{
    Q_Q(SocialImagesDatabase);

    QList<SocialImage::ConstPtr> data;

    QString queryString = QLatin1String("SELECT accountId, "
                                        "imageUrl, imageFile, createdTime, expires, imageId "
                                        "FROM images "
                                        "WHERE accountId = :accountId");

    if (olderThan.isValid()) {
        queryString.append(" AND createdTime < :createdTime");
    }

    QSqlQuery query = q->prepare(queryString);
    query.bindValue(":accountId", accountId);

    if (olderThan.isValid()) {
        query.bindValue(":createdTime", olderThan.toTime_t());
    }

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query images:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(SocialImage::create(query.value(0).toInt(),                             // accountId
                                        query.value(1).toString(),                          // imageUrl
                                        query.value(2).toString(),                          // imageFile
                                        QDateTime::fromTime_t(query.value(3).toUInt()),     // createdTime
                                        QDateTime::fromTime_t(query.value(4).toUInt()),     // expires
                                        query.value(5).toString()));                        // imageId
    }

    return data;
}

QList<SocialImage::ConstPtr> SocialImagesDatabasePrivate::queryExpired(int accountId)
{
    Q_Q(SocialImagesDatabase);

    QList<SocialImage::ConstPtr> data;

    int currentTime = QDateTime::currentDateTime().toTime_t();

    QString queryString = QLatin1String("SELECT accountId, "
                                        "imageUrl, imageFile, createdTime, expires, imageId "
                                        "FROM images "
                                        "WHERE accountId = :accountId AND expires < :currentTime");

    QSqlQuery query = q->prepare(queryString);
    query.bindValue(":accountId", accountId);
    query.bindValue(":currentTime", currentTime);

    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to query images:" << query.lastError().text();
        return data;
    }

    while (query.next()) {
        data.append(SocialImage::create(query.value(0).toInt(),                             // accountId
                                        query.value(1).toString(),                          // imageUrl
                                        query.value(2).toString(),                          // imageFile
                                        QDateTime::fromTime_t(query.value(3).toUInt()),     // createdTime
                                        QDateTime::fromTime_t(query.value(4).toUInt()),     // expires
                                        query.value(5).toString()));                        // imageId
    }

    return data;
}

bool operator==(const SocialImage::ConstPtr &image1,
                const SocialImage::ConstPtr &image2)
{
    return image1->accountId() == image2->accountId()
            && image1->imageUrl() == image2->imageUrl();
}

SocialImagesDatabase::SocialImagesDatabase()
    : AbstractSocialCacheDatabase(*(new SocialImagesDatabasePrivate(this)))
{
}

SocialImagesDatabase::~SocialImagesDatabase()
{
    wait();
}

void SocialImagesDatabase::purgeAccount(int accountId)
{
    Q_D(SocialImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.purgeAccounts.append(accountId);
}

SocialImage::ConstPtr SocialImagesDatabase::image(const QString &imageUrl) const
{
    Q_D(const SocialImagesDatabase);

    if (d->queue.insertImages.contains(imageUrl)) {
        return d->queue.insertImages.value(imageUrl);
    }

    QSqlQuery query = prepare(
                "SELECT accountId, "
                "imageUrl, imageFile, createdTime, expires, imageId "
                "FROM images WHERE imageUrl = :imageUrl");
    query.bindValue(":imageUrl", imageUrl);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from images table:" << query.lastError();
        return SocialImage::Ptr();
    }

    // If we have the image, we have a result, otherwise we won't have the image
    if (!query.next()) {
        return SocialImage::Ptr();
    }

    return SocialImage::create(query.value(0).toInt(),                           // accountId
                               query.value(1).toString(),                        // imageUrl
                               query.value(2).toString(),                        // imageFile
                               QDateTime::fromTime_t(query.value(3).toUInt()),   // createdTime
                               QDateTime::fromTime_t(query.value(4).toUInt()),   // expires
                               query.value(5).toString());                       // imageId
}

SocialImage::ConstPtr SocialImagesDatabase::imageById(const QString &imageId) const
{
    Q_D(const SocialImagesDatabase);

    QMap<QString, SocialImage::ConstPtr>::const_iterator i = d->queue.insertImages.constBegin();
    while (i != d->queue.insertImages.constEnd()) {
        if (i.value()->imageId() == imageId) {
            return i.value();
        }
        ++i;
    }

    QSqlQuery query = prepare(
                "SELECT accountId, "
                "imageUrl, imageFile, createdTime, expires, imageId "
                "FROM images WHERE imageId = :imageId");
    query.bindValue(":imageId", imageId);
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from images table:" << query.lastError();
        return SocialImage::Ptr();
    }

    // If we have the image, we have a result, otherwise we won't have the image
    if (!query.next()) {
        return SocialImage::Ptr();
    }

    return SocialImage::create(query.value(0).toInt(),                           // accountId
                               query.value(1).toString(),                        // imageUrl
                               query.value(2).toString(),                        // imageFile
                               QDateTime::fromTime_t(query.value(3).toUInt()),   // createdTime
                               QDateTime::fromTime_t(query.value(4).toUInt()),   // expires
                               query.value(5).toString());                       // imageId
}

void SocialImagesDatabase::removeImage(const QString &imageUrl)
{
    Q_D(SocialImagesDatabase);
    QMutexLocker locker(&d->mutex);

    d->queue.insertImages.remove(imageUrl);
    d->queue.removeImages.append(imageUrl);
}

void SocialImagesDatabase::removeImages(QList<SocialImage::ConstPtr> images)
{
    Q_D(SocialImagesDatabase);
    QMutexLocker locker(&d->mutex);

    foreach (SocialImage::ConstPtr image, images) {
        d->queue.insertImages.remove(image->imageUrl());
        d->queue.removeImages.append(image->imageUrl());
    }
}

void SocialImagesDatabase::addImage(int accountId,
                                    const QString &imageUrl,
                                    const QString &imageFile,
                                    const QDateTime &createdTime,
                                    const QDateTime &expires,
                                    const QString &imageId)
{
    Q_D(SocialImagesDatabase);
    SocialImage::Ptr image = SocialImage::create(accountId, imageUrl, imageFile,
                                                 createdTime, expires, imageId);
    QMutexLocker locker(&d->mutex);

    d->queue.removeImages.removeAll(imageUrl);
    d->queue.insertImages.insert(imageUrl, image);
}

void SocialImagesDatabase::commit()
{
    executeWrite();
}

QList<SocialImage::ConstPtr> SocialImagesDatabase::images() const
{
    return d_func()->result.images;
}

void SocialImagesDatabase::queryImages(int accountId,
                                       const QDateTime &olderThan)
{
    Q_D(SocialImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.queryExpired = false;
        d->query.accountId = accountId;
        d->query.olderThan = olderThan;
    }
    executeRead();
}

void SocialImagesDatabase::queryExpired(int accountId)
{
    Q_D(SocialImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);
        d->query.accountId = accountId;
        d->query.queryExpired = true;
    }
    executeRead();
}

bool SocialImagesDatabase::read()
{
    Q_D(SocialImagesDatabase);
    QMutexLocker locker(&d->mutex);

    if (d->query.queryExpired) {
        d->query.images = d->queryExpired(d->query.accountId);
    } else {
        d->query.images = d->queryImages(d->query.accountId, d->query.olderThan);
    }
    return true;
}

void SocialImagesDatabase::readFinished()
{
    Q_D(SocialImagesDatabase);
    {
        QMutexLocker locker(&d->mutex);

        d->result.images = d->query.images;
        d->query.images.clear();
    }
    emit queryFinished();
}

bool SocialImagesDatabase::write()
{
    Q_D(SocialImagesDatabase);
    QMutexLocker locker(&d->mutex);

    qWarning() << "Queued images being saved:" << d->queue.insertImages.count();

    const QList<int> purgeAccounts = d->queue.purgeAccounts;
    const QStringList removeImages = d->queue.removeImages;

    QList<SocialImage::ConstPtr> insertImages;
    QMap<QString, SocialImage::ConstPtr>::const_iterator i = d->queue.insertImages.constBegin();
    while (i != d->queue.insertImages.constEnd()) {
        insertImages.append(i.value());
        ++i;
    }

    d->queue.purgeAccounts.clear();
    d->queue.removeImages.clear();
    d->queue.insertImages.clear();

    locker.unlock();

    bool success = true;
    QSqlQuery query;

    if (!purgeAccounts.isEmpty()) {
        QVariantList accountIds;
        Q_FOREACH (int accountId, purgeAccounts) {
            accountIds.append(accountId);
        }

        query = prepare(QStringLiteral(
                    "SELECT imageFile FROM images "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        if (!query.exec()) {
            qWarning() << Q_FUNC_INFO << "Failed to exec images selection query:"
                       << query.lastError().text();
            success = false;
        } else {
            d->clearCachedImages(query);
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE accountId = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removeImages.isEmpty()) {
        QVariantList imageUrls;
        foreach (const QString image, removeImages) {
            imageUrls.append(image);
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE imageUrl = :imageUrl"));
        query.bindValue(QStringLiteral(":imageUrl"), imageUrls);
        executeBatchSocialCacheQuery(query);
    }

    if (!insertImages.isEmpty()) {
        QVariantList accountIds;
        QVariantList imageUrls, imageFiles;
        QVariantList createdTimes, expireTimes;
        QVariantList imageIds;

        Q_FOREACH (const SocialImage::ConstPtr &image, insertImages) {
            accountIds.append(image->accountId());
            imageUrls.append(image->imageUrl());
            imageFiles.append(image->imageFile());
            createdTimes.append(image->createdTime().toTime_t());
            expireTimes.append(image->expires().toTime_t());
            imageIds.append(image->imageId());
        }

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO images ("
                    " accountId, imageUrl, imageFile, createdTime, expires, imageId) "
                    "VALUES ("
                    " :accountId, :imageUrl, :imageFile, :createdTime, :expires, :imageId)"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        query.bindValue(QStringLiteral(":imageUrl"), imageUrls);
        query.bindValue(QStringLiteral(":imageFile"), imageFiles);
        query.bindValue(QStringLiteral(":createdTime"), createdTimes);
        query.bindValue(QStringLiteral(":expires"), expireTimes);
        query.bindValue(QStringLiteral(":imageId"), imageIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool SocialImagesDatabase::createTables(QSqlDatabase database) const
{
    // create the db table
    QSqlQuery query(database);
    query.prepare("CREATE TABLE IF NOT EXISTS images ("
                  "accountId INTEGER,"
                  "imageUrl TEXT,"
                  "imageFile TEXT,"
                  "createdTime INTEGER,"
                  "expires INTEGER,"
                  "imageId STRING)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool SocialImagesDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare("DROP TABLE IF EXISTS images");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Failed to delete images table:" << query.lastError().text();
        return false;
    }

    return true;
}
