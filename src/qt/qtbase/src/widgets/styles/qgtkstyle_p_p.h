/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QGTKSTYLE_P_P_H
#define QGTKSTYLE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#if !defined(QT_NO_STYLE_GTK)

#include <QtCore/qstring.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qcoreapplication.h>

#include <QtWidgets/QFileDialog>

#include <private/qgtkstyle_p.h>
#include <private/qcommonstyle_p.h>
#include <private/qgtkglobal_p.h>

#define Q_GTK_IS_WIDGET(widget) widget && G_TYPE_CHECK_INSTANCE_TYPE ((widget), QGtkStylePrivate::gtk_widget_get_type())

QT_BEGIN_NAMESPACE

class QHashableLatin1Literal
{
public:
    int size() const { return m_size; }
    const char *data() const { return m_data; }

#ifdef __SUNPRO_CC
        QHashableLatin1Literal(const char* str)
        : m_size(strlen(str)), m_data(str) {}
#else
    template <int N>
        QHashableLatin1Literal(const char (&str)[N])
        : m_size(N - 1), m_data(str) {}
#endif

    QHashableLatin1Literal(const QHashableLatin1Literal &other)
        : m_size(other.m_size), m_data(other.m_data)
    {}

    QHashableLatin1Literal &operator=(const QHashableLatin1Literal &other)
    {
        if (this == &other)
            return *this;
        *const_cast<int *>(&m_size) = other.m_size;
        *const_cast<char **>(&m_data) = const_cast<char *>(other.m_data);
        return *this;
    }

    QString toString() const { return QString::fromLatin1(m_data, m_size); }

    static QHashableLatin1Literal fromData(const char *str)
    {
        return QHashableLatin1Literal(str, qstrlen(str));
    }

private:
    QHashableLatin1Literal(const char *str, int length)
        : m_size(length), m_data(str)
    {}

    const int m_size;
    const char *m_data;
};

bool operator==(const QHashableLatin1Literal &l1, const QHashableLatin1Literal &l2);
inline bool operator!=(const QHashableLatin1Literal &l1, const QHashableLatin1Literal &l2) { return !operator==(l1, l2); }
uint qHash(const QHashableLatin1Literal &key);

QT_END_NAMESPACE

class GConf;
class GConfClient;
typedef struct _XDisplay Display;

typedef GConfClient* (*Ptr_gconf_client_get_default)();
typedef char* (*Ptr_gconf_client_get_string)(GConfClient*, const char*, GError **);
typedef bool (*Ptr_gconf_client_get_bool)(GConfClient*, const char*, GError **);

typedef void (*Ptr_gtk_init)(int *, char ***);
typedef GtkWidget* (*Ptr_gtk_window_new) (GtkWindowType);
typedef GtkStyle* (*Ptr_gtk_style_attach)(GtkStyle *, GdkWindow *);
typedef void (*Ptr_gtk_widget_destroy) (GtkWidget *);
typedef void (*Ptr_gtk_widget_realize) (GtkWidget *);
typedef void (*Ptr_gtk_widget_set_default_direction) (GtkTextDirection);
typedef void (*Ptr_gtk_widget_modify_color)(GtkWidget *widget, GtkStateType state, const GdkColor *color);
typedef GtkWidget* (*Ptr_gtk_arrow_new)(GtkArrowType, GtkShadowType);
typedef GtkWidget* (*Ptr_gtk_menu_item_new_with_label)(const gchar *);
typedef GtkWidget* (*Ptr_gtk_separator_menu_item_new)(void);
typedef GtkWidget* (*Ptr_gtk_check_menu_item_new_with_label)(const gchar *);
typedef GtkWidget* (*Ptr_gtk_menu_bar_new)(void);
typedef GtkWidget* (*Ptr_gtk_menu_new)(void);
typedef GtkWidget* (*Ptr_gtk_combo_box_new)(void);
typedef GtkWidget* (*Ptr_gtk_combo_box_entry_new)(void);
typedef GtkWidget* (*Ptr_gtk_toolbar_new)(void);
typedef GtkWidget* (*Ptr_gtk_spin_button_new)(GtkAdjustment*, double, int);
typedef GtkWidget* (*Ptr_gtk_button_new)(void);
typedef GtkWidget* (*Ptr_gtk_tool_button_new)(GtkWidget *, const gchar *);
typedef GtkWidget* (*Ptr_gtk_hbutton_box_new)(void);
typedef GtkWidget* (*Ptr_gtk_check_button_new)(void);
typedef GtkWidget* (*Ptr_gtk_radio_button_new)(GSList *);
typedef GtkWidget* (*Ptr_gtk_notebook_new)(void);
typedef GtkWidget* (*Ptr_gtk_progress_bar_new)(void);
typedef GtkWidget* (*Ptr_gtk_hscale_new)(GtkAdjustment*);
typedef GtkWidget* (*Ptr_gtk_vscale_new)(GtkAdjustment*);
typedef GtkWidget* (*Ptr_gtk_hscrollbar_new)(GtkAdjustment*);
typedef GtkWidget* (*Ptr_gtk_vscrollbar_new)(GtkAdjustment*);
typedef GtkWidget* (*Ptr_gtk_scrolled_window_new)(GtkAdjustment*, GtkAdjustment*);
typedef gchar* (*Ptr_gtk_check_version)(guint, guint, guint);
typedef GtkToolItem* (*Ptr_gtk_separator_tool_item_new) (void);
typedef GtkWidget* (*Ptr_gtk_entry_new)(void);
typedef GtkWidget* (*Ptr_gtk_tree_view_new)(void);
typedef GtkTreeViewColumn* (*Ptr_gtk_tree_view_get_column)(GtkTreeView *, gint);
typedef GtkWidget* (*Ptr_gtk_frame_new)(const gchar *);
typedef GtkWidget* (*Ptr_gtk_expander_new)(const gchar*);
typedef GtkWidget* (*Ptr_gtk_statusbar_new)(void);
typedef GtkSettings* (*Ptr_gtk_settings_get_default)(void);
typedef GtkAdjustment* (*Ptr_gtk_range_get_adjustment)(GtkRange *);
typedef void (*Ptr_gtk_range_set_adjustment)(GtkRange *, GtkAdjustment *);
typedef void (*Ptr_gtk_range_set_inverted)(GtkRange*, bool);
typedef void (*Ptr_gtk_container_add)(GtkContainer *container, GtkWidget *widget);
typedef GtkIconSet* (*Ptr_gtk_icon_factory_lookup_default) (const gchar*);
typedef GtkIconTheme* (*Ptr_gtk_icon_theme_get_default) (void);
typedef GtkStyle* (*Ptr_gtk_widget_get_style)(GtkWidget *);
typedef void (*Ptr_gtk_widget_style_get)(GtkWidget *, const gchar *first_property_name, ...);
typedef GtkTreeViewColumn* (*Ptr_gtk_tree_view_column_new)(void);
typedef GtkWidget* (*Ptr_gtk_fixed_new)(void);
typedef GdkPixbuf* (*Ptr_gtk_icon_set_render_icon)(GtkIconSet *, GtkStyle *, GtkTextDirection, GtkStateType, GtkIconSize, GtkWidget *,const char *);
typedef void (*Ptr_gtk_tree_view_append_column) (GtkTreeView*, GtkTreeViewColumn*);
typedef void (*Ptr_gtk_adjustment_configure) (GtkAdjustment *, double, double, double, double, double, double);
typedef GtkAdjustment* (*Ptr_gtk_adjustment_new) (double, double, double, double, double, double);
typedef void (*Ptr_gtk_menu_item_set_submenu) (GtkMenuItem *, GtkWidget *);
typedef void (*Ptr_gtk_container_forall) (GtkContainer *, GtkCallback, gpointer);
typedef void (*Ptr_gtk_widget_size_allocate) (GtkWidget *, GtkAllocation*);
typedef void (*Ptr_gtk_widget_size_request) (GtkWidget *widget, GtkRequisition *requisition);
typedef void (*Ptr_gtk_widget_set_direction) (GtkWidget *, GtkTextDirection);
typedef void (*Ptr_gtk_widget_path) (GtkWidget *, guint *, gchar **, gchar**);

typedef void (*Ptr_gtk_toolbar_insert) (GtkToolbar *toolbar, GtkToolItem *item, int pos);
typedef void (*Ptr_gtk_menu_shell_append)(GtkMenuShell *, GtkWidget *);
typedef GType (*Ptr_gtk_container_get_type) (void);
typedef GType (*Ptr_gtk_window_get_type) (void);
typedef GType (*Ptr_gtk_widget_get_type) (void);
typedef GtkWidget* (*Ptr_gtk_widget_get_parent) (GtkWidget *);
typedef gboolean (*Ptr_gtk_widget_is_toplevel) (GtkWidget *);
typedef GtkWidget* (*Ptr_gtk_widget_get_toplevel) (GtkWidget *);
typedef GtkStyle* (*Ptr_gtk_rc_get_style_by_paths) (GtkSettings *, const char *, const char *, GType);
typedef gint (*Ptr_pango_font_description_get_size) (const PangoFontDescription *);
typedef PangoWeight (*Ptr_pango_font_description_get_weight) (const PangoFontDescription *);
typedef const char* (*Ptr_pango_font_description_get_family) (const PangoFontDescription *);
typedef PangoStyle (*Ptr_pango_font_description_get_style) (const PangoFontDescription *desc);
typedef void (*Ptr_gtk_border_free)(GtkBorder *);
typedef void (*Ptr_gtk_widget_get_allocation) (GtkWidget*, GtkAllocation*);
typedef void (*Ptr_gtk_widget_set_allocation) (GtkWidget*, const GtkAllocation*);

typedef void (*Ptr_gtk_widget_set_can_default) (GtkWidget*, gboolean);
typedef void (*Ptr_gtk_window_set_default) (GtkWindow*, GtkWidget*);

typedef GdkEvent* (*Ptr_gdk_event_new) (GdkEventType);
typedef void (*Ptr_gdk_event_free) (GdkEvent*);
typedef void (*Ptr_gtk_widget_send_focus_change) (GtkWidget*, GdkEvent*);

typedef guchar* (*Ptr_gdk_pixbuf_get_pixels) (const GdkPixbuf *pixbuf);
typedef int (*Ptr_gdk_pixbuf_get_width) (const GdkPixbuf *pixbuf);
typedef void (*Ptr_gdk_color_free) (const GdkColor *);
typedef int (*Ptr_gdk_pixbuf_get_height) (const GdkPixbuf *pixbuf);
typedef GdkPixbuf* (*Ptr_gdk_pixbuf_new) (GdkColorspace colorspace, gboolean has_alpha,
                                    int bits_per_sample, int width, int height);
typedef void (*Ptr_gdk_pixbuf_unref)(GdkPixbuf *);
typedef void (*Ptr_gdk_x11_window_set_user_time) (GdkWindow *window, guint32);
typedef XID  (*Ptr_gdk_x11_drawable_get_xid) (GdkDrawable *);
typedef Display* (*Ptr_gdk_x11_drawable_get_xdisplay) ( GdkDrawable *);


QT_BEGIN_NAMESPACE

class QGtkPainter;
class QGtkStylePrivate;

class QGtkStyleFilter : public QObject
{
public:
    QGtkStyleFilter(QGtkStylePrivate* sp)
        : stylePrivate(sp)
    {}
private:
    QGtkStylePrivate* stylePrivate;
    bool eventFilter(QObject *obj, QEvent *e);
};

typedef enum {
  GNOME_ICON_LOOKUP_FLAGS_NONE = 0,
  GNOME_ICON_LOOKUP_FLAGS_EMBEDDING_TEXT = 1<<0,
  GNOME_ICON_LOOKUP_FLAGS_SHOW_SMALL_IMAGES_AS_THEMSELVES = 1<<1,
  GNOME_ICON_LOOKUP_FLAGS_ALLOW_SVG_AS_THEMSELVES = 1<<2
} GnomeIconLookupFlags;

typedef enum {
  GNOME_ICON_LOOKUP_RESULT_FLAGS_NONE = 0,
  GNOME_ICON_LOOKUP_RESULT_FLAGS_THUMBNAIL = 1<<0
} GnomeIconLookupResultFlags;

struct GnomeThumbnailFactory;
typedef gboolean (*Ptr_gnome_vfs_init) (void);
typedef char* (*Ptr_gnome_icon_lookup_sync)  (
        GtkIconTheme *icon_theme,
        GnomeThumbnailFactory *,
        const char *file_uri,
        const char *custom_icon,
        GnomeIconLookupFlags flags,
        GnomeIconLookupResultFlags *result);

class QGtkStylePrivate : public QCommonStylePrivate
{
    Q_DECLARE_PUBLIC(QGtkStyle)
public:
    QGtkStylePrivate();
    ~QGtkStylePrivate();

    QGtkStyleFilter filter;

    static QGtkPainter* gtkPainter(QPainter *painter = 0);
    static GtkWidget* gtkWidget(const QHashableLatin1Literal &path);
    static GtkStyle* gtkStyle(const QHashableLatin1Literal &path = QHashableLatin1Literal("GtkWindow"));
    static void gtkWidgetSetFocus(GtkWidget *widget, bool focus);

    virtual void resolveGtk() const;
    virtual void initGtkMenu() const;
    virtual void initGtkTreeview() const;
    virtual void initGtkWidgets() const;

    static void cleanupGtkWidgets();

    static bool isKDE4Session();
    void applyCustomPaletteHash();
    static QFont getThemeFont();
    static bool isThemeAvailable() { return gtkStyle() != 0; }

    static bool getGConfBool(const QString &key, bool fallback = 0);
    static QString getGConfString(const QString &key, const QString &fallback = QString());

    static QString getThemeName();
    virtual int getSpinboxArrowSize() const;

    static QIcon getFilesystemIcon(const QFileInfo &);

    static Ptr_gtk_container_forall gtk_container_forall;
    static Ptr_gtk_init gtk_init;
    static Ptr_gtk_style_attach gtk_style_attach;
    static Ptr_gtk_window_new gtk_window_new;
    static Ptr_gtk_widget_destroy gtk_widget_destroy;
    static Ptr_gtk_widget_realize gtk_widget_realize;
    static Ptr_gtk_widget_set_default_direction gtk_widget_set_default_direction;
    static Ptr_gtk_widget_modify_color gtk_widget_modify_fg;
    static Ptr_gtk_widget_modify_color gtk_widget_modify_bg;
    static Ptr_gtk_menu_item_new_with_label gtk_menu_item_new_with_label;
    static Ptr_gtk_arrow_new gtk_arrow_new;
    static Ptr_gtk_check_menu_item_new_with_label gtk_check_menu_item_new_with_label;
    static Ptr_gtk_menu_bar_new gtk_menu_bar_new;
    static Ptr_gtk_menu_new gtk_menu_new;
    static Ptr_gtk_expander_new gtk_expander_new;
    static Ptr_gtk_button_new gtk_button_new;
    static Ptr_gtk_tool_button_new gtk_tool_button_new;
    static Ptr_gtk_hbutton_box_new gtk_hbutton_box_new;
    static Ptr_gtk_check_button_new gtk_check_button_new;
    static Ptr_gtk_radio_button_new gtk_radio_button_new;
    static Ptr_gtk_spin_button_new gtk_spin_button_new;
    static Ptr_gtk_separator_tool_item_new gtk_separator_tool_item_new;
    static Ptr_gtk_toolbar_insert gtk_toolbar_insert;
    static Ptr_gtk_frame_new gtk_frame_new;
    static Ptr_gtk_statusbar_new gtk_statusbar_new;
    static Ptr_gtk_entry_new gtk_entry_new;
    static Ptr_gtk_hscale_new gtk_hscale_new;
    static Ptr_gtk_vscale_new gtk_vscale_new;
    static Ptr_gtk_hscrollbar_new gtk_hscrollbar_new;
    static Ptr_gtk_vscrollbar_new gtk_vscrollbar_new;
    static Ptr_gtk_scrolled_window_new gtk_scrolled_window_new;
    static Ptr_gtk_notebook_new gtk_notebook_new;
    static Ptr_gtk_toolbar_new gtk_toolbar_new;
    static Ptr_gtk_tree_view_new gtk_tree_view_new;
    static Ptr_gtk_tree_view_get_column gtk_tree_view_get_column;
    static Ptr_gtk_combo_box_new gtk_combo_box_new;
    static Ptr_gtk_combo_box_entry_new gtk_combo_box_entry_new;
    static Ptr_gtk_progress_bar_new gtk_progress_bar_new;
    static Ptr_gtk_container_add gtk_container_add;
    static Ptr_gtk_menu_shell_append gtk_menu_shell_append;
    static Ptr_gtk_range_get_adjustment gtk_range_get_adjustment;
    static Ptr_gtk_range_set_adjustment gtk_range_set_adjustment;
    static Ptr_gtk_range_set_inverted gtk_range_set_inverted;
    static Ptr_gtk_icon_factory_lookup_default gtk_icon_factory_lookup_default;
    static Ptr_gtk_icon_theme_get_default gtk_icon_theme_get_default;
    static Ptr_gtk_widget_get_style gtk_widget_get_style;
    static Ptr_gtk_widget_style_get gtk_widget_style_get;
    static Ptr_gtk_icon_set_render_icon gtk_icon_set_render_icon;
    static Ptr_gtk_fixed_new gtk_fixed_new;
    static Ptr_gtk_tree_view_column_new gtk_tree_view_column_new;
    static Ptr_gtk_tree_view_append_column gtk_tree_view_append_column;
    static Ptr_gtk_adjustment_configure gtk_adjustment_configure;
    static Ptr_gtk_adjustment_new gtk_adjustment_new;
    static Ptr_gtk_menu_item_set_submenu gtk_menu_item_set_submenu;
    static Ptr_gtk_settings_get_default gtk_settings_get_default;
    static Ptr_gtk_separator_menu_item_new gtk_separator_menu_item_new;
    static Ptr_gtk_widget_size_allocate gtk_widget_size_allocate;
    static Ptr_gtk_widget_size_request gtk_widget_size_request;
    static Ptr_gtk_widget_set_direction gtk_widget_set_direction;
    static Ptr_gtk_widget_path gtk_widget_path;
    static Ptr_gtk_container_get_type gtk_container_get_type;
    static Ptr_gtk_window_get_type gtk_window_get_type;
    static Ptr_gtk_widget_get_type gtk_widget_get_type;
    static Ptr_gtk_widget_get_parent gtk_widget_get_parent;
    static Ptr_gtk_widget_is_toplevel gtk_widget_is_toplevel;
    static Ptr_gtk_widget_get_toplevel gtk_widget_get_toplevel;
    static Ptr_gtk_rc_get_style_by_paths gtk_rc_get_style_by_paths;
    static Ptr_gtk_check_version gtk_check_version;
    static Ptr_gtk_border_free gtk_border_free;
    static Ptr_gtk_widget_get_allocation gtk_widget_get_allocation;
    static Ptr_gtk_widget_set_allocation gtk_widget_set_allocation;
    static Ptr_gtk_widget_set_can_default gtk_widget_set_can_default;
    static Ptr_gtk_window_set_default gtk_window_set_default;

    static Ptr_gdk_event_new gdk_event_new;
    static Ptr_gdk_event_free gdk_event_free;
    static Ptr_gtk_widget_send_focus_change gtk_widget_send_focus_change;

    static Ptr_pango_font_description_get_size pango_font_description_get_size;
    static Ptr_pango_font_description_get_weight pango_font_description_get_weight;
    static Ptr_pango_font_description_get_family pango_font_description_get_family;
    static Ptr_pango_font_description_get_style pango_font_description_get_style;

    static Ptr_gdk_pixbuf_get_pixels gdk_pixbuf_get_pixels;
    static Ptr_gdk_pixbuf_get_width gdk_pixbuf_get_width;
    static Ptr_gdk_pixbuf_get_height gdk_pixbuf_get_height;
    static Ptr_gdk_pixbuf_new gdk_pixbuf_new;
    static Ptr_gdk_pixbuf_unref gdk_pixbuf_unref;
    static Ptr_gdk_color_free gdk_color_free;
    static Ptr_gdk_x11_window_set_user_time gdk_x11_window_set_user_time;
    static Ptr_gdk_x11_drawable_get_xid gdk_x11_drawable_get_xid;
    static Ptr_gdk_x11_drawable_get_xdisplay gdk_x11_drawable_get_xdisplay;

    static Ptr_gconf_client_get_default gconf_client_get_default;
    static Ptr_gconf_client_get_string gconf_client_get_string;
    static Ptr_gconf_client_get_bool gconf_client_get_bool;

    static Ptr_gnome_icon_lookup_sync gnome_icon_lookup_sync;
    static Ptr_gnome_vfs_init gnome_vfs_init;

    virtual QPalette gtkWidgetPalette(const QHashableLatin1Literal &gtkWidgetName) const;

protected:
    typedef QHash<QHashableLatin1Literal, GtkWidget*> WidgetMap;

    static inline void destroyWidgetMap()
    {
        cleanupGtkWidgets();
        delete widgetMap;
        widgetMap = 0;
    }

    static inline WidgetMap *gtkWidgetMap()
    {
        if (!widgetMap) {
            widgetMap = new WidgetMap();
            qAddPostRoutine(destroyWidgetMap);
        }
        return widgetMap;
    }

    static QStringList extract_filter(const QString &rawFilter);

    virtual GtkWidget* getTextColorWidget() const;
    static void setupGtkWidget(GtkWidget* widget);
    static void addWidgetToMap(GtkWidget* widget);
    static void addAllSubWidgets(GtkWidget *widget, gpointer v = 0);
    static void addWidget(GtkWidget *widget);
    static void removeWidgetFromMap(const QHashableLatin1Literal &path);

    virtual void init();

    enum {
        menuItemFrame        =  2, // menu item frame width
        menuItemHMargin      =  3, // menu item hor text margin
        menuArrowHMargin     =  6, // menu arrow horizontal margin
        menuItemVMargin      =  2, // menu item ver text margin
        menuRightBorder      = 15, // right border on menus
        menuCheckMarkWidth   = 12  // checkmarks width on menus
    };

private:
    static QList<QGtkStylePrivate *> instances;
    static WidgetMap *widgetMap;
    friend class QGtkStyleUpdateScheduler;
};

// Helper to ensure that we have polished all our gtk widgets
// before updating our own palettes
class QGtkStyleUpdateScheduler : public QObject
{
    Q_OBJECT
public slots:
    void updateTheme();
};

QT_END_NAMESPACE

#endif // !QT_NO_STYLE_GTK
#endif // QGTKSTYLE_P_P_H
