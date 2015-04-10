/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/**
 * @file    ewk_view.h
 * @brief   WebKit main smart object.
 *
 * This object allows the high level access to WebKit-EFL component.
 * It is responsible for managing the main frame and other
 * critical resources.
 *
 * Every ewk_view has at least one frame, called "main frame" and
 * retrieved with ewk_view_frame_main_get(). Direct frame access is
 * often discouraged, it is recommended to use ewk_view functions
 * instead.
 *
 * The following signals (see evas_object_smart_callback_add()) are emitted:
 *
 *  - "colorchooser,create", Ewk_Color: a new color chooser should be created.
 *  - "colorchooser,willdelete", void: reports that a previously created color
 *    chooser will be deleted.
 *  - "colorchooser,color,changed", Ewk_Color: the value at the color input widget
 *    corresponding to the color chooser has changed.
 *  - "download,request", Ewk_Download: reports a download is being requested
 *  - "editorclient,contents,changed", void: reports to the view that editor
 *    client's contents were changed
 *  - "frame,created", Evas_Object*: a new frame is created.
 *  - "icon,received", void: main frame received an icon.
 *  - "inputmethod,changed", Eina_Bool: reports that input method was changed and
 *    it gives a boolean value whether it's enabled or not as an argument.
 *  - "inspector,view,close", Evas_Object*: request to close the view for inspector.
 *  - "inspector,view,create", void: request to create the new view for inspector.
 *  - "js,windowobject,clear", void: Report that the JS window object has been cleared.
 *  - "link,hover,in", const char *link[2]: reports mouse is over a link.
 *    It gives the url in link[0] and link's title in link[1] as an argument.
 *  - "link,hover,out", void: reports mouse moved out from a link.
 *  - "load,document,finished", Evas_Object*: a DOM document object in a frame has finished loading.
 *  - "load,error", const Ewk_Frame_Load_Error*: reports load failed
 *  - "load,finished", const Ewk_Frame_Load_Error*: reports load
 *    finished and it gives @c NULL on success or pointer to
 *    structure defining the error.
 *  - "load,newwindow,show", void: reports that a new window was created and can be shown.
 *    and it gives a pointer to structure defining the error as an argument.
 *  - "load,progress", double*: load progress is changed (overall value
 *    from 0.0 to 1.0, connect to individual frames for fine grained).
 *  - "load,provisional", void: view started provisional load.
 *  - "load,provisional,failed", Ewk_Frame_Load_Error*: view provisional load failed.
 *  - "load,resource,finished", unsigned long*: reports resource load finished and it gives
 *    a pointer to its identifier.
 *  - "load,resource,failed", Ewk_Frame_Load_Error*: reports resource load failure and it
 *    gives a pointer to structure defining the error as an argument.
 *  - "load,started", Evas_Object*: frame started loading the document.
 *  - "menubar,visible,get", Eina_Bool *: expects a @c EINA_TRUE if menubar is
 *    visible; @c EINA_FALSE, otherwise.
 *  - "menubar,visible,set", Eina_Bool: sets menubar visibility.
 *  - "mixedcontent,displayed", void: any of the containing frames has loaded and displayed mixed content.
 *  - "mixedcontent,run", void: any of the containing frames has loaded and run mixed content.
 *  - "navigate,with,data", Ewk_View_Navigation_Data*: reports that view did navigation and gives the navigation details.
 *  - "perform,client,redirect", Ewk_View_Redirection_Data*: reports that view performed a client redirect and gives the redirection details.
 *  - "perform,server,redirect", Ewk_View_Redirection_Data*: reports that view performed a server redirect and gives the redirection details.
 *  - "protocolhandler,registration,requested", Ewk_Custom_Handler_Data: add a handler url for the given protocol.
 *  - "protocolhandler,isregistered", Ewk_Custom_Handler_Data: query whether the handler is registered or not.
 *  - "protocolhandler,unregistration,requested", Ewk_Custom_Handler_Data: remove a handler url for the given protocol.
 *  - "onload,event", Evas_Object*: a frame onload event has been received.
 *  - "populate,visited,links": tells the client to fill the visited links set.
 *  - "ready", void: page is fully loaded.
 *  - "resource,request,new", Ewk_Frame_Resource_Request*: reports that
 *    there's a new resource request.
 *  - "resource,request,willsend", Ewk_Frame_Resource_Messages*: a resource will be requested.
 *    and the possible redirect response.
 *  - "resource,response,received", Ewk_Frame_Resource_Response*: the network response for a resource.
 *  - "scrollbars,visible,get", Eina_Bool *: expects a @c EINA_TRUE if scrollbars
 *    are visible; @c EINA_FALSE, otherwise.
 *  - "scrollbars,visible,set", Eina_Bool: sets scrollbars visibility.
 *  - "statusbar,text,set", const char *: sets statusbar text.
 *  - "statusbar,visible,get", Eina_Bool *: expects a @c EINA_TRUE if statusbar is
 *    visible; @c EINA_FALSE, otherwise.
 *  - "statusbar,visible,set", Eina_Bool: sets statusbar visibility.
 *  - "title,changed", Ewk_Text_With_Direction*: title of the main frame was changed.
 *  - "toolbars,visible,get", Eina_Bool *: expects a @c EINA_TRUE if toolbar
 *    is visible; @c EINA_FALSE, otherwise.
 *  - "toolbars,visible,set", Eina_Bool: sets toolbar visibility.
 *  - "popup,create", Ewk_Menu: reports that a new menu was created.
 *  - "popup,willdeleted", Ewk_Menu: reports that a previously created menu
 *    will be deleted.
 *  - "restore", Evas_Object *: reports that view should be restored to default conditions
 *    and it gives a frame that originated restore as an argument.
 *  - "tooltip,text,set", const char*: tooltip was set.
 *  - "tooltip,text,unset", void: tooltip was unset.
 *  - "uri,changed", const char*: uri of the main frame was changed.
 *  - "view,resized", void: view object's size was changed.
 *  - "viewport,changed", void: reports that viewport was changed.
 *  - "zoom,animated,end", void: requested animated zoom is finished.
 */

#ifndef ewk_view_h
#define ewk_view_h

#include "ewk_contextmenu.h"
#include "ewk_file_chooser.h"
#include "ewk_frame.h"
#include "ewk_history.h"
#include "ewk_js.h"
#include "ewk_window_features.h"

#include <Evas.h>
#include <cairo.h>
#include <libsoup/soup.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Creates a type name for @a _Ewk_View_Smart_Data.
typedef struct _Ewk_View_Smart_Data Ewk_View_Smart_Data;

/// Creates a type name for a Resource Handler Callback
typedef void* (*Ewk_View_Resource_Handler_Cb)(const char *, size_t *, char **, void *);

/// Creates a type name for @a _Ewk_View_Smart_Class.
typedef struct _Ewk_View_Smart_Class Ewk_View_Smart_Class;

// Defines the direction of focus change. Keep in sync with
// WebCore::FocusDirection.
enum _Ewk_Focus_Direction {
    EWK_FOCUS_DIRECTION_FORWARD = 1,
    EWK_FOCUS_DIRECTION_BACKWARD,
};
typedef enum _Ewk_Focus_Direction Ewk_Focus_Direction;

/// Ewk view's class, to be overridden by sub-classes.
struct _Ewk_View_Smart_Class {
    Evas_Smart_Class sc; /**< All but 'data' is free to be changed. */
    unsigned long version;

    Evas_Object *(*window_create)(Ewk_View_Smart_Data *sd, Eina_Bool javascript, const Ewk_Window_Features *window_features); /**< creates a new window, requested by webkit */
    void (*window_close)(Ewk_View_Smart_Data *sd); /**< closes a window */
    // hooks to allow different backing stores
    Evas_Object *(*backing_store_add)(Ewk_View_Smart_Data *sd); /**< must be defined */
    Eina_Bool (*scrolls_process)(Ewk_View_Smart_Data *sd); /**< must be defined */
    Eina_Bool (*repaints_process)(Ewk_View_Smart_Data *sd); /**< must be defined */
    Eina_Bool (*contents_resize)(Ewk_View_Smart_Data *sd, int w, int h);
    Eina_Bool (*zoom_set)(Ewk_View_Smart_Data *sd, float zoom, Evas_Coord cx, Evas_Coord cy);
    Eina_Bool (*zoom_weak_set)(Ewk_View_Smart_Data *sd, float zoom, Evas_Coord cx, Evas_Coord cy);
    void (*zoom_weak_smooth_scale_set)(Ewk_View_Smart_Data *sd, Eina_Bool smooth_scale);
    void (*bg_color_set)(Ewk_View_Smart_Data *sd, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
    void (*flush)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*pre_render_region)(Ewk_View_Smart_Data *sd, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, float zoom);
    Eina_Bool (*pre_render_relative_radius)(Ewk_View_Smart_Data *sd, unsigned int n, float zoom);
    Eina_Bool (*pre_render_start)(Ewk_View_Smart_Data *sd);
    void (*pre_render_cancel)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*disable_render)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*enable_render)(Ewk_View_Smart_Data *sd);

    // event handling:
    //  - returns true if handled
    //  - if overridden, have to call parent method if desired
    Eina_Bool (*focus_in)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*focus_out)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*fullscreen_enter)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*fullscreen_exit)(Ewk_View_Smart_Data *sd);
    Eina_Bool (*mouse_wheel)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Wheel *ev);
    Eina_Bool (*mouse_down)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Down *ev);
    Eina_Bool (*mouse_up)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Up *ev);
    Eina_Bool (*mouse_move)(Ewk_View_Smart_Data *sd, const Evas_Event_Mouse_Move *ev);
    Eina_Bool (*key_down)(Ewk_View_Smart_Data *sd, const Evas_Event_Key_Down *ev);
    Eina_Bool (*key_up)(Ewk_View_Smart_Data *sd, const Evas_Event_Key_Up *ev);

    void (*add_console_message)(Ewk_View_Smart_Data *sd, const char *message, unsigned int lineNumber, const char *sourceID);
    void (*run_javascript_alert)(Ewk_View_Smart_Data *sd, Evas_Object *frame, const char *message);
    Eina_Bool (*run_javascript_confirm)(Ewk_View_Smart_Data *sd, Evas_Object *frame, const char *message);
    Eina_Bool (*run_before_unload_confirm)(Ewk_View_Smart_Data *sd, Evas_Object *frame, const char *message);
    Eina_Bool (*run_javascript_prompt)(Ewk_View_Smart_Data *sd, Evas_Object *frame, const char *message, const char *defaultValue, const char **value);
    Eina_Bool (*should_interrupt_javascript)(Ewk_View_Smart_Data *sd);
    int64_t (*exceeded_application_cache_quota)(Ewk_View_Smart_Data *sd, Ewk_Security_Origin* origin, int64_t defaultOriginQuota, int64_t totalSpaceNeeded);
    uint64_t (*exceeded_database_quota)(Ewk_View_Smart_Data *sd, Evas_Object *frame, const char *databaseName, uint64_t current_size, uint64_t expected_size);

    Eina_Bool (*run_open_panel)(Ewk_View_Smart_Data *sd, Evas_Object *frame, Ewk_File_Chooser *file_chooser, Eina_List **selected_filenames);

    Eina_Bool (*navigation_policy_decision)(Ewk_View_Smart_Data *sd, Ewk_Frame_Resource_Request *request, Ewk_Navigation_Type navigation_type);
    Eina_Bool (*focus_can_cycle)(Ewk_View_Smart_Data *sd, Ewk_Focus_Direction direction);
};

/**
 * The version you have to put into the version field
 * in the @a Ewk_View_Smart_Class structure.
 */
#define EWK_VIEW_SMART_CLASS_VERSION 7UL

/**
 * Initializes a whole @a Ewk_View_Smart_Class structure.
 *
 * @param smart_class_init initializer to use for the "base" field
 * @a Evas_Smart_Class
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 */
#define EWK_VIEW_SMART_CLASS_INIT(smart_class_init) {smart_class_init, EWK_VIEW_SMART_CLASS_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Initializes to zero a whole @a Ewk_View_Smart_Class structure.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NULL EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/**
 * Initializes to zero a whole @a Ewk_View_Smart_Class structure
 * and sets the version.
 *
 * Similar to @a EWK_VIEW_SMART_CLASS_INIT_NULL, but it sets the version field of
 * @a Evas_Smart_Class (base field) to latest @a EVAS_SMART_CLASS_VERSION.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_VERSION EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_VERSION)

/**
 * Initializes to zero a whole @a Ewk_View_Smart_Class structure
 * and sets the name and version.
 *
 * Similar to @a EWK_VIEW_SMART_CLASS_INIT_NULL, but it sets the version field of
 * @a Evas_Smart_Class (base field) to latest @a EVAS_SMART_CLASS_VERSION
 * and the name to the specific value.
 *
 * It will keep a reference to the name field as a "const char *", that is,
 * name must be available while the structure is used (hint: static or global!)
 * and it will not be modified.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(name) EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NAME_VERSION(name))

/// Defines the input method hints.
enum _Ewk_Imh {
    EWK_IMH_TELEPHONE = (1 << 0),
    EWK_IMH_NUMBER = (1 << 1),
    EWK_IMH_EMAIL = (1 << 2),
    EWK_IMH_URL = (1 << 3),
    EWK_IMH_PASSWORD = (1 << 4)
};
/// Creates a type name for @a _Ewk_Imh.
typedef enum _Ewk_Imh Ewk_Imh;

/// Creates a type name for @a _Ewk_View_Private_Data.
typedef struct _Ewk_View_Private_Data Ewk_View_Private_Data;

/// Defines the types of the items for the context menu.
enum _Ewk_Menu_Item_Type {
    EWK_MENU_SEPARATOR,
    EWK_MENU_GROUP,
    EWK_MENU_OPTION
};
/// Creates a type name for @a _Ewk_Menu_Item_Type.
typedef enum _Ewk_Menu_Item_Type Ewk_Menu_Item_Type;

/// Creates a type name for @a _Ewk_Menu_Item.
typedef struct _Ewk_Menu_Item Ewk_Menu_Item;
/// Contains data of each menu item.
struct _Ewk_Menu_Item {
    const char *text; /**< Text of the item. */
    Ewk_Menu_Item_Type type; /** Type of the item. */
};

/// Creates a type name for @a _Ewk_Menu.
typedef struct _Ewk_Menu Ewk_Menu;
/// Contains Popup menu data.
struct _Ewk_Menu {
        Eina_List *items; /**< List of items. */
        int x; /**< The horizontal position of Popup menu. */
        int y; /**< The vertical position of Popup menu. */
        int width; /**< Popup menu width. */
        int height; /**< Popup menu height. */
};

/// Creates a type name for @a _Ewk_Download.
typedef struct _Ewk_Download Ewk_Download;
/// Contains Download data.
struct _Ewk_Download {
    const char *url; /**< URL of resource. */
    const char *suggested_name; /**< suggested name from download attributes */
    /* to be extended */
};

/// Creates a type name for @a _Ewk_View_Navigation_Data.
typedef struct _Ewk_View_Navigation_Data Ewk_View_Navigation_Data;

/**
 * @brief Structure containing details about a view navigation.
 *
 * Details of a view navigation. It is used in "navigate,with,data" signal.
 */
struct _Ewk_View_Navigation_Data {
    const char *url;  /**< URL for the history. */
    const char *title;  /**< Title of the navigated page. */
    Ewk_Frame_Resource_Request *request;  /**< Navigation request. */
    Ewk_Frame_Resource_Response *response;  /**< Navigation response. */
    Eina_Bool has_substitute_data;  /**< Data substitution flag. */
    const char *client_redirect_source;  /**< Client redirect source URL. */
};


/// Creates a type name for @a _Ewk_View_Redirection_Data.
typedef struct _Ewk_View_Redirection_Data Ewk_View_Redirection_Data;

/**
 * @brief Structure containing details about a view redirection.
 *
 * Details of a client or server redirection. It is used in "perform,client,redirect" and "perform,server,redirect" signals.
 */
struct _Ewk_View_Redirection_Data {
    const char *source_url;  /**< Redirect source URL. */
    const char *destination_url;  /**< Redirect destination URL. */ 
};

/// Creates a type name for @a _Ewk_Color.
typedef struct _Ewk_Color Ewk_Color;
/// Represents a color using the RGBA format.
struct _Ewk_Color {
    unsigned char r; /**< Red channel. */
    unsigned char g; /**< Green channel. */
    unsigned char b; /**< Blue channel. */
    unsigned char a; /**< Alpha channel. */
};

/// Defines the handler states.
enum _Ewk_Custom_Handlers_State {
    EWK_CUSTOM_HANDLERS_NEW,
    EWK_CUSTOM_HANDLERS_REGISTERED,
    EWK_CUSTOM_HANDLERS_DECLINED
};
/// Creates a type name for @a _Ewk_Custom_Handlers_State.
typedef enum _Ewk_Custom_Handlers_State Ewk_Custom_Handlers_State;

/// Creates a type name for @a _Ewk_Custom_Handler_Data.
typedef struct _Ewk_Custom_Handler_Data Ewk_Custom_Handler_Data;
/// Contains the target scheme and the url which take care of the target.
struct _Ewk_Custom_Handler_Data {
    Evas_Object *ewkView; /**< Reference to the view object. */
    const char *scheme; /**< Reference to the scheme that will be handled. (eg. "application/x-soup") */
    const char *base_url; /**< Reference to the resolved url if the url is relative url. (eg. "https://www.example.com/") */
    const char *url; /**< Reference to the url which will handle the given protocol. (eg. "soup?url=%s") */
    const char *title; /**< Reference to the descriptive title of the handler. (eg. "SoupWeb") */
    Ewk_Custom_Handlers_State result; /**< Result of the query that the protocol handler is registered or not. */
};

/**
 * @brief Contains an internal View data.
 *
 * It is to be considered private by users, but may be extended or
 * changed by sub-classes (that's why it's in the public header file).
 */
struct _Ewk_View_Smart_Data {
    Evas_Object_Smart_Clipped_Data base;
    const Ewk_View_Smart_Class *api; /**< Reference to casted class instance. */
    Evas_Object *self; /**< Reference to owner object. */
    Evas_Object *main_frame; /**< Reference to main frame object. */
    Evas_Object *backing_store; /**< Reference to backing store. */
    Evas_Object *events_rect; /**< The rectangle that receives mouse events. */
    Ewk_View_Private_Data *_priv; /**< Should @b never be accessed, c++ stuff. */
    struct {
        Evas_Coord x, y, w, h;
    } view; /**< Contains the position and size of last used viewport. */
    struct {
        struct {
            float start;
            float end;
            float current; /**< if > 0.0, then doing animated zoom. */
        } zoom;
    } animated_zoom;
    struct {
        unsigned char r, g, b, a;
    } bg_color; /**< Keeps the background color. */
    Eina_Bool zoom_weak_smooth_scale:1;
    struct {
        Eina_Bool any:1;
        Eina_Bool size:1;
        Eina_Bool position:1;
        Eina_Bool frame_rect:1;
    } changed; /**< Keeps what changed since last smart_calculate. */
    struct {
        Evas_Coord x, y;
        float zoom;
    } previousView;
};

/// Defines the modes of view.
enum _Ewk_View_Mode {
    EWK_VIEW_MODE_INVALID,
    EWK_VIEW_MODE_WINDOWED,
    EWK_VIEW_MODE_FLOATING,
    EWK_VIEW_MODE_FULLSCREEN,
    EWK_VIEW_MODE_MAXIMIZED,
    EWK_VIEW_MODE_MINIMIZED
};
/// Creates a type name for @a _Ewk_View_Mode.
typedef enum _Ewk_View_Mode Ewk_View_Mode;

/// Defines the font families.
enum _Ewk_Font_Family {
    EWK_FONT_FAMILY_STANDARD = 0,
    EWK_FONT_FAMILY_CURSIVE,
    EWK_FONT_FAMILY_FANTASY,
    EWK_FONT_FAMILY_MONOSPACE,
    EWK_FONT_FAMILY_SERIF,
    EWK_FONT_FAMILY_SANS_SERIF
};
/// Creates a type name for @a _Ewk_Font_Family.
typedef enum _Ewk_Font_Family Ewk_Font_Family;

/// Contains commands to execute.
enum _Ewk_Editor_Command {
    EWK_EDITOR_COMMAND_NONE = -1,
    EWK_EDITOR_COMMAND_UNDO = 0,
    EWK_EDITOR_COMMAND_REDO,
    EWK_EDITOR_COMMAND_TOGGLE_BOLD,
    EWK_EDITOR_COMMAND_TOGGLE_ITALIC,
    EWK_EDITOR_COMMAND_TOGGLE_UNDERLINE,
    EWK_EDITOR_COMMAND_TOGGLE_STRIKETHROUGH,
    EWK_EDITOR_COMMAND_TOGGLE_SUBSCRIPT,
    EWK_EDITOR_COMMAND_TOGGLE_SUPERSCRIPT,
    EWK_EDITOR_COMMAND_INDENT,
    EWK_EDITOR_COMMAND_OUTDENT,
    EWK_EDITOR_COMMAND_INSERT_ORDEREDLIST,
    EWK_EDITOR_COMMAND_INSERT_UNORDEREDLIST,
    EWK_EDITOR_COMMAND_INSERT_IMAGE,
    EWK_EDITOR_COMMAND_INSERT_TEXT,
    EWK_EDITOR_COMMAND_INSERT_HTML,
    EWK_EDITOR_COMMAND_INSERT_PARAGRAPH,
    EWK_EDITOR_COMMAND_INSERT_PARAGRAPH_SEPARATOR,
    EWK_EDITOR_COMMAND_INSERT_LINE_SEPARATOR,
    EWK_EDITOR_COMMAND_BACK_COLOR,
    EWK_EDITOR_COMMAND_FORE_COLOR,
    EWK_EDITOR_COMMAND_HILITE_COLOR,
    EWK_EDITOR_COMMAND_FONT_SIZE,
    EWK_EDITOR_COMMAND_ALIGN_CENTER,
    EWK_EDITOR_COMMAND_ALIGN_JUSTIFIED,
    EWK_EDITOR_COMMAND_ALIGN_LEFT,
    EWK_EDITOR_COMMAND_ALIGN_RIGHT,
    EWK_EDITOR_COMMAND_MOVE_TO_NEXT_CHAR,
    EWK_EDITOR_COMMAND_MOVE_TO_PREVIOUS_CHAR,
    EWK_EDITOR_COMMAND_MOVE_TO_NEXT_WORD,
    EWK_EDITOR_COMMAND_MOVE_TO_PREVIOUS_WORD,
    EWK_EDITOR_COMMAND_MOVE_TO_NEXT_LINE,
    EWK_EDITOR_COMMAND_MOVE_TO_PREVIOUS_LINE,
    EWK_EDITOR_COMMAND_MOVE_TO_BEGINNING_OF_LINE,
    EWK_EDITOR_COMMAND_MOVE_TO_END_OF_LINE,
    EWK_EDITOR_COMMAND_MOVE_TO_BEGINNING_OF_PARAGRAPH,
    EWK_EDITOR_COMMAND_MOVE_TO_END_OF_PARAGRAPH,
    EWK_EDITOR_COMMAND_MOVE_TO_BEGINNING_OF_DOCUMENT,
    EWK_EDITOR_COMMAND_MOVE_TO_END_OF_DOCUMENT,
    EWK_EDITOR_COMMAND_SELECT_NONE,
    EWK_EDITOR_COMMAND_SELECT_ALL,
    EWK_EDITOR_COMMAND_SELECT_PARAGRAPH,
    EWK_EDITOR_COMMAND_SELECT_SENTENCE,
    EWK_EDITOR_COMMAND_SELECT_LINE,
    EWK_EDITOR_COMMAND_SELECT_WORD,
    EWK_EDITOR_COMMAND_SELECT_NEXT_CHAR,
    EWK_EDITOR_COMMAND_SELECT_PREVIOUS_CHAR,
    EWK_EDITOR_COMMAND_SELECT_NEXT_WORD,
    EWK_EDITOR_COMMAND_SELECT_PREVIOUS_WORD,
    EWK_EDITOR_COMMAND_SELECT_NEXT_LINE,
    EWK_EDITOR_COMMAND_SELECT_PREVIOUS_LINE,
    EWK_EDITOR_COMMAND_SELECT_START_OF_LINE,
    EWK_EDITOR_COMMAND_SELECT_END_OF_LINE,
    EWK_EDITOR_COMMAND_SELECT_START_OF_PARAGRAPH,
    EWK_EDITOR_COMMAND_SELECT_END_OF_PARAGRAPH,
    EWK_EDITOR_COMMAND_SELECT_START_OF_DOCUMENT,
    EWK_EDITOR_COMMAND_SELECT_END_OF_DOCUMENT,
    EWK_EDITOR_COMMAND_DELETE_WORD_BACKWARD,
    EWK_EDITOR_COMMAND_DELETE_WORD_FORWARD
};

/// Creates a type name for @a _Ewk_Editor_Command.
typedef enum _Ewk_Editor_Command Ewk_Editor_Command;

/**
 * @brief Creates a type name for @a _Ewk_Tile_Unused_Cache.
 *
 * Cache (pool) that contains unused tiles for ewk_view_tiled.
 *
 * This cache will maintain unused tiles and flush them when the total
 * memory exceeds the set amount when
 * ewk_tile_unused_cache_auto_flush() or explicitly set value when
 * ewk_tile_unused_cache_flush() is called.
 *
 * The tile may be shared among different ewk_view_tiled instances to
 * group maximum unused memory resident in the system.
 */
typedef struct _Ewk_Tile_Unused_Cache Ewk_Tile_Unused_Cache;

/**
 * Changes cache capacity of unused tiles.
 *
 * @param tuc cache of unused tiles to set a new capacity of unused tiles
 *
 * @param max a new capacity of cache, in bytes
 *
 * @note This will not flush cache, use ewk_tile_unused_cache_flush() or
 * ewk_tile_unused_cache_auto_flush() to do so.
 */
EAPI void   ewk_tile_unused_cache_max_set(Ewk_Tile_Unused_Cache *tuc, size_t max);

/**
 * Retrieves maximum cache capacity of unused tiles.
 *
 * @param tuc cache of unused tiles to get maximum cache capacity of unused tiles
 *
 * @return maximum cache capacity, in bytes on success or @c 0 on failure
 */
EAPI size_t ewk_tile_unused_cache_max_get(const Ewk_Tile_Unused_Cache *tuc);

/**
 * Retrieves the used cache capacity of unused tiles.
 *
 * @param tuc cache of unused tiles to get used cache capacity of unused tiles
 *
 * @return used cache capacity, in bytes on success or @c 0 on failure
 */
EAPI size_t ewk_tile_unused_cache_used_get(const Ewk_Tile_Unused_Cache *tuc);

/**
 * Flushes given amount of bytes from cache of unused tiles.
 *
 * After calling this function, near @a bytes are freed from cache. It
 * may be less if cache did not contain that amount of bytes (ie: an
 * empty cache has nothing to free!) or more if the cache just
 * contained objects that were larger than the requested amount (this
 * is usually the case).
 *
 * @param tuc cache of unused tiles to flush @bytes from cache
 * @param bytes amount bytes to free
 *
 * @return amount really freed bytes
 *
 * @see ewk_tile_unused_cache_used_get()
 */
EAPI size_t ewk_tile_unused_cache_flush(Ewk_Tile_Unused_Cache *tuc, size_t bytes);

/**
 * Flushes enough bytes to make cache of unused tiles usage lower than maximum.
 *
 * Just like ewk_tile_unused_cache_flush(), but this will make the cache
 * free enough tiles to respect maximum cache size as defined with
 * ewk_tile_unused_cache_max_set().
 *
 * This function is usually called when system becomes idle. This way
 * we keep memory low but do not impact performance when
 * creating/deleting tiles.
 *
 * @param tuc cache of unused tiles to flush cache of unused tiles
 */
EAPI void   ewk_tile_unused_cache_auto_flush(Ewk_Tile_Unused_Cache *tuc);

/**
 * Sets the smart class api without any backing store, enabling view
 * to be inherited.
 *
 * @param api class definition to set, all members with the
 *        exception of @a Evas_Smart_Class->data may be overridden, must
 *        @b not be @c NULL
 *
 * @note @a Evas_Smart_Class->data is used to implement type checking and
 *       is not supposed to be changed/overridden. If you need extra
 *       data for your smart class to work, just extend
 *       Ewk_View_Smart_Class instead.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (probably
 *         version mismatch)
 *
 * @see ewk_view_single_smart_set()
 * @see ewk_view_tiled_smart_set()
 */
EAPI Eina_Bool    ewk_view_base_smart_set(Ewk_View_Smart_Class *api);

/**
 * Sets the smart class api using single backing store, enabling view
 * to be inherited.
 *
 * @param api class definition to set, all members with the
 *        exception of @a Evas_Smart_Class->data may be overridden, must
 *        @b not be @c NULL
 *
 * @note @a Evas_Smart_Class->data is used to implement type checking and
 *       is not supposed to be changed/overridden. If you need extra
 *       data for your smart class to work, just extend
 *       @a Ewk_View_Smart_Class instead.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (probably
 *         version mismatch)
 *
 * @see ewk_view_base_smart_set()
 */
EAPI Eina_Bool    ewk_view_single_smart_set(Ewk_View_Smart_Class *api);

/**
 * Sets the smart class api using tiled backing store, enabling view
 * to be inherited.
 *
 * @param api class definition to set, all members with the
 *        exception of @a Evas_Smart_Class->data may be overridden, must
 *        @b not be @c NULL
 *
 * @note @a Evas_Smart_Class->data is used to implement type checking and
 *       is not supposed to be changed/overridden. If you need extra
 *       data for your smart class to work, just extend
 *       Ewk_View_Smart_Class instead.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (probably
 *         version mismatch)
 *
 * @see ewk_view_base_smart_set()
 */
EAPI Eina_Bool    ewk_view_tiled_smart_set(Ewk_View_Smart_Class *api);

/**
 * Creates a new EFL WebKit View object.
 *
 * View objects are the recommended way to deal with EFL WebKit as it
 * abstracts the complex pieces of the process.
 *
 * Each view is composed by a set of frames. The set has at least one
 * frame, called 'main_frame'. See ewk_view_frame_main_get() and
 * ewk_view_frame_focused_get().
 *
 * @param e canvas object where to create the view object
 *
 * @return view object on success or @c NULL on failure
 *
 * @see ewk_view_uri_set()
 */
EAPI Evas_Object *ewk_view_single_add(Evas *e);

/**
 * Creates a new EFL WebKit View object using tiled backing store.
 *
 * View objects are the recommended way to deal with EFL WebKit as it
 * abstracts the complex pieces of the process.
 *
 * This object is almost the same as the one returned by the ewk_view_single_add()
 * function, but it uses the tiled backing store instead of the default
 * backing store.
 *
 * @param e canvas object where to create the view object
 *
 * @return the view object on success or @c NULL on failure
 *
 * @see ewk_view_uri_set()
 */
EAPI Evas_Object *ewk_view_tiled_add(Evas *e);

/**
 * Gets the cache object of unused tiles used by this view.
 *
 * @param o the view object to get the cache object
 *
 * @return the cache object of unused tiles or @c NULL on failure
 */
EAPI Ewk_Tile_Unused_Cache *ewk_view_tiled_unused_cache_get(const Evas_Object *o);

/**
 * Sets the cache object of unused tiles used by this view.
 *
 * It can be used to share a single cache amongst different views.
 * The tiles from one view will not be used by the other!
 * This is just to limit the group with amount of unused memory.
 *
 * @note If @c NULL is provided as a @a cache, then a new one is created.
 *
 * @param o the view object to set the cache object
 * @param the cache object of unused tiles
 */
EAPI void                   ewk_view_tiled_unused_cache_set(Evas_Object *o, Ewk_Tile_Unused_Cache *cache);

/**
 * Sets a fixed layout size to be used, dissociating it from viewport size.
 *
 * Setting a width different than zero enables fixed layout on that
 * size. It's automatically scaled based on zoom, but will not change
 * if viewport changes.
 *
 * Setting both @a w and @a h to zero will disable fixed layout.
 *
 * @param o view object to change fixed layout
 * @param w fixed width to use, this size will be automatically scaled
 *        based on zoom level
 * @param h fixed height to use, this size will be automatically scaled
 *        based on zoom level
 */
EAPI void         ewk_view_fixed_layout_size_set(Evas_Object *o, Evas_Coord w, Evas_Coord h);

/**
 * Gets fixed layout size.
 *
 * @param o view object to get fixed layout size
 * @param w the pointer to store fixed width, returns @c NULL on failure or if there is no
 *        fixed layout in use
 * @param h the pointer to store fixed height, returns @c NULL on failure or if there is no
 *        fixed layout in use
 */
EAPI void         ewk_view_fixed_layout_size_get(const Evas_Object *o, Evas_Coord *w, Evas_Coord *h);

/**
 * Sets the theme path that will be used by this view.
 *
 * This also sets the theme on the main frame. As frames inherit theme
 * from their parent, this will have all frames with unset theme to
 * use this one.
 *
 * @param o view object to change theme
 * @param path theme path
 */
EAPI void         ewk_view_theme_set(Evas_Object *o, const char *path);

/**
 * Gets the theme set on this view.
 *
 * This returns the value set by ewk_view_theme_set().
 *
 * @param o view object to get theme path
 *
 * @return the theme path, may be @c NULL if not set
 */
EAPI const char  *ewk_view_theme_get(const Evas_Object *o);

/**
 * Gets the object that represents the main frame.
 *
 * @param o view object to get main frame
 *
 * @return frame smart object or @c NULL if none yet
 */
EAPI Evas_Object *ewk_view_frame_main_get(const Evas_Object *o);

/**
 * Gets the currently focused frame object.
 *
 * @param o view object to get focused frame
 *
 * @return frame smart object or @c NULL if none yet
 */
EAPI Evas_Object *ewk_view_frame_focused_get(const Evas_Object *o);

/**
 * Asks the main frame to load the given URI.
 *
 * @param o view object to load @a uri
 * @param uri uniform resource identifier to load
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_uri_set(Evas_Object *o, const char *uri);

/**
 * Gets the current uri loaded by main frame.
 *
 * It returns a internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o view object to get current uri.
 *
 * @return current uri on success or @c NULL on failure
 */
EAPI const char  *ewk_view_uri_get(const Evas_Object *o);

/**
 * Gets the current title of the main frame.
 *
 * It returns a internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o view object to get current title
 *
 * @return current title on success or @c NULL on failure
 */
EAPI const Ewk_Text_With_Direction  *ewk_view_title_get(const Evas_Object *o);

/**
 * Queries if the main frame is editable.
 *
 * @param o view object to query editable state
 *
 * @return @c EINA_TRUE if the main frame is editable, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_editable_get(const Evas_Object *o);

/**
 * Sets if main frame is editable.
 *
 * @param o view object to set editable state
 * @param editable a new state to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_editable_set(Evas_Object *o, Eina_Bool editable);

/**
 * Sets the background color and transparency of the view.
 *
 * Just as in Evas, colors are pre-multiplied, so 50% red is
 * (128, 0, 0, 128) and not (255, 0, 0, 128)!
 *
 * @warning Watch out performance issues with transparency! Object
 *          will be handled as transparent image by evas even if the
 *          webpage specifies a background color. That mean you'll pay
 *          a price even if it's not really transparent, thus
 *          scrolling/panning and zooming will be likely slower than
 *          if transparency is off.
 *
 * @param o view object to change the background color
 * @param r red color component
 * @param g green color component
 * @param b blue color component
 * @param a transparency
 */
EAPI void         ewk_view_bg_color_set(Evas_Object *o, int r, int g, int b, int a);

/**
 * Gets the background color of the view.
 *
 * Just as in Evas, colors are pre-multiplied, so 50% red is
 * (128, 0, 0, 128) and not (255, 0, 0, 128)!
 *
 * @param o view object to get the background color
 * @param r the pointer to store red color component
 * @param g the pointer to store green color component
 * @param b the pointer to store blue color component
 * @param a the pointer to store alpha value
 */
EAPI void         ewk_view_bg_color_get(const Evas_Object *o, int *r, int *g, int *b, int *a);

/**
 * Gets the copy of the selected text.
 *
 * The returned string @b should be freed by eina_stringshare_del() after use.
 *
 * @param o view object to get selected text
 *
 * @return a newly allocated string or @c NULL if nothing is selected or on failure
 */
EAPI const char        *ewk_view_selection_get(const Evas_Object *o);

/**
 * Forwards a request of a new Context Menu to WebCore.
 *
 * @param o view object to forward a request of a new Context Menu
 * @param ev mouse down event data
 *
 * @return @c EINA_TRUE if operation was executed, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_context_menu_forward_event(Evas_Object *o, const Evas_Event_Mouse_Down *ev);

/**
 * Executes editor command.
 *
 * @param o view object to execute command
 * @param command editor command to execute
 * @param value the value to be passed into command
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_editor_command_execute(const Evas_Object *o, const Ewk_Editor_Command command, const char *value);

/**
 * Destroys a previously created color chooser.
 *
 * Before destroying, it informs client that color chooser's data is ready to be
 * destroyed by sending a "colorchooser,willdelete". Then it removes any reference
 * to the color chooser inside webkit. It's safe to call this function either from
 * inside webkit or from browser.
 *
 * @param o view object
 *
 * @return @c EINA_TRUE in case color chooser was successfully destroyed or @c EINA_TRUE in
 * case there wasn't any color chooser to be destroyed
 */
EAPI Eina_Bool    ewk_view_color_chooser_destroy(Evas_Object* o);

/**
 * Changes the selected color.
 *
 * Changes the color selected in the color input widget. The browser should call
 * this when the user chooses a new color. It's likely that ewk_view_color_chooser_destroy
 * will be called afterwards.
 *
 * @param o view object
 * @param r red color component
 * @param g green color component
 * @param b blue color component
 */
EAPI void         ewk_view_color_chooser_color_set(Evas_Object* o, int r, int g, int b);

/**
 * Changes currently selected item.
 *
 * Changes the option selected in select widget. This is called by browser
 * whenever user has chosen a different item. Most likely after calling this, a
 * call to ewk_view_popup_destroy might be made in order to close the popup.
 *
 * @param o view object to change currently selected item
 * @index index a new index to set
 */
EAPI void         ewk_view_popup_selected_set(Evas_Object *o, int index);

/**
 * Destroys a previously created menu.
 *
 * Before destroying, it informs client that menu's data is ready to be
 * destroyed by sending a "popup,willdelete" with a list of menu items. Then it
 * removes any reference to menu inside webkit. It's safe to call this
 * function either from inside webkit or from browser.
 *
 * @param o view object
 *
 * @return @c EINA_TRUE in case menu was successfully destroyed or @c EINA_TRUE in
 * case there wasn't any menu to be destroyed
 */
EAPI Eina_Bool    ewk_view_popup_destroy(Evas_Object *o);

/**
 * Searches the given string in a document.
 *
 * @param o view object where to search the text
 * @param string reference string to search
 * @param case_sensitive if search should be case sensitive or not
 * @param forward if search is from cursor and on or backwards
 * @param wrap if search should wrap at the end
 *
 * @return @c EINA_TRUE if the given string was found, @c EINA_FALSE if not or failure
 */
EAPI Eina_Bool    ewk_view_text_search(const Evas_Object *o, const char *string, Eina_Bool case_sensitive, Eina_Bool forward, Eina_Bool wrap);

/**
 * Marks matches the given string in a document.
 *
 * @param o view object where to search text
 * @param string reference string to match
 * @param case_sensitive if match should be case sensitive or not
 * @param highlight if matches should be highlighted
 * @param limit maximum amount of matches, or zero to unlimited
 *
 * @return number of matched @a string
 */
EAPI unsigned int ewk_view_text_matches_mark(Evas_Object *o, const char *string, Eina_Bool case_sensitive, Eina_Bool highlight, unsigned int limit);

/**
 * Unmarks all marked matches in a document.
 *
 * Reverses the effect of ewk_frame_text_matches_mark().
 *
 * @param o view object where to unmark matches
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_text_matches_unmark_all(Evas_Object *o);

/**
 * Sets if should highlight matches marked with ewk_frame_text_matches_mark().
 *
 * @param o view object where to set if matches are highlighted or not
 * @param highlight @c EINA_TRUE if matches are highlighted, @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_text_matches_highlight_set(Evas_Object *o, Eina_Bool highlight);

/**
 * Gets if should highlight matches marked with ewk_frame_text_matches_mark().
 *
 * @param o view object to query if matches are highlighted or not
 *
 * @return @c EINA_TRUE if matches are highlighted, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_text_matches_highlight_get(const Evas_Object *o);

/**
 * Gets the current load progress of page.
 *
 * The progress estimates from 0.0 to 1.0.
 *
 * @param o view object to get the current progress
 *
 * @return the load progres of page, value from 0.0 to 1.0 on success
 *       or -1.0 on failure
 */
EAPI double       ewk_view_load_progress_get(const Evas_Object *o);

/**
 * Asks the main frame to stop loading.
 *
 * @param o view object to stop loading
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool    ewk_view_stop(Evas_Object *o);

/**
 * Asks the main frame to reload the current document.
 *
 * @param o view object to reload current document
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_view_reload_full()
 */
EAPI Eina_Bool    ewk_view_reload(Evas_Object *o);

/**
 * Asks the main frame to fully reload the current document, using no caches.
 *
 * @param o view object to reload current document
 *
 * @return @c EINA_TRUE on success o r@c EINA_FALSE otherwise
 *
 * @see ewk_view_reload()
 */
EAPI Eina_Bool    ewk_view_reload_full(Evas_Object *o);

/**
 * Asks the frame to navigate back in the history.
 *
 * @param o view object to navigate back
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_frame_back()
 */
EAPI Eina_Bool    ewk_view_back(Evas_Object *o);

/**
 * Asks frame to navigate forward in the history.
 *
 * @param o view object to navigate forward
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_frame_forward()
 */
EAPI Eina_Bool    ewk_view_forward(Evas_Object *o);

/**
 * Navigates back or forward in the history.
 *
 * @param o view object to navigate in the history
 * @param steps if positive navigates that amount forwards, if negative
 *        does backwards
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_frame_navigate()
 */
EAPI Eina_Bool    ewk_view_navigate(Evas_Object *o, int steps);

/**
 * Queries if it's possible to navigate backwards one item in the history.
 *
 * @param o view object to query if backward navigation is possible
 *
 * @return @c EINA_TRUE if it's possible to navigate backward one item in the history, @c EINA_FALSE otherwise
 * @see ewk_view_navigate_possible()
 */
EAPI Eina_Bool    ewk_view_back_possible(Evas_Object *o);

/**
 * Queries if it's possible to navigate forwards one item in the history.
 *
 * @param o view object to query if forward navigation is possible
 *
 * @return @c EINA_TRUE if it's possible to navigate forwards in the history, @c EINA_FALSE otherwise
 *
 * @see ewk_view_navigate_possible()
 */
EAPI Eina_Bool    ewk_view_forward_possible(Evas_Object *o);

/**
 * Queries if it's possible to navigate given @a steps in the history.
 *
 * @param o view object to query if it's possible to navigate @a steps in the history
 * @param steps if positive navigates that amount forwards, if negative
 *        does backwards
 *
 * @return @c EINA_TRUE if it's possible to navigate @a steps in the history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_navigate_possible(Evas_Object *o, int steps);

/**
 * Queries if navigation in the history (back-forward lists) is enabled.
 *
 * @param o view object to query if navigation history is enabled
 *
 * @return @c EINA_TRUE if view keeps history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_history_enable_get(const Evas_Object *o);

/**
 * Enables/disables navigation in the history (back-forward lists).
 *
 * @param o view object to enable/disable navigation in the history
 * @param enable @c EINA_TRUE to enable navigation in the history,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_history_enable_set(Evas_Object *o, Eina_Bool enable);

/**
 * Gets the history (back-forward list) associated with this view.
 *
 * The returned instance is unique for this view and thus multiple calls
 * to this function with the same view as parameter returns the same
 * handle. This handle is alive while view is alive, thus one
 * might want to listen for EVAS_CALLBACK_DEL on given view
 * (@a o) to know when to stop using returned handle.
 *
 * @param o view object to get navigation history
 *
 * @return the history instance handle associated with this
 *         view on succes or @c NULL on failure (including when the history
 *         navigation is not enabled with ewk_view_history_enable_set())
 *
 * @see ewk_view_history_enable_set()
 */
EAPI Ewk_History *ewk_view_history_get(const Evas_Object *o);

/**
 * Adds @a visited_url to the view's visited links cache.
 *
 * This function is to be invoked by the client managing persistent history storage
 * when "populate,visited,links" signal is received.
 *
 * @param o view object to add visited links data.
 * @param visited_url visited url.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool  ewk_view_visited_link_add(Evas_Object *o, const char *visited_url);

/**
 * Gets the current page zoom level of the main frame.
 *
 * @param o view object to get the zoom level
 *
 * @return current zoom level in use on success or @c -1.0 on failure
 */
EAPI float        ewk_view_page_zoom_get(const Evas_Object *o);

/**
 * Sets the current page zoom level of the main frame.
 *
 * @param o view object to set the zoom level
 * @param page_zoom_factor a new level to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_page_zoom_set(Evas_Object *o, float page_zoom_factor);

/**
 * Gets the current scale factor of the page.
 *
 * @param o view object to get the scale factor 
 *
 * @return current scale factor in use on success or @c -1.0 on failure
 */
EAPI float        ewk_view_scale_get(const Evas_Object *o);

/**
 * Scales the current page, centered at the given point.
 *
 * @param o view object to set the zoom level
 * @param scale_factor a new level to set
 * @param cx x of center coordinate
 * @param cy y of center coordinate
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_scale_set(Evas_Object *o, float scale_factor, Evas_Coord cx, Evas_Coord cy);

/**
 * Gets the current text zoom level of the main frame.
 *
 * @param o view object to get the zoom level
 *
 * @return current zoom level in use on success or @c -1.0 on failure
 */
EAPI float        ewk_view_text_zoom_get(const Evas_Object *o);

/**
 * Sets the current text zoom level of the main frame.
 *
 * @param o view object to set the zoom level
 * @param textZoomFactor a new level to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_text_zoom_set(Evas_Object *o, float text_zoom_factor);

/**
 * Gets the current zoom level of the main frame.
 *
 * @param o view object to get the zoom level
 *
 * @return current zoom level in use on success or @c -1.0 on failure
 */
EAPI float        ewk_view_zoom_get(const Evas_Object *o);

/**
 * Sets the current zoom level of the main frame, centered at the given point.
 *
 * @param o view object to set the zoom level
 * @param zoom a new level to set
 * @param cx x of center coordinate
 * @param cy y of center coordinate
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_zoom_set(Evas_Object *o, float zoom, Evas_Coord cx, Evas_Coord cy);

/**
 * Queries if the smooth scale is enabled while the weak zoom.
 *
 * @param o view object to query if the smooth scale is enabled while the weak zoom
 *
 * @return @c EINA_TRUE if the smooth scale is enabled while the weak zoom, or
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_zoom_weak_smooth_scale_get(const Evas_Object *o);

/**
 * Enables/disables the smooth scale while the weak zoom.
 *
 * @param o view object to set the smooth scale while the weak zoom
 * @param smooth_scale @c EINA_TRUE to enable the smooth scale
 *        @c EINA_FALSE to disable
 */
EAPI void         ewk_view_zoom_weak_smooth_scale_set(Evas_Object *o, Eina_Bool smooth_scale);

/**
 * Sets the current zoom level of backing store, centered at given point.
 *
 * Unlike ewk_view_zoom_set(), this call do not ask WebKit to render
 * at new size, but scale what is already rendered, being much faster
 * but worse quality.
 *
 * Often one should use ewk_view_zoom_animated_set(), it will call the
 * same machinery internally.
 *
 * @note this will set variables used by ewk_view_zoom_animated_set()
 *       so sub-classes will not reset internal state on their
 *       "calculate" phase. To unset those and enable sub-classes to
 *       reset their internal state, call
 *       ewk_view_zoom_animated_mark_stop(). Namely, this call will
 *       set ewk_view_zoom_animated_mark_start() to actual webkit zoom
 *       level, ewk_view_zoom_animated_mark_end() and
 *       ewk_view_zoom_animated_mark_current() to given zoom level.
 *
 * @param o view object to set the weak zoom level
 * @param zoom a new level to scale backing store
 * @param cx horizontal center offset, relative to object (w/2 is middle)
 * @param cy vertical center offset, relative to object (h/2 is middle)
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_zoom_weak_set(Evas_Object *o, float zoom, Evas_Coord cx, Evas_Coord cy);

/**
 * Sets start of an internal zoom animation state to the given zoom.
 *
 * This does not modify any actual zoom in WebKit or backing store,
 * just set needed flag so sub-classes knows they should not reset
 * their an internal state.
 *
 * @param o view object to set start of an internal zoom animation
 * @param zoom a new start value
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_view_zoom_animated_set()
 * @see ewk_view_zoom_weak_set()
 * @see ewk_view_zoom_animated_mark_stop()
 * @see ewk_view_zoom_animated_mark_end()
 * @see ewk_view_zoom_animated_mark_current()
 */
EAPI Eina_Bool    ewk_view_zoom_animated_mark_start(Evas_Object *o, float zoom);

/**
 * Sets end of an internal zoom animation state to given zoom.
 *
 * This does not modify any actual zoom in WebKit or backing store,
 * just set needed flag so sub-classes knows they should not reset
 * their an internal state.
 *
 * @param o view object to set end of an internal zoom animation
 * @param zoom a new end value
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_view_zoom_animated_set()
 * @see ewk_view_zoom_weak_set()
 * @see ewk_view_zoom_animated_mark_stop()
 * @see ewk_view_zoom_animated_mark_start()
 * @see ewk_view_zoom_animated_mark_current()
 */
EAPI Eina_Bool    ewk_view_zoom_animated_mark_end(Evas_Object *o, float zoom);

/**
 * Sets an internal current zoom animation state to given zoom.
 *
 * This does not modify any actual zoom in WebKit or backing store,
 * just set needed flag so sub-classes knows they should not reset
 * their an internal state.
 *
 * @param o view object to set an internal current zoom animation
 * @param zoom a new current value
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 *
 * @see ewk_view_zoom_animated_set()
 * @see ewk_view_zoom_weak_set()
 * @see ewk_view_zoom_animated_mark_stop()
 * @see ewk_view_zoom_animated_mark_start()
 * @see ewk_view_zoom_animated_mark_end()
 */
EAPI Eina_Bool    ewk_view_zoom_animated_mark_current(Evas_Object *o, float zoom);

/**
 * Unmarks an internal zoom animation state.
 *
 * The start, end and current values of an internal zoom animation are zeroed.
 *
 * @param o view object to unmark an internal zoom animation state
 *
 * @see ewk_view_zoom_animated_mark_start()
 * @see ewk_view_zoom_animated_mark_end()
 * @see ewk_view_zoom_animated_mark_current()
 * @see ewk_view_zoom_weak_set()
 */
EAPI Eina_Bool    ewk_view_zoom_animated_mark_stop(Evas_Object *o);

/**
 * Sets the current zoom level while animating.
 *
 * If the view was already animating to another zoom, it will start
 * from current point to the next provided zoom (@a zoom parameter)
 * and duration (@a duration parameter).
 *
 * This is the recommended way to do transitions from one level to
 * another. However, one may wish to do those from outside, in that
 * case use ewk_view_zoom_weak_set() and later control intermediate
 * states with ewk_view_zoom_animated_mark_current(),
 * ewk_view_zoom_animated_mark_end() and
 * ewk_view_zoom_animated_mark_stop().
 *
 * @param o view object to animate
 * @param zoom final zoom level to use
 * @param duration time in seconds the animation should take.
 * @param cx offset inside object that defines zoom center. 0 is left side
 * @param cy offset inside object that defines zoom center. 0 is top side
 * @return @c EINA_TRUE if animation will be started, @c EINA_FALSE if not
 *            because zoom is too small/big
 */
EAPI Eina_Bool    ewk_view_zoom_animated_set(Evas_Object *o, float zoom, float duration, Evas_Coord cx, Evas_Coord cy);

/**
 * Asks engine to pre-render region.
 *
 * Engines and backing store might be able to pre-render regions in
 * order to speed up zooming or scrolling to that region. Not all
 * engines might implement that and they will return @c EINA_FALSE
 * in that case.
 *
 * The given region is a hint. Engines might do bigger or smaller area
 * that covers that region. Pre-render might not be immediate, it may
 * be postponed to a thread, operated cooperatively in the main loop
 * and may be even ignored or cancelled afterwards.
 *
 * Multiple requests might be queued by engines. One can clear/forget
 * about them with ewk_view_pre_render_cancel().
 *
 * @param o view to ask pre-render of given region
 * @param x absolute coordinate (0=left) to pre-render at zoom
 * @param y absolute coordinate (0=top) to pre-render at zoom
 * @param w width to pre-render starting from @a x at zoom
 * @param h height to pre-render starting from @a y at zoom
 * @param zoom desired zoom
 *
 * @return @c EINA_TRUE if request was accepted, @c EINA_FALSE
 *         otherwise (errors, pre-render feature not supported, etc)
 *
 * @see ewk_view_pre_render_cancel()
 */
EAPI Eina_Bool    ewk_view_pre_render_region(Evas_Object *o, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, float zoom);

/**
 * Asks engine to pre-render region, given @a n extra cols/rows.
 *
 * This is an alternative method to ewk_view_pre_render_region(). It does not
 * make sense in all engines and therefore it might not be implemented at all.
 *
 * It's only useful if engine divide the area being rendered in smaller tiles,
 * forming a grid. Then, browser could call this function to pre-render @a n
 * rows/cols involving the current viewport.
 *
 * @param o view to ask pre-render
 * @param n number of cols/rows that must be part of the region pre-rendered
 *
 * @return @c EINA_TRUE if request was accepted, @c EINA_FALSE
 *         otherwise (errors, pre-render feature not supported, etc)
 *
 * @see ewk_view_pre_render_region()
 */
EAPI Eina_Bool    ewk_view_pre_render_relative_radius(Evas_Object *o, unsigned int n);

/**
 * Asks engine to start pre-rendering.
 *
 * This is an alternative method to pre-render around the view area.
 * The first step is to find the center view area where to start pre-rendering.
 * And then from the center of the view area the backing store append the render request
 * outward in spiral order. So that the tiles which are close to view area are displayed
 * sooner than outside.
 *
 * @param o view to ask pre-render
 *
 * @return @c EINA_TRUE if request was accepted, @c EINA_FALSE
 *         otherwise (errors, pre-render feature not supported, etc)
 *
 */
EAPI Eina_Bool    ewk_view_pre_render_start(Evas_Object *o);

/**
 * Cancels and clears previous the pre-render requests.
 *
 * @param o view to clear pre-render requests
 */
EAPI void         ewk_view_pre_render_cancel(Evas_Object *o);

/**
 * Enables (resumes) rendering.
 *
 * @param o view object to enable rendering
 *
 * @return @c EINA_TRUE if rendering was enabled, @c EINA_FALSE
 *         otherwise (errors, rendering suspension feature not supported)
 *
 * @see ewk_view_disable_render()
 */
EAPI Eina_Bool    ewk_view_enable_render(const Evas_Object *o);

/**
  * Disables (suspends) rendering.
  *
  * @param o view object to disable rendering
  *
  * @return @c EINA_TRUE if rendering was disabled, @c EINA_FALSE
  *         otherwise (errors, rendering suspension not supported)
  */
EAPI Eina_Bool    ewk_view_disable_render(const Evas_Object *o);

/**
 * Gets the input method hints.
 *
 * @param o view object to get the input method hints
 *
 * @see Ewk_Imh
 *
 * @return the input method hints as @a Ewk_Imh bits-field
 */
EAPI unsigned int ewk_view_imh_get(const Evas_Object *o);

/**
 * Gets the user agent string.
 *
 * @param o view object to get the user agent string
 *
 * @return the user agent string
 */
EAPI const char  *ewk_view_setting_user_agent_get(const Evas_Object *o);

/**
 * Sets the user agent string.
 *
 * @param o view object to set the user agent string
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_user_agent_set(Evas_Object *o, const char *user_agent);

/**
 * Queries if the images are loaded automatically.
 *
 * @param o view object to query if the images are loaded automatically
 *
 * @return @c EINA_TRUE if the images are loaded automatically,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_auto_load_images_get(const Evas_Object *o);

/**
 * Enables/disables auto loading of the images.
 *
 * @param o view object to set auto loading of the images
 * @param automatic @c EINA_TRUE to enable auto loading of the images,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_auto_load_images_set(Evas_Object *o, Eina_Bool automatic);

/**
 * Queries if the images are shrinked automatically
 *
 * @param o view object to query if the images are shrinked automatically
 *
 * @return @c EINA_TRUE if the images are shrinked automatically,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_auto_shrink_images_get(const Evas_Object *o);

/**
 * Enables/disables auto shrinking of the images.
 *
 * @param o view object to set auto shrinking of the images
 * @param automatic @c EINA_TRUE to enable auto shrinking of the images,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_auto_shrink_images_set(Evas_Object *o, Eina_Bool automatic);

/**
 * Queries if the view can be resized automatically.
 *
 * @param o view object to query if the view can be resized automatically
 *
 * @return @c EINA_TRUE if view can be resized automatically,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_auto_resize_window_get(const Evas_Object *o);

/**
 * Enables/disables if the view can be resized automatically.
 *
 * @param o view object to set if the view can be resized automatically
 * @param resizable @c EINA_TRUE if view can be resizable automatically,
 *        @c EINA_TRUE if not
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_auto_resize_window_set(Evas_Object *o, Eina_Bool resizable);

/**
 * Queries if the scripts can be executed.
 *
 * @param o view object to query if the scripts can be executed
 *
 * @return @c EINA_TRUE if the scripts can be executed
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_scripts_get(const Evas_Object *o);

/**
 * Enables/disables scripts executing.
 *
 * @param o view object to set script executing
 * @param enable @c EINA_TRUE to enable scripts executing
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_scripts_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the plug-ins are enabled.
 *
 * @param o view object to query if the plug-ins are enabled
 *
 * @return @c EINA_TRUE if the plugins are enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_plugins_get(const Evas_Object *o);

/**
 * Enables/disables the plug-ins.
 *
 * @param o view object to set the plug-ins
 * @param enable @c EINA_TRUE to enable the plug-ins
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_plugins_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the frame flattening feature is enabled.
 *
 * @param o view object to query if the frame flattening feature is enabled
 *
 * @return @c EINA_TRUE if the frame flattening feature is enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_frame_flattening_get(const Evas_Object *o);

/**
 * Enables/disables the frame flattening feature.
 *
 * @param o view object to set the frame flattening feature
 * @param enable @c EINA_TRUE to enable the frame flattening feature
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_frame_flattening_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the scripts can open the new windows.
 *
 * @param o view object to query if the scripts can open the new windows
 *
 * @return @c EINA_TRUE if the scripts can open the new windows
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_scripts_can_open_windows_get(const Evas_Object *o);

/**
 * Enables/disables if the scripts can open the new windows.
 *
 * @param o view object to set if the scripts can open the new windows
 * @param allow @c EINA_TRUE if the scripts can open the new windows
 *        @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure (scripts are disabled)
 *
 * @see ewk_view_setting_enable_scripts_set
 */
EAPI Eina_Bool    ewk_view_setting_scripts_can_open_windows_set(Evas_Object *o, Eina_Bool allow);

/**
 * Returns whether scripts can close windows automatically.
 *
 * @param o View whose settings to check.
 *
 * @return @c EINA_TRUE if scripts can close windows, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool    ewk_view_setting_scripts_can_close_windows_get(const Evas_Object *o);

/**
 * Sets whether scripts are allowed to close windows automatically.
 *
 * @param o View whose settings to change.
 * @param allow @c EINA_TRUE to allow scripts to close windows,
 *              @c EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE if the setting could be changed successfully,
 *         @c EINA_FALSE in case an error occurred.
 */
EAPI Eina_Bool    ewk_view_setting_scripts_can_close_windows_set(Evas_Object *o, Eina_Bool allow);

/**
 * Returns whether scripts can access clipboard.
 *
 * @param o View whose settings to check.
 *
 * @return @c EINA_TRUE if scripts can access clipboard, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool    ewk_view_setting_scripts_can_access_clipboard_get(const Evas_Object *o);

/**
 * Sets whether scripts are allowed to access clipboard.
 *
 * The default value is @c EINA_FALSE. If set to @c EINA_TRUE, document.execCommand()
 * allows cut, copy and paste commands. 
 *
 * @param o View whose settings to change.
 * @param allow @c EINA_TRUE to allow scripts access clipboard,
 *              @c EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE if the setting could be changed successfully,
 *         @c EINA_FALSE in case an error occurred.
 */
EAPI Eina_Bool    ewk_view_setting_scripts_can_access_clipboard_set(Evas_Object *o, Eina_Bool allow);

/**
 * Queries if HTML elements @c textarea can be resizable.
 *
 * @param o view object to query if the textarea elements can be resizable
 *
 * @return @c EINA_TRUE if the textarea elements can be resizable
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_resizable_textareas_get(const Evas_Object *o);

/**
 * Enables/disables if HTML elements @c textarea can be resizable.
 *
 * @param o view object to set if the textarea elements can be resizable
 * @param enable @c EINA_TRUE if the textarea elements can be resizable
 *        @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_resizable_textareas_set(Evas_Object *o, Eina_Bool enable);

/**
 * Gets the user style sheet.
 *
 * @param o view object to get the user style sheet
 *
 * @return the user style sheet
 */
EAPI const char  *ewk_view_setting_user_stylesheet_get(const Evas_Object *o);

/**
 * Sets the user style sheet.
 *
 * @param o view object to set the user style sheet
 * @param uri uniform resource identifier to user style sheet
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_user_stylesheet_set(Evas_Object *o, const char *uri);

/**
 * Queries if the private browsing feature is enabled.
 *
 * @param o view object to query if the private browsing feature is enabled
 *
 * @return @c EINA_TRUE if the private browsing feature is enabled, or
 *         @c EINA_FALSE if not or on failure
 *
 * @see ewk_view_setting_private_browsing_set
 */
EAPI Eina_Bool    ewk_view_setting_private_browsing_get(const Evas_Object *o);

/**
 * Enables/disables the private browsing feature.
 *
 * When this option is set, WebCore will avoid storing any record of browsing
 * activity  that may persist on disk or remain displayed when the
 * option is reset.
 *
 * This option does not affect the storage of such information in RAM.
 *
 * The following functions respect this setting:
 *  - HTML5/DOM Storage
 *  - Icon Database
 *  - Console Messages
 *  - MemoryCache
 *  - Application Cache
 *  - Back/Forward Page History
 *  - Page Search Results
 *  - HTTP Cookies
 *  - Plug-ins (that support NPNVprivateModeBool)
 *
 * @param o view object to set the private browsing feature
 * @param enable @c EINA_TRUE to enable the private browsing feature
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_private_browsing_set(Evas_Object *o, Eina_Bool enable);

/**
 * Returns whether HTML5 application cache support is enabled for this view.
 *
 * The Offline Application Caching APIs are part of HTML5 and allow applications to store data locally that is accessed
 * when the network cannot be reached.
 *
 * Application cache support is enabled by default.
 *
 * @param o view object whose settings to query
 *
 * @return @c EINA_TRUE if the application cache is enabled,
 *         @c EINA_FALSE if not or on failure
 *
 * @sa ewk_settings_application_cache_path_set
 */
EAPI Eina_Bool    ewk_view_setting_application_cache_get(const Evas_Object *o);

/**
 * Enables/disables the HTML5 application cache for this view.
 *
 * The Offline Application Caching APIs are part of HTML5 and allow applications to store data locally that is accessed
 * when the network cannot be reached.
 *
 * Application cache support is enabled by default.
 *
 * @param o view object whose settings to change
 * @param enable @c EINA_TRUE to enable the application cache,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @sa ewk_settings_application_cache_path_set
 */
EAPI Eina_Bool    ewk_view_setting_application_cache_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the caret browsing feature is enabled.
 *
 * @param o view object to query if the caret browsing feature is enabled
 *
 * @return @c EINA_TRUE if the caret browsing feature is enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_caret_browsing_get(const Evas_Object *o);

/**
 * Enables/disables the caret browsing feature.
 *
 * @param o view object to set caret browsing feature
 * @param enable @c EINA_TRUE to enable the caret browsing feature
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_caret_browsing_set(Evas_Object *o, Eina_Bool enable);

/**
 * Gets the current encoding.
 *
 * @param o view object to get the current encoding
 *
 * @return @c eina_strinshare containing the current encoding, or
 *         @c NULL if it's not set
 */
EAPI const char  *ewk_view_setting_encoding_custom_get(const Evas_Object *o);

/**
 * Sets the encoding and reloads the page.
 *
 * @param o view to set the encoding
 * @param encoding the new encoding to set or @c NULL to restore the default one
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_encoding_custom_set(Evas_Object *o, const char *encoding);

/**
 * Gets the default encoding.
 *
 * @param o view object to get the default encoding
 *
 * @return @c eina_strinshare containing the default encoding, or
 *         @c NULL if it's not set
 */
EAPI const char  *ewk_view_setting_encoding_default_get(const Evas_Object *o);

/**
 * Sets the default encoding.
 *
 * @param o view to set the default encoding
 * @param encoding the new encoding to set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_encoding_default_set(Evas_Object *o, const char *encoding);

/**
 * Gets the minimum font size.
 *
 * @param o view object to get the minimum font size
 *
 * @return the minimum font size, or @c 0 on failure
 */
EAPI int          ewk_view_setting_font_minimum_size_get(const Evas_Object *o);

/**
 * Sets the minimum font size.
 *
 * @param o view object to set the minimum font size
 * @param size a new minimum font size to set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_font_minimum_size_set(Evas_Object *o, int size);

/**
 * Gets the minimum logical font size.
 *
 * @param o view object to get the minimum logical font size
 *
 * @return the minimum logical font size, or @c 0 on failure
 */
EAPI int          ewk_view_setting_font_minimum_logical_size_get(const Evas_Object *o);

/**
 * Sets the minimum logical font size.
 *
 * @param o view object to set the minimum font size
 * @param size a new minimum logical font size to set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_font_minimum_logical_size_set(Evas_Object *o, int size);

/**
 * Gets the default font size.
 *
 * @param o view object to get the default font size
 *
 * @return the default font size, or @c 0 on failure
 */
EAPI int          ewk_view_setting_font_default_size_get(const Evas_Object *o);

/**
 * Sets the default font size.
 *
 * @param o view object to set the default font size
 * @param size a new default font size to set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_font_default_size_set(Evas_Object *o, int size);

/**
 * Gets the Monospace font size.
 *
 * @param o view object to get the Monospace font size
 *
 * @return the Monospace font size, or @c 0 on failure
 */
EAPI int          ewk_view_setting_font_monospace_size_get(const Evas_Object *o);

/**
 * Sets the Monospace font size.
 *
 * @param o view object to set the Monospace font size
 * @param size a new Monospace font size to set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_font_monospace_size_set(Evas_Object *o, int size);

/**
 * Gets the name of font for the given font family.
 *
 * @param o view object to get name of font the for font family
 * @param font_family the font family as @a Ewk_Font_Family enum to get font name
 *
 * @return the name of font family
 */
EAPI const char *ewk_view_font_family_name_get(const Evas_Object *o, Ewk_Font_Family font_family);

/**
 * Sets the font for the given family.
 *
 * @param o view object to set font for the given family
 * @param font_family the font family as @a Ewk_Font_Family enum
 * @param name the font name to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_font_family_name_set(Evas_Object *o, Ewk_Font_Family font_family, const char *name);

/**
 * Queries if the spatial naviagtion feature is enabled.
 *
 * @param o view object to query if spatial navigation feature is enabled
 *
 * @return @c EINA_TRUE if spatial navigation is enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_spatial_navigation_get(const Evas_Object *o);

/**
 * Enables/disables the spatial navigation feature.
 *
 * @param o view object to set spatial navigation feature
 * @param enable @c EINA_TRUE to enable the spatial navigation feature,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_spatial_navigation_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the local storage feature of HTML5 is enabled.
 *
 * @param o view object to query if the local storage feature is enabled
 *
 * @return @c EINA_TRUE if local storage is enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_local_storage_get(const Evas_Object *o);

/**
 * Enables/disables the local storage feature of HTML5.
 *
 * Please notice that by default there is no storage path specified for the database.
 * This means that the contents of @c window.localStorage will not be saved to disk and
 * will be lost when the view is removed.
 * To set the path where the storage database will be stored, use
 * ewk_view_setting_local_storage_database_path_set.
 *
 * @param o view object to set if local storage is enabled
 * @param enable @c EINA_TRUE to enable the local storage feature,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @sa ewk_view_setting_local_storage_database_path_set
 */
EAPI Eina_Bool    ewk_view_setting_local_storage_set(Evas_Object *o, Eina_Bool enable);

/**
 * Returns the path where the HTML5 local storage database is stored on disk.
 *
 * By default, there is no path set, which means changes to @c window.localStorage will not
 * be saved to disk whatsoever.
 *
 * @param o view object to get the database path to the local storage feature
 *
 * @return @c eina_stringshare containing the database path to the local storage feature, or
 *         @c NULL if it's not set
 *
 * @sa ewk_view_setting_local_storage_database_path_set
 */
EAPI const char  *ewk_view_setting_local_storage_database_path_get(const Evas_Object *o);

/**
 * Sets the path where the HTML5 local storage database is stored on disk.
 *
 * By default, there is no path set, which means changes to @c window.localStorage will not
 * be saved to disk whatsoever.
 *
 * @param o view object to set the database path to the local storage feature
 * @param path a new database path to the local storage feature
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @sa ewk_view_setting_local_storage_set
 */
EAPI Eina_Bool    ewk_view_setting_local_storage_database_path_set(Evas_Object *o, const char *path);

/**
 * Queries if the page cache feature is enabled.
 *
 * @param o view object to query if page cache feature is enabled
 *
 * @return @c EINA_TRUE if page cache is enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_page_cache_get(const Evas_Object *o);

/**
 * Enables/disables the page cache feature.
 *
 * @param o view object to set page cache feature
 * @param enable @c EINA_TRUE to enable the page cache feature,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_page_cache_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the encoding detector is enabled.
 *
 * @param o view object to query if the encoding detector is enabled
 *
 * @return @c EINA_TRUE if the encoding feature is enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool    ewk_view_setting_encoding_detector_get(const Evas_Object *o);

/**
 * Enables/disables the encoding detector.
 *
 * @param o view object to set the encoding detector
 * @param enable @c EINA_TRUE to enable the encoding detector,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_view_setting_encoding_detector_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if developer extensions are enabled.
 *
 * Currently, this is used to know whether the Web Inspector is enabled for a
 * given view.
 *
 * @param o view object to query if developer extensions are enabled
 *
 * @return @c EINA_TRUE if developer extensions are enabled, @c EINA_FALSE
 *         otherwise
 */
EAPI Eina_Bool    ewk_view_setting_enable_developer_extras_get(const Evas_Object *o);

/**
 * Enables/disables developer extensions.
 *
 * This currently controls whether the Web Inspector should be enabled.
 *
 * @param o view object to set developer extensions
 * @param enable @c EINA_TRUE to enable developer extras, @c EINA_FALSE to
 *               disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_enable_developer_extras_set(Evas_Object *o, Eina_Bool enable);

/**
 * Sets the minimum interval for DOMTimers on current page.
 *
 * @param o view object to set the minimum interval
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_minimum_timer_interval_set(Evas_Object *o, double interval);

/**
 * Gets the minimum interval for DOMTimers on current page.
 *
 * @param o view object to get the minimum interval
 *
 * @return the minimum interval on success or @c -1.0 on failure
 */
EAPI double       ewk_view_setting_minimum_timer_interval_get(const Evas_Object *o);

/**
 * Queries whether WebGL support is enabled.
 *
 * WebGL support is enabled by default.
 *
 * @param o view object to query whether WebGL support is enabled
 *
 * @return @c EINA_TRUE if WebGL support enabled,
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_view_setting_enable_webgl_get(const Evas_Object *o);

/**
 * Enables/disables WebGL support.
 *
 * WebGL support is enabled by default.
 *
 * @param o view object to set WebGL
 * @param enable @c EINA_TRUE to enable WebGL support,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_setting_enable_webgl_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if tab key focusing of page elements is enabled.
 *
 * When this setting is enabled, the elements in the page (links and form controls) will be cycled through by pressing the tab key.
 * Tab key focusing of page elements is enabled by default.
 *
 * @param o view object to query if tab key focusing of page elements is enabled
 *
 * @return @c EINA_TRUE if tab key focusing of page elements is enabled, @c EINA_FALSE
 *         otherwise
 */
EAPI Eina_Bool ewk_view_setting_include_links_in_focus_chain_get(const Evas_Object *o);

/**
 * Enables/disables tab key focusing of page elements.
 *
 * When this setting is enabled, the elements in the page (links and form controls) will be cycled through by pressing the tab key.
 * Tab key focusing of page elements is enabled by default.
 *
 * @param o view object to set tab key focusing of page elements
 * @param enable @c EINA_TRUE to enable tab key focusing of page elements, @c EINA_FALSE to
 *               disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_setting_include_links_in_focus_chain_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if hyperlink auditing is enabled.
 *
 * Hyperlink auditing is used if a hyperlink created by an @c a or @c area element has a @c ping attribute.
 * Please refer to the hyperlink auditing specification at http://www.whatwg.org/specs/web-apps/current-work/#hyperlink-auditing.
 * Hyperlink auditing is disabled by default.
 *
 * @param o view object to query if  hyperlink auditing is enabled
 *
 * @return @c EINA_TRUE if hyperlink auditing is enabled, @c EINA_FALSE
 *         otherwise
 */
EAPI Eina_Bool ewk_view_setting_enable_hyperlink_auditing_get(const Evas_Object *o);

/**
 * Enables/disables hyperlink auditing.
 *
 * Hyperlink auditing is used if a hyperlink created by an @c a or @c area element has a @c ping attribute.
 * Please refer to the hyperlink auditing specification at http://www.whatwg.org/specs/web-apps/current-work/#hyperlink-auditing.
 * Hyperlink auditing is disabled by default.
 *
 * @param o view object to set hyperlink auditing
 * @param enable @c EINA_TRUE to enable hyperlink auditing, @c EINA_FALSE to
 *               disable
 *
 * @return @c EINA_TRUE on success or @EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_setting_enable_hyperlink_auditing_set(Evas_Object *o, Eina_Bool enable);

/**
 * Enables/disables allowing universal access from file URLs.
 *
 * This setting specifies whether locally loaded documents are allowed to access remote urls.
 * By default this setting is enabled.
 *
 * @param o view object to set allowing universal access from file URLs
 * @param enable @c EINA_TRUE to enable universal access from file URLs,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_allow_universal_access_from_file_urls_set(Evas_Object *o, Eina_Bool flag);

/**
 * Gets if allowing universal access from file URLs is enabled.
 *
 * @param o view object to query if allowing universal access from file URLs is enabled.
 *
 * @return @c EINA_TRUE if allowing universal access from file URLs is enabled, @c EINA_FALSE
 *         otherwise
 *
 * @see ewk_view_setting_allow_universal_access_from_file_urls_set()
 */
EAPI Eina_Bool    ewk_view_setting_allow_universal_access_from_file_urls_get(const Evas_Object *o);

/**
 * Enables/disables allowing file access from file URLs.
 *
 * This setting specifies whether locally loaded documents are allowed to access other local urls.
 * By default this setting is enabled.
 *
 * @param o view object to set allowing file access from file URLs
 * @param enable @c EINA_TRUE to enable file access from file URLs,
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_view_setting_allow_file_access_from_file_urls_set(Evas_Object *o, Eina_Bool flag);

/**
 * Gets if allowing file access from file URLs is enabled.
 *
 * @param o view object to query if allowing file access from file URLs is enabled.
 *
 * @return @c EINA_TRUE if allowing file access from file URLs is enabled, @c EINA_FALSE
 *         otherwise
 *
 * @see ewk_view_setting_allow_file_access_from_file_urls_set()
 */
EAPI Eina_Bool    ewk_view_setting_allow_file_access_from_file_urls_get(const Evas_Object *o);

/**
 * Gets the internal data of @a o.
 *
 * This is similar to evas_object_smart_data_get(), but additionally does type checking.
 *
 * @param o view object to get the internal data
 *
 * @return the internal data of @a o, or @c NULL on failure
 */
EAPI Ewk_View_Smart_Data *ewk_view_smart_data_get(const Evas_Object *o);

/**
 * Process scrolls.
 *
 * @param priv the pointer to the private data of the view to process scrolls
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
EAPI void ewk_view_scrolls_process(Ewk_View_Smart_Data *sd);

/**
 * Paints using given graphics context the given area.
 *
 * This uses viewport relative area and will also handle scrollbars
 * and other extra elements. See ewk_view_paint_contents() for the
 * alternative function.
 *
 * @param priv the pointer to the private data of the view to use as paint source
 * @param cr the cairo context to use as paint destination, its state will
 *        be saved before operation and restored afterwards
 * @param area viewport relative geometry to paint
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @note This is an easy to use version, but internal structures are
 *       always created, then graphics context is clipped, then
 *       painted, restored and destroyed. This might not be optimum,
 *       so using @a Ewk_View_Paint_Context may be a better solutions
 *       for large number of operations.
 *
 * @see ewk_view_paint_contents()
 * @see ewk_view_paint_context_paint()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
EAPI Eina_Bool ewk_view_paint(Ewk_View_Private_Data *priv, cairo_t *cr, const Eina_Rectangle *area);

/**
 * Paints just contents using given graphics context the given area.
 *
 * This uses absolute coordinates for area and will just handle
 * contents, no scrollbars or extras. See ewk_view_paint() for the
 * alternative solution.
 *
 * @param priv the pointer to the private data of the view to use as paint source
 * @param cr the cairo context to use as paint destination, its state will
 *        be saved before operation and restored afterwards
 * @param area absolute geometry to paint
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @note This is an easy to use version, but internal structures are
 *       always created, then graphics context is clipped, then
 *       painted, restored and destroyed. This might not be optimum,
 *       so using @a Ewk_View_Paint_Context may be a better solutions
 *       for large number of operations.
 *
 * @see ewk_view_paint()
 * @see ewk_view_paint_context_paint_contents()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
EAPI Eina_Bool ewk_view_paint_contents(Ewk_View_Private_Data *priv, cairo_t *cr, const Eina_Rectangle *area);

/**
 * Gets the attributes of the viewport meta tag.
 *
 * Properties are returned in the respective pointers. Passing @c NULL to any of
 * these pointers will make that property to not be returned.
 *
 * @param o view object to get the viewport attributes
 * @param w the pointer to store the width of the viewport
 * @param h the pointer to store the height of the viewport
 * @param init_scale the pointer to store the initial scale value
 * @param max_scale the pointer to store the maximum scale value
 * @param min_scale the pointer to store the minimum scale value
 * @param device_pixel_ratio the pointer to store the device pixel ratio value
 * @param user_scalable the pointer to store if user can scale viewport
 */
EAPI void ewk_view_viewport_attributes_get(const Evas_Object *o, int *w, int *h, float *init_scale, float *max_scale, float *min_scale, float *device_pixel_ratio , Eina_Bool *user_scalable);

/**
 * Sets the zoom range.
 *
 * @param o view object to set the zoom range
 * @param min_scale the minimum value of the zoom range
 * @param max_scale the maximum value of the zoom range
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_zoom_range_set(Evas_Object *o, float min_scale, float max_scale);

/**
 * Gets the minimum value of the zoom range.
 *
 * @param o view object to get the minimum value of the zoom range
 *
 * @return the minimum value of the zoom range on success, or
 *         @c -1 on failure
 */
EAPI float ewk_view_zoom_range_min_get(const Evas_Object *o);

/**
 * Gets the maximum value of the zoom range.
 *
 * @param o view object to get the maximum value of the zoom range
 *
 * @return the maximum value of the zoom range on success, or
 *         @c -1.0 on failure
 */
EAPI float ewk_view_zoom_range_max_get(const Evas_Object *o);

/**
 * Enables/disables the zoom.
 *
 * @param o view to set zoom
 * @param user_scalable @c EINA_TRUE to enable zoom, @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_user_scalable_set(Evas_Object *o, Eina_Bool user_scalable);

/**
 * Queries if the zoom is enabled.
 *
 * @param o view to query if zoom is enabled
 *
 * @return @c EINA_TRUE if the zoom is enabled, @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_view_user_scalable_get(const Evas_Object *o);

/**
 * Queries the ratio between the CSS units and device pixels when the content is unscaled.
 *
 * When designing touch-friendly contents, knowing the approximated target size on a device
 * is important for contents providers in order to get the intented layout and element
 * sizes.
 *
 * As most first generation touch devices had a PPI of approximately 160, this became a
 * de-facto value, when used in conjunction with the viewport meta tag.
 *
 * Devices with a higher PPI learning towards 240 or 320, applies a pre-scaling on all
 * content, of either 1.5 or 2.0, not affecting the CSS scale or pinch zooming.
 *
 * This value can be set using this property and it is exposed to CSS media queries using
 * the -webkit-device-pixel-ratio query.
 *
 * For instance, if you want to load an image without having it upscaled on a web view
 * using a device pixel ratio of 2.0 it can be done by loading an image of say 100x100
 * pixels but showing it at half the size.
 *
 * @media (-webkit-min-device-pixel-ratio: 1.5) {
 *     .icon {
 *         width: 50px;
 *         height: 50px;
 *         url: "/images/icon@2x.png"; // This is actually a 100x100 image
 *     }
 * }
 *
 * If the above is used on a device with device pixel ratio of 1.5, it will be scaled
 * down but still provide a better looking image.
 *
 * @param o view object to get device pixel ratio
 *
 * @return the ratio between the CSS units and device pixels,
 *         or @c -1.0 on failure
 */
EAPI float ewk_view_device_pixel_ratio_get(const Evas_Object *o);

/**
 * Sets the ratio between the CSS units and device pixels when the content is unscaled.
 *
 * @param o view object to set device pixel ratio
 *
 * @return @c EINA_TRUE if the device pixel ratio was set, @c EINA_FALSE otherwise
 *
 * @see ewk_view_device_pixel_ratio_get()
 */
EAPI Eina_Bool ewk_view_device_pixel_ratio_set(Evas_Object *o, float ratio);

/**
 * Changes the text direction of the selected input node.
 *
 * @param o view object to set text direction.
 * @param direction text direction.
 */
EAPI void ewk_view_text_direction_set(Evas_Object *o, Ewk_Text_Direction direction);

/**
 * Sets the view mode.
 *
 * The view-mode media feature describes the mode in which the
 * Web application is being shown as a running application.
 *
 * @param o view object to change the view mode
 * @param view_mode page view mode to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_view_mode_set(Evas_Object *o, Ewk_View_Mode view_mode);

/**
 * Gets the view mode.
 *
 * @param o view object to get the view mode
 *
 * @return enum value of @a Ewk_View_Mode that indicates current view mode on success or
 *         EWK_VIEW_MODE_INVALID otherwise
 *
 * @see ewk_view_mode_set()
 */
EAPI Ewk_View_Mode ewk_view_mode_get(const Evas_Object *o);

/**
 * Creates a JS object named @a obj_name as property of the window object. This should be called on a callback connectedto the
 * js,windowobject,clear signal.
 *
 * @param o view.
 * @param obj object that will be added(see: @a ewk_js_object_new).
 * @param obj_name name of the object.
 *
 * @return @c EINA_TRUE if object was added, @c EINA_FALSE if not.
 */
EAPI Eina_Bool ewk_view_js_object_add(Evas_Object *o, Ewk_JS_Object *obj, const char *obj_name);

/// Defines the page visibility status.
enum _Ewk_Page_Visibility_State {
    EWK_PAGE_VISIBILITY_STATE_VISIBLE,
    EWK_PAGE_VISIBILITY_STATE_HIDDEN,
    EWK_PAGE_VISIBILITY_STATE_PRERENDER,
    EWK_PAGE_VISIBILITY_STATE_UNLOADED
};
/// Creates a type name for @a _Ewk_Page_Visibility_State.
typedef enum _Ewk_Page_Visibility_State Ewk_Page_Visibility_State;

/**
 * Sets the visibility state of the page.
 *
 * This function let WebKit knows the visibility status of the page.
 * WebKit will save the current status, and fire a "visibilitychange"
 * event which web application can listen. Web application could slow
 * down or stop itself when it gets a "visibilitychange" event and its
 * visibility state is hidden. If its visibility state is visible, then
 * the web application could use more resources.
 *
 * This feature makes that web application could use the resources efficiently,
 * such as power, CPU, and etc.
 *
 * If more detailed description is needed, please see the specification.
 * (http://www.w3.org/TR/page-visibility)
 *
 * @param o view object to set the visibility state.
 * @param page_visible_state the visible state of the page to set.
 * @param initial_state @c EINA_TRUE if this function is called at page initialization time,
 *                      @c EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure.
 */
EAPI Eina_Bool ewk_view_visibility_state_set(Evas_Object *o, Ewk_Page_Visibility_State page_visible_state, Eina_Bool initial_state);

/**
 * Gets the visibility state of the page.
 *
 * @param o view object
 *
 * @return enum value of @a Ewk_Page_Visibility_State that indicates current visibility status of the page.
 *
 * @see ewk_view_visibility_state_set()
 */
EAPI Ewk_Page_Visibility_State ewk_view_visibility_state_get(const Evas_Object *o);

/**
 * Returns whether the view has displayed mixed content.
 *
 * When a view has displayed mixed content, any of its frames has loaded an HTTPS URI
 * which has itself loaded and displayed a resource (such as an image) from an insecure,
 * that is, non-HTTPS, URI.
 *
 * The status is reset only when a load event occurs (eg. the page is reloaded or a new page is loaded).
 *
 * When one of the containing frames displays mixed content, the view emits the "mixedcontent,displayed" signal.
 *
 * @param o The view to query.
 *
 * @sa ewk_frame_mixed_content_displayed_get
 */
EAPI Eina_Bool ewk_view_mixed_content_displayed_get(const Evas_Object *o);

/**
 * Returns whether the view has run mixed content.
 *
 * When a view has run mixed content, any of its frames has loaded an HTTPS URI
 * which has itself loaded and run a resource (such as an image) from an insecure,
 * that is, non-HTTPS, URI.
 *
 * The status is reset only when a load event occurs (eg. the page is reloaded or a new page is loaded).
 *
 * When one of the containing frames runs mixed content, the view emits the "mixedcontent,run" signal.
 *
 * @param o The view to query.
 *
 * @sa ewk_frame_mixed_content_run_get
 */
EAPI Eina_Bool ewk_view_mixed_content_run_get(const Evas_Object *o);

/**
 * Returns the SoupSession associated with this view.
 *
 * By default, all views share the same, default soup session also available
 * by calling ewk_network_default_soup_session_get.
 *
 * @param o The view to query.
 *
 * @sa ewk_view_soup_session_set, ewk_network_default_soup_session_get
 */
EAPI SoupSession* ewk_view_soup_session_get(const Evas_Object *o);

/**
 * Associates a new SoupSession with this view.
 *
 * Only sessions of type SoupSessionAsync are supported.
 *
 * @note Changing the SoupSession should not be needed in most cases. If
 *       a different SoupSession is used, the cookie management and some
 *       network functions in ewk will not have any effect on it.
 *
 * @param o The view to change.
 * @param session The new SoupSession.
 *
 * @sa ewk_view_soup_session_get, ewk_network_default_soup_session_get
 */
EAPI void ewk_view_soup_session_set(Evas_Object *o, SoupSession *session);

/**
 * Returns whether XSSAuditor feature is enabled.
 *
 * @param o view object to query whether XSSAuditor feature is enabled.
 *
 * @return @c EINA_TRUE if the XSSAuditor feature is enabled,
 *         @c EINA_FALSE if not or on failure.
 */
EAPI Eina_Bool ewk_view_setting_enable_xss_auditor_get(const Evas_Object *o);

/**
 * Enables/disables the XSSAuditor feature.
 * 
 * The XSSAuditor (cross-site scripting protection) feature provides protection 
 * from reflected XSS attacks on vulnerable web sites. When XSS is encountered
 * in the page, frame sends a signal "xss,detected" with additional information 
 * on whether the entire page was blocked or only injected scripts were removed. 
 * This feature is enabled by default.
 *
 * @param o view object to set the XSSAuditor feature.
 * @param enable @c EINA_TRUE to enable the XSSAuditor feature,
 *        @c EINA_FALSE to disable.
 */
EAPI void ewk_view_setting_enable_xss_auditor_set(Evas_Object *o, Eina_Bool enable);

/**
 * Returns whether video captions display feature is enabled.
 *
 * Video captions display is disabled by default.
 *
 * @param o view object to query whether video captions display feature is enabled.
 *
 * @return @c EINA_TRUE if the video captions display feature is enabled,
 *         @c EINA_FALSE if not or on failure.
 */
EAPI Eina_Bool ewk_view_setting_should_display_captions_get(const Evas_Object *o);

/**
 * Enables/disables the video captions display feature.
 *
 * The video captions display feature is part of track support for HTML5 video.
 *
 * @param o view object to set the video captions display feature.
 * @param enable @c EINA_TRUE to enable the video captions display feature,
 * @c EINA_FALSE to disable.
 */
EAPI void ewk_view_setting_should_display_captions_set(Evas_Object *o, Eina_Bool enable);

/**
 * Returns whether video subtitles display feature is enabled.
 *
 * Video subtitles display is disabled by default.
 *
 * @param o view object to query whether video subtitles display feature is enabled.
 *
 * @return @c EINA_TRUE if the video subtitles display feature is enabled,
 *         @c EINA_FALSE if not or on failure.
 */
EAPI Eina_Bool ewk_view_setting_should_display_subtitles_get(const Evas_Object *o);

/**
 * Enables/disables the video subtitles display feature.
 *
 * The video subtitles display feature is part of track support for HTML5 video.
 *
 * @param o view object to set the video subtitles display feature.
 * @param enable @c EINA_TRUE to enable the video subtitles display feature,
 * @c EINA_FALSE to disable.
 */
EAPI void ewk_view_setting_should_display_subtitles_set(Evas_Object *o, Eina_Bool enable);

/**
 * Returns whether video text descriptions display feature is enabled.
 *
 * Video text descriptions display is disabled by default.
 *
 * @param o view object to query whether video text descriptions display feature is enabled.
 *
 * @return @c EINA_TRUE if the video text descriptions display feature is enabled,
 *         @c EINA_FALSE if not or on failure.
 */
EAPI Eina_Bool ewk_view_setting_should_display_text_descriptions_get(const Evas_Object *o);

/**
 * Enables/disables the video text descriptions display feature.
 *
 * The video text descriptions display feature is part of track support for HTML5 video.
 *
 * @param o view object to set the video text descriptions display feature.
 * @param enable @c EINA_TRUE to enable the video text descriptions display feature,
 * @c EINA_FALSE to disable.
 */
EAPI void ewk_view_setting_should_display_text_descriptions_set(Evas_Object *o, Eina_Bool enable);

/**
 * Show the inspector to debug a web page.
 *
 * The following signals are emiited.
 * "inspector,view,create" and "inspector,view,close"
 * The first one will be called to request the view for inspector on view that will be inspected.
 * This callback should create the view for inspector and set the view with ewk_view_inspector_view_set().
 * The second one will be called to close the view for inspector on view having the inspector after disconnecting frontend
 * This callback should remove the view for inspector.
 *
 * When the view having the inspector is removed,
 * please emit the "inspector,view,destroy" signal on view for inspector.
 *
 * @param o The view to show the inspector.
 *
 * @see ewk_view_inspector_close()
 * @see ewk_view_inspector_view_set()
 */
EAPI void ewk_view_inspector_show(const Evas_Object *o);

/**
 * Close the inspector view
 *
 * @param o The view to close the inspector.
 *
 * @see ewk_view_inspector_show()
 * @see ewk_view_inspector_view_get()
 */
EAPI void ewk_view_inspector_close(const Evas_Object *o);

/**
 * Get the view of inspector.
 *
 * @param o The view that is inspected.
 *
 * @return view object on success or @c NULL on failure
 */
EAPI Evas_Object* ewk_view_inspector_view_get(const Evas_Object *o);

/**
 * Set the view of inspector.
 *
 * @param o The view that is inspected.
 * @param inspector_view The view of inspector.
 */
EAPI void ewk_view_inspector_view_set(Evas_Object *o, Evas_Object *inspector_view);

/**
 * Enables/disables the fullscreen mode by javascript fullscreen API.
 * The javascript API allows to request full screen mode, for more information see:
 * http://dvcs.w3.org/hg/fullscreen/raw-file/tip/Overview.html
 *
 * Default value for javascript fullscreen API setting is @c EINA_TRUE.
 *
 * @param o view object to enable javascript fullscreen API
 * @param enable @c EINA_TRUE to enable javascript fullscreen API or
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_view_setting_enable_fullscreen_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the fullscreen mode is enabled.
 *
 * @param o view object to query whether javascript fullscreen API is enabled
 *
 * @return @c EINA_TRUE if the javascript fullscreen API is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_view_setting_enable_fullscreen_get(const Evas_Object *o);

/**
 * Enables/disables the WebCore's tiled backing store.
 *
 * @param o view object
 * @oaram enable Enable or Disable WebCore's tiled backing store for given View
 *
 * @return true on success, or false on failure
 *
 * @note this is not for general use. It should be used for single view only.
 */
EAPI Eina_Bool ewk_view_setting_tiled_backing_store_enabled_set(Evas_Object *o, Eina_Bool enable);

/**
 * Queries if the WebCore's tiled backing store is enabled.
 *
 * @param o view object to query
 *
 * @return @c EINA_TRUE if the WebCore's tiled backing store is enabled
 *         @c EINA_FALSE if not or on failure
 */
EAPI Eina_Bool ewk_view_setting_tiled_backing_store_enabled_get(Evas_Object *o);

/**
 * Gets the context menu object.
 * @param o The view that contains context menu.
 *
 * @return context menu structure on success or @c NULL on failure
 */
EAPI Ewk_Context_Menu *ewk_view_context_menu_get(const Evas_Object *o);

#ifdef __cplusplus
}
#endif
#endif // ewk_view_h
