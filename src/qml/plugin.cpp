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

#include <QQmlExtensionPlugin>
#include <QQmlEngine>
#include <QtQml>

#include <QGuiApplication>
#include <QTranslator>
#include <QLocale>

#include "facebook/facebookimagecachemodel.h"
#include "facebook/facebookpostsmodel.h"
#include "facebook/facebooknotificationsmodel.h"
#include "twitter/twitterpostsmodel.h"

#ifndef NO_DEPS
#include "synchelper.h"
#include "keyproviderhelper.h"
#endif

// using custom translator so it gets properly removed from qApp when engine is deleted
class AppTranslator: public QTranslator
{
    Q_OBJECT
public:
    AppTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qApp->installTranslator(this);
    }

    virtual ~AppTranslator()
    {
        qApp->removeTranslator(this);
    }
};

static QObject *facebookImageDownloader_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    FacebookImageDownloader *downloader = new FacebookImageDownloader();
    return downloader;
}


class JollaSocialPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.nemomobile.socialcache")

public:
    void initializeEngine(QQmlEngine *engine, const char *uri)
    {
        Q_UNUSED(uri)
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.nemomobile.socialcache"));

        AppTranslator *engineeringEnglish = new AppTranslator(engine);
        AppTranslator *translator = new AppTranslator(engine);
        engineeringEnglish->load("socialcache_eng_en", "/usr/share/translations");
        translator->load(QLocale(), "socialcache", "-", "/usr/share/translations");
    }

    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("org.nemomobile.socialcache"));
        qRegisterMetaType<SocialCacheModelRow>("SocialCacheModelRow");
        qRegisterMetaType<SocialCacheModelData>("SocialCacheModelData");

        qmlRegisterType<FacebookImageCacheModel>(uri, 1, 0, "FacebookImageCacheModel");
        qmlRegisterType<FacebookPostsModel>(uri, 1, 0, "FacebookPostsModel");
        qmlRegisterType<FacebookNotificationsModel>(uri, 1, 0, "FacebookNotificationsModel");

        qmlRegisterSingletonType<FacebookImageDownloader>(uri, 1, 0, "FacebookImageDownloader",
                                                          &facebookImageDownloader_provider);

        qmlRegisterType<TwitterPostsModel>(uri, 1, 0, "TwitterPostsModel");




#ifndef NO_DEPS
        qmlRegisterUncreatableType<SocialSyncInterface>(uri, 1, 0, "SocialSync",
                                                        QLatin1String("Cannot create"));
        qmlRegisterType<SyncHelper>(uri, 1, 0, "SyncHelper");
        qmlRegisterType<KeyProviderHelper>(uri, 1, 0, "KeyProviderHelper");
#endif
    }
};

#include "plugin.moc"

