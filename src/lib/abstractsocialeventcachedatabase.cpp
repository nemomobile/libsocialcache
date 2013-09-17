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

#include "abstractsocialeventcachedatabase.h"
#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static const char *INVALID = "invalid";
static const char *PHOTO = "photo";
static const char *VIDEO = "video";

bool sortImages(const SocialPostImage::ConstPtr &image1,
                const SocialPostImage::ConstPtr &image2)
{
    return image1->position() < image2->position();
}

struct SocialPostImagePrivate
{
    explicit SocialPostImagePrivate(int position, const QString &url, SocialPostImage::Type type);
    int position;
    QString url;
    SocialPostImage::Type type;
};

SocialPostImagePrivate::SocialPostImagePrivate(int position, const QString &url,
                                                 SocialPostImage::Type type)
    : position(position), url(url), type(type)
{
}

SocialPostImage::SocialPostImage(int position, const QString &url, Type type)
    : d_ptr(new SocialPostImagePrivate(position, url, type))
{
}

SocialPostImage::~SocialPostImage()
{
}

SocialPostImage::Ptr SocialPostImage::create(int position, const QString &url, Type type)
{
    return SocialPostImage::Ptr(new SocialPostImage(position, url, type));
}

int SocialPostImage::position() const
{
    Q_D(const SocialPostImage);
    return d->position;
}

QString SocialPostImage::url() const
{
    Q_D(const SocialPostImage);
    return d->url;
}

SocialPostImage::Type SocialPostImage::type() const
{
    Q_D(const SocialPostImage);
    return d->type;
}

struct SocialPostPrivate
{
    explicit SocialPostPrivate(const QString &identifier, const QString &title,
                               const QString &body, const QDateTime &timestamp,
                               const QString &footer,
                               const QVariantMap &extra = QVariantMap(),
                               const QList<int> &accounts = QList<int>());
    QString identifier;
    QString title;
    QString body;
    QDateTime timestamp;
    QString footer;
    QMap<int, SocialPostImage::ConstPtr> images;
    QVariantMap extra;
    QList<int> accounts;
};

SocialPostPrivate::SocialPostPrivate(const QString &identifier, const QString &title,
                                     const QString &body, const QDateTime &timestamp,
                                     const QString &footer,
                                     const QVariantMap &extra, const QList<int> &accounts)
    : identifier(identifier), title(title), body(body), timestamp(timestamp)
    , footer(footer), extra(extra), accounts(accounts)
{
}

SocialPost::SocialPost(const QString &identifier, const QString &title,
                         const QString &body, const QDateTime &timestamp,
                         const QString &footer,
                         const QMap<int, SocialPostImage::ConstPtr> &images,
                         const QVariantMap &extra, const QList<int> &accounts)
    : d_ptr(new SocialPostPrivate(identifier, title, body, timestamp, footer ,extra,
                                   accounts))
{
    setImages(images);
}

SocialPost::~SocialPost()
{
}

SocialPost::Ptr SocialPost::create(const QString &identifier, const QString &title,
                                   const QString &body, const QDateTime &timestamp,
                                   const QString &footer,
                                   const QMap<int, SocialPostImage::ConstPtr> &images,
                                   const QVariantMap &extra, const QList<int> &accounts)
{
    return SocialPost::Ptr(new SocialPost(identifier, title, body, timestamp, footer, images,
                                          extra, accounts));
}

QString SocialPost::identifier() const
{
    Q_D(const SocialPost);
    return d->identifier;
}

QString SocialPost::title() const
{
    Q_D(const SocialPost);
    return d->title;
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

QString SocialPost::footer() const
{
    Q_D(const SocialPost);
    return d->footer;
}

SocialPostImage::ConstPtr SocialPost::icon() const
{
    Q_D(const SocialPost);
    if (d->images.isEmpty()) {
        return SocialPostImage::ConstPtr();
    }

    return d->images.value(-1, SocialPostImage::ConstPtr());
}

QList<SocialPostImage::ConstPtr> SocialPost::images() const
{
    Q_D(const SocialPost);
    QList<SocialPostImage::ConstPtr> images;
    foreach (const SocialPostImage::ConstPtr &image, d->images) {
        if (image->position() >= 0) {
            images.append(image);
        }
    }

    std::sort(images.begin(), images.end(), sortImages);
    return images;
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
    AbstractSocialPostCacheDatabasePrivate(AbstractSocialPostCacheDatabase *q);
private:
    static void createEventsEntries(const QMap<QString, SocialPost::ConstPtr> &events,
                                    QStringList &eventsKeys,
                                    QStringList &imagesKeys, QStringList &extraKeys,
                                    QMap<QString, QVariantList> &eventsEntries,
                                    QMap<QString, QVariantList> &imagesEntries,
                                    QMap<QString, QVariantList> &extraEntries);
    static void createAccountsEntries(const QMultiMap<QString, int> &accounts,
                                      QStringList &keys,
                                      QMap<QString, QVariantList> &entries);
    QMap<QString, SocialPost::ConstPtr> queuedEvents;
    QMultiMap<QString, int> queuedEventsAccounts;
    Q_DECLARE_PUBLIC(AbstractSocialPostCacheDatabase)
};

AbstractSocialPostCacheDatabasePrivate::AbstractSocialPostCacheDatabasePrivate(AbstractSocialPostCacheDatabase *q)
    : AbstractSocialCacheDatabasePrivate(q)
{
}

void AbstractSocialPostCacheDatabasePrivate::createEventsEntries(const QMap<QString, SocialPost::ConstPtr> &events,
                                                                 QStringList &eventsKeys,
                                                                 QStringList &imagesKeys,
                                                                 QStringList &extraKeys,
                                                                 QMap<QString, QVariantList> &eventsEntries,
                                                                 QMap<QString, QVariantList> &imagesEntries,
                                                                 QMap<QString, QVariantList> &extraEntries)
{
    eventsKeys.clear();
    imagesKeys.clear();
    extraKeys.clear();
    eventsKeys << QLatin1String("identifier") << QLatin1String("title") << QLatin1String("body")
               << QLatin1String("timestamp") << QLatin1String("footer");
    imagesKeys << QLatin1String("eventId") << QLatin1String("position") << QLatin1String("url")
               << QLatin1String("type");
    extraKeys << QLatin1String("eventId") << QLatin1String("key") << QLatin1String("value");

    eventsEntries.clear();
    imagesEntries.clear();
    extraEntries.clear();

    foreach (const SocialPost::ConstPtr &event, events) {
        eventsEntries[QLatin1String("identifier")].append(event->identifier());
        eventsEntries[QLatin1String("title")].append(event->title());
        eventsEntries[QLatin1String("body")].append(event->body());
        eventsEntries[QLatin1String("timestamp")].append(event->timestamp().toTime_t());
        eventsEntries[QLatin1String("footer")].append(event->footer());

        foreach (const SocialPostImage::ConstPtr &image, event->images()) {
            imagesEntries[QLatin1String("eventId")].append(event->identifier());
            imagesEntries[QLatin1String("position")].append(image->position());
            imagesEntries[QLatin1String("url")].append(image->url());
            switch (image->type()) {
            case SocialPostImage::Photo:
                imagesEntries[QLatin1String("type")].append(QLatin1String(PHOTO));
                break;
            case SocialPostImage::Video:
                imagesEntries[QLatin1String("type")].append(QLatin1String(VIDEO));
                break;
            default:
                imagesEntries[QLatin1String("type")].append(QLatin1String(INVALID));
                break;
            }
        }

        QVariantMap extra = event->extra();
        foreach (const QString &key, extra.keys()) {
            extraEntries[QLatin1String("eventId")].append(event->identifier());
            extraEntries[QLatin1String("key")].append(key);
            extraEntries[QLatin1String("value")].append(extra.value(key).toString());
        }
    }

}

void AbstractSocialPostCacheDatabasePrivate::createAccountsEntries(const QMultiMap<QString, int> &accounts,
                                                                   QStringList &keys,
                                                                   QMap<QString, QVariantList> &entries)
{
    keys.clear();
    keys << QLatin1String("eventId") << QLatin1String("account");

    entries.clear();

    foreach (const QString &key, accounts.keys()) {
        foreach (int value, accounts.values(key)) {
            entries[QLatin1String("eventId")].append(key);
            entries[QLatin1String("account")].append(value);
        }
    }
}

AbstractSocialPostCacheDatabase::AbstractSocialPostCacheDatabase()
    : AbstractSocialCacheDatabase(*(new AbstractSocialPostCacheDatabasePrivate(this)))
{
}

QList<SocialPost::ConstPtr> AbstractSocialPostCacheDatabase::events() const
{
    // This might be slow
    Q_D(const AbstractSocialPostCacheDatabase);
    QSqlQuery query (d->db);

    query.prepare("SELECT identifier, title, body, timestamp, footer FROM events "\
                  "ORDER BY timestamp DESC");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Error reading from events table:" << query.lastError();
        return QList<SocialPost::ConstPtr>();
    }

    QList<SocialPost::ConstPtr> events;
    while (query.next()) {
        QString identifier = query.value(0).toString();

        qWarning() << "Retrieved identifier" << identifier;

        QString title = query.value(1).toString();
        QString body = query.value(2).toString();
        int timestamp = query.value(3).toInt();
        QString footer = query.value(4).toString();
        SocialPost::Ptr event = SocialPost::create(identifier, title, body,
                                                     QDateTime::fromTime_t(timestamp), footer);

        QSqlQuery eventQuery (d->db);
        eventQuery.prepare("SELECT position, url, type FROM images WHERE eventId = :eventId");
        eventQuery.bindValue(":eventId", identifier);

        QMap<int, SocialPostImage::ConstPtr> images;
        if (eventQuery.exec()) {
            while (eventQuery.next()) {
                SocialPostImage::Type type = SocialPostImage::Invalid;
                QString typeString = eventQuery.value(2).toString();
                if (typeString == QLatin1String(PHOTO)) {
                    type = SocialPostImage::Photo;
                } else if (typeString == QLatin1String(VIDEO)) {
                    type = SocialPostImage::Video;
                }

                int position = eventQuery.value(0).toInt();
                SocialPostImage::Ptr image  = SocialPostImage::create(position,
                                                                      eventQuery.value(1).toString(),
                                                                      type);
                images.insert(position, image);
            }
            event->setImages(images);
        } else {
            qWarning() << Q_FUNC_INFO << "Error reading from images table:"
                       << eventQuery.lastError();
        }

        eventQuery.prepare("SELECT key, value FROM extra WHERE eventId = :eventId");
        eventQuery.bindValue(":eventId", identifier);

        QVariantMap extra;
        if (eventQuery.exec()) {
            while (eventQuery.next()) {
                QString key = eventQuery.value(0).toString();
                QVariant value = eventQuery.value(1);
                extra.insert(key, value);
            }
        } else {
            qWarning() << Q_FUNC_INFO << "Error reading from extra table:"
                       << eventQuery.lastError();
        }

        event->setExtra(extra);

        eventQuery.prepare("SELECT account FROM link_event_account WHERE eventId = :eventId");
        eventQuery.bindValue(":eventId", identifier);

        QList<int> accounts;
        if (eventQuery.exec()) {
            while (eventQuery.next()) {
                accounts.append(eventQuery.value(0).toInt());
            }
        }

        event->setAccounts(accounts);

        events.append(event);
    }

    return events;
}

void AbstractSocialPostCacheDatabase::addEvent(const QString &identifier, const QString &title,
                                               const QString &body, const QDateTime &timestamp,
                                               const QString &footer,
                                               const QMap<int, SocialPostImage::ConstPtr> &images,
                                               const QVariantMap &extra, int account)
{
    Q_D(AbstractSocialPostCacheDatabase);
    d->queuedEvents.insert(identifier, SocialPost::create(identifier, title, body, timestamp,
                                                          footer, images, extra));
    d->queuedEventsAccounts.insert(identifier, account);
}

bool AbstractSocialPostCacheDatabase::write()
{
    Q_D(AbstractSocialPostCacheDatabase);
    if (!dbBeginTransaction()) {
        return false;
    }

    QStringList eventsKeys;
    QStringList imagesKeys;
    QStringList extraKeys;

    QMap<QString, QVariantList> eventsEntries;
    QMap<QString, QVariantList> imagesEntries;
    QMap<QString, QVariantList> extraEntries;

    bool ok = true;
    d->createEventsEntries(d->queuedEvents, eventsKeys, imagesKeys, extraKeys, eventsEntries,
                           imagesEntries, extraEntries);

    if (!dbWrite(QLatin1String("events"), eventsKeys, eventsEntries, InsertOrReplace)) {
        ok = false;
    }

    if (!dbWrite(QLatin1String("images"), imagesKeys, imagesEntries, InsertOrReplace)) {
        ok = false;
    }

    if (!dbWrite(QLatin1String("extra"), extraKeys, extraEntries, InsertOrReplace)) {
        ok = false;
    }

    QStringList keys;
    QMap<QString, QVariantList> entries;
    d->createAccountsEntries(d->queuedEventsAccounts, keys, entries);
    if (!dbWrite(QLatin1String("link_event_account"), keys, entries, InsertOrReplace)) {
        ok = false;
    }

    if (!dbCommitTransaction()) {
        return false;
    }

    return ok;
}

bool AbstractSocialPostCacheDatabase::dbCreateTables()
{
    Q_D(AbstractSocialPostCacheDatabase);
    QSqlQuery query (d->db);

    // Heavily inspired from libeventfeeds
    // events is composed of
    // * identifier is the identifier of the data (from social
    //    network, like the facebook id)
    // * title is the displayed title. It is usually the name
    //   of the poster. Twitter, that requires both the name and
    //   nickname of the poster, will need an extra field, passed
    //   to the extra table.
    // * body is the content of the entry.
    // * timestamp is the timestamp, converted to milliseconds
    //   from epoch (makes sorting easier).
    // * footer is a footer that is used to display informations
    //   like the number of likes.
    query.prepare( "CREATE TABLE IF NOT EXISTS events ("\
                   "identifier STRING UNIQUE PRIMARY KEY,"\
                   "title STRING,"\
                   "body STRING,"\
                   "timestamp VARCHAR(30),"\
                   "footer STRING)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create events table" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS images ("\
                  "eventId STRING, "\
                  "position INTEGER, "\
                  "url STRING, "\
                  "type VARCHAR(20))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS extra ("\
                  "eventId STRING, "\
                  "key VARCHAR(30), "\
                  "value STRING)");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create images table" << query.lastError().text();
        d->db.close();
        return false;
    }

    query.prepare("CREATE TABLE IF NOT EXISTS link_event_account ("\
                  "eventId STRING, "\
                  "account INTEGER, "\
                  "CONSTRAINT id PRIMARY KEY (eventId, account))");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to create link_event_account table"
                   << query.lastError().text();
        return false;
    }

    if (!dbCreatePragmaVersion(POST_DB_VERSION)) {
        return false;
    }

    return true;
}

bool AbstractSocialPostCacheDatabase::dbDropTables()
{
    Q_D(AbstractSocialPostCacheDatabase);
    QSqlQuery query(d->db);
    query.prepare("DROP TABLE IF EXISTS events");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events table"
                   << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS images");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events images"
                   << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS extra");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events extra"
                   << query.lastError().text();
        return false;
    }

    query.prepare("DROP TABLE IF EXISTS link_event_account");
    if (!query.exec()) {
        qWarning() << Q_FUNC_INFO << "Unable to delete events link_event_account"
                   << query.lastError().text();
        return false;
    }

    return true;
}
