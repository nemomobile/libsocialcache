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

#include <synchronizelists_p.h>

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

template <> bool compareIdentity<SocialCacheModelRow>(
        const SocialCacheModelRow &item, const SocialCacheModelRow &reference)
{
    return item.value(0) == reference.value(0);
}

template <>
int updateRange<AbstractSocialCacheModelPrivate, SocialCacheModelData>(
        AbstractSocialCacheModelPrivate *d,
        int index,
        int count,
        const SocialCacheModelData &source,
        int sourceIndex)
{
    d->updateRange(index, count, source, sourceIndex);

    return count;
}

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

void AbstractWorkerObject::quitGracefully()
{
    m_quitMutex.lock();
    finalCleanup();
    m_quitWC.wakeAll();
    m_quitMutex.unlock();
}

AbstractSocialCacheModelPrivate::AbstractSocialCacheModelPrivate(AbstractSocialCacheModel *q,
                                                                 QObject *parent)
    :  QObject(parent), m_workerObject(0), q_ptr(q)
{
}

AbstractSocialCacheModelPrivate::~AbstractSocialCacheModelPrivate()
{
    if (m_workerThread.isRunning()) {
        // tell worker object to quit gracefully.
        m_workerObject->m_quitMutex.lock();
        QMetaObject::invokeMethod(m_workerObject, "quitGracefully", Qt::QueuedConnection);
        m_workerObject->m_quitWC.wait(&m_workerObject->m_quitMutex);
        m_workerObject->m_quitMutex.unlock();

        // now terminate the thread
        m_workerThread.quit();
        m_workerThread.wait();
    }

    // and delete the worker object - this will ensure the database gets closed.
    delete m_workerObject;
}

void AbstractSocialCacheModelPrivate::clearData()
{
    Q_Q(AbstractSocialCacheModel);
    if (m_data.count() > 0) {
        q->beginRemoveRows(QModelIndex(), 0, m_data.count() - 1);
        m_data.clear();
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
        m_workerThread.start(QThread::IdlePriority);
        m_workerObject = workerObjectToSet;
        m_workerObject->moveToThread(&m_workerThread);
        connect(this, &AbstractSocialCacheModelPrivate::nodeIdentifierChanged,
                m_workerObject, &AbstractWorkerObject::setNodeIdentifier);
        connect(this, &AbstractSocialCacheModelPrivate::refreshRequested,
                m_workerObject, &AbstractWorkerObject::triggerRefresh);
        connect(m_workerObject, &AbstractWorkerObject::dataUpdated,
                this, &AbstractSocialCacheModelPrivate::updateData);
        connect(m_workerObject, &AbstractWorkerObject::rowUpdated,
                this, &AbstractSocialCacheModelPrivate::updateRow);
    }

}

void AbstractSocialCacheModelPrivate::insertRange(
        int index, int count, const SocialCacheModelData &source, int sourceIndex)
{
    Q_Q(AbstractSocialCacheModel);

    q->beginInsertRows(QModelIndex(), index, index + count - 1);
    m_data = m_data.mid(0, index) + source.mid(sourceIndex, count) + m_data.mid(index);
    q->endInsertRows();
}

void AbstractSocialCacheModelPrivate::removeRange(int index, int count)
{
    Q_Q(AbstractSocialCacheModel);

    q->beginRemoveRows(QModelIndex(), index, index + count - 1);
    m_data = m_data.mid(0, index) + m_data.mid(index + count);
    q->endRemoveRows();
}

void AbstractSocialCacheModelPrivate::updateRange(
        int index, int count, const SocialCacheModelData &source, int sourceIndex)
{
    Q_Q(AbstractSocialCacheModel);

    for  (int i = 0; i < count; ++i) {
        m_data[index + i] = source[sourceIndex + i];
    }

    emit q->dataChanged(q->createIndex(index, 0), q->createIndex(index + count - 1, 0));
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
    return d->m_data.count();
}

QVariant AbstractSocialCacheModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    return getField(row, role);
}

QVariant AbstractSocialCacheModel::getField(int row, int role) const
{
    Q_D(const AbstractSocialCacheModel);
    if (row < 0 || row >= d->m_data.count()) {
        return QVariant();
    }

    return d->m_data.at(row).value(role);
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

    const int count = d->m_data.count();

    synchronizeList(d, d->m_data, data);

    if (d->m_data.count() != count) {
        emit countChanged();
    }
    emit modelUpdated();
}

void AbstractSocialCacheModel::updateRow(int row, const SocialCacheModelRow &data)
{
    Q_D(AbstractSocialCacheModel);
    foreach (int key, data.keys()) {
        d->m_data[row].insert(key, data.value(key));
    }
    emit dataChanged(index(row), index(row));
}

void AbstractSocialCacheModel::refresh()
{
    Q_D(AbstractSocialCacheModel);
    emit d->refreshRequested();
}
