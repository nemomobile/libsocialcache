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

#ifndef SYNCHELPER_H
#define SYNCHELPER_H

#include <QtCore/QObject>
#include <QtQml/QQmlParserStatus>
#include "socialsyncinterface.h"
#include <buteosyncfw5/SyncClientInterface.h>

class SyncHelper : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_PROPERTY(SocialSyncInterface::SocialNetwork socialNetwork READ socialNetwork
               WRITE setSocialNetwork NOTIFY socialNetworkChanged)
    Q_PROPERTY(SocialSyncInterface::DataType dataType READ dataType
               WRITE setDataType NOTIFY dataTypeChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
public:
    explicit SyncHelper(QObject *parent = 0);

    void classBegin();
    void componentComplete();

    SocialSyncInterface::SocialNetwork socialNetwork() const;
    void setSocialNetwork(SocialSyncInterface::SocialNetwork socialNetwork);
    SocialSyncInterface::DataType dataType() const;
    void setDataType(SocialSyncInterface::DataType dataType);
    bool loading() const;
public Q_SLOTS:
    void sync();
Q_SIGNALS:
    void socialNetworkChanged();
    void dataTypeChanged();
    void loadingChanged();
    
private slots:
    void slotSyncStatus(const QString &aProfileId, int aStatus, const QString &aMessage,
                        int aStatusDetails);
private:
    void checkCurrentRun();
    Buteo::SyncClientInterface *m_interface;
    SocialSyncInterface::SocialNetwork m_socialNetwork;
    SocialSyncInterface::DataType m_dataType;
    bool m_complete;
    bool m_loading;
};

#endif // SYNCHELPER_H
