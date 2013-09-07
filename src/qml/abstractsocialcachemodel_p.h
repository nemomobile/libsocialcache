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
#include <QtCore/QThread>

class AbstractSocialCacheModel;
class AbstractSocialCacheModelPrivate;
class AbstractWorkerObject: public QObject
{
    Q_OBJECT
public:
    explicit AbstractWorkerObject();
    virtual ~AbstractWorkerObject();
    bool isLoading() const;
public Q_SLOTS:
    // Do the refresh operation, using refresh()
    void triggerRefresh();
    // Slot to set the node identifier
    void setNodeIdentifier(const QString &nodeIdentifierToSet);
Q_SIGNALS:
    // Sent when new data should be inserted in the model
    void dataRead(const SocialCacheModelData &dataToSet);
    // Emitted when a row of the model should be updated
    void rowUpdated(int row, const SocialCacheModelRow &updatedRow);
protected:
    QString nodeIdentifier; // Matches the node identifier in ASCMP
    void setLoading(bool loading);
    // Reimplement to perform model refreshing operation
    virtual void refresh() = 0;
private:
    bool m_loading;
};

class AbstractSocialCacheModelPrivate: public QObject
{
    Q_OBJECT
public:
    virtual ~AbstractSocialCacheModelPrivate();
    QString nodeIdentifier;
public Q_SLOTS:
    void clearData();
    void setData(const SocialCacheModelData &dataToSet);
    void updateRow(int row, const SocialCacheModelRow &updatedRow);
Q_SIGNALS:
    void nodeIdentifierChanged(const QString &nodeIdentifier);
    void refreshRequested();
protected:
    explicit AbstractSocialCacheModelPrivate(AbstractSocialCacheModel *q,
                                             QObject *parent = 0);
    // Prepare the worker object by establishing connections
    // implement if needed. Only call it in class constructor,
    // as it is unsafe to call it in private class
    // constructors.
    virtual void initWorkerObject(AbstractWorkerObject *workerObject);
    QList<QMap<int, QVariant> > data;
    AbstractSocialCacheModel * const q_ptr;
private:
    QThread m_workerThread;
    AbstractWorkerObject *m_workerObject;
    Q_DECLARE_PUBLIC(AbstractSocialCacheModel)
};

#endif // ABSTRACTSOCIALCACHEMODEL_P_H
