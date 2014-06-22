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

#include "config.h"

#if ENABLE(VIDEO) && !defined(GST_API_VERSION_1)

#include "FullscreenVideoControllerEfl.h"

#include "MediaPlayer.h"
#include "NotImplemented.h"

#include <gst/gst.h>

using namespace std;
using namespace WebCore;

FullscreenVideoController::FullscreenVideoController()
    : m_hudTimeoutId(0)
    , m_progressBarUpdateId(0)
    , m_seekLock(false)
    , m_window(0)
    , m_hudWindow(0)
{
}

FullscreenVideoController::~FullscreenVideoController()
{
    exitFullscreen();
}

void FullscreenVideoController::setMediaElement(HTMLMediaElement* mediaElement)
{
    if (mediaElement == m_mediaElement)
        return;

    m_mediaElement = mediaElement;
    if (!m_mediaElement) {
        // Can't do full-screen, just get out
        exitFullscreen();
    }
}

void FullscreenVideoController::showHud(bool /*autoHide*/)
{
    notImplemented();
}

void FullscreenVideoController::hideHud()
{
    notImplemented();
}

void FullscreenVideoController::enterFullscreen()
{
    notImplemented();
}

void FullscreenVideoController::updateHudPosition()
{
    notImplemented();
}

void FullscreenVideoController::exitOnUserRequest()
{
    notImplemented();
}

void FullscreenVideoController::exitFullscreen()
{
    notImplemented();
}

bool FullscreenVideoController::canPlay() const
{
    notImplemented();
    return false;
}

void FullscreenVideoController::play()
{
    notImplemented();
}

void FullscreenVideoController::pause()
{
    notImplemented();
}

void FullscreenVideoController::playStateChanged()
{
    notImplemented();
}

void FullscreenVideoController::togglePlay()
{
    notImplemented();
}

float FullscreenVideoController::volume() const
{
    notImplemented();
    return 0;
}

bool FullscreenVideoController::muted() const
{
    notImplemented();
    return false;
}

void FullscreenVideoController::setVolume(float /*volume*/)
{
    notImplemented();
}

void FullscreenVideoController::volumeChanged()
{
    notImplemented();
}

void FullscreenVideoController::muteChanged()
{
    notImplemented();
}

float FullscreenVideoController::currentTime() const
{
    notImplemented();
    return 0;
}

void FullscreenVideoController::setCurrentTime(float /*value*/)
{
    notImplemented();
}

float FullscreenVideoController::duration() const
{
    notImplemented();
    return 0;
}

float FullscreenVideoController::percentLoaded() const
{
    notImplemented();
    return 0;
}

void FullscreenVideoController::beginSeek()
{
    notImplemented();
}

void FullscreenVideoController::doSeek()
{
    notImplemented();
}

void FullscreenVideoController::endSeek()
{
    notImplemented();
}

bool FullscreenVideoController::updateHudProgressBar()
{
    notImplemented();
    return false;
}

void FullscreenVideoController::createHud()
{
    notImplemented();
}

#endif
