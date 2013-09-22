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

#include "abstractsocialcachemodel.h"
#include "abstractsocialcachemodel_p.h"

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

AbstractWorkerObject::AbstractWorkerObject():
    m_loading(false)
{
}

AbstractWorkerObject::~AbstractWorkerObject()
{
}

bool AbstractWorkerObject::isLoading() const
{
    return m_loading;
}

void AbstractWorkerObject::triggerRefresh()
{
    if (!isLoading()) {
        refresh();
    }
}

void AbstractWorkerObject::setNodeIdentifier(const QString &nodeIdentifierToSet)
{
    nodeIdentifier = nodeIdentifierToSet;
}

void AbstractWorkerObject::setLoading(bool loading)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(locker)
    m_loading = loading;
}

AbstractSocialCacheModelPrivate::AbstractSocialCacheModelPrivate(AbstractSocialCacheModel *q,
                                                                 QObject *parent)
    :  QObject(parent), workerObject(0), q_ptr(q)
{
}

AbstractSocialCacheModelPrivate::~AbstractSocialCacheModelPrivate()
{
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait();
    }
    workerObject->deleteLater();
}

void AbstractSocialCacheModelPrivate::clearData()
{
    Q_Q(AbstractSocialCacheModel);
    if (data.count() > 0) {
        q->beginRemoveRows(QModelIndex(), 0, data.count() - 1);
        data.clear();
        q->endRemoveRows();
    }
}

void AbstractSocialCacheModelPrivate::updateData(const SocialCacheModelData &data)
{
    Q_Q(AbstractSocialCacheModel);
    q->updateData(data);
}

void AbstractSocialCacheModelPrivate::updateRow(int row, const SocialCacheModelRow &data)
{
    Q_Q(AbstractSocialCacheModel);
    q->updateRow(row, data);
}

void AbstractSocialCacheModelPrivate::initWorkerObject(AbstractWorkerObject *workerObjectToSet)
{
    if (workerObjectToSet) {
        m_workerThread.start(QThread::LowestPriority);
        workerObject = workerObjectToSet;
        workerObject->moveToThread(&m_workerThread);
        connect(this, &AbstractSocialCacheModelPrivate::nodeIdentifierChanged,
                workerObject, &AbstractWorkerObject::setNodeIdentifier);
        connect(this, &AbstractSocialCacheModelPrivate::refreshRequested,
                workerObject, &AbstractWorkerObject::triggerRefresh);
        connect(workerObject, &AbstractWorkerObject::dataUpdated,
                this, &AbstractSocialCacheModelPrivate::updateData);
        connect(workerObject, &AbstractWorkerObject::rowUpdated,
                this, &AbstractSocialCacheModelPrivate::updateRow);
    }

}

AbstractSocialCacheModel::AbstractSocialCacheModel(AbstractSocialCacheModelPrivate &dd,
                                                   QObject *parent)
    : QAbstractListModel(parent), d_ptr(&dd)
{
}

AbstractSocialCacheModel::~AbstractSocialCacheModel()
{
}

int AbstractSocialCacheModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    Q_D(const AbstractSocialCacheModel);
    return d->data.count();
}

QVariant AbstractSocialCacheModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    return getField(row, role);
}

QVariant AbstractSocialCacheModel::getField(int row, int role) const
{
    Q_D(const AbstractSocialCacheModel);
    if (row < 0 || row >= d->data.count()) {
        return QVariant();
    }

    return d->data.at(row).value(role);
}

QString AbstractSocialCacheModel::nodeIdentifier() const
{
    Q_D(const AbstractSocialCacheModel);
    return d->nodeIdentifier;
}

void AbstractSocialCacheModel::setNodeIdentifier(const QString &nodeIdentifier)
{
    Q_D(AbstractSocialCacheModel);
    if (d->nodeIdentifier != nodeIdentifier) {
        d->nodeIdentifier = nodeIdentifier;
        emit nodeIdentifierChanged();
        emit d->nodeIdentifierChanged(nodeIdentifier);
    }
}

int AbstractSocialCacheModel::count() const
{
    return rowCount();
}

void AbstractSocialCacheModel::updateData(const SocialCacheModelData &data)
{
    Q_D(AbstractSocialCacheModel);
    d->clearData();
    if (!data.isEmpty()) {
        beginInsertRows(QModelIndex(), 0, data.count() - 1);
        d->data = data;
        endInsertRows();

        emit countChanged();
    }
}

void AbstractSocialCacheModel::updateRow(int row, const SocialCacheModelRow &data)
{
    Q_D(AbstractSocialCacheModel);
    foreach (int key, data.keys()) {
        d->data[row].insert(key, data.value(key));
    }
    emit dataChanged(index(row), index(row));
}

void AbstractSocialCacheModel::refresh()
{
    Q_D(AbstractSocialCacheModel);
    emit d->refreshRequested();
}
