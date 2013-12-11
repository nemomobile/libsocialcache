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

#ifndef ABSTRACTSOCIALCACHEMODEL_P_H
#define ABSTRACTSOCIALCACHEMODEL_P_H

#include "abstractsocialcachemodel.h"

#include <QtCore/QMap>

class AbstractSocialCacheModelPrivate
{
public:
    virtual ~AbstractSocialCacheModelPrivate();
    QString nodeIdentifier;

    void insertRange(int index, int count, const SocialCacheModelData &source, int sourceIndex);
    void updateRange(int index, int count, const SocialCacheModelData &source, int sourceIndex);
    void removeRange(int index, int count);

    void clearData();
    void updateData(const SocialCacheModelData &data);
    void updateRow(int row, const SocialCacheModelRow &data);

    QList<QMap<int, QVariant> > m_data;

protected:
    explicit AbstractSocialCacheModelPrivate(AbstractSocialCacheModel *q);

    virtual void nodeIdentifierChanged() {}

    AbstractSocialCacheModel * const q_ptr;
private:
    Q_DECLARE_PUBLIC(AbstractSocialCacheModel)
};

#endif // ABSTRACTSOCIALCACHEMODEL_P_H
