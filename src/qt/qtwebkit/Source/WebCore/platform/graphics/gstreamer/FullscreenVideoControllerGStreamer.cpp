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

#include "config.h"

#if ENABLE(VIDEO) && USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)

#include "FullscreenVideoControllerGStreamer.h"

#if PLATFORM(GTK)
#include "FullscreenVideoControllerGtk.h"
#endif

#include "GStreamerGWorld.h"
#include "MediaPlayer.h"
#include "MediaPlayerPrivateGStreamerBase.h"
#include <gst/gst.h>
#include <wtf/text/CString.h>

#define VOLUME_UP_OFFSET 0.05 // 5%
#define VOLUME_DOWN_OFFSET 0.05 // 5%

namespace WebCore {

void playerVolumeChangedCallback(GObject *element, GParamSpec *pspec, FullscreenVideoControllerGStreamer* controller)
{
    controller->volumeChanged();
}

void playerMuteChangedCallback(GObject *element, GParamSpec *pspec, FullscreenVideoControllerGStreamer* controller)
{
    controller->muteChanged();
}

PassOwnPtr<FullscreenVideoControllerGStreamer> FullscreenVideoControllerGStreamer::create(MediaPlayerPrivateGStreamerBase* player)
{
#if PLATFORM(GTK)
   return adoptPtr(new FullscreenVideoControllerGtk(player));
#else
   return nullptr;
#endif
}

FullscreenVideoControllerGStreamer::FullscreenVideoControllerGStreamer(MediaPlayerPrivateGStreamerBase* player)
    : m_player(player)
    , m_client(player->mediaPlayer()->mediaPlayerClient())
    , m_gstreamerGWorld(player->platformMedia().media.gstreamerGWorld)
    , m_playerVolumeSignalHandler(0)
    , m_playerMuteSignalHandler(0)
{
}

FullscreenVideoControllerGStreamer::~FullscreenVideoControllerGStreamer()
{
    exitFullscreen();
}

void FullscreenVideoControllerGStreamer::enterFullscreen()
{
    if (!m_gstreamerGWorld)
        return;

    if (!m_gstreamerGWorld->enterFullscreen())
        return;

    initializeWindow();

    GstElement* pipeline = m_gstreamerGWorld->pipeline();
    m_playerVolumeSignalHandler = g_signal_connect(pipeline, "notify::volume", G_CALLBACK(playerVolumeChangedCallback), this);
    m_playerMuteSignalHandler = g_signal_connect(pipeline, "notify::mute", G_CALLBACK(playerMuteChangedCallback), this);
}

void FullscreenVideoControllerGStreamer::exitFullscreen()
{
    destroyWindow();

    GstElement* pipeline = m_gstreamerGWorld->pipeline();
    if (m_playerVolumeSignalHandler) {
        g_signal_handler_disconnect(pipeline, m_playerVolumeSignalHandler);
        m_playerVolumeSignalHandler = 0;
    }

    if (m_playerMuteSignalHandler) {
        g_signal_handler_disconnect(pipeline, m_playerMuteSignalHandler);
        m_playerMuteSignalHandler = 0;
    }

    m_gstreamerGWorld->exitFullscreen();
}

void FullscreenVideoControllerGStreamer::exitOnUserRequest()
{
    m_client->mediaPlayerExitFullscreen();
}

void FullscreenVideoControllerGStreamer::togglePlay()
{
    if (m_client->mediaPlayerIsPaused())
        m_client->mediaPlayerPlay();
    else
        m_client->mediaPlayerPause();
    playStateChanged();
}

void FullscreenVideoControllerGStreamer::increaseVolume()
{
    setVolume(m_player->volume() + VOLUME_UP_OFFSET);
}

void FullscreenVideoControllerGStreamer::decreaseVolume()
{
    setVolume(m_player->volume() - VOLUME_DOWN_OFFSET);
}

void FullscreenVideoControllerGStreamer::setVolume(float volume)
{
    volume = CLAMP(volume, 0.0, 1.0);
    m_player->setVolume(volume);
}

String FullscreenVideoControllerGStreamer::timeToString(float time)
{
    if (!std::isfinite(time))
        time = 0;
    int seconds = fabsf(time);
    int hours = seconds / (60 * 60);
    int minutes = (seconds / 60) % 60;
    seconds %= 60;

    if (hours) {
        if (hours > 9)
            return String::format("%s%02d:%02d:%02d", (time < 0 ? "-" : ""), hours, minutes, seconds);
        return String::format("%s%01d:%02d:%02d", (time < 0 ? "-" : ""), hours, minutes, seconds);
    }

    return String::format("%s%02d:%02d", (time < 0 ? "-" : ""), minutes, seconds);
}

} // namespace WebCore
#endif
