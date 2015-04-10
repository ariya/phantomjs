/*
 *  Copyright (C) 2010 Igalia S.L
 *  Copyright (C) 2010 Samsung Electronics
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

#ifndef FullscreenVideoControllerEfl_h
#define FullscreenVideoControllerEfl_h

#if ENABLE(VIDEO) && !defined(GST_API_VERSION_1)

#include "GStreamerGWorld.h"
#include "HTMLMediaElement.h"
#include <wtf/RefPtr.h>

class FullscreenVideoController {
    WTF_MAKE_NONCOPYABLE(FullscreenVideoController);
public:
    FullscreenVideoController();
    virtual ~FullscreenVideoController();

    void setMediaElement(WebCore::HTMLMediaElement*);
    WebCore::HTMLMediaElement* mediaElement() const { return m_mediaElement.get(); }

    void enterFullscreen();
    void exitFullscreen();

    void exitOnUserRequest();
    void togglePlay();
    void beginSeek();
    void doSeek();
    void endSeek();

    void hideHud();
    void showHud(bool);
    bool updateHudProgressBar();

    float volume() const;
    void setVolume(float);
    void volumeChanged();
    void muteChanged();

private:
    bool canPlay() const;
    void play();
    void pause();
    void playStateChanged();

    bool muted() const;

    float currentTime() const;
    void setCurrentTime(float);

    float duration() const;
    float percentLoaded() const;

    void createHud();
    void updateHudPosition();

    RefPtr<WebCore::HTMLMediaElement> m_mediaElement;
    RefPtr<WebCore::GStreamerGWorld> m_gstreamerGWorld;

    uint m_hudTimeoutId;
    uint m_progressBarUpdateId;
    uint m_progressBarFillUpdateId;
    uint m_hscaleUpdateId;
    uint m_volumeUpdateId;
    bool m_seekLock;
    PlatformWidget* m_window;
    PlatformWidget* m_hudWindow;
    PlatformWidget* m_timeHScale;
    PlatformWidget* m_timeLabel;
    PlatformWidget* m_volumeButton;
};

#endif

#endif // FullscreenVideoControllerEfl_h
