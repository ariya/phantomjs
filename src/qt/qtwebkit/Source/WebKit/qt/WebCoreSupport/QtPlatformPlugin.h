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

#ifndef QtPlatformPlugin_h
#define QtPlatformPlugin_h

#include <QPluginLoader>
#include <wtf/PassOwnPtr.h>

class QWebSelectMethod;
class QWebKitPlatformPlugin;
class QWebNotificationPresenter;
class QWebHapticFeedbackPlayer;
class QWebSelectData;
class QWebTouchModifier;
#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
class QWebFullScreenVideoHandler;
#endif
class QWebSpellChecker;

namespace WebCore {

class QtPlatformPlugin {
public:
    QtPlatformPlugin()
        : m_loaded(false)
        , m_plugin(0)
    {
    }

    ~QtPlatformPlugin();

    PassOwnPtr<QWebSelectMethod> createSelectInputMethod();
    PassOwnPtr<QWebNotificationPresenter> createNotificationPresenter();
    PassOwnPtr<QWebHapticFeedbackPlayer> createHapticFeedbackPlayer();
    PassOwnPtr<QWebTouchModifier> createTouchModifier();
#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
    PassOwnPtr<QWebFullScreenVideoHandler> createFullScreenVideoHandler();
#endif
    PassOwnPtr<QWebSpellChecker> createSpellChecker();

    QWebKitPlatformPlugin* plugin();

private:
    bool m_loaded;
    QWebKitPlatformPlugin* m_plugin;
    QPluginLoader m_loader;
    bool load();
    bool load(const QString& file);
    bool loadStaticallyLinkedPlugin();
};

}

#endif // QtPlatformPlugin_h
