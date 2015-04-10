/*
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */


#ifndef MediaPlayerProxy_h
#define MediaPlayerProxy_h

#if ENABLE(VIDEO)

#include <wtf/Forward.h>

namespace WebCore {

    class IntRect;
    class IntSize;
    class MediaPlayer;
    class PluginView;
    class HTMLMediaElement;

    enum MediaPlayerProxyNotificationType {
        MediaPlayerNotificationPlayPauseButtonPressed,
        Idle,
        Loading,
        Loaded,
        FormatError,
        NetworkError,
        DecodeError
    };

    class WebMediaPlayerProxy {
    public:
        WebMediaPlayerProxy(MediaPlayer* player);
        ~WebMediaPlayerProxy();

        MediaPlayer* mediaPlayer() {return m_mediaPlayer;}
        void initEngine();
        void load(const String& url);
        HTMLMediaElement* element();
        void invokeMethod(const String& methodName);
        RefPtr<JSC::Bindings::Instance> pluginInstance();

    private:
        MediaPlayer* m_mediaPlayer;
        bool m_init;
        WebCore::PluginView* m_pluginView;
        bool m_hasSentResponseToPlugin;
        ScriptInstance m_instance;
    };

}
#endif // ENABLE(VIDEO)

#endif
