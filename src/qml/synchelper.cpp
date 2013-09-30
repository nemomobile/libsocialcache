/*
 * Copyright (C) 2013 Lucien XU <sfietkonstantin@free.fr>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * The names of its contributors may not be used to endorse or promote 
 *     products derived from this software without specific prior written 
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */ 

#include "synchelper.h"
#include <buteosyncfw5/SyncCommonDefs.h>

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

void SyncHelper::sync()
{
    m_interface->startSync(SocialSyncInterface::profileName(m_socialNetwork, m_dataType));
}

void SyncHelper::slotSyncStatus(const QString &aProfileId, int aStatus,
                                const QString &aMessage, int aStatusDetails)
{
    Q_UNUSED(aProfileId)
    Q_UNUSED(aMessage)
    Q_UNUSED(aStatusDetails)

    if (aProfileId != SocialSyncInterface::profileName(m_socialNetwork, m_dataType)) {
        return;
    }


    bool newLoading = (aStatus == Sync::SYNC_QUEUED || aStatus == Sync::SYNC_STARTED
                       || aStatus == Sync::SYNC_PROGRESS);

    if (m_loading != newLoading) {
        m_loading = newLoading;
        emit loadingChanged();
    }
}

void SyncHelper::checkCurrentRun()
{
    // Get the current running syncs to see if we are running
    QString profileId = SocialSyncInterface::profileName(m_socialNetwork, m_dataType);
    bool loading = m_interface->getRunningSyncList().contains(profileId);

    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}
