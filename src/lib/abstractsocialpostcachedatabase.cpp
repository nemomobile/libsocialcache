/*
 * Copyright (C) 2013 Lucien Xu <sfietkonstantin@free.fr>
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

#include "abstractsocialpostcachedatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include "socialsyncinterface.h"
#include <QtCore/QDebug>
#include <QtCore/QRunnable>
#include <QtCore/QStringList>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static const char *INVALID = "invalid";
static const char *PHOTO = "photo";
static const char *VIDEO = "video";

static const int POST_DB_VERSION = 1;

struct SocialPostImagePrivate
{
    explicit SocialPostImagePrivate(const QString &url, SocialPostImage::ImageType type);
    QString url;
    SocialPostImage::ImageType type;
};

SocialPostImagePrivate::SocialPostImagePrivate(const QString &url, SocialPostImage::ImageType type)
    : url(url), type(type)
{
}

SocialPostImage::SocialPostImage()
{
}

SocialPostImage::SocialPostImage(const QString &url, ImageType type)
    : d_ptr(new SocialPostImagePrivate(url, type))
{
}

SocialPostImage::~SocialPostImage()
{
}

SocialPostImage::Ptr SocialPostImage::create(const QString &url, ImageType type)
{
    return SocialPostImage::Ptr(new SocialPostImage(url, type));
}

QString SocialPostImage::url() const
{
    Q_D(const SocialPostImage);
    return d->url;
}

SocialPostImage::ImageType SocialPostImage::type() const
{
    Q_D(const SocialPostImage);
    return d->type;
}

struct SocialPostPrivate
{
    explicit SocialPostPrivate(const QString &identifier, const QString &name,
                               const QString &body, const QDateTime &timestamp,
                               const QVariantMap &extra = QVariantMap(),
                               const QList<int> &accounts = QList<int>());
    QString identifier;
    QString name;
    QString body;
    QDateTime timestamp;
    QMap<int, SocialPostImage::ConstPtr> images;
    QVariantMap extra;
    QList<int> accounts;
};

SocialPostPrivate::SocialPostPrivate(const QString &identifier, const QString &name,
                                     const QString &body, const QDateTime &timestamp,
                                     const QVariantMap &extra, const QList<int> &accounts)
    : identifier(identifier), name(name), body(body), timestamp(timestamp)
    , extra(extra), accounts(accounts)
{
}

SocialPost::SocialPost(const QString &identifier, const QString &name, const QString &body,
                       const QDateTime &timestamp,
                       const QMap<int, SocialPostImage::ConstPtr> &images, const QVariantMap &extra,
                       const QList<int> &accounts)
    : d_ptr(new SocialPostPrivate(identifier, body, name, timestamp ,extra,
                                   accounts))
{
    setImages(images);
}

SocialPost::~SocialPost()
{
}

SocialPost::Ptr SocialPost::create(const QString &identifier, const QString &name,
                                   const QString &body, const QDateTime &timestamp,
                                   const QMap<int, SocialPostImage::ConstPtr> &images,
                                   const QVariantMap &extra, const QList<int> &accounts)
{
    return SocialPost::Ptr(new SocialPost(identifier, name, body, timestamp, images, extra,
                                          accounts));
}

QString SocialPost::identifier() const
{
    Q_D(const SocialPost);
    return d->identifier;
}

QString SocialPost::name() const
{
    Q_D(const SocialPost);
    return d->name;
}

QString SocialPost::body() const
{
    Q_D(const SocialPost);
    return d->body;
}

QDateTime SocialPost::timestamp() const
{
    Q_D(const SocialPost);
    return d->timestamp;
}

QString SocialPost::icon() const
{
    Q_D(const SocialPost);
    if (d->images.isEmpty()) {
        return QString();
    }

    return d->images.value(0)->url();
}

QList<SocialPostImage::ConstPtr> SocialPost::images() const
{
    Q_D(const SocialPost);
    QList<SocialPostImage::ConstPtr> images;
    Q_FOREACH (int key, d->images.keys()) {
        if (key > 0) {
            images.append(d->images.value(key));
        }
    }

    return images;
}

QMap<int, SocialPostImage::ConstPtr> SocialPost::allImages() const
{
    Q_D(const SocialPost);
    return d->images;
}

void SocialPost::setImages(const QMap<int, SocialPostImage::ConstPtr> &images)
{
    Q_D(SocialPost);
    d->images = images;
}

QVariantMap SocialPost::extra() const
{
    Q_D(const SocialPost);
    return d->extra;
}

void SocialPost::setExtra(const QVariantMap &extra)
{
    Q_D(SocialPost);
    d->extra = extra;
}

QList<int> SocialPost::accounts() const
{
    Q_D(const SocialPost);
    return d->accounts;
}

void SocialPost::setAccounts(const QList<int> &accounts)
{
    Q_D(SocialPost);
    d->accounts = accounts;
}

class AbstractSocialPostCacheDatabasePrivate: public AbstractSocialCacheDatabasePrivate
{
public:
    AbstractSocialPostCacheDatabasePrivate(
            AbstractSocialPostCacheDatabase *q, const QString &serviceName, const QString &databaseFile);
private:
    struct {
        QMap<QString, SocialPost::ConstPtr> insertPosts;
        QMultiMap<QString, int> mapPostsToAccounts;
        QList<int> removePostsForAccount;
        QList<QString> removePosts;
        bool removeAll;
    } queue;

    QList<SocialPost::ConstPtr> asyncPosts;
    QList<SocialPost::ConstPtr> posts;
    QVariantList accountIdFilter;

    Q_DECLARE_PUBLIC(AbstractSocialPostCacheDatabase)
};

AbstractSocialPostCacheDatabasePrivate::AbstractSocialPostCacheDatabasePrivate(
        AbstractSocialPostCacheDatabase *q, const QString &serviceName, const QString &databaseFile)
    : AbstractSocialCacheDatabasePrivate(
            q,
            serviceName,
            SocialSyncInterface::dataType(SocialSyncInterface::Posts),
            databaseFile,
            POST_DB_VERSION)
{
    queue.removeAll = false;
}

AbstractSocialPostCacheDatabase::~AbstractSocialPostCacheDatabase()
{
    cancelRead();
    wait();
}

AbstractSocialPostCacheDatabase::AbstractSocialPostCacheDatabase(
        const QString &serviceName, const QString &databaseFile)
    : AbstractSocialCacheDatabase(
            *(new AbstractSocialPostCacheDatabasePrivate(this, serviceName, databaseFile)))
{
}

QVariantList AbstractSocialPostCacheDatabase::accountIdFilter() const
{
    return d_func()->accountIdFilter;
}

void AbstractSocialPostCacheDatabase::setAccountIdFilter(const QVariantList &accountIds)
{
    if (accountIds != d_func()->accountIdFilter) {
        d_func()->accountIdFilter = accountIds;
        emit accountIdFilterChanged();
    }
}

QList<SocialPost::ConstPtr> AbstractSocialPostCacheDatabase::posts() const
{
    return d_func()->posts;
}

void AbstractSocialPostCacheDatabase::addPost(const QString &identifier, const QString &name,
                                              const QString &body, const QDateTime &timestamp,
                                              const QString &icon,
                                              const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                                              const QVariantMap &extra, int account)
{
    Q_D(AbstractSocialPostCacheDatabase);
    QMutexLocker locker(&d->mutex);
    QMap<int, SocialPostImage::ConstPtr> formattedImages;
    if (!icon.isEmpty()) {
        formattedImages.insert(0, SocialPostImage::create(icon, SocialPostImage::Photo));
    }

    for (int i = 0; i < images.count(); i++) {
        const QPair<QString, SocialPostImage::ImageType> &imagePair = images.at(i);
        formattedImages.insert(i + 1, SocialPostImage::create(imagePair.first, imagePair.second));
    }

    d->queue.insertPosts.insert(identifier, SocialPost::create(identifier, name, body, timestamp,
                                                         formattedImages, extra));
    d->queue.mapPostsToAccounts.insert(identifier, account);
}

void AbstractSocialPostCacheDatabase::removePosts(int accountId)
{
    Q_D(AbstractSocialPostCacheDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removePostsForAccount.contains(accountId)) {
        d->queue.removePostsForAccount.append(accountId);
    }
}

void AbstractSocialPostCacheDatabase::removePost(const QString &identifier)
{
    Q_D(AbstractSocialPostCacheDatabase);

    QMutexLocker locker(&d->mutex);
    if (!d->queue.removePosts.contains(identifier)) {
        d->queue.removePosts.append(identifier);
    }
    d->queue.insertPosts.remove(identifier);
}

void AbstractSocialPostCacheDatabase::removeAll()
{
    Q_D(AbstractSocialPostCacheDatabase);

    {
        QMutexLocker locker(&d->mutex);
        d->queue.insertPosts.clear();
        d->queue.mapPostsToAccounts.clear();
        d->queue.removePostsForAccount.clear();
        d->queue.removeAll = true;
    }

    executeWrite();
}

void AbstractSocialPostCacheDatabase::commit()
{
    executeWrite();
}

void AbstractSocialPostCacheDatabase::refresh()
{
    executeRead();
}

bool AbstractSocialPostCacheDatabase::read()
{
    Q_D(AbstractSocialPostCacheDatabase);
    // This might be slow

    QString accountQueryString = QLatin1String(
                "SELECT account, postId "
                "FROM link_post_account");
    if (!d->accountIdFilter.isEmpty()) {
        QStringList accountIds;
        for (int i=0; i<d->accountIdFilter.count(); i++) {
            if (d->accountIdFilter[i].type() == QVariant::Int) {
                accountIds << d->accountIdFilter[i].toString();
            }
        }
        if (accountIds.count()) {
            accountQueryString += " WHERE account IN (" + accountIds.join(',') + ')';
        }
    }

    QSqlQuery accountQuery = prepare(accountQueryString);
    if (!accountQuery.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from link_post_account table:" << accountQuery.lastError();
        return false;
    }

    QStringList filteredPostIds;
    QHash<QString,QList<int> > accounts;
    if (accountQuery.exec()) {
        while (accountQuery.next()) {
            int accountId = accountQuery.value(0).toInt();
            QString postId = accountQuery.value(1).toString();
            accounts[postId].append(accountId);
            filteredPostIds.append(postId);
        }
    }

    QString postQueryString = QLatin1String(
                "SELECT identifier, name, body, timestamp "
                "FROM posts");
    if (!d->accountIdFilter.isEmpty()) {
        postQueryString += " WHERE identifier IN (\"" + filteredPostIds.join("\",\"") + "\")";
    }
    postQueryString += " ORDER BY timestamp DESC";

    QSqlQuery postQuery = prepare(postQueryString);
    QSqlQuery imageQuery = prepare(QLatin1String(
                "SELECT position, url, type "
                "FROM images "
                "WHERE postId = :postId "
                "ORDER BY position"));
    QSqlQuery extraQuery = prepare(QLatin1String(
                "SELECT key, value "
                "FROM extra "
                "WHERE postId = :postId"));


    if (!postQuery.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from posts table:" << postQuery.lastError();
        return false;
    }

    QList<SocialPost::ConstPtr> posts;
    while (postQuery.next()) {
        QString identifier = postQuery.value(0).toString();

        QString name = postQuery.value(1).toString();
        QString body = postQuery.value(2).toString();
        int timestamp = postQuery.value(3).toInt();
        SocialPost::Ptr post = SocialPost::create(identifier, name, body,
                                                  QDateTime::fromTime_t(timestamp));

        imageQuery.bindValue(":postId", identifier);

        QMap<int, SocialPostImage::ConstPtr> images;
        if (imageQuery.exec()) {
            while (imageQuery.next()) {
                SocialPostImage::ImageType type = SocialPostImage::Invalid;
                QString typeString = imageQuery.value(2).toString();
                if (typeString == QLatin1String(PHOTO)) {
                    type = SocialPostImage::Photo;
                } else if (typeString == QLatin1String(VIDEO)) {
                    type = SocialPostImage::Video;
                }

                int position = imageQuery.value(0).toInt();
                SocialPostImage::Ptr image  = SocialPostImage::create(imageQuery.value(1).toString(),
                                                                      type);
                images.insert(position, image);
            }
            post->setImages(images);
        } else {
            qWarning() << Q_FUNC_INFO << "Error reading from images table:"
                       << imageQuery.lastError();
        }

        extraQuery.bindValue(":postId", identifier);

        QVariantMap extra;
        if (extraQuery.exec()) {
            while (extraQuery.next()) {
                QString key = extraQuery.value(0).toString();
                QVariant value = extraQuery.value(1);
                extra.insert(key, value);
            }
        } else {
            qWarning() << Q_FUNC_INFO << "Error reading from extra table:"
                       << extraQuery.lastError();
        }

        post->setExtra(extra);
        post->setAccounts(accounts[identifier]);

        posts.append(post);
    }

    QMutexLocker locker(&d->mutex);
    d->asyncPosts = posts;

    return true;
}

bool AbstractSocialPostCacheDatabase::write()
{
    Q_D(AbstractSocialPostCacheDatabase);

    QMutexLocker locker(&d->mutex);

    const QMap<QString, SocialPost::ConstPtr> insertPosts = d->queue.insertPosts;
    const QMultiMap<QString, int> mapPostsToAccounts = d->queue.mapPostsToAccounts;
    const QList<int> removePostsForAccount = d->queue.removePostsForAccount;
    const QList<QString> removePosts = d->queue.removePosts;
    bool removeAll = d->queue.removeAll;

    d->queue.insertPosts.clear();
    d->queue.mapPostsToAccounts.clear();
    d->queue.removePostsForAccount.clear();
    d->queue.removePosts.clear();
    d->queue.removeAll = false;

    locker.unlock();

    bool success = true;

    QSqlQuery query;

    // perform removals first.
    if (!removePosts.isEmpty()) {
        QVariantList postIds;

        Q_FOREACH (const QString postId, removePosts) {
            postIds.append(postId);
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM posts "
                    "WHERE identifier = :postId"));
        query.bindValue(QStringLiteral(":postId"), postIds);
        executeBatchSocialCacheQuery(query);
    }

    if (!removePostsForAccount.isEmpty() || removeAll) {
        QVariantList accountIds;

        if (removeAll) {
            // query all accounts
            QSqlQuery accountQuery = prepare(QLatin1String(
                        "SELECT DISTINCT account "
                        "FROM link_post_account"));

            if (!accountQuery.exec()) {
                qWarning() << Q_FUNC_INFO << "Error querying account list from posts table:" << accountQuery.lastError();
                return false;
            }

            while (accountQuery.next()) {
                accountIds.append(accountQuery.value(0).toInt());
            }
        } else {
            Q_FOREACH (int accountId, removePostsForAccount) {
                accountIds.append(accountId);
            }
        }

        query = prepare(QStringLiteral(
                    "DELETE FROM link_post_account "
                    "WHERE account = :accountId"));
        query.bindValue(QStringLiteral(":accountId"), accountIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM extra "
                    "WHERE postId NOT IN ("
                    "SELECT postId FROM link_post_account)"));
        executeSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE postId NOT IN ("
                    "SELECT postId FROM link_post_account)"));
        executeSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM posts "
                    "WHERE identifier NOT IN ("
                    "SELECT postId FROM link_post_account)"));
        executeSocialCacheQuery(query);
    }

    struct {
        QVariantList postIds;
        QVariantList names;
        QVariantList bodies;
        QVariantList timestamps;
    } posts;

    struct {
        QVariantList postIds;
        QVariantList positions;
        QVariantList urls;
        QVariantList types;
    } images;

    struct {
        QVariantList postIds;
        QVariantList keys;
        QVariantList values;
    } extras;

    struct {
        QVariantList postIds;
        QVariantList accountIds;
    } accounts;

    const QVariant invalidImageType = QLatin1String(INVALID);
    const QVariant photoImageType = QLatin1String(PHOTO);
    const QVariant videoImageType = QLatin1String(VIDEO);

    Q_FOREACH (const SocialPost::ConstPtr &post, insertPosts) {
        const QVariant postId = post->identifier();

        posts.postIds.append(postId);
        posts.names.append(post->name());
        posts.bodies.append(post->body());
        posts.timestamps.append(post->timestamp().toTime_t());

        const QMap<int, SocialPostImage::ConstPtr> postImages = post->allImages();
        typedef QMap<int, SocialPostImage::ConstPtr>::const_iterator iterator;
        for (iterator it = postImages.begin(); it != postImages.end(); ++it) {
            images.postIds.append(postId);
            images.positions.append(it.key());
            images.urls.append(it.value()->url());

            switch (it.value()->type()) {
            case SocialPostImage::Photo:
                images.types.append(photoImageType);
                break;
            case SocialPostImage::Video:
                images.types.append(videoImageType);
                break;
            default:
                images.types.append(invalidImageType);
                break;
            }
        }

        const QVariantMap extra = post->extra();
        for (QVariantMap::const_iterator it = extra.begin(); it != extra.end(); ++it) {
            extras.postIds.append(postId);
            extras.keys.append(it.key());
            extras.values.append(it.value());
        }
    }

    for (QMultiMap<QString, int>::const_iterator it = mapPostsToAccounts.begin();
            it != mapPostsToAccounts.end();
            ++it) {
        accounts.postIds.append(it.key());
        accounts.accountIds.append(it.value());
    }

    if (!posts.postIds.isEmpty()) {
        query = prepare(QStringLiteral(
                    "DELETE FROM images "
                    "WHERE postId = :postId"));
        query.bindValue(QStringLiteral(":postId"), posts.postIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM extra "
                    "WHERE postId = :postId"));
        query.bindValue(QStringLiteral(":postId"), posts.postIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "DELETE FROM link_post_account "
                    "WHERE postId = :postId"));
        query.bindValue(QStringLiteral(":postId"), posts.postIds);
        executeBatchSocialCacheQuery(query);

        query = prepare(QStringLiteral(
                    "INSERT OR REPLACE INTO posts ("
                    " identifier, name, body, timestamp) "
                    "VALUES ("
                    " :postId, :name, :body, :timestamp)"));
        query.bindValue(QStringLiteral(":postId"), posts.postIds);
        query.bindValue(QStringLiteral(":name"), posts.names);
        query.bindValue(QStringLiteral(":body"), posts.bodies);
        query.bindValue(QStringLiteral(":timestamp"), posts.timestamps);
        executeBatchSocialCacheQuery(query);
    }

    if (!images.postIds.isEmpty()) {
        query = prepare(QStringLiteral(
                    "INSERT INTO images ("
                    " postId, position, url, type) "
                    "VALUES ("
                    " :postId, :position, :url, :type)"));
        query.bindValue(QStringLiteral(":postId"), images.postIds);
        query.bindValue(QStringLiteral(":position"), images.positions);
        query.bindValue(QStringLiteral(":url"), images.urls);
        query.bindValue(QStringLiteral(":type"), images.types);
        executeBatchSocialCacheQuery(query);
    }

    if (!extras.postIds.isEmpty()) {
        query = prepare(QStringLiteral(
                    "INSERT INTO extra ("
                    " postId, key, value) "
                    "VALUES ("
                    " :postId, :key, :value)"));
        query.bindValue(QStringLiteral(":postId"), extras.postIds);
        query.bindValue(QStringLiteral(":key"), extras.keys);
        query.bindValue(QStringLiteral(":value"), extras.values);
        executeBatchSocialCacheQuery(query);
    }

    if (!accounts.postIds.isEmpty()) {
        query = prepare(QStringLiteral(
                    "INSERT INTO link_post_account ("
                    " postId, account) "
                    "VALUES ("
                    " :postId, :account)"));
        query.bindValue(QStringLiteral(":postId"), accounts.postIds);
        query.bindValue(QStringLiteral(":account"), accounts.accountIds);
        executeBatchSocialCacheQuery(query);
    }

    return success;
}

bool AbstractSocialPostCacheDatabase::createTables(QSqlDatabase database) const
{
    QSqlQuery query (database);

    // Heavily inspired from libeventfeeds
    // posts is composed of
    // * identifier is the identifier of the data (from social
    //    network, like the facebook id)
    // * name is the displayed name of the poster. Twitter, that
    //   requires both the name and "screen name" of the poster,
    //   uses an extra field, passed to the extra table.
    // * body is the content of the entry.
    // * timestamp is the timestamp, converted to milliseconds
    //   from epoch (makes sorting easier).
    query.prepare( "CREATE TABLE IF NOT EXISTS posts ("\
                   "identifier TEXT UNIQUE PRIMARY KEY,"\
                   "name TEXT,"\
                   "body TEXT,"\
                   "timestamp INTEGER)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create posts table" << query.lastError().text();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS images ("\
                  "postId TEXT, "\
                  "position INTEGER, "\
                  "url TEXT, "\
                  "type TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table" << query.lastError().text();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS extra ("\
                  "postId TEXT, "\
                  "key TEXT, "\
                  "value TEXT)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create extra table" << query.lastError().text();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS link_post_account ("\
                  "postId TEXT, "\
                  "account INTEGER, "\
                  "CONSTRAINT id PRIMARY KEY (postId, account))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create link_post_account table"
                   << query.lastError().text();
        return false;
    }

    return true;
}

bool AbstractSocialPostCacheDatabase::dropTables(QSqlDatabase database) const
{
    QSqlQuery query(database);
    query.prepare("DROP TABLE IF EXISTS posts");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete posts table"
                   << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS images");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete images table"
                   << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS extra");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete extra table"
                   << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS link_post_account");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete link_post_account table"
                   << query.lastError().text();
        return false;
    }

    return true;
}

void AbstractSocialPostCacheDatabase::readFinished()
{
    Q_D(AbstractSocialPostCacheDatabase);
    QMutexLocker locker(&d->mutex);

    d->posts = d->asyncPosts;
    d->asyncPosts.clear();

    locker.unlock();

    emit postsChanged();
}
