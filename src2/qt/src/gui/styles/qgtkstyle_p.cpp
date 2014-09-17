/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgtkstyle_p.h"

// This file is responsible for resolving all GTK functions we use
// dynamically. This is done to avoid link-time dependancy on GTK
// as well as crashes occurring due to usage of the GTK_QT engines
//
// Additionally we create a map of common GTK widgets that we can pass
// to the GTK theme engine as many engines resort to querying the
// actual widget pointers for details that are not covered by the
// state flags

#include <QtCore/qglobal.h>
#if !defined(QT_NO_STYLE_GTK)

#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtCore/QLibrary>
#include <QtCore/QDebug>

#include <private/qapplication_p.h>
#include <private/qiconloader_p.h>

#include <QtGui/QMenu>
#include <QtGui/QStyle>
#include <QtGui/QApplication>
#include <QtGui/QPixmapCache>
#include <QtGui/QStatusBar>
#include <QtGui/QMenuBar>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>
#include <QtGui/QX11Info>

#include <private/qt_x11_p.h>

QT_BEGIN_NAMESPACE

static bool displayDepth  =  -1;
Q_GLOBAL_STATIC(QGtkStyleUpdateScheduler, styleScheduler)

Ptr_gtk_container_forall QGtkStylePrivate::gtk_container_forall = 0;
Ptr_gtk_init QGtkStylePrivate::gtk_init = 0;
Ptr_gtk_style_attach QGtkStylePrivate::gtk_style_attach = 0;
Ptr_gtk_window_new QGtkStylePrivate::gtk_window_new = 0;
Ptr_gtk_widget_destroy QGtkStylePrivate::gtk_widget_destroy = 0;
Ptr_gtk_widget_realize QGtkStylePrivate::gtk_widget_realize = 0;
Ptr_gtk_widget_set_default_direction QGtkStylePrivate::gtk_widget_set_default_direction = 0;
Ptr_gtk_widget_modify_color QGtkStylePrivate::gtk_widget_modify_fg = 0;
Ptr_gtk_widget_modify_color QGtkStylePrivate::gtk_widget_modify_bg = 0;
Ptr_gtk_arrow_new QGtkStylePrivate::gtk_arrow_new = 0;
Ptr_gtk_menu_item_new_with_label QGtkStylePrivate::gtk_menu_item_new_with_label = 0;
Ptr_gtk_check_menu_item_new_with_label QGtkStylePrivate::gtk_check_menu_item_new_with_label = 0;
Ptr_gtk_menu_bar_new QGtkStylePrivate::gtk_menu_bar_new = 0;
Ptr_gtk_menu_new QGtkStylePrivate::gtk_menu_new = 0;
Ptr_gtk_button_new QGtkStylePrivate::gtk_button_new = 0;
Ptr_gtk_tool_button_new QGtkStylePrivate::gtk_tool_button_new = 0;
Ptr_gtk_hbutton_box_new QGtkStylePrivate::gtk_hbutton_box_new = 0;
Ptr_gtk_check_button_new QGtkStylePrivate::gtk_check_button_new = 0;
Ptr_gtk_radio_button_new QGtkStylePrivate::gtk_radio_button_new = 0;
Ptr_gtk_spin_button_new QGtkStylePrivate::gtk_spin_button_new = 0;
Ptr_gtk_frame_new QGtkStylePrivate::gtk_frame_new = 0;
Ptr_gtk_expander_new QGtkStylePrivate::gtk_expander_new = 0;
Ptr_gtk_statusbar_new QGtkStylePrivate::gtk_statusbar_new = 0;
Ptr_gtk_entry_new QGtkStylePrivate::gtk_entry_new = 0;
Ptr_gtk_hscale_new QGtkStylePrivate::gtk_hscale_new = 0;
Ptr_gtk_vscale_new QGtkStylePrivate::gtk_vscale_new = 0;
Ptr_gtk_hscrollbar_new QGtkStylePrivate::gtk_hscrollbar_new = 0;
Ptr_gtk_vscrollbar_new QGtkStylePrivate::gtk_vscrollbar_new = 0;
Ptr_gtk_scrolled_window_new QGtkStylePrivate::gtk_scrolled_window_new = 0;
Ptr_gtk_notebook_new QGtkStylePrivate::gtk_notebook_new = 0;
Ptr_gtk_toolbar_new QGtkStylePrivate::gtk_toolbar_new = 0;
Ptr_gtk_toolbar_insert QGtkStylePrivate::gtk_toolbar_insert = 0;
Ptr_gtk_separator_tool_item_new QGtkStylePrivate::gtk_separator_tool_item_new = 0;
Ptr_gtk_tree_view_new QGtkStylePrivate::gtk_tree_view_new = 0;
Ptr_gtk_combo_box_new QGtkStylePrivate::gtk_combo_box_new = 0;
Ptr_gtk_combo_box_entry_new QGtkStylePrivate::gtk_combo_box_entry_new = 0;
Ptr_gtk_progress_bar_new QGtkStylePrivate::gtk_progress_bar_new = 0;
Ptr_gtk_container_add QGtkStylePrivate::gtk_container_add = 0;
Ptr_gtk_menu_shell_append QGtkStylePrivate::gtk_menu_shell_append = 0;
Ptr_gtk_progress_configure QGtkStylePrivate::gtk_progress_configure = 0;
Ptr_gtk_range_get_adjustment QGtkStylePrivate::gtk_range_get_adjustment = 0;
Ptr_gtk_range_set_adjustment QGtkStylePrivate::gtk_range_set_adjustment = 0;
Ptr_gtk_range_set_inverted QGtkStylePrivate::gtk_range_set_inverted = 0;
Ptr_gtk_icon_factory_lookup_default QGtkStylePrivate::gtk_icon_factory_lookup_default = 0;
Ptr_gtk_icon_theme_get_default QGtkStylePrivate::gtk_icon_theme_get_default = 0;
Ptr_gtk_widget_style_get QGtkStylePrivate::gtk_widget_style_get = 0;
Ptr_gtk_icon_set_render_icon QGtkStylePrivate::gtk_icon_set_render_icon = 0;
Ptr_gtk_fixed_new QGtkStylePrivate::gtk_fixed_new = 0;
Ptr_gtk_tree_view_column_new QGtkStylePrivate::gtk_tree_view_column_new = 0;
Ptr_gtk_tree_view_get_column QGtkStylePrivate::gtk_tree_view_get_column = 0;
Ptr_gtk_tree_view_append_column QGtkStylePrivate::gtk_tree_view_append_column = 0;
Ptr_gtk_paint_check QGtkStylePrivate::gtk_paint_check = 0;
Ptr_gtk_paint_box QGtkStylePrivate::gtk_paint_box = 0;
Ptr_gtk_paint_box_gap QGtkStylePrivate::gtk_paint_box_gap = 0;
Ptr_gtk_paint_flat_box QGtkStylePrivate::gtk_paint_flat_box = 0;
Ptr_gtk_paint_option QGtkStylePrivate::gtk_paint_option = 0;
Ptr_gtk_paint_extension QGtkStylePrivate::gtk_paint_extension = 0;
Ptr_gtk_paint_slider QGtkStylePrivate::gtk_paint_slider = 0;
Ptr_gtk_paint_shadow QGtkStylePrivate::gtk_paint_shadow = 0;
Ptr_gtk_paint_resize_grip QGtkStylePrivate::gtk_paint_resize_grip = 0;
Ptr_gtk_paint_focus QGtkStylePrivate::gtk_paint_focus = 0;
Ptr_gtk_paint_arrow QGtkStylePrivate::gtk_paint_arrow = 0;
Ptr_gtk_paint_handle QGtkStylePrivate::gtk_paint_handle = 0;
Ptr_gtk_paint_expander QGtkStylePrivate::gtk_paint_expander = 0;
Ptr_gtk_adjustment_configure QGtkStylePrivate::gtk_adjustment_configure = 0;
Ptr_gtk_adjustment_new QGtkStylePrivate::gtk_adjustment_new = 0;
Ptr_gtk_paint_hline QGtkStylePrivate::gtk_paint_hline = 0;
Ptr_gtk_paint_vline QGtkStylePrivate::gtk_paint_vline = 0;
Ptr_gtk_menu_item_set_submenu QGtkStylePrivate::gtk_menu_item_set_submenu = 0;
Ptr_gtk_settings_get_default QGtkStylePrivate::gtk_settings_get_default = 0;
Ptr_gtk_separator_menu_item_new QGtkStylePrivate::gtk_separator_menu_item_new = 0;
Ptr_gtk_widget_size_allocate QGtkStylePrivate::gtk_widget_size_allocate = 0;
Ptr_gtk_widget_size_request QGtkStylePrivate::gtk_widget_size_request = 0;
Ptr_gtk_widget_set_direction QGtkStylePrivate::gtk_widget_set_direction = 0;
Ptr_gtk_widget_path QGtkStylePrivate::gtk_widget_path = 0;
Ptr_gtk_container_get_type QGtkStylePrivate::gtk_container_get_type = 0;
Ptr_gtk_window_get_type QGtkStylePrivate::gtk_window_get_type = 0;
Ptr_gtk_widget_get_type QGtkStylePrivate::gtk_widget_get_type = 0;
Ptr_gtk_rc_get_style_by_paths QGtkStylePrivate::gtk_rc_get_style_by_paths = 0;
Ptr_gtk_check_version QGtkStylePrivate::gtk_check_version = 0;
Ptr_gtk_border_free QGtkStylePrivate::gtk_border_free = 0;
Ptr_pango_font_description_get_size QGtkStylePrivate::pango_font_description_get_size = 0;
Ptr_pango_font_description_get_weight QGtkStylePrivate::pango_font_description_get_weight = 0;
Ptr_pango_font_description_get_family QGtkStylePrivate::pango_font_description_get_family = 0;
Ptr_pango_font_description_get_style QGtkStylePrivate::pango_font_description_get_style = 0;

Ptr_gtk_file_filter_new QGtkStylePrivate::gtk_file_filter_new = 0;
Ptr_gtk_file_filter_set_name QGtkStylePrivate::gtk_file_filter_set_name = 0;
Ptr_gtk_file_filter_add_pattern QGtkStylePrivate::gtk_file_filter_add_pattern = 0;
Ptr_gtk_file_chooser_add_filter QGtkStylePrivate::gtk_file_chooser_add_filter = 0;
Ptr_gtk_file_chooser_set_filter QGtkStylePrivate::gtk_file_chooser_set_filter = 0;
Ptr_gtk_file_chooser_get_filter QGtkStylePrivate::gtk_file_chooser_get_filter = 0;
Ptr_gtk_file_chooser_dialog_new QGtkStylePrivate::gtk_file_chooser_dialog_new = 0;
Ptr_gtk_file_chooser_set_current_folder QGtkStylePrivate::gtk_file_chooser_set_current_folder = 0;
Ptr_gtk_file_chooser_get_filename QGtkStylePrivate::gtk_file_chooser_get_filename = 0;
Ptr_gtk_file_chooser_get_filenames QGtkStylePrivate::gtk_file_chooser_get_filenames = 0;
Ptr_gtk_file_chooser_set_current_name QGtkStylePrivate::gtk_file_chooser_set_current_name = 0;
Ptr_gtk_dialog_run QGtkStylePrivate::gtk_dialog_run = 0;
Ptr_gtk_file_chooser_set_filename QGtkStylePrivate::gtk_file_chooser_set_filename = 0;

Ptr_gdk_pixbuf_get_pixels QGtkStylePrivate::gdk_pixbuf_get_pixels = 0;
Ptr_gdk_pixbuf_get_width QGtkStylePrivate::gdk_pixbuf_get_width = 0;
Ptr_gdk_pixbuf_get_height QGtkStylePrivate::gdk_pixbuf_get_height = 0;
Ptr_gdk_pixmap_new QGtkStylePrivate::gdk_pixmap_new = 0;
Ptr_gdk_pixbuf_new QGtkStylePrivate::gdk_pixbuf_new = 0;
Ptr_gdk_pixbuf_get_from_drawable QGtkStylePrivate::gdk_pixbuf_get_from_drawable = 0;
Ptr_gdk_draw_rectangle QGtkStylePrivate::gdk_draw_rectangle = 0;
Ptr_gdk_pixbuf_unref QGtkStylePrivate::gdk_pixbuf_unref = 0;
Ptr_gdk_drawable_unref QGtkStylePrivate::gdk_drawable_unref = 0;
Ptr_gdk_drawable_get_depth QGtkStylePrivate::gdk_drawable_get_depth = 0;
Ptr_gdk_color_free QGtkStylePrivate::gdk_color_free = 0;
Ptr_gdk_x11_window_set_user_time QGtkStylePrivate::gdk_x11_window_set_user_time = 0;
Ptr_gdk_x11_drawable_get_xid QGtkStylePrivate::gdk_x11_drawable_get_xid = 0;
Ptr_gdk_x11_drawable_get_xdisplay QGtkStylePrivate::gdk_x11_drawable_get_xdisplay = 0;

Ptr_gconf_client_get_default QGtkStylePrivate::gconf_client_get_default = 0;
Ptr_gconf_client_get_string QGtkStylePrivate::gconf_client_get_string = 0;
Ptr_gconf_client_get_bool QGtkStylePrivate::gconf_client_get_bool = 0;

Ptr_gnome_icon_lookup_sync QGtkStylePrivate::gnome_icon_lookup_sync = 0;
Ptr_gnome_vfs_init QGtkStylePrivate::gnome_vfs_init = 0;

typedef int (*x11ErrorHandler)(Display*, XErrorEvent*);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QGtkStylePrivate*);

QT_BEGIN_NAMESPACE

static void gtkStyleSetCallback(GtkWidget*)
{
    qRegisterMetaType<QGtkStylePrivate *>();

    // We have to let this function return and complete the event
    // loop to ensure that all gtk widgets have been styled before
    // updating
    QMetaObject::invokeMethod(styleScheduler(), "updateTheme", Qt::QueuedConnection);
}

static void update_toolbar_style(GtkWidget *gtkToolBar, GParamSpec *, gpointer)
{
    GtkToolbarStyle toolbar_style = GTK_TOOLBAR_ICONS;
    g_object_get(gtkToolBar, "toolbar-style", &toolbar_style, NULL);
    QWidgetList widgets = QApplication::allWidgets();
    for (int i = 0; i < widgets.size(); ++i) {
        QWidget *widget = widgets.at(i);
        if (qobject_cast<QToolButton*>(widget)) {
            QEvent event(QEvent::StyleChange);
            QApplication::sendEvent(widget, &event);
        }
    }
}

static QHashableLatin1Literal classPath(GtkWidget *widget)
{
    char *class_path;
    QGtkStylePrivate::gtk_widget_path (widget, NULL, &class_path, NULL);

    char *copy = class_path;
    if (strncmp(copy, "GtkWindow.", 10) == 0)
        copy += 10;
    if (strncmp(copy, "GtkFixed.", 9) == 0)
        copy += 9;

    copy = strdup(copy);

    g_free(class_path);

    return QHashableLatin1Literal::fromData(copy);
}



bool QGtkStyleFilter::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::ApplicationPaletteChange) {
        // Only do this the first time since this will also
        // generate applicationPaletteChange events
        if (!qt_app_palettes_hash() ||  qt_app_palettes_hash()->isEmpty()) {
            stylePrivate->applyCustomPaletteHash();
        }
    }
    return QObject::eventFilter(obj, e);
}

QList<QGtkStylePrivate *> QGtkStylePrivate::instances;
QGtkStylePrivate::WidgetMap *QGtkStylePrivate::widgetMap = 0;

QGtkStylePrivate::QGtkStylePrivate()
  : QCleanlooksStylePrivate()
  , filter(this)
{
    instances.append(this);
}

QGtkStylePrivate::~QGtkStylePrivate()
{
    instances.removeOne(this);
}

void QGtkStylePrivate::init()
{
    resolveGtk();
    initGtkWidgets();
}

GtkWidget* QGtkStylePrivate::gtkWidget(const QHashableLatin1Literal &path)
{
    GtkWidget *widget = gtkWidgetMap()->value(path);
    if (!widget) {
        // Theme might have rearranged widget internals
        widget = gtkWidgetMap()->value(path);
    }
    return widget;
}

GtkStyle* QGtkStylePrivate::gtkStyle(const QHashableLatin1Literal &path)
{
    if (GtkWidget *w = gtkWidgetMap()->value(path))
        return w->style;
    return 0;
}

/*! \internal
 *  Get references to gtk functions after we dynamically load the library.
 */
void QGtkStylePrivate::resolveGtk() const
{
    // enforce the "0" suffix, so we'll open libgtk-x11-2.0.so.0
    QLibrary libgtk(QLS("gtk-x11-2.0"), 0, 0);
    libgtk.setLoadHints(QLibrary::ImprovedSearchHeuristics);

    gtk_init = (Ptr_gtk_init)libgtk.resolve("gtk_init");
    gtk_window_new = (Ptr_gtk_window_new)libgtk.resolve("gtk_window_new");
    gtk_style_attach = (Ptr_gtk_style_attach)libgtk.resolve("gtk_style_attach");
    gtk_widget_destroy = (Ptr_gtk_widget_destroy)libgtk.resolve("gtk_widget_destroy");
    gtk_widget_realize = (Ptr_gtk_widget_realize)libgtk.resolve("gtk_widget_realize");

    gtk_file_chooser_set_current_folder = (Ptr_gtk_file_chooser_set_current_folder)libgtk.resolve("gtk_file_chooser_set_current_folder");
    gtk_file_filter_new = (Ptr_gtk_file_filter_new)libgtk.resolve("gtk_file_filter_new");
    gtk_file_filter_set_name = (Ptr_gtk_file_filter_set_name)libgtk.resolve("gtk_file_filter_set_name");
    gtk_file_filter_add_pattern = (Ptr_gtk_file_filter_add_pattern)libgtk.resolve("gtk_file_filter_add_pattern");
    gtk_file_chooser_add_filter = (Ptr_gtk_file_chooser_add_filter)libgtk.resolve("gtk_file_chooser_add_filter");
    gtk_file_chooser_set_filter = (Ptr_gtk_file_chooser_set_filter)libgtk.resolve("gtk_file_chooser_set_filter");
    gtk_file_chooser_get_filter = (Ptr_gtk_file_chooser_get_filter)libgtk.resolve("gtk_file_chooser_get_filter");
    gtk_file_chooser_dialog_new = (Ptr_gtk_file_chooser_dialog_new)libgtk.resolve("gtk_file_chooser_dialog_new");
    gtk_file_chooser_set_current_folder = (Ptr_gtk_file_chooser_set_current_folder)libgtk.resolve("gtk_file_chooser_set_current_folder");
    gtk_file_chooser_get_filename = (Ptr_gtk_file_chooser_get_filename)libgtk.resolve("gtk_file_chooser_get_filename");
    gtk_file_chooser_get_filenames = (Ptr_gtk_file_chooser_get_filenames)libgtk.resolve("gtk_file_chooser_get_filenames");
    gtk_file_chooser_set_current_name = (Ptr_gtk_file_chooser_set_current_name)libgtk.resolve("gtk_file_chooser_set_current_name");
    gtk_dialog_run = (Ptr_gtk_dialog_run)libgtk.resolve("gtk_dialog_run");
    gtk_file_chooser_set_filename = (Ptr_gtk_file_chooser_set_filename)libgtk.resolve("gtk_file_chooser_set_filename");

    gdk_pixbuf_get_pixels = (Ptr_gdk_pixbuf_get_pixels)libgtk.resolve("gdk_pixbuf_get_pixels");
    gdk_pixbuf_get_width = (Ptr_gdk_pixbuf_get_width)libgtk.resolve("gdk_pixbuf_get_width");
    gdk_pixbuf_get_height = (Ptr_gdk_pixbuf_get_height)libgtk.resolve("gdk_pixbuf_get_height");
    gdk_pixmap_new = (Ptr_gdk_pixmap_new)libgtk.resolve("gdk_pixmap_new");
    gdk_pixbuf_new = (Ptr_gdk_pixbuf_new)libgtk.resolve("gdk_pixbuf_new");
    gdk_pixbuf_get_from_drawable = (Ptr_gdk_pixbuf_get_from_drawable)libgtk.resolve("gdk_pixbuf_get_from_drawable");
    gdk_draw_rectangle = (Ptr_gdk_draw_rectangle)libgtk.resolve("gdk_draw_rectangle");
    gdk_pixbuf_unref = (Ptr_gdk_pixbuf_unref)libgtk.resolve("gdk_pixbuf_unref");
    gdk_drawable_unref = (Ptr_gdk_drawable_unref)libgtk.resolve("gdk_drawable_unref");
    gdk_drawable_get_depth = (Ptr_gdk_drawable_get_depth)libgtk.resolve("gdk_drawable_get_depth");
    gdk_color_free = (Ptr_gdk_color_free)libgtk.resolve("gdk_color_free");
    gdk_x11_window_set_user_time = (Ptr_gdk_x11_window_set_user_time)libgtk.resolve("gdk_x11_window_set_user_time");
    gdk_x11_drawable_get_xid = (Ptr_gdk_x11_drawable_get_xid)libgtk.resolve("gdk_x11_drawable_get_xid");
    gdk_x11_drawable_get_xdisplay = (Ptr_gdk_x11_drawable_get_xdisplay)libgtk.resolve("gdk_x11_drawable_get_xdisplay");

    gtk_widget_set_default_direction = (Ptr_gtk_widget_set_default_direction)libgtk.resolve("gtk_widget_set_default_direction");
    gtk_widget_modify_fg = (Ptr_gtk_widget_modify_color)libgtk.resolve("gtk_widget_modify_fg");
    gtk_widget_modify_bg = (Ptr_gtk_widget_modify_color)libgtk.resolve("gtk_widget_modify_bg");
    gtk_arrow_new = (Ptr_gtk_arrow_new)libgtk.resolve("gtk_arrow_new");
    gtk_menu_item_new_with_label = (Ptr_gtk_menu_item_new_with_label)libgtk.resolve("gtk_menu_item_new_with_label");
    gtk_check_menu_item_new_with_label = (Ptr_gtk_check_menu_item_new_with_label)libgtk.resolve("gtk_check_menu_item_new_with_label");
    gtk_menu_bar_new = (Ptr_gtk_menu_bar_new)libgtk.resolve("gtk_menu_bar_new");
    gtk_menu_new = (Ptr_gtk_menu_new)libgtk.resolve("gtk_menu_new");
    gtk_toolbar_new = (Ptr_gtk_toolbar_new)libgtk.resolve("gtk_toolbar_new");
    gtk_separator_tool_item_new = (Ptr_gtk_separator_tool_item_new)libgtk.resolve("gtk_separator_tool_item_new");
    gtk_toolbar_insert = (Ptr_gtk_toolbar_insert)libgtk.resolve("gtk_toolbar_insert");
    gtk_button_new = (Ptr_gtk_button_new)libgtk.resolve("gtk_button_new");
    gtk_tool_button_new = (Ptr_gtk_tool_button_new)libgtk.resolve("gtk_tool_button_new");
    gtk_hbutton_box_new = (Ptr_gtk_hbutton_box_new)libgtk.resolve("gtk_hbutton_box_new");
    gtk_check_button_new = (Ptr_gtk_check_button_new)libgtk.resolve("gtk_check_button_new");
    gtk_radio_button_new = (Ptr_gtk_radio_button_new)libgtk.resolve("gtk_radio_button_new");
    gtk_notebook_new = (Ptr_gtk_notebook_new)libgtk.resolve("gtk_notebook_new");
    gtk_progress_bar_new = (Ptr_gtk_progress_bar_new)libgtk.resolve("gtk_progress_bar_new");
    gtk_spin_button_new = (Ptr_gtk_spin_button_new)libgtk.resolve("gtk_spin_button_new");
    gtk_hscale_new = (Ptr_gtk_hscale_new)libgtk.resolve("gtk_hscale_new");
    gtk_vscale_new = (Ptr_gtk_vscale_new)libgtk.resolve("gtk_vscale_new");
    gtk_hscrollbar_new = (Ptr_gtk_hscrollbar_new)libgtk.resolve("gtk_hscrollbar_new");
    gtk_vscrollbar_new = (Ptr_gtk_vscrollbar_new)libgtk.resolve("gtk_vscrollbar_new");
    gtk_scrolled_window_new = (Ptr_gtk_scrolled_window_new)libgtk.resolve("gtk_scrolled_window_new");
    gtk_menu_shell_append = (Ptr_gtk_menu_shell_append)libgtk.resolve("gtk_menu_shell_append");
    gtk_entry_new = (Ptr_gtk_entry_new)libgtk.resolve("gtk_entry_new");
    gtk_tree_view_new = (Ptr_gtk_tree_view_new)libgtk.resolve("gtk_tree_view_new");
    gtk_combo_box_new = (Ptr_gtk_combo_box_new)libgtk.resolve("gtk_combo_box_new");
    gtk_progress_configure = (Ptr_gtk_progress_configure)libgtk.resolve("gtk_progress_configure");
    gtk_range_get_adjustment = (Ptr_gtk_range_get_adjustment)libgtk.resolve("gtk_range_get_adjustment");
    gtk_range_set_adjustment = (Ptr_gtk_range_set_adjustment)libgtk.resolve("gtk_range_set_adjustment");
    gtk_range_set_inverted = (Ptr_gtk_range_set_inverted)libgtk.resolve("gtk_range_set_inverted");
    gtk_container_add = (Ptr_gtk_container_add)libgtk.resolve("gtk_container_add");
    gtk_icon_factory_lookup_default = (Ptr_gtk_icon_factory_lookup_default)libgtk.resolve("gtk_icon_factory_lookup_default");
    gtk_icon_theme_get_default = (Ptr_gtk_icon_theme_get_default)libgtk.resolve("gtk_icon_theme_get_default");
    gtk_widget_style_get = (Ptr_gtk_widget_style_get)libgtk.resolve("gtk_widget_style_get");
    gtk_icon_set_render_icon = (Ptr_gtk_icon_set_render_icon)libgtk.resolve("gtk_icon_set_render_icon");
    gtk_fixed_new = (Ptr_gtk_fixed_new)libgtk.resolve("gtk_fixed_new");
    gtk_tree_view_column_new = (Ptr_gtk_tree_view_column_new)libgtk.resolve("gtk_tree_view_column_new");
    gtk_tree_view_append_column= (Ptr_gtk_tree_view_append_column )libgtk.resolve("gtk_tree_view_append_column");
    gtk_tree_view_get_column = (Ptr_gtk_tree_view_get_column )libgtk.resolve("gtk_tree_view_get_column");
    gtk_paint_check = (Ptr_gtk_paint_check)libgtk.resolve("gtk_paint_check");
    gtk_paint_box = (Ptr_gtk_paint_box)libgtk.resolve("gtk_paint_box");
    gtk_paint_flat_box = (Ptr_gtk_paint_flat_box)libgtk.resolve("gtk_paint_flat_box");
    gtk_paint_check = (Ptr_gtk_paint_check)libgtk.resolve("gtk_paint_check");
    gtk_paint_box = (Ptr_gtk_paint_box)libgtk.resolve("gtk_paint_box");
    gtk_paint_resize_grip = (Ptr_gtk_paint_resize_grip)libgtk.resolve("gtk_paint_resize_grip");
    gtk_paint_focus = (Ptr_gtk_paint_focus)libgtk.resolve("gtk_paint_focus");
    gtk_paint_shadow = (Ptr_gtk_paint_shadow)libgtk.resolve("gtk_paint_shadow");
    gtk_paint_slider = (Ptr_gtk_paint_slider)libgtk.resolve("gtk_paint_slider");
    gtk_paint_expander = (Ptr_gtk_paint_expander)libgtk.resolve("gtk_paint_expander");
    gtk_paint_handle = (Ptr_gtk_paint_handle)libgtk.resolve("gtk_paint_handle");
    gtk_paint_option = (Ptr_gtk_paint_option)libgtk.resolve("gtk_paint_option");
    gtk_paint_arrow = (Ptr_gtk_paint_arrow)libgtk.resolve("gtk_paint_arrow");
    gtk_paint_box_gap = (Ptr_gtk_paint_box_gap)libgtk.resolve("gtk_paint_box_gap");
    gtk_paint_extension = (Ptr_gtk_paint_extension)libgtk.resolve("gtk_paint_extension");
    gtk_paint_hline = (Ptr_gtk_paint_hline)libgtk.resolve("gtk_paint_hline");
    gtk_paint_vline = (Ptr_gtk_paint_vline)libgtk.resolve("gtk_paint_vline");
    gtk_adjustment_configure = (Ptr_gtk_adjustment_configure)libgtk.resolve("gtk_adjustment_configure");
    gtk_adjustment_new = (Ptr_gtk_adjustment_new)libgtk.resolve("gtk_adjustment_new");
    gtk_menu_item_set_submenu = (Ptr_gtk_menu_item_set_submenu)libgtk.resolve("gtk_menu_item_set_submenu");
    gtk_settings_get_default = (Ptr_gtk_settings_get_default)libgtk.resolve("gtk_settings_get_default");
    gtk_separator_menu_item_new = (Ptr_gtk_separator_menu_item_new)libgtk.resolve("gtk_separator_menu_item_new");
    gtk_frame_new = (Ptr_gtk_frame_new)libgtk.resolve("gtk_frame_new");
    gtk_expander_new = (Ptr_gtk_expander_new)libgtk.resolve("gtk_expander_new");
    gtk_statusbar_new = (Ptr_gtk_statusbar_new)libgtk.resolve("gtk_statusbar_new");
    gtk_combo_box_entry_new = (Ptr_gtk_combo_box_entry_new)libgtk.resolve("gtk_combo_box_entry_new");
    gtk_container_forall = (Ptr_gtk_container_forall)libgtk.resolve("gtk_container_forall");
    gtk_widget_size_allocate =(Ptr_gtk_widget_size_allocate)libgtk.resolve("gtk_widget_size_allocate");
    gtk_widget_size_request =(Ptr_gtk_widget_size_request)libgtk.resolve("gtk_widget_size_request");
    gtk_widget_set_direction =(Ptr_gtk_widget_set_direction)libgtk.resolve("gtk_widget_set_direction");
    gtk_widget_path =(Ptr_gtk_widget_path)libgtk.resolve("gtk_widget_path");
    gtk_container_get_type =(Ptr_gtk_container_get_type)libgtk.resolve("gtk_container_get_type");
    gtk_window_get_type =(Ptr_gtk_window_get_type)libgtk.resolve("gtk_window_get_type");
    gtk_widget_get_type =(Ptr_gtk_widget_get_type)libgtk.resolve("gtk_widget_get_type");

    gtk_rc_get_style_by_paths =(Ptr_gtk_rc_get_style_by_paths)libgtk.resolve("gtk_rc_get_style_by_paths");
    gtk_check_version =(Ptr_gtk_check_version)libgtk.resolve("gtk_check_version");
    gtk_border_free =(Ptr_gtk_border_free)libgtk.resolve("gtk_border_free");
    pango_font_description_get_size = (Ptr_pango_font_description_get_size)libgtk.resolve("pango_font_description_get_size");
    pango_font_description_get_weight = (Ptr_pango_font_description_get_weight)libgtk.resolve("pango_font_description_get_weight");
    pango_font_description_get_family = (Ptr_pango_font_description_get_family)libgtk.resolve("pango_font_description_get_family");
    pango_font_description_get_style = (Ptr_pango_font_description_get_style)libgtk.resolve("pango_font_description_get_style");

    gnome_icon_lookup_sync = (Ptr_gnome_icon_lookup_sync)QLibrary::resolve(QLS("gnomeui-2"), 0, "gnome_icon_lookup_sync");
    gnome_vfs_init= (Ptr_gnome_vfs_init)QLibrary::resolve(QLS("gnomevfs-2"), 0, "gnome_vfs_init");
}

/* \internal
 * Initializes a number of gtk menu widgets.
 * The widgets are cached.
 */
void QGtkStylePrivate::initGtkMenu() const
{
    // Create menubar
    GtkWidget *gtkMenuBar = QGtkStylePrivate::gtk_menu_bar_new();
    setupGtkWidget(gtkMenuBar);

    GtkWidget *gtkMenuBarItem = QGtkStylePrivate::gtk_menu_item_new_with_label("X");
    gtk_menu_shell_append((GtkMenuShell*)(gtkMenuBar), gtkMenuBarItem);
    gtk_widget_realize(gtkMenuBarItem);

    // Create menu
    GtkWidget *gtkMenu = QGtkStylePrivate::gtk_menu_new();
    gtk_menu_item_set_submenu((GtkMenuItem*)(gtkMenuBarItem), gtkMenu);
    gtk_widget_realize(gtkMenu);

    GtkWidget *gtkMenuItem = QGtkStylePrivate::gtk_menu_item_new_with_label("X");
    gtk_menu_shell_append((GtkMenuShell*)gtkMenu, gtkMenuItem);
    gtk_widget_realize(gtkMenuItem);

    GtkWidget *gtkCheckMenuItem = QGtkStylePrivate::gtk_check_menu_item_new_with_label("X");
    gtk_menu_shell_append((GtkMenuShell*)gtkMenu, gtkCheckMenuItem);
    gtk_widget_realize(gtkCheckMenuItem);

    GtkWidget *gtkMenuSeparator = QGtkStylePrivate::gtk_separator_menu_item_new();
    gtk_menu_shell_append((GtkMenuShell*)gtkMenu, gtkMenuSeparator);

    addAllSubWidgets(gtkMenuBar);
    addAllSubWidgets(gtkMenu);
}


void QGtkStylePrivate::initGtkTreeview() const
{
    GtkWidget *gtkTreeView = gtk_tree_view_new();
    gtk_tree_view_append_column((GtkTreeView*)gtkTreeView, gtk_tree_view_column_new());
    gtk_tree_view_append_column((GtkTreeView*)gtkTreeView, gtk_tree_view_column_new());
    gtk_tree_view_append_column((GtkTreeView*)gtkTreeView, gtk_tree_view_column_new());
    addWidget(gtkTreeView);
}


/* \internal
 * Initializes a number of gtk widgets that we can later on use to determine some of our styles.
 * The widgets are cached.
 */
void QGtkStylePrivate::initGtkWidgets() const
{
    // From gtkmain.c
    uid_t ruid = getuid ();
    uid_t rgid = getgid ();
    uid_t euid = geteuid ();
    uid_t egid = getegid ();
    if (ruid != euid || rgid != egid) {
        qWarning("\nThis process is currently running setuid or setgid.\nGTK+ does not allow this "
                 "therefore Qt cannot use the GTK+ integration.\nTry launching your app using \'gksudo\', "
                 "\'kdesudo\' or a similar tool.\n\n"
                 "See http://www.gtk.org/setuid.html for more information.\n");
        return;
    }

    static QString themeName;
    if (!gtkWidgetMap()->contains("GtkWindow") && themeName.isEmpty()) {
        themeName = getThemeName();

        if (themeName.isEmpty()) {
            qWarning("QGtkStyle was unable to detect the current GTK+ theme.");
            return;
        } else if (themeName == QLS("Qt") || themeName == QLS("Qt4")) {
            // Due to namespace conflicts with Qt3 and obvious recursion with Qt4,
            // we cannot support the GTK_Qt Gtk engine
            qWarning("QGtkStyle cannot be used together with the GTK_Qt engine.");
            return;
        }
    }

    if (QGtkStylePrivate::gtk_init) {
        // Gtk will set the Qt error handler so we have to reset it afterwards
        x11ErrorHandler qt_x_errhandler = XSetErrorHandler(0);
        QGtkStylePrivate::gtk_init (NULL, NULL);
        XSetErrorHandler(qt_x_errhandler);

        // make a window
        GtkWidget* gtkWindow = QGtkStylePrivate::gtk_window_new(GTK_WINDOW_POPUP);
        QGtkStylePrivate::gtk_widget_realize(gtkWindow);
        if (displayDepth == -1)
            displayDepth = QGtkStylePrivate::gdk_drawable_get_depth(gtkWindow->window);
        QHashableLatin1Literal widgetPath = QHashableLatin1Literal::fromData(strdup("GtkWindow"));
        removeWidgetFromMap(widgetPath);
        gtkWidgetMap()->insert(widgetPath, gtkWindow);


        // Make all other widgets. respect the text direction
        if (qApp->layoutDirection() == Qt::RightToLeft)
            QGtkStylePrivate::gtk_widget_set_default_direction(GTK_TEXT_DIR_RTL);

        if (!gtkWidgetMap()->contains("GtkButton")) {
            GtkWidget *gtkButton = QGtkStylePrivate::gtk_button_new();
            addWidget(gtkButton);
            g_signal_connect(gtkButton, "style-set", G_CALLBACK(gtkStyleSetCallback), 0);
            addWidget(QGtkStylePrivate::gtk_tool_button_new(NULL, "Qt"));
            addWidget(QGtkStylePrivate::gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE));
            addWidget(QGtkStylePrivate::gtk_hbutton_box_new());
            addWidget(QGtkStylePrivate::gtk_check_button_new());
            addWidget(QGtkStylePrivate::gtk_radio_button_new(NULL));
            addWidget(QGtkStylePrivate::gtk_combo_box_new());
            addWidget(QGtkStylePrivate::gtk_combo_box_entry_new());
            addWidget(QGtkStylePrivate::gtk_entry_new());
            addWidget(QGtkStylePrivate::gtk_frame_new(NULL));
            addWidget(QGtkStylePrivate::gtk_expander_new(""));
            addWidget(QGtkStylePrivate::gtk_statusbar_new());
            addWidget(QGtkStylePrivate::gtk_hscale_new((GtkAdjustment*)(QGtkStylePrivate::gtk_adjustment_new(1, 0, 1, 0, 0, 0))));
            addWidget(QGtkStylePrivate::gtk_hscrollbar_new(NULL));
            addWidget(QGtkStylePrivate::gtk_scrolled_window_new(NULL, NULL));

            initGtkMenu();
            addWidget(QGtkStylePrivate::gtk_notebook_new());
            addWidget(QGtkStylePrivate::gtk_progress_bar_new());
            addWidget(QGtkStylePrivate::gtk_spin_button_new((GtkAdjustment*)
                                             (QGtkStylePrivate::gtk_adjustment_new(1, 0, 1, 0, 0, 0)), 0.1, 3));
            GtkWidget *toolbar = gtk_toolbar_new();
            g_signal_connect (toolbar, "notify::toolbar-style", G_CALLBACK (update_toolbar_style), toolbar);
            gtk_toolbar_insert((GtkToolbar*)toolbar, gtk_separator_tool_item_new(), -1);
            addWidget(toolbar);
            initGtkTreeview();
            addWidget(gtk_vscale_new((GtkAdjustment*)(QGtkStylePrivate::gtk_adjustment_new(1, 0, 1, 0, 0, 0))));
            addWidget(gtk_vscrollbar_new(NULL));
        }
        else // Rebuild map
        {
            // When styles change subwidgets can get rearranged
            // as with the combo box. We need to update the widget map
            // to reflect this;
            QHash<QHashableLatin1Literal, GtkWidget*> oldMap = *gtkWidgetMap();
            gtkWidgetMap()->clear();
            QHashIterator<QHashableLatin1Literal, GtkWidget*> it(oldMap);
            while (it.hasNext()) {
                it.next();
                if (!strchr(it.key().data(), '.')) {
                    addAllSubWidgets(it.value());
                }
                free(const_cast<char *>(it.key().data()));
            }
        }
    } else {
        qWarning("QGtkStyle could not resolve GTK. Make sure you have installed the proper libraries.");
    }
}

/*! \internal
 * destroys all previously buffered widgets.
 */
void QGtkStylePrivate::cleanupGtkWidgets()
{
    if (!widgetMap)
        return;
    if (widgetMap->contains("GtkWindow")) // Gtk will destroy all children
        gtk_widget_destroy(widgetMap->value("GtkWindow"));
    for (QHash<QHashableLatin1Literal, GtkWidget *>::const_iterator it = widgetMap->constBegin();
         it != widgetMap->constEnd(); ++it)
        free(const_cast<char *>(it.key().data()));
}

static bool resolveGConf()
{
    if (!QGtkStylePrivate::gconf_client_get_default) {
        QGtkStylePrivate::gconf_client_get_default = (Ptr_gconf_client_get_default)QLibrary::resolve(QLS("gconf-2"), 4, "gconf_client_get_default");
        QGtkStylePrivate::gconf_client_get_string =  (Ptr_gconf_client_get_string)QLibrary::resolve(QLS("gconf-2"), 4, "gconf_client_get_string");
        QGtkStylePrivate::gconf_client_get_bool =  (Ptr_gconf_client_get_bool)QLibrary::resolve(QLS("gconf-2"), 4, "gconf_client_get_bool");
    }
    return (QGtkStylePrivate::gconf_client_get_default !=0);
}

QString QGtkStylePrivate::getGConfString(const QString &value, const QString &fallback)
{
    QString retVal = fallback;
    if (resolveGConf()) {
        g_type_init();
        GConfClient* client = gconf_client_get_default();
        GError *err = 0;
        char *str = gconf_client_get_string(client, qPrintable(value), &err);
        if (!err) {
            retVal = QString::fromUtf8(str);
            g_free(str);
        }
        g_object_unref(client);
        if (err)
            g_error_free (err);
    }
    return retVal;
}

bool QGtkStylePrivate::getGConfBool(const QString &key, bool fallback)
{
    bool retVal = fallback;
    if (resolveGConf()) {
        g_type_init();
        GConfClient* client = gconf_client_get_default();
        GError *err = 0;
        bool result = gconf_client_get_bool(client, qPrintable(key), &err);
        g_object_unref(client);
        if (!err)
            retVal = result;
        else
            g_error_free (err);
    }
    return retVal;
}

QString QGtkStylePrivate::getThemeName()
{
    QString themeName;
    // We try to parse the gtkrc file first
    // primarily to avoid resolving Gtk functions if
    // the KDE 3 "Qt" style is currently in use
    QString rcPaths = QString::fromLocal8Bit(qgetenv("GTK2_RC_FILES"));
    if (!rcPaths.isEmpty()) {
        QStringList paths = rcPaths.split(QLS(":"));
        foreach (const QString &rcPath, paths) {
            if (!rcPath.isEmpty()) {
                QFile rcFile(rcPath);
                if (rcFile.exists() && rcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&rcFile);
                    while(!in.atEnd()) {
                        QString line = in.readLine();
                        if (line.contains(QLS("gtk-theme-name"))) {
                            line = line.right(line.length() - line.indexOf(QLatin1Char('=')) - 1);
                            line.remove(QLatin1Char('\"'));
                            line = line.trimmed();
                            themeName = line;
                            break;
                        }
                    }
                }
            }
            if (!themeName.isEmpty())
                break;
        }
    }

    // Fall back to gconf
    if (themeName.isEmpty() && resolveGConf())
        themeName = getGConfString(QLS("/desktop/gnome/interface/gtk_theme"));

    return themeName;
}

// Get size of the arrow controls in a GtkSpinButton
int QGtkStylePrivate::getSpinboxArrowSize() const
{
    const int MIN_ARROW_WIDTH = 6;
    GtkWidget *spinButton = gtkWidget("GtkSpinButton");
    GtkStyle *style = spinButton->style;
    gint size = pango_font_description_get_size (style->font_desc);
    gint arrow_size;
    arrow_size = qMax(PANGO_PIXELS (size), MIN_ARROW_WIDTH) + style->xthickness;
    arrow_size += arrow_size%2 + 1;
    return arrow_size;
}


bool QGtkStylePrivate::isKDE4Session()
{
    static int version = -1;
    if (version == -1)
        version = qgetenv("KDE_SESSION_VERSION").toInt();
    return (version == 4);
}

void QGtkStylePrivate::applyCustomPaletteHash()
{
    QPalette menuPal = gtkWidgetPalette("GtkMenu");
    GdkColor gdkBg = gtkWidget("GtkMenu")->style->bg[GTK_STATE_NORMAL];
    QColor bgColor(gdkBg.red>>8, gdkBg.green>>8, gdkBg.blue>>8);
    menuPal.setBrush(QPalette::Base, bgColor);
    menuPal.setBrush(QPalette::Window, bgColor);
    qApp->setPalette(menuPal, "QMenu");

    QPalette toolbarPal = gtkWidgetPalette("GtkToolbar");
    qApp->setPalette(toolbarPal, "QToolBar");

    QPalette menuBarPal = gtkWidgetPalette("GtkMenuBar");
    qApp->setPalette(menuBarPal, "QMenuBar");
}

/*! \internal
 *  Returns the gtk Widget that should be used to determine text foreground and background colors.
*/
GtkWidget* QGtkStylePrivate::getTextColorWidget() const
{
    return  gtkWidget("GtkEntry");
}

void QGtkStylePrivate::setupGtkWidget(GtkWidget* widget)
{
    if (Q_GTK_IS_WIDGET(widget)) {
        static GtkWidget* protoLayout = 0;
        if (!protoLayout) {
            protoLayout = QGtkStylePrivate::gtk_fixed_new();
            QGtkStylePrivate::gtk_container_add((GtkContainer*)(gtkWidgetMap()->value("GtkWindow")), protoLayout);
        }
        Q_ASSERT(protoLayout);

        if (!widget->parent && !GTK_WIDGET_TOPLEVEL(widget))
            QGtkStylePrivate::gtk_container_add((GtkContainer*)(protoLayout), widget);
        QGtkStylePrivate::gtk_widget_realize(widget);
    }
}

void QGtkStylePrivate::removeWidgetFromMap(const QHashableLatin1Literal &path)
{
    WidgetMap *map = gtkWidgetMap();
    WidgetMap::iterator it = map->find(path);
    if (it != map->end()) {
        free(const_cast<char *>(it.key().data()));
        map->erase(it);
    }
}

void QGtkStylePrivate::addWidgetToMap(GtkWidget *widget)
{
    if (Q_GTK_IS_WIDGET(widget)) {
        gtk_widget_realize(widget);
        QHashableLatin1Literal widgetPath = classPath(widget);

        removeWidgetFromMap(widgetPath);
        gtkWidgetMap()->insert(widgetPath, widget);
#ifdef DUMP_GTK_WIDGET_TREE
        qWarning("Inserted Gtk Widget: %s", widgetPath.data());
#endif
    }
 }

void QGtkStylePrivate::addAllSubWidgets(GtkWidget *widget, gpointer v)
{
    Q_UNUSED(v);
    addWidgetToMap(widget);
    if (GTK_CHECK_TYPE ((widget), gtk_container_get_type()))
        gtk_container_forall((GtkContainer*)widget, addAllSubWidgets, NULL);
}

// Updates window/windowtext palette based on the indicated gtk widget
QPalette QGtkStylePrivate::gtkWidgetPalette(const QHashableLatin1Literal &gtkWidgetName) const
{
    GtkWidget *gtkWidget = QGtkStylePrivate::gtkWidget(gtkWidgetName);
    Q_ASSERT(gtkWidget);
    QPalette pal = QApplication::palette();
    GdkColor gdkBg = gtkWidget->style->bg[GTK_STATE_NORMAL];
    GdkColor gdkText = gtkWidget->style->fg[GTK_STATE_NORMAL];
    GdkColor gdkDisabledText = gtkWidget->style->fg[GTK_STATE_INSENSITIVE];
    QColor bgColor(gdkBg.red>>8, gdkBg.green>>8, gdkBg.blue>>8);
    QColor textColor(gdkText.red>>8, gdkText.green>>8, gdkText.blue>>8);
    QColor disabledTextColor(gdkDisabledText.red>>8, gdkDisabledText.green>>8, gdkDisabledText.blue>>8);
    pal.setBrush(QPalette::Window, bgColor);
    pal.setBrush(QPalette::Button, bgColor);
    pal.setBrush(QPalette::All, QPalette::WindowText, textColor);
    pal.setBrush(QPalette::Disabled, QPalette::WindowText, disabledTextColor);
    pal.setBrush(QPalette::All, QPalette::ButtonText, textColor);
    pal.setBrush(QPalette::Disabled, QPalette::ButtonText, disabledTextColor);
    return pal;
}


void QGtkStyleUpdateScheduler::updateTheme()
{
    static QString oldTheme(QLS("qt_not_set"));
    QPixmapCache::clear();

    QFont font = QGtkStylePrivate::getThemeFont();
    if (QApplication::font() != font)
        qApp->setFont(font);

      if (oldTheme != QGtkStylePrivate::getThemeName()) {
          oldTheme = QGtkStylePrivate::getThemeName();
          QPalette newPalette = qApp->style()->standardPalette();
          QApplicationPrivate::setSystemPalette(newPalette);
          QApplication::setPalette(newPalette);
          if (!QGtkStylePrivate::instances.isEmpty()) {
              QGtkStylePrivate::instances.last()->initGtkWidgets();
              QGtkStylePrivate::instances.last()->applyCustomPaletteHash();
          }
          QList<QWidget*> widgets = QApplication::allWidgets();
          // Notify all widgets that size metrics might have changed
          foreach (QWidget *widget, widgets) {
              QEvent e(QEvent::StyleChange);
              QApplication::sendEvent(widget, &e);
          }
      }
    QIconLoader::instance()->updateSystemTheme();
}

void QGtkStylePrivate::addWidget(GtkWidget *widget)
{
    if (widget) {
        setupGtkWidget(widget);
        addAllSubWidgets(widget);
    }
}


// Fetch the application font from the pango font description
// contained in the theme.
QFont QGtkStylePrivate::getThemeFont()
{
    QFont font;
    GtkStyle *style = gtkStyle();
    if (style && qApp->desktopSettingsAware())
    {
        PangoFontDescription *gtk_font = style->font_desc;
        font.setPointSizeF((float)(pango_font_description_get_size(gtk_font))/PANGO_SCALE);

        QString family = QString::fromLatin1(pango_font_description_get_family(gtk_font));
        if (!family.isEmpty())
            font.setFamily(family);

        int weight = pango_font_description_get_weight(gtk_font);
        if (weight >= PANGO_WEIGHT_HEAVY)
            font.setWeight(QFont::Black);
        else if (weight >= PANGO_WEIGHT_BOLD)
            font.setWeight(QFont::Bold);
        else if (weight >= PANGO_WEIGHT_SEMIBOLD)
            font.setWeight(QFont::DemiBold);
        else if (weight >= PANGO_WEIGHT_NORMAL)
            font.setWeight(QFont::Normal);
        else
            font.setWeight(QFont::Light);

        PangoStyle fontstyle = pango_font_description_get_style(gtk_font);
        if (fontstyle == PANGO_STYLE_ITALIC)
            font.setStyle(QFont::StyleItalic);
        else if (fontstyle == PANGO_STYLE_OBLIQUE)
            font.setStyle(QFont::StyleOblique);
        else
            font.setStyle(QFont::StyleNormal);
    }
    return font;
}


// ----------- Native file dialogs -----------

// Extract filter list from expressions of type: foo (*.a *.b *.c)"
QStringList QGtkStylePrivate::extract_filter(const QString &rawFilter)
{
    QString result = rawFilter;
    QRegExp r(QString::fromLatin1("^([^()]*)\\(([a-zA-Z0-9_.*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$"));
    int index = r.indexIn(result);
    if (index >= 0)
        result = r.cap(2);
    return result.split(QLatin1Char(' '));
}

extern QStringList qt_make_filter_list(const QString &filter);

void QGtkStylePrivate::setupGtkFileChooser(GtkWidget* gtkFileChooser, QWidget *parent,
                                const QString &dir, const QString &filter, QString *selectedFilter,
                                QFileDialog::Options options, bool isSaveDialog,
                                QMap<GtkFileFilter *, QString> *filterMap)
{
    g_object_set(gtkFileChooser, "do-overwrite-confirmation", gboolean(!(options & QFileDialog::DontConfirmOverwrite)), NULL);
    g_object_set(gtkFileChooser, "local_only", gboolean(true), NULL);
    if (!filter.isEmpty()) {
        QStringList filters = qt_make_filter_list(filter);
        foreach (const QString &rawfilter, filters) {
            GtkFileFilter *gtkFilter = QGtkStylePrivate::gtk_file_filter_new ();
            QString name = rawfilter.left(rawfilter.indexOf(QLatin1Char('(')));
            QStringList extensions = extract_filter(rawfilter);
            QGtkStylePrivate::gtk_file_filter_set_name(gtkFilter, qPrintable(name.isEmpty() ? extensions.join(QLS(", ")) : name));

            foreach (const QString &fileExtension, extensions) {
                // Note Gtk file dialogs are by default case sensitive
                // and only supports basic glob syntax so we
                // rewrite .xyz to .[xX][yY][zZ]
                QString caseInsensitive;
                for (int i = 0 ; i < fileExtension.length() ; ++i) {
                    QChar ch = fileExtension.at(i);
                    if (ch.isLetter()) {
                        caseInsensitive.append(
                                QLatin1Char('[') +
                                ch.toLower() +
                                ch.toUpper() +
                                QLatin1Char(']'));
                    } else {
                        caseInsensitive.append(ch);
                    }
                }
                QGtkStylePrivate::gtk_file_filter_add_pattern (gtkFilter, qPrintable(caseInsensitive));

            }
            if (filterMap)
                filterMap->insert(gtkFilter, rawfilter);
            QGtkStylePrivate::gtk_file_chooser_add_filter((GtkFileChooser*)gtkFileChooser, gtkFilter);
            if (selectedFilter && (rawfilter == *selectedFilter))
                QGtkStylePrivate::gtk_file_chooser_set_filter((GtkFileChooser*)gtkFileChooser, gtkFilter);
        }
    }

    // Using the currently active window is not entirely correct, however
    // it gives more sensible behavior for applications that do not provide a
    // parent
    QWidget *modalFor = parent ? parent->window() : qApp->activeWindow();
    if (modalFor) {
        QGtkStylePrivate::gtk_widget_realize(gtkFileChooser); // Creates X window
        XSetTransientForHint(QGtkStylePrivate::gdk_x11_drawable_get_xdisplay(gtkFileChooser->window),
                             QGtkStylePrivate::gdk_x11_drawable_get_xid(gtkFileChooser->window),
                             modalFor->winId());
        QGtkStylePrivate::gdk_x11_window_set_user_time (gtkFileChooser->window, QX11Info::appUserTime());

    }

    QFileInfo fileinfo(dir);
    if (dir.isEmpty())
        fileinfo.setFile(QDir::currentPath());
    fileinfo.makeAbsolute();
    if (fileinfo.isDir()) {
        QGtkStylePrivate::gtk_file_chooser_set_current_folder((GtkFileChooser*)gtkFileChooser, qPrintable(dir));
    } else if (isSaveDialog) {
        QGtkStylePrivate::gtk_file_chooser_set_current_folder((GtkFileChooser*)gtkFileChooser, qPrintable(fileinfo.absolutePath()));
        QGtkStylePrivate::gtk_file_chooser_set_current_name((GtkFileChooser*)gtkFileChooser, qPrintable(fileinfo.fileName()));
    } else {
        QGtkStylePrivate::gtk_file_chooser_set_filename((GtkFileChooser*)gtkFileChooser, qPrintable(dir));
    }
}

QString QGtkStylePrivate::openFilename(QWidget *parent, const QString &caption, const QString &dir, const QString &filter,
                            QString *selectedFilter, QFileDialog::Options options)
{
    QMap<GtkFileFilter *, QString> filterMap;
    GtkWidget *gtkFileChooser = QGtkStylePrivate::gtk_file_chooser_dialog_new (qPrintable(caption),
                                                             NULL,
                                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                             NULL);

    setupGtkFileChooser(gtkFileChooser, parent, dir, filter, selectedFilter, options, false, &filterMap);

    QWidget modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    QString filename;
    if (QGtkStylePrivate::gtk_dialog_run ((GtkDialog*)gtkFileChooser) == GTK_RESPONSE_ACCEPT) {
        char *gtk_filename = QGtkStylePrivate::gtk_file_chooser_get_filename ((GtkFileChooser*)gtkFileChooser);
        filename = QString::fromUtf8(gtk_filename);
        g_free (gtk_filename);
        if (selectedFilter) {
            GtkFileFilter *gtkFilter = QGtkStylePrivate::gtk_file_chooser_get_filter ((GtkFileChooser*)gtkFileChooser);
            *selectedFilter = filterMap.value(gtkFilter);
        }
    }

    QApplicationPrivate::leaveModal(&modal_widget);
    gtk_widget_destroy (gtkFileChooser);
    return filename;
}


QString QGtkStylePrivate::openDirectory(QWidget *parent, const QString &caption, const QString &dir, QFileDialog::Options options)
{
    QMap<GtkFileFilter *, QString> filterMap;
    GtkWidget *gtkFileChooser = QGtkStylePrivate::gtk_file_chooser_dialog_new (qPrintable(caption),
                                                             NULL,
                                                             GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                             NULL);

    setupGtkFileChooser(gtkFileChooser, parent, dir, QString(), 0, options);
    QWidget modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    QString filename;
    if (QGtkStylePrivate::gtk_dialog_run ((GtkDialog*)gtkFileChooser) == GTK_RESPONSE_ACCEPT) {
        char *gtk_filename = QGtkStylePrivate::gtk_file_chooser_get_filename ((GtkFileChooser*)gtkFileChooser);
        filename = QString::fromUtf8(gtk_filename);
        g_free (gtk_filename);
    }

    QApplicationPrivate::leaveModal(&modal_widget);
    gtk_widget_destroy (gtkFileChooser);
    return filename;
}

QStringList QGtkStylePrivate::openFilenames(QWidget *parent, const QString &caption, const QString &dir, const QString &filter,
                                 QString *selectedFilter, QFileDialog::Options options)
{
    QStringList filenames;
    QMap<GtkFileFilter *, QString> filterMap;
    GtkWidget *gtkFileChooser = QGtkStylePrivate::gtk_file_chooser_dialog_new (qPrintable(caption),
                                                             NULL,
                                                             GTK_FILE_CHOOSER_ACTION_OPEN,
                                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                             GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                                             NULL);

    setupGtkFileChooser(gtkFileChooser, parent, dir, filter, selectedFilter, options, false, &filterMap);
    g_object_set(gtkFileChooser, "select-multiple", gboolean(true), NULL);

    QWidget modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    if (gtk_dialog_run ((GtkDialog*)gtkFileChooser) == GTK_RESPONSE_ACCEPT) {
        GSList *gtk_file_names = QGtkStylePrivate::gtk_file_chooser_get_filenames((GtkFileChooser*)gtkFileChooser);
        for (GSList *iterator  = gtk_file_names ; iterator; iterator = iterator->next)
            filenames << QString::fromUtf8((const char*)iterator->data);
        g_slist_free(gtk_file_names);
        if (selectedFilter) {
            GtkFileFilter *gtkFilter = QGtkStylePrivate::gtk_file_chooser_get_filter ((GtkFileChooser*)gtkFileChooser);
            *selectedFilter = filterMap.value(gtkFilter);
        }
    }

    QApplicationPrivate::leaveModal(&modal_widget);
    gtk_widget_destroy (gtkFileChooser);
    return filenames;
}

QString QGtkStylePrivate::saveFilename(QWidget *parent, const QString &caption, const QString &dir, const QString &filter,
                           QString *selectedFilter, QFileDialog::Options options)
{
    QMap<GtkFileFilter *, QString> filterMap;
    GtkWidget *gtkFileChooser = QGtkStylePrivate::gtk_file_chooser_dialog_new (qPrintable(caption),
                                                             NULL,
                                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                             GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                                             NULL);
    setupGtkFileChooser(gtkFileChooser, parent, dir, filter, selectedFilter, options, true, &filterMap);

    QWidget modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    QString filename;
    if (QGtkStylePrivate::gtk_dialog_run ((GtkDialog*)gtkFileChooser) == GTK_RESPONSE_ACCEPT) {
        char *gtk_filename = QGtkStylePrivate::gtk_file_chooser_get_filename ((GtkFileChooser*)gtkFileChooser);
        filename = QString::fromUtf8(gtk_filename);
        g_free (gtk_filename);
        if (selectedFilter) {
            GtkFileFilter *gtkFilter = QGtkStylePrivate::gtk_file_chooser_get_filter ((GtkFileChooser*)gtkFileChooser);
            *selectedFilter = filterMap.value(gtkFilter);
        }
    }

    QApplicationPrivate::leaveModal(&modal_widget);
    gtk_widget_destroy (gtkFileChooser);
    return filename;
}

QIcon QGtkStylePrivate::getFilesystemIcon(const QFileInfo &info)
{
    QIcon icon;
    if (gnome_vfs_init && gnome_icon_lookup_sync) {
        gnome_vfs_init();
        GtkIconTheme *theme = gtk_icon_theme_get_default();
        QByteArray fileurl = QUrl::fromLocalFile(info.absoluteFilePath()).toEncoded();
        char * icon_name = gnome_icon_lookup_sync(theme,
                                                  NULL,
                                                  fileurl.data(),
                                                  NULL,
                                                  GNOME_ICON_LOOKUP_FLAGS_NONE,
                                                  NULL);
        QString iconName = QString::fromUtf8(icon_name);
        g_free(icon_name);
        if (iconName.startsWith(QLatin1Char('/')))
            return QIcon(iconName);
        return QIcon::fromTheme(iconName);
    }
    return icon;
}

bool operator==(const QHashableLatin1Literal &l1, const QHashableLatin1Literal &l2)
{
    return l1.size() == l2.size() || qstrcmp(l1.data(), l2.data()) == 0;
}

// copied from qHash.cpp
uint qHash(const QHashableLatin1Literal &key)
{
    int n = key.size();
    const uchar *p = reinterpret_cast<const uchar *>(key.data());
    uint h = 0;
    uint g;

    while (n--) {
        h = (h << 4) + *p++;
        if ((g = (h & 0xf0000000)) != 0)
            h ^= g >> 23;
        h &= ~g;
    }
    return h;
}

QT_END_NAMESPACE

#endif // !defined(QT_NO_STYLE_GTK)
