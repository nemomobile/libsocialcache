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

#include "abstractfacebookcachemodel.h"
#include "abstractfacebookcachemodel_p.h"

#include <QtCore/QDebug>
#ifndef NO_KEY_PROVIDER
#include <sailfishkeyprovider.h>
#endif

AbstractFacebookCacheModelPrivate::AbstractFacebookCacheModelPrivate(AbstractSocialCacheModel *q,
                                                                     QObject *parent)
    : AbstractSocialCacheModelPrivate(q, parent)
{
}

AbstractFacebookCacheModel::AbstractFacebookCacheModel(AbstractFacebookCacheModelPrivate &dd,
                                                       QObject *parent)
    : AbstractSocialCacheModel(dd, parent)
{
}

QString AbstractFacebookCacheModel::clientId() const
{
    Q_D(const AbstractFacebookCacheModel);
    return d->clientId;
}

void AbstractFacebookCacheModel::requestClientId()
{
#ifndef NO_KEY_PROVIDER
    Q_D(AbstractFacebookCacheModel);
    char *cClientId = NULL;
    int success = SailfishKeyProvider_storedKey("facebook", "facebook-sync", "client_id", &cClientId);
    if (success != 0) {
        qWarning() << Q_FUNC_INFO << "could not retrieve stored client id from SailfishKeyProvider";
        free(cClientId);
    }

    d->clientId = QLatin1String(cClientId);
    emit clientIdChanged();
    free(cClientId);
#else
    qWarning() << Q_FUNC_INFO << "Compiled with NO_KEY_PROVIDER";
#endif
}
