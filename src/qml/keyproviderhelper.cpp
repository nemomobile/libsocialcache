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

#include "keyproviderhelper.h"
#include <sailfishkeyprovider.h>

KeyProviderHelper::KeyProviderHelper(QObject *parent) :
    QObject(parent), m_triedLoadingFacebook(false)
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
