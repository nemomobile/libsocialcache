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

#ifndef KEYPROVIDERHELPER_H
#define KEYPROVIDERHELPER_H

#include <QtCore/QObject>

class KeyProviderHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString facebookClientId READ facebookClientId CONSTANT)
    Q_PROPERTY(QString twitterConsumerKey READ twitterConsumerKey CONSTANT)
    Q_PROPERTY(QString twitterConsumerSecret READ twitterConsumerSecret CONSTANT)
public:
    explicit KeyProviderHelper(QObject *parent = 0);
    QString facebookClientId();
    QString twitterConsumerKey();
    QString twitterConsumerSecret();
private:
    void loadFacebook();
    void loadTwitter();
    bool m_triedLoadingFacebook;
    QString m_facebookClientId;
    bool m_triedLoadingTwitter;
    QString m_twitterConsumerKey;
    QString m_twitterConsumerSecret;
};

#endif // KEYPROVIDERHELPER_H
