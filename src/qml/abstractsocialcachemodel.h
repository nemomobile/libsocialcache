/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Lucien Xu <lucien.xu@jollamobile.com>
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

#ifndef ABSTRACTSOCIALCACHEMODEL_H
#define ABSTRACTSOCIALCACHEMODEL_H

#include <QtCore/QAbstractListModel>

typedef QMap<int, QVariant> SocialCacheModelRow;
typedef QList<SocialCacheModelRow> SocialCacheModelData;

class AbstractSocialCacheModelPrivate;
class AbstractSocialCacheModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString nodeIdentifier READ nodeIdentifier WRITE setNodeIdentifier
               NOTIFY nodeIdentifierChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    virtual ~AbstractSocialCacheModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    Q_INVOKABLE QVariant getField(int row, int role) const;

    // properties
    QString nodeIdentifier() const;
    void setNodeIdentifier(const QString &nodeIdentifier);
    int count() const;

    // Methods used to update the model in the C++ side
    void updateData(const SocialCacheModelData &data);
    void updateRow(int row, const SocialCacheModelRow &data);

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void nodeIdentifierChanged();
    void countChanged();
    void modelUpdated();

protected:
    explicit AbstractSocialCacheModel(AbstractSocialCacheModelPrivate &dd, QObject *parent = 0);
    QScopedPointer<AbstractSocialCacheModelPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(AbstractSocialCacheModel)
};

Q_DECLARE_METATYPE(SocialCacheModelRow)
Q_DECLARE_METATYPE(SocialCacheModelData)

#endif // ABSTRACTSOCIALCACHEMODEL_H
