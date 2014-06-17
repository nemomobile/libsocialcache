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

#include "synchelper.h"
#include <buteosyncfw5/SyncCommonDefs.h>
#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Service>

SyncHelper::SyncHelper(QObject *parent) :
    QObject(parent), QQmlParserStatus(), m_socialNetwork(SocialSyncInterface::InvalidSocialNetwork)
    , m_dataType(SocialSyncInterface::InvalidDataType), m_complete(false), m_loading(false)
{
    m_interface = new Buteo::SyncClientInterface();
    connect(m_interface, &Buteo::SyncClientInterface::syncStatus,
            this, &SyncHelper::slotSyncStatus);
}

void SyncHelper::classBegin()
{
}

void SyncHelper::componentComplete()
{
    m_complete = true;
    checkCurrentRun();
}

SocialSyncInterface::SocialNetwork SyncHelper::socialNetwork() const
{
    return m_socialNetwork;
}

void SyncHelper::setSocialNetwork(SocialSyncInterface::SocialNetwork socialNetwork)
{
    if (m_socialNetwork != socialNetwork) {
        m_socialNetwork = socialNetwork;
        emit socialNetworkChanged();

        if (m_complete) {
            checkCurrentRun();
        }
    }
}

SocialSyncInterface::DataType SyncHelper::dataType() const
{
    return m_dataType;
}

void SyncHelper::setDataType(SocialSyncInterface::DataType dataType)
{
    if (m_dataType != dataType) {
        m_dataType = dataType;
        emit dataTypeChanged();

        if (m_complete) {
            checkCurrentRun();
        }
    }
}

bool SyncHelper::loading() const
{
    return m_loading;
}

void SyncHelper::setLoading(bool loading)
{
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}

void SyncHelper::sync()
{
    Accounts::Manager am;
    Accounts::AccountIdList accountIds = am.accountList();
    QList<Buteo::SyncProfile*> allProfiles = m_profileManager.allSyncProfiles();
    QString profileNamePrefix = SocialSyncInterface::profileName(m_socialNetwork, m_dataType) + "-";

    foreach (Buteo::SyncProfile *profile, allProfiles) {
        if (profile->name().startsWith(profileNamePrefix)) {
            int accountId = profile->name().split("-").last().toInt();
            if (accountIds.indexOf(accountId) != -1) {
                m_interface->startSync(profile->name());
            }
        }
    }

    qDeleteAll(allProfiles);
}

void SyncHelper::slotSyncStatus(const QString &aProfileId, int aStatus,
                                const QString &aMessage, int aStatusDetails)
{
    Q_UNUSED(aMessage)
    Q_UNUSED(aStatusDetails)

    // Per-account profile names follow the convention 'socialnetworkName.DataType-<account index>'
    // Generate a prefix and check if it matches to incoming profile name.
    QString profileNamePrefix = SocialSyncInterface::profileName(m_socialNetwork, m_dataType) + "-";
    if (!aProfileId.startsWith(profileNamePrefix)) {
        return;
    }

    bool newLoading = (aStatus == Sync::SYNC_QUEUED || aStatus == Sync::SYNC_STARTED
                       || aStatus == Sync::SYNC_PROGRESS);

    if (!newLoading) {
        m_activeSyncs.removeAll(aProfileId);
        setLoading(m_activeSyncs.count() > 0);
    } else if (!m_activeSyncs.contains(aProfileId)) {
        m_activeSyncs.append(aProfileId);
        setLoading(true);
    }
}

void SyncHelper::checkCurrentRun()
{
    // Get the current running syncs to see if we are running
    QStringList runningSyncs = m_interface->getRunningSyncList();
    foreach (const QString profile, runningSyncs) {
        slotSyncStatus(profile, Sync::SYNC_PROGRESS, "", 0);
    }
}
