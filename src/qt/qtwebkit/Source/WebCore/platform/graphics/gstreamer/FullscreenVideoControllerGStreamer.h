/*
 *  Copyright (C) 2013 Igalia S.L
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

#ifndef FullscreenVideoControllerGStreamer_h
#define FullscreenVideoControllerGStreamer_h

#if ENABLE(VIDEO) && USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class GStreamerGWorld;
class MediaPlayerPrivateGStreamerBase;
class MediaPlayerClient;

class FullscreenVideoControllerGStreamer {
    WTF_MAKE_NONCOPYABLE(FullscreenVideoControllerGStreamer);
public:
    static PassOwnPtr<FullscreenVideoControllerGStreamer> create(MediaPlayerPrivateGStreamerBase*);
    FullscreenVideoControllerGStreamer(MediaPlayerPrivateGStreamerBase*);
    virtual ~FullscreenVideoControllerGStreamer();

    void enterFullscreen();
    void exitFullscreen();
    void exitOnUserRequest();

    void togglePlay();
    virtual void playStateChanged() { }

    void increaseVolume();
    void decreaseVolume();
    void setVolume(float);

    virtual void volumeChanged() { }
    virtual void muteChanged() { }

protected:
    String timeToString(float time);

    MediaPlayerPrivateGStreamerBase* m_player;
    MediaPlayerClient* m_client;
    GStreamerGWorld* m_gstreamerGWorld;

private:
    virtual void initializeWindow() { }
    virtual void destroyWindow() { }

    unsigned long m_playerVolumeSignalHandler;
    unsigned long m_playerMuteSignalHandler;
};

}
#endif

#endif // FullscreenVideoControllerGStreamer_h
