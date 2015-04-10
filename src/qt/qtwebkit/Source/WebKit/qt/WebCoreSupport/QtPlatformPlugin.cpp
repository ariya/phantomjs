/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtPlatformPlugin.h"

#include "qwebkitplatformplugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>

namespace WebCore {

bool QtPlatformPlugin::load(const QString& file)
{
    m_loader.setFileName(file);
    if (!m_loader.load())
        return false;

    QObject* obj = m_loader.instance();
    if (obj) {
        m_plugin = qobject_cast<QWebKitPlatformPlugin*>(obj);
        if (m_plugin)
            return true;
    }

    m_loader.unload();
    return false;
}

bool QtPlatformPlugin::load()
{
    const QLatin1String suffix("/webkit/");
    const QStringList paths = QCoreApplication::libraryPaths();

    for (int i = 0; i < paths.count(); ++i) {
        const QDir dir(paths[i] + suffix);
        if (!dir.exists())
            continue;
        const QStringList files = dir.entryList(QDir::Files);
        for (int j = 0; j < files.count(); ++j) {
            if (load(dir.absoluteFilePath(files.at(j))))
                return true;
        }
    }
    return false;
}

QtPlatformPlugin::~QtPlatformPlugin()
{
    m_loader.unload();
}

bool QtPlatformPlugin::loadStaticallyLinkedPlugin()
{
    QObjectList objs = QPluginLoader::staticInstances();
    for (int i = 0; i < objs.size(); ++i) {
        m_plugin = qobject_cast<QWebKitPlatformPlugin*>(objs[i]);
        if (m_plugin)
            return true;
    }
    return false;
}

QWebKitPlatformPlugin* QtPlatformPlugin::plugin()
{
    if (m_loaded)
        return m_plugin;
    m_loaded = true;

    if (loadStaticallyLinkedPlugin())
        return m_plugin;

    // Plugin path is stored in a static variable to avoid searching for the plugin
    // more then once.
    static QString pluginPath;

    if (pluginPath.isNull()) {
        if (load())
            pluginPath = m_loader.fileName();
    } else
        load(pluginPath);

    return m_plugin;
}

PassOwnPtr<QWebSelectMethod> QtPlatformPlugin::createSelectInputMethod()
{
    QWebKitPlatformPlugin* p = plugin();
    return adoptPtr(p ? static_cast<QWebSelectMethod*>(p->createExtension(QWebKitPlatformPlugin::MultipleSelections)) : 0);
}

PassOwnPtr<QWebNotificationPresenter> QtPlatformPlugin::createNotificationPresenter()
{
    QWebKitPlatformPlugin* p = plugin();
    return adoptPtr(p ? static_cast<QWebNotificationPresenter*>(p->createExtension(QWebKitPlatformPlugin::Notifications)) : 0);
}

PassOwnPtr<QWebHapticFeedbackPlayer> QtPlatformPlugin::createHapticFeedbackPlayer()
{
    QWebKitPlatformPlugin* p = plugin();
    return adoptPtr(p ? static_cast<QWebHapticFeedbackPlayer*>(p->createExtension(QWebKitPlatformPlugin::Haptics)) : 0);
}

PassOwnPtr<QWebTouchModifier> QtPlatformPlugin::createTouchModifier()
{
    QWebKitPlatformPlugin* p = plugin();
    return adoptPtr(p ? static_cast<QWebTouchModifier*>(p->createExtension(QWebKitPlatformPlugin::TouchInteraction)) : 0);
}

#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
PassOwnPtr<QWebFullScreenVideoHandler> QtPlatformPlugin::createFullScreenVideoHandler()
{
    QWebKitPlatformPlugin* p = plugin();
    return adoptPtr(p ? static_cast<QWebFullScreenVideoHandler*>(p->createExtension(QWebKitPlatformPlugin::FullScreenVideoPlayer)) : 0);
}
#endif

PassOwnPtr<QWebSpellChecker> QtPlatformPlugin::createSpellChecker()
{
    QWebKitPlatformPlugin* p = plugin();
    return adoptPtr(p ? static_cast<QWebSpellChecker*>(p->createExtension(QWebKitPlatformPlugin::SpellChecker)) : 0);
}

}
