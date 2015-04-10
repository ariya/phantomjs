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

#ifndef FullscreenVideoControllerGtk_h
#define FullscreenVideoControllerGtk_h

#if ENABLE(VIDEO) && USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)

#include "FullscreenVideoControllerGStreamer.h"

namespace WebCore {

class FullscreenVideoControllerGtk : public FullscreenVideoControllerGStreamer {
public:
    FullscreenVideoControllerGtk(MediaPlayerPrivateGStreamerBase*);

    void gtkConfigure(GdkEventConfigure*);

    void playStateChanged();

    void beginSeek();
    void doSeek();
    void endSeek();

    void hideHud();
    void showHud(bool);
    gboolean updateHudProgressBar();

    void volumeChanged();
    void muteChanged();

private:
    void initializeWindow();
    void destroyWindow();

    void createHud();
    void updateHudPosition();

    guint m_hudTimeoutId;
    guint m_progressBarUpdateId;
    guint m_progressBarFillUpdateId;
    guint m_hscaleUpdateId;
    guint m_volumeUpdateId;
    bool m_seekLock;
    GtkWidget* m_window;
    GtkWidget* m_hudWindow;
    GtkAction* m_playPauseAction;
    GtkAction* m_exitFullscreenAction;
    GtkWidget* m_timeHScale;
    GtkWidget* m_timeLabel;
    GtkWidget* m_volumeButton;

    unsigned long m_keyPressSignalId;
    unsigned long m_destroySignalId;
    unsigned long m_isActiveSignalId;
    unsigned long m_motionNotifySignalId;
    unsigned long m_configureEventSignalId;
    unsigned long m_hudMotionNotifySignalId;
    unsigned long m_timeScaleButtonPressedSignalId;
    unsigned long m_timeScaleButtonReleasedSignalId;
    unsigned long m_playActionActivateSignalId;
    unsigned long m_exitFullcreenActionActivateSignalId;
};

}
#endif

#endif // FullscreenVideoControllerGtk_h
