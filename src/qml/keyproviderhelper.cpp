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

#include "keyproviderhelper.h"
#include <sailfishkeyprovider.h>

KeyProviderHelper::KeyProviderHelper(QObject *parent)
    : QObject(parent)
    , m_triedLoadingFacebook(false)
    , m_triedLoadingTwitter(false)
    , m_triedLoadingOneDrive(false)
{
}

QString KeyProviderHelper::facebookClientId()
{
    if (!m_triedLoadingFacebook) {
        loadFacebook();
    }

    return m_facebookClientId;
}

QString KeyProviderHelper::twitterConsumerKey()
{
    if (!m_triedLoadingTwitter) {
        loadTwitter();
    }

    return m_twitterConsumerKey;
}

QString KeyProviderHelper::twitterConsumerSecret()
{
    if (!m_triedLoadingTwitter) {
        loadTwitter();
    }

    return m_twitterConsumerSecret;
}

QString KeyProviderHelper::oneDriveClientId()
{
    if (!m_triedLoadingOneDrive) {
        loadOneDrive();
    }

    return m_oneDriveClientId;
}

void KeyProviderHelper::loadFacebook()
{
    m_triedLoadingFacebook = true;
    char *cClientId = NULL;
    int cSuccess = SailfishKeyProvider_storedKey("facebook", "facebook-sync", "client_id",
                                                 &cClientId);
    if (cSuccess != 0 || cClientId == NULL) {
        return;
    }

    m_facebookClientId = QLatin1String(cClientId);
    free(cClientId);
}

void KeyProviderHelper::loadTwitter()
{
    m_triedLoadingTwitter = true;
    char *cConsumerKey = NULL;
    char *cConsumerSecret = NULL;
    int ckSuccess = SailfishKeyProvider_storedKey("twitter", "twitter-sync", "consumer_key", &cConsumerKey);
    int csSuccess = SailfishKeyProvider_storedKey("twitter", "twitter-sync", "consumer_secret", &cConsumerSecret);

    if (ckSuccess != 0 || cConsumerKey == NULL || csSuccess != 0 || cConsumerSecret == NULL) {
        return;
    }

    m_twitterConsumerKey = QLatin1String(cConsumerKey);
    m_twitterConsumerSecret = QLatin1String(cConsumerSecret);
    free(cConsumerKey);
    free(cConsumerSecret);
}

void KeyProviderHelper::loadOneDrive()
{
    m_triedLoadingOneDrive = true;
    char *cClientId = NULL;
    int cSuccess = SailfishKeyProvider_storedKey("onedrive", "onedrive-sync", "client_id",
                                                 &cClientId);
    if (cSuccess != 0 || cClientId == NULL) {
        return;
    }

    m_oneDriveClientId = QLatin1String(cClientId);
    free(cClientId);
}
