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

#include "FullscreenVideoControllerGtk.h"

#include "FullscreenVideoControllerGStreamer.h"
#include "GRefPtrGtk.h"
#include "GStreamerGWorld.h"
#include "GtkVersioning.h"
#include "MediaPlayer.h"
#include "MediaPlayerPrivateGStreamerBase.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <wtf/text/CString.h>

#define HUD_AUTO_HIDE_INTERVAL 3000 // 3 seconds
#define PROGRESS_BAR_UPDATE_INTERVAL 150 // 150ms

// Use symbolic icons only if we build with GTK+-3 support. They could
// be enabled for the GTK+2 build but we'd need to bump the required
// version to at least 2.22.
#if GTK_MAJOR_VERSION > 2
#define ICON_NAME_SUFFIX "-symbolic"
#else
#define ICON_NAME_SUFFIX
#endif

#define PLAY_ICON_NAME "media-playback-start" ICON_NAME_SUFFIX
#define PAUSE_ICON_NAME "media-playback-pause" ICON_NAME_SUFFIX
#define EXIT_FULLSCREEN_ICON_NAME "view-restore" ICON_NAME_SUFFIX

namespace WebCore {

static gboolean hideHudCallback(FullscreenVideoControllerGtk* controller)
{
    controller->hideHud();
    return FALSE;
}

static gboolean onFullscreenGtkMotionNotifyEvent(GtkWidget* widget, GdkEventMotion* event,  FullscreenVideoControllerGtk* controller)
{
    controller->showHud(true);
    return TRUE;
}

static void onFullscreenGtkActiveNotification(GtkWidget* widget, GParamSpec* property, FullscreenVideoControllerGtk* controller)
{
    if (!gtk_window_is_active(GTK_WINDOW(widget)))
        controller->hideHud();
}

static gboolean onFullscreenGtkConfigureEvent(GtkWidget* widget, GdkEventConfigure* event, FullscreenVideoControllerGtk* controller)
{
    controller->gtkConfigure(event);
    return TRUE;
}

static void onFullscreenGtkDestroy(GtkWidget* widget, FullscreenVideoControllerGtk* controller)
{
    controller->exitFullscreen();
}

static void togglePlayPauseActivated(GtkAction* action, FullscreenVideoControllerGtk* controller)
{
    controller->togglePlay();
}

static void exitFullscreenActivated(GtkAction* action, FullscreenVideoControllerGtk* controller)
{
    controller->exitOnUserRequest();
}

static gboolean progressBarUpdateCallback(FullscreenVideoControllerGtk* controller)
{
    return controller->updateHudProgressBar();
}

static gboolean timeScaleButtonPressed(GtkWidget* widget, GdkEventButton* event, FullscreenVideoControllerGtk* controller)
{
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    controller->beginSeek();
    return FALSE;
}

static gboolean timeScaleButtonReleased(GtkWidget* widget, GdkEventButton* event, FullscreenVideoControllerGtk* controller)
{
    controller->endSeek();
    return FALSE;
}

static void timeScaleValueChanged(GtkWidget* widget, FullscreenVideoControllerGtk* controller)
{
    controller->doSeek();
}

static void volumeValueChanged(GtkScaleButton *button, gdouble value, FullscreenVideoControllerGtk* controller)
{
    controller->setVolume(static_cast<float>(value));
}


FullscreenVideoControllerGtk::FullscreenVideoControllerGtk(MediaPlayerPrivateGStreamerBase* player)
    : FullscreenVideoControllerGStreamer(player)
    , m_hudTimeoutId(0)
    , m_progressBarUpdateId(0)
    , m_seekLock(false)
    , m_window(0)
    , m_hudWindow(0)
    , m_volumeButton(0)
    , m_keyPressSignalId(0)
    , m_destroySignalId(0)
    , m_isActiveSignalId(0)
    , m_motionNotifySignalId(0)
    , m_configureEventSignalId(0)
    , m_hudMotionNotifySignalId(0)
    , m_timeScaleButtonPressedSignalId(0)
    , m_timeScaleButtonReleasedSignalId(0)
    , m_playActionActivateSignalId(0)
    , m_exitFullcreenActionActivateSignalId(0)
{
}

void FullscreenVideoControllerGtk::gtkConfigure(GdkEventConfigure* event)
{
    updateHudPosition();
}

void FullscreenVideoControllerGtk::showHud(bool autoHide)
{
    if (!m_hudWindow)
        return;

    if (m_hudTimeoutId) {
        g_source_remove(m_hudTimeoutId);
        m_hudTimeoutId = 0;
    }

    // Show the cursor.
    GdkWindow* window = gtk_widget_get_window(m_window);
    gdk_window_set_cursor(window, 0);

    // Update the progress bar immediately before showing the window.
    updateHudProgressBar();
    gtk_widget_show_all(m_hudWindow);
    updateHudPosition();

    // Start periodic updates of the progress bar.
    if (!m_progressBarUpdateId)
        m_progressBarUpdateId = g_timeout_add(PROGRESS_BAR_UPDATE_INTERVAL, reinterpret_cast<GSourceFunc>(progressBarUpdateCallback), this);

    // Hide the hud in few seconds, if requested.
    if (autoHide)
        m_hudTimeoutId = g_timeout_add(HUD_AUTO_HIDE_INTERVAL, reinterpret_cast<GSourceFunc>(hideHudCallback), this);
}

void FullscreenVideoControllerGtk::hideHud()
{
    if (m_hudTimeoutId) {
        g_source_remove(m_hudTimeoutId);
        m_hudTimeoutId = 0;
    }

    if (!m_hudWindow)
        return;

    // Keep the hud visible if a seek is in progress or if the volume
    // popup is visible.
    GtkWidget* volumePopup = gtk_scale_button_get_popup(GTK_SCALE_BUTTON(m_volumeButton));
    if (m_seekLock || gtk_widget_get_visible(volumePopup)) {
        showHud(true);
        return;
    }

    GdkWindow* window = gtk_widget_get_window(m_window);
    GRefPtr<GdkCursor> cursor = adoptGRef(gdk_cursor_new(GDK_BLANK_CURSOR));
    gdk_window_set_cursor(window, cursor.get());

    gtk_widget_hide(m_hudWindow);

    if (m_progressBarUpdateId) {
        g_source_remove(m_progressBarUpdateId);
        m_progressBarUpdateId = 0;
    }
}

static gboolean onFullscreenGtkKeyPressEvent(GtkWidget* widget, GdkEventKey* event, FullscreenVideoControllerGtk* controller)
{
    switch (event->keyval) {
    case GDK_Escape:
        controller->exitOnUserRequest();
        break;
    case GDK_space:
    case GDK_Return:
        controller->togglePlay();
        break;
    case GDK_Up:
        controller->increaseVolume();
        break;
    case GDK_Down:
        controller->decreaseVolume();
        break;
    default:
        break;
    }

    return TRUE;
}

void FullscreenVideoControllerGtk::initializeWindow()
{
    m_window = reinterpret_cast<GtkWidget*>(m_gstreamerGWorld->platformVideoWindow()->window());

    if (!m_hudWindow)
        createHud();

    // Ensure black background.
#ifdef GTK_API_VERSION_2
    GdkColor color = { 1, 0, 0, 0 };
    gtk_widget_modify_bg(m_window, GTK_STATE_NORMAL, &color);
#else
    GdkRGBA color = { 0, 0, 0, 1};
    gtk_widget_override_background_color(m_window, GTK_STATE_FLAG_NORMAL, &color);
#endif
    gtk_widget_set_double_buffered(m_window, FALSE);

    m_keyPressSignalId = g_signal_connect(m_window, "key-press-event", G_CALLBACK(onFullscreenGtkKeyPressEvent), this);
    m_destroySignalId = g_signal_connect(m_window, "destroy", G_CALLBACK(onFullscreenGtkDestroy), this);
    m_isActiveSignalId = g_signal_connect(m_window, "notify::is-active", G_CALLBACK(onFullscreenGtkActiveNotification), this);

    gtk_widget_show_all(m_window);

    GdkWindow* window = gtk_widget_get_window(m_window);
    GRefPtr<GdkCursor> cursor = adoptGRef(gdk_cursor_new(GDK_BLANK_CURSOR));
    gdk_window_set_cursor(window, cursor.get());

    m_motionNotifySignalId = g_signal_connect(m_window, "motion-notify-event", G_CALLBACK(onFullscreenGtkMotionNotifyEvent), this);
    m_configureEventSignalId = g_signal_connect(m_window, "configure-event", G_CALLBACK(onFullscreenGtkConfigureEvent), this);

    gtk_window_fullscreen(GTK_WINDOW(m_window));
    showHud(true);
}

void FullscreenVideoControllerGtk::updateHudPosition()
{
    if (!m_hudWindow)
        return;

    // Get the screen rectangle.
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(m_window));
    GdkWindow* window = gtk_widget_get_window(m_window);
    GdkRectangle fullscreenRectangle;
    gdk_screen_get_monitor_geometry(screen, gdk_screen_get_monitor_at_window(screen, window), &fullscreenRectangle);

    // Get the popup window size.
    int hudWidth, hudHeight;
    gtk_window_get_size(GTK_WINDOW(m_hudWindow), &hudWidth, &hudHeight);

    // Resize the hud to the full width of the screen.
    gtk_window_resize(GTK_WINDOW(m_hudWindow), fullscreenRectangle.width, hudHeight);

    // Move the hud to the bottom of the screen.
    gtk_window_move(GTK_WINDOW(m_hudWindow), fullscreenRectangle.x, fullscreenRectangle.height + fullscreenRectangle.y - hudHeight);
}

void FullscreenVideoControllerGtk::destroyWindow()
{
    if (!m_hudWindow)
        return;

    g_signal_handler_disconnect(m_window, m_keyPressSignalId);
    g_signal_handler_disconnect(m_window, m_destroySignalId);
    g_signal_handler_disconnect(m_window, m_isActiveSignalId);
    g_signal_handler_disconnect(m_window, m_motionNotifySignalId);
    g_signal_handler_disconnect(m_window, m_configureEventSignalId);
    g_signal_handler_disconnect(m_hudWindow, m_hudMotionNotifySignalId);
    g_signal_handler_disconnect(m_timeHScale, m_timeScaleButtonPressedSignalId);
    g_signal_handler_disconnect(m_timeHScale, m_timeScaleButtonReleasedSignalId);
    g_signal_handler_disconnect(m_timeHScale, m_hscaleUpdateId);
    g_signal_handler_disconnect(m_volumeButton, m_volumeUpdateId);
    g_signal_handler_disconnect(m_playPauseAction, m_playActionActivateSignalId);
    g_signal_handler_disconnect(m_exitFullscreenAction, m_exitFullcreenActionActivateSignalId);

    if (m_hudTimeoutId) {
        g_source_remove(m_hudTimeoutId);
        m_hudTimeoutId = 0;
    }

    if (m_progressBarUpdateId) {
        g_source_remove(m_progressBarUpdateId);
        m_progressBarUpdateId = 0;
    }

    gtk_widget_hide(m_window);

    if (m_hudWindow)
        gtk_widget_destroy(m_hudWindow);
    m_hudWindow = 0;
}

void FullscreenVideoControllerGtk::playStateChanged()
{
    if (m_client->mediaPlayerIsPaused())
        g_object_set(m_playPauseAction, "tooltip", _("Play"), "icon-name", PLAY_ICON_NAME, NULL);
    else
        g_object_set(m_playPauseAction, "tooltip", _("Pause"), "icon-name", PAUSE_ICON_NAME, NULL);
    showHud(!m_client->mediaPlayerIsPaused());
}

void FullscreenVideoControllerGtk::volumeChanged()
{
    if (!m_volumeButton)
        return;

    g_signal_handler_block(m_volumeButton, m_volumeUpdateId);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(m_volumeButton), m_player->volume());
    g_signal_handler_unblock(m_volumeButton, m_volumeUpdateId);
}

void FullscreenVideoControllerGtk::muteChanged()
{
    if (!m_volumeButton)
        return;

    g_signal_handler_block(m_volumeButton, m_volumeUpdateId);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(m_volumeButton), m_player->muted() ? 0 : m_player->volume());
    g_signal_handler_unblock(m_volumeButton, m_volumeUpdateId);
}

void FullscreenVideoControllerGtk::beginSeek()
{
    m_seekLock = true;
}

void FullscreenVideoControllerGtk::doSeek()
{
    if (!m_seekLock)
        return;

    m_player->seek(gtk_range_get_value(GTK_RANGE(m_timeHScale))*m_player->duration() / 100);
}

void FullscreenVideoControllerGtk::endSeek()
{
    m_seekLock = false;
}

gboolean FullscreenVideoControllerGtk::updateHudProgressBar()
{
    float mediaDuration(m_player->duration());
    float mediaPosition(m_player->currentTime());

    if (!m_seekLock) {
        gdouble value = 0.0;

        if (mediaPosition && mediaDuration)
            value = (mediaPosition * 100.0) / mediaDuration;

        GtkAdjustment* adjustment = gtk_range_get_adjustment(GTK_RANGE(m_timeHScale));
        gtk_adjustment_set_value(adjustment, value);
    }

    gtk_range_set_fill_level(GTK_RANGE(m_timeHScale), (m_player->maxTimeLoaded() / mediaDuration)* 100);

    gchar* label = g_strdup_printf("%s / %s", timeToString(mediaPosition).utf8().data(), timeToString(mediaDuration).utf8().data());
    gtk_label_set_text(GTK_LABEL(m_timeLabel), label);
    g_free(label);
    return TRUE;
}

void FullscreenVideoControllerGtk::createHud()
{
    m_hudWindow = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_gravity(GTK_WINDOW(m_hudWindow), GDK_GRAVITY_SOUTH_WEST);
    gtk_window_set_type_hint(GTK_WINDOW(m_hudWindow), GDK_WINDOW_TYPE_HINT_NORMAL);

    m_hudMotionNotifySignalId = g_signal_connect(m_hudWindow, "motion-notify-event", G_CALLBACK(onFullscreenGtkMotionNotifyEvent), this);

#ifdef GTK_API_VERSION_2
    GtkWidget* hbox = gtk_hbox_new(FALSE, 4);
#else
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#endif
    gtk_container_add(GTK_CONTAINER(m_hudWindow), hbox);

    m_playPauseAction = gtk_action_new("play", _("Play / Pause"), _("Play or pause the media"), PAUSE_ICON_NAME);
    m_playActionActivateSignalId = g_signal_connect(m_playPauseAction, "activate", G_CALLBACK(togglePlayPauseActivated), this);

    GtkWidget* item = gtk_action_create_tool_item(m_playPauseAction);
    gtk_box_pack_start(GTK_BOX(hbox), item, FALSE, TRUE, 0);

    GtkWidget* label = gtk_label_new(_("Time:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

    GtkAdjustment* adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 100.0, 0.1, 1.0, 1.0));
#ifdef GTK_API_VERSION_2
    m_timeHScale = gtk_hscale_new(adjustment);
#else
    m_timeHScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
#endif
    gtk_scale_set_draw_value(GTK_SCALE(m_timeHScale), FALSE);
    gtk_range_set_show_fill_level(GTK_RANGE(m_timeHScale), TRUE);
    m_timeScaleButtonPressedSignalId = g_signal_connect(m_timeHScale, "button-press-event", G_CALLBACK(timeScaleButtonPressed), this);
    m_timeScaleButtonReleasedSignalId = g_signal_connect(m_timeHScale, "button-release-event", G_CALLBACK(timeScaleButtonReleased), this);
    m_hscaleUpdateId = g_signal_connect(m_timeHScale, "value-changed", G_CALLBACK(timeScaleValueChanged), this);

    gtk_box_pack_start(GTK_BOX(hbox), m_timeHScale, TRUE, TRUE, 0);

    m_timeLabel = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), m_timeLabel, FALSE, TRUE, 0);

    // Volume button.
    m_volumeButton = gtk_volume_button_new();
    gtk_box_pack_start(GTK_BOX(hbox), m_volumeButton, FALSE, TRUE, 0);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(m_volumeButton), m_player->volume());
    m_volumeUpdateId = g_signal_connect(m_volumeButton, "value-changed", G_CALLBACK(volumeValueChanged), this);

    m_exitFullscreenAction = gtk_action_new("exit", _("Exit Fullscreen"), _("Exit from fullscreen mode"), EXIT_FULLSCREEN_ICON_NAME);
    m_exitFullcreenActionActivateSignalId = g_signal_connect(m_exitFullscreenAction, "activate", G_CALLBACK(exitFullscreenActivated), this);
    g_object_set(m_exitFullscreenAction, "icon-name", EXIT_FULLSCREEN_ICON_NAME, NULL);
    item = gtk_action_create_tool_item(m_exitFullscreenAction);
    gtk_box_pack_start(GTK_BOX(hbox), item, FALSE, TRUE, 0);

    m_progressBarUpdateId = g_timeout_add(PROGRESS_BAR_UPDATE_INTERVAL, reinterpret_cast<GSourceFunc>(progressBarUpdateCallback), this);

    playStateChanged();
}

} // namespace WebCore
#endif
