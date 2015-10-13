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

#ifndef ABSTRACTSOCIALPOSTCACHEDATABASE_H
#define ABSTRACTSOCIALPOSTCACHEDATABASE_H

#include "abstractsocialcachedatabase.h"
#include <QtCore/QSharedPointer>
#include <QtCore/QDateTime>
#include <QtCore/QVariantMap>

class SocialPostImagePrivate;
class SocialPostImage
{
public:

    enum ImageType {
        Invalid,
        Photo,
        Video
    };

    typedef QSharedPointer<SocialPostImage> Ptr;
    typedef QSharedPointer<const SocialPostImage> ConstPtr;

    explicit SocialPostImage();
    virtual ~SocialPostImage();

    QString url() const;
    ImageType type() const;

    static SocialPostImage::Ptr create(const QString &url, ImageType type);

protected:
    QScopedPointer<SocialPostImagePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(SocialPostImage)
    explicit SocialPostImage(const QString &url, ImageType type);
};

class SocialPostPrivate;
class SocialPost
{
public:
    typedef QSharedPointer<SocialPost> Ptr;
    typedef QSharedPointer<const SocialPost> ConstPtr;

    virtual ~SocialPost();

    static SocialPost::Ptr create(const QString &identifier, const QString &name,
                                  const QString &body, const QDateTime &timestamp,
                                  const QMap<int, SocialPostImage::ConstPtr> &images = QMap<int, SocialPostImage::ConstPtr>(),
                                  const QVariantMap &extra = QVariantMap(),
                                  const QList<int> &accounts = QList<int>());

    QString identifier() const;
    QString name() const;
    QString body() const;
    QDateTime timestamp() const;
    QString icon() const;
    QList<SocialPostImage::ConstPtr> images() const;
    QMap<int, SocialPostImage::ConstPtr> allImages() const;
    void setImages(const QMap<int, SocialPostImage::ConstPtr> &images);
    QVariantMap extra() const;
    void setExtra(const QVariantMap &extra);
    QList<int> accounts() const;
    void setAccounts(const QList<int> &accounts);

protected:
    QScopedPointer<SocialPostPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(SocialPost)
    explicit SocialPost(const QString &identifier, const QString &name,
                        const QString &body, const QDateTime &timestamp,
                        const QMap<int, SocialPostImage::ConstPtr> &images,
                        const QVariantMap &extra = QVariantMap(),
                        const QList<int> &accounts = QList<int>());
};

class AbstractSocialPostCacheDatabasePrivate;
class AbstractSocialPostCacheDatabase: public AbstractSocialCacheDatabase
{
    Q_OBJECT
    Q_PROPERTY(QVariantList accountIdFilter READ accountIdFilter WRITE setAccountIdFilter NOTIFY accountIdFilterChanged)
public:
    explicit AbstractSocialPostCacheDatabase(
            const QString &serviceName, const QString &databaseFile);
    ~AbstractSocialPostCacheDatabase();

    QVariantList accountIdFilter() const;
    void setAccountIdFilter(const QVariantList &accountIds);

    QList<SocialPost::ConstPtr> posts() const;

    void addPost(const QString &identifier, const QString &name,
                 const QString &body, const QDateTime &timestamp,
                 const QString &icon,
                 const QList<QPair<QString, SocialPostImage::ImageType> > &images,
                 const QVariantMap &extra, int account);

    void removePosts(int accountId);
    void removePost(const QString &identifier);
    void removeAll();

    void commit();
    void refresh();

Q_SIGNALS:
    void postsChanged();
    void accountIdFilterChanged();

protected:
    bool read();
    bool write();
    bool createTables(QSqlDatabase database) const;
    bool dropTables(QSqlDatabase database) const;

    void readFinished();


private:
    Q_DECLARE_PRIVATE(AbstractSocialPostCacheDatabase)
};


#endif // ABSTRACTSOCIALPOSTCACHEDATABASE_H
