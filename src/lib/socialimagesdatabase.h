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

#ifndef SOCIALIMAGESDATABASE_H
#define SOCIALIMAGESDATABASE_H

#include "abstractsocialcachedatabase_p.h"
#include <QtCore/QDateTime>
#include <QtCore/QStringList>

class SocialImagePrivate;
class SocialImage
{
public:
    typedef QSharedPointer<SocialImage> Ptr;
    typedef QSharedPointer<const SocialImage> ConstPtr;

    virtual ~SocialImage();

    static SocialImage::Ptr create(int accountId,
                                   const QString &imageUrl,
                                   const QString &imageFile,
                                   const QDateTime &createdTime,
                                   const QDateTime &expires,
                                   const QString &imageId);

    int accountId() const;
    QString imageUrl() const;
    QString imageFile() const;
    QDateTime createdTime() const;
    QDateTime expires() const;
    QString imageId() const;

protected:
    QScopedPointer<SocialImagePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(SocialImage)
    explicit SocialImage(int accountId,
                         const QString & imageUrl,
                         const QString & imageFile,
                         const QDateTime &createdTime,
                         const QDateTime &expires,
                         const QString &imageId);
};

bool operator==(const SocialImage::ConstPtr &image1, const SocialImage::ConstPtr &image2);

class SocialImagesDatabasePrivate;
class SocialImagesDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT

public:
    explicit SocialImagesDatabase();
    ~SocialImagesDatabase();

    void purgeAccount(int accountId);
    SocialImage::ConstPtr image(const QString &imageUrl) const;
    SocialImage::ConstPtr imageById(const QString &imageId) const;
    void addImage(int accountId,
                  const QString & imageUrl,
                  const QString & imageFile,
                  const QDateTime &createdTime,
                  const QDateTime &expires,
                  const QString & imageId = QString());
    void removeImage(const QString &imageUrl);
    void removeImages(QList<SocialImage::ConstPtr> images);
    void queryImages(int accountId, const QDateTime &olderThan = QDateTime());
    void queryExpired(int accountId);
    QList<SocialImage::ConstPtr> images() const;

    void commit();

Q_SIGNALS:
    void queryFinished();

protected:
    bool read();
    void readFinished();

    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;


private:
    Q_DECLARE_PRIVATE(SocialImagesDatabase)
};

#endif // SOCIALIMAGESDATABASE_H
