/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

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
 * @file    ewk_frame.h
 * @brief   WebKit frame smart object.
 *
 * This object is the low level access to WebKit-EFL browser
 * component. It represents both the main and internal frames that
 * HTML pages contain.
 *
 * Every ewk_view has at least one frame, called "main frame" and
 * retrieved with ewk_view_frame_main_get(). One can retrieve frame's
 * owner view with ewk_frame_view_get(). Parent frame can be retrieved
 * with standard smart object's evas_object_smart_parent_get().
 * Children can be accessed with ewk_frame_children_iterator_new() or
 * ewk_frame_child_find().
 *
 * The following signals (see evas_object_smart_callback_add()) are emitted:
 *
 *  - "contents,size,changed", Evas_Coord[2]: reports contents size
 *     were changed due new layout, script actions or any other events.
 *  - "editorclient,contents,changed", void: reports that editor client's
 *    contents were changed
 *  - "icon,changed", void: frame favicon changed.
 *  - "load,committed", void: reports load committed.
 *  - "load,document,finished", void: frame finished loading the document.
 *  - "load,error", const Ewk_Frame_Load_Error*: reports load failed
 *    and it gives a pointer to structure defining the error as an argument.
 *  - "load,finished", const Ewk_Frame_Load_Error*: reports load
 *    finished and it gives @c NULL on success or pointer to
 *    structure defining the error.
 *  - "load,resource,finished", unsigned long*: reports resource load finished and it gives
 *    a pointer to its identifier.
 *  - "load,resource,failed", Ewk_Frame_Load_Error*: reports resource load failure and it
 *    gives a pointer to structure defining the error as an argument.
 *  - "load,firstlayout,finished", void: frame finished first layout.
 *  - "load,nonemptylayout,finished", void: frame finished first
 *    non-empty layout.
 *  - "load,progress", double*: load progress is changed (overall value
 *    from 0.0 to 1.0, connect to individual frames for fine grained).
 *  - "load,provisional", void: frame started provisional load.
 *  - "load,provisional,failed", Ewk_Frame_Load_Error*: frame provisional load failed.
 *  - "load,started", void: frame started loading the document.
 *  - "mixedcontent,displayed", void: frame has loaded and displayed mixed content.
 *  - "mixedcontent,run", void: frame has loaded and run mixed content.
 *  - "navigation,first", void: first navigation was occurred.
 *  - "redirect,cancelled", void: client redirect was cancelled.
 *  - "redirect,load,provisional", void: received server redirect for provisional load.
 *  - "redirect,requested", const char*: url of the client redirect that will be performed.
 *  - "resource,request,new", Ewk_Frame_Resource_Request*: reports that
 *    there's a new resource request.
 *  - "resource,request,willsend", Ewk_Frame_Resource_Messages*: a resource will be requested.
 *    and the possible redirect response.
 *  - "resource,response,received", Ewk_Frame_Resource_Response*: reports that a response
 *    to a resource request was received.
 *  - "state,save", void: frame's state will be saved as a history item.
 *  - "title,changed", Ewk_Text_With_Direction*: title of the main frame was changed.
 *  - "uri,changed", const char*: uri of the main frame was changed.
 *  - "xss,detected", Ewk_Frame_Xss_Notification*: reflected XSS is encountered in the page and suppressed.
 */

#ifndef ewk_frame_h
#define ewk_frame_h

#include "ewk_security_origin.h"

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Creates a type name for _Ewk_Frame_Load_Error.
typedef struct _Ewk_Frame_Load_Error Ewk_Frame_Load_Error;
/**
 * @brief   Structure used to report load errors.
 *
 * Load errors are reported as signal by ewk_view. All the strings are
 * temporary references and should @b not be used after the signal
 * callback returns. If it's required, make copies with strdup() or
 * eina_stringshare_add() (they are not even guaranteed to be
 * stringshared, so must use eina_stringshare_add() and not
 * eina_stringshare_ref()).
 */
struct _Ewk_Frame_Load_Error {
    int code; /**< numeric error code */
    Eina_Bool is_cancellation; /**< if load failed because it was canceled */
    const char *domain; /**< error domain name */
    const char *description; /**< error description already localized */
    const char *failing_url; /**< the url that failed to load */
    unsigned long resource_identifier; /**< identifier of resource */
    Evas_Object *frame; /**< frame where the failure happened */
};

/// Creates a type name for _Ewk_Frame_Resource_Request.
typedef struct _Ewk_Frame_Resource_Request Ewk_Frame_Resource_Request;
/**
 * @brief   Structure containing details about a resource request.
 *
 * Details given before a resource is loaded on a given frame. It's used in
 * Ewk_Frame_Resource_Messages to inform about the details of a resource request.
 */
struct _Ewk_Frame_Resource_Request {
    const char *url; /**< url of the resource */
    const char *first_party; /**< first party for cookies, can not be changed */
    const char *http_method; /**< http method, can not be changed */
    const unsigned long identifier; /**< identifier of resource, can not be changed */
    Evas_Object *frame; /**< frame where the resource is requested */
    Eina_Bool is_main_frame_request; /** < indicates if the request is for the main frame */
};

/// Creates a type name for _Ewk_Frame_Resource_Response.
typedef struct _Ewk_Frame_Resource_Response Ewk_Frame_Resource_Response;

/**
 * @brief Structure containing details about a response to a resource request.
 *
 * Details given in the response to a resource request. It's used by
 * ewk_frame_response_received() to inform about the details of a response.
 */
struct _Ewk_Frame_Resource_Response {
    const char *url; /**< url of the resource */
    int status_code; /**< http status code */
    unsigned long identifier; /**< identifier of resource */
    const char *mime_type; /**< MIME type of the resource */
};

/// Creates a type name for _Ewk_Frame_Resource_Messages.
typedef struct _Ewk_Frame_Resource_Messages Ewk_Frame_Resource_Messages;

struct _Ewk_Frame_Resource_Messages {
    Ewk_Frame_Resource_Request *request; /**< resource request */
    Ewk_Frame_Resource_Response *redirect_response; /**< redirect response, can not be changed */
};

/// Enum containing text directionality values.
typedef enum {
    EWK_TEXT_DIRECTION_DEFAULT, /**< Natural writing direction ("inherit") */
    EWK_TEXT_DIRECTION_LEFT_TO_RIGHT,
    EWK_TEXT_DIRECTION_RIGHT_TO_LEFT
} Ewk_Text_Direction;

/// Creates a type name for Ewk_Text_With_Direction.
typedef struct _Ewk_Text_With_Direction Ewk_Text_With_Direction;

struct _Ewk_Text_With_Direction {
    const char *string;
    Ewk_Text_Direction direction;
};

/// Creates a type name for Ewk_Frame_Xss_Notification.
typedef struct _Ewk_Frame_Xss_Notification Ewk_Frame_Xss_Notification;

/**
 * @brief   Structure used to report reflected XSS is encountered in the page.
 *
 * This structure contains information received from the XSSAuditor when reflected XSS 
 * is encountered in the page. The string is temporary reference and should @b not 
 * be used after the signal callback returns. If it's required, make a copy of it.
 */
struct _Ewk_Frame_Xss_Notification {
    const char *insecure_url; /**< insecure url of the document */
    Eina_Bool is_entire_page_blocked; /** < indicates if the entire page was blocked by XSSAuditor */
};

/// Enum containing hit test data types
typedef enum {
    EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT = 1 << 1,
    EWK_HIT_TEST_RESULT_CONTEXT_LINK = 1 << 2,
    EWK_HIT_TEST_RESULT_CONTEXT_IMAGE = 1 << 3,
    EWK_HIT_TEST_RESULT_CONTEXT_MEDIA = 1 << 4,
    EWK_HIT_TEST_RESULT_CONTEXT_SELECTION = 1 << 5,
    EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE = 1 << 6
} Ewk_Hit_Test_Result_Context;

/// Enum containing navigation types
typedef enum  {
    EWK_NAVIGATION_TYPE_LINK_CLICKED,
    EWK_NAVIGATION_TYPE_FORM_SUBMITTED,
    EWK_NAVIGATION_TYPE_BACK_FORWARD,
    EWK_NAVIGATION_TYPE_RELOAD,
    EWK_NAVIGATION_TYPE_FORM_RESUBMITTED,
    EWK_NAVIGATION_TYPE_OTHER
} Ewk_Navigation_Type;

/// Creates a type name for _Ewk_Hit_Test.
typedef struct _Ewk_Hit_Test Ewk_Hit_Test;
/// Structure used to report hit test result.
struct _Ewk_Hit_Test {
    int x; /**< the horizontal position of the hit test */
    int y; /**< the vertical position of the hit test */
    struct {
        int x, y, w, h;
    } bounding_box; /**< DEPRECATED, see ewk_frame_hit_test_new() */
    Ewk_Text_With_Direction title; /**< title of the element */
    const char *alternate_text; /**< the alternate text for image, area, input and applet */
    Evas_Object *frame; /**< the pointer to frame where hit test was requested */
    struct {
        const char *text; /**< the text of the link */
        const char *url; /**< URL of the link */
        const char *title; /**< the title of link */
        Evas_Object *target_frame;
    } link;
    const char *image_uri;
    const char *media_uri;

    Ewk_Hit_Test_Result_Context context;
};

/// Represents actions of touch events.
typedef enum {
    EWK_TOUCH_START,
    EWK_TOUCH_MOVE,
    EWK_TOUCH_END,
    EWK_TOUCH_CANCEL
} Ewk_Touch_Event_Type;

/// Represents states of touch events.
typedef enum {
    EWK_TOUCH_POINT_RELEASED,
    EWK_TOUCH_POINT_PRESSED,
    EWK_TOUCH_POINT_MOVED,
    EWK_TOUCH_POINT_STATIONARY,
    EWK_TOUCH_POINT_CANCELLED,
    EWK_TOUCH_POINT_END
} Ewk_Touch_Point_Type;

/// Creates a type name for _Ewk_Touch_Point.
typedef struct _Ewk_Touch_Point Ewk_Touch_Point;
/// Represents a touch point.
struct _Ewk_Touch_Point {
    unsigned int id; /**< identifier of the touch event */
    int x; /**< the horizontal position of the touch event */
    int y; /**< the horizontal position of the touch event */
    Ewk_Touch_Point_Type state; /**< state of the touch event */
};

typedef enum {
    EWK_TEXT_SELECTION_NONE,
    EWK_TEXT_SELECTION_CARET,
    EWK_TEXT_SELECTION_RANGE
} Ewk_Text_Selection_Type;

/**
 * Represents the validity of a X.509 certificate related to the current frame.
 *
 * A certificate is considered untrusted when any problem is found in it: it may have
 * expired, the certificate's Common Name does not match the current host, the certificate
 * does not validate against the current set of root CA certificates etc.
 *
 * @sa ewk_frame_certificate_status_get, ewk_network_tls_ca_certificates_path_set
 */
typedef enum {
    EWK_CERTIFICATE_STATUS_NO_CERTIFICATE = 1 << 1, /**< No certificate provided (non-HTTPS connection). */
    EWK_CERTIFICATE_STATUS_UNTRUSTED = 1 << 2, /**< The certificate provided is not trusted. */
    EWK_CERTIFICATE_STATUS_TRUSTED = 1 << 3 /**< The certificate is valid and fully trusted. */
} Ewk_Certificate_Status;

/**
 * Retrieves the ewk_view object that owns this frame.
 *
 * @param o frame object to get view object
 *
 * @return view object or @c NULL on failure
 */
EAPI Evas_Object *ewk_frame_view_get(const Evas_Object *o);

/**
 * Retrieves the Ewk_Security_Origin of this frame.
 *
 * The returned object should be freed by ewk_security_origin_free().
 *
 * @param o frame object to get the security origin
 *
 * @return security origin object
 */
EAPI Ewk_Security_Origin *ewk_frame_security_origin_get(const Evas_Object *o);

/**
 * Returns a new iterator over all direct children frames.
 *
 * Keep frame object intact while iteration happens otherwise frame
 * may be destroyed while iterated.
 *
 * Iteration results are Evas_Object*, so give eina_iterator_next() a
 * pointer to it.
 *
 * Returned iterator should be freed by eina_iterator_free().
 *
 * @param o frame object to create the iterator
 *
 * @return a newly allocated iterator on sucess, or @c NULL if not possible to
 *      create the iterator
 */
EAPI Eina_Iterator *ewk_frame_children_iterator_new(Evas_Object *o);

/**
 * Finds a child frame by its name, recursively.
 *
 * For pre-defined names, returns @a o if @a name is "_self" or
 * "_current", returns @a o's parent frame if @a name is "_parent",
 * and returns the main frame if @a name is "_top". Also returns @a o
 * if it is the main frame and @a name is either "_parent" or
 * "_top". For other names, this function returns the first frame that
 * matches @a name. This function searches @a o and its descendents
 * first, then @a o's parent and its children moving up the hierarchy
 * until a match is found. If no match is found in @a o's hierarchy,
 * this function will search for a matching frame in other main frame
 * hierarchies.
 *
 * @param o frame object to find a child frame
 * @param name child frame name
 *
 * @return child frame of the given frame, or @c NULL if the the child wasn't found
 */
EAPI Evas_Object   *ewk_frame_child_find(Evas_Object *o, const char *name);

/**
 * Asks the main frame to load the given URI.
 *
 * @param o frame object to load uri
 * @param uri uniform resource identifier to load
 *
 * @return @c EINA_TRUE on success, or @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_frame_uri_set(Evas_Object *o, const char *uri);

/**
 * Gets the uri of this frame.
 *
 * It returns an internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o frame object to get uri
 *
 * @return frame uri on success or @c NULL on failure
 */
EAPI const char  *ewk_frame_uri_get(const Evas_Object *o);

/**
 * Gets the title of this frame.
 *
 * It returns an internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o frame object to get title
 *
 * @return frame title on success or @c NULL on failure
 */
EAPI const Ewk_Text_With_Direction  *ewk_frame_title_get(const Evas_Object *o);

/**
 * Gets the name of this frame.
 *
 * It returns an internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param o frame object to get name
 *
 * @return frame name on success or @c NULL on failure
 */
EAPI const char  *ewk_frame_name_get(const Evas_Object *o);

/**
 * Gets last known contents size.
 *
 * @param o frame object to get contents size
 * @param w pointer to store contents size width, may be @c NULL
 * @param h pointer to store contents size height, may be @c NULL
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure and
 *         @a w and @a h will be zeroed
 */
EAPI Eina_Bool    ewk_frame_contents_size_get(const Evas_Object *o, Evas_Coord *w, Evas_Coord *h);

/**
 * Requests loading the given contents in this frame.
 *
 * @param o frame object to load document
 * @param contents what to load into frame
 * @param contents_size size of @a contents (in bytes),
 *        if @c 0 is given, length of @a contents is used
 * @param mime_type type of @a contents data, if @c NULL is given "text/html" is assumed
 * @param encoding encoding for @a contents data, if @c NULL is given "UTF-8" is assumed
 * @param base_uri base uri to use for relative resources, may be @c NULL,
 *        if provided @b must be an absolute uri
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EAPI Eina_Bool    ewk_frame_contents_set(Evas_Object *o, const char *contents, size_t contents_size, const char *mime_type, const char *encoding, const char *base_uri);

/**
 * Requests loading alternative contents for unreachable URI in this frame.
 *
 * This is similar to ewk_frame_contents_set(), but is used when some
 * URI was failed to load, using the provided content instead. The main
 * difference is that back-forward navigation list is not changed.
 *
 * @param o frame object to load alternative content
 * @param contents what to load into frame, must @b not be @c NULL
 * @param contents_size size of @a contents (in bytes),
 *        if @c 0 is given, length of @a contents is used
 * @param mime_type type of @a contents data, if @c NULL is given "text/html" is assumed
 * @param encoding encoding used for @a contents data, if @c NULL is given "UTF-8" is assumed
 * @param base_uri base URI to use for relative resources, may be @c NULL,
 *        if provided must be an absolute uri
 * @param unreachable_uri the URI that failed to load and is getting the
 *        alternative representation, must @b not be @c NULL
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EAPI Eina_Bool    ewk_frame_contents_alternate_set(Evas_Object *o, const char *contents, size_t contents_size, const char *mime_type, const char *encoding, const char *base_uri, const char *unreachable_uri);

/**
 * Requests execution of the given script.
 *
 * The returned string @b should be freed by eina_stringshare_del() after use.
 *
 * @param o frame object to execute script
 * @param script Java Script to execute
 *
 * @return newly allocated string for result or @c NULL if the result cannot be converted to string or failure
 */
EAPI const char        *ewk_frame_script_execute(Evas_Object *o, const char *script);

/**
 * Queries if the frame is editable.
 *
 * @param o the frame object to query editable state
 *
 * @return @c EINA_TRUE if the frame is editable, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_editable_get(const Evas_Object *o);

/**
 * Sets editable state for frame.
 *
 * @param o the frame object to set editable state
 * @param editable a new state to set
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_editable_set(Evas_Object *o, Eina_Bool editable);

/**
 * Gets the copy of the selected text.
 *
 * The returned string @b should be freed by eina_stringshare_del() after use.
 *
 * @param o the frame object to get selected text
 *
 * @return a newly allocated string or @c NULL if nothing is selected or on failure
 */
EAPI const char        *ewk_frame_selection_get(const Evas_Object *o);

/**
 * Searches the given string in a document.
 *
 * @param o frame object where to search the text
 * @param string reference string to search
 * @param case_sensitive if search should be case sensitive or not
 * @param forward if search is from cursor and on or backwards
 * @param wrap if search should wrap at the end
 *
 * @return @c EINA_TRUE if the given string was found, @c EINA_FALSE if not or failure
 */
EAPI Eina_Bool    ewk_frame_text_search(const Evas_Object *o, const char *string, Eina_Bool case_sensitive, Eina_Bool forward, Eina_Bool wrap);

/**
 * Marks matches the given string in a document.
 *
 * @param o frame object where to search text
 * @param string reference string to match
 * @param case_sensitive if match should be case sensitive or not
 * @param highlight if matches should be highlighted
 * @param limit maximum amount of matches, or zero to unlimited
 *
 * @return number of matched @a string
 */
EAPI unsigned int ewk_frame_text_matches_mark(Evas_Object *o, const char *string, Eina_Bool case_sensitive, Eina_Bool highlight, unsigned int limit);

/**
 * Unmarks all marked matches in a document.
 * Reverses the effect of ewk_frame_text_matches_mark().
 *
 * @param o frame object where to unmark matches
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_frame_text_matches_unmark_all(Evas_Object *o);

/**
 * Sets whether matches marked with ewk_frame_text_matches_mark() should be highlighted.
 *
 * @param o frame object where to set if matches are highlighted or not
 * @param highlight @c EINA_TRUE if matches are highlighted, @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EAPI Eina_Bool    ewk_frame_text_matches_highlight_set(Evas_Object *o, Eina_Bool highlight);

/**
 * Returns whether matches marked with ewk_frame_text_matches_mark() are highlighted.
 *
 * @param o frame object to query if matches are highlighted or not
 *
 * @return @c EINA_TRUE if matches are highlighted, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_text_matches_highlight_get(const Evas_Object *o);

/**
 * Returns the position of the n-th matched text in the frame.
 *
 * @param o frame object where matches are marked
 * @param n index of element 
 * @param x the pointer to store the horizontal position of @a n matched text, may be @c NULL
 * @param y the pointer to store the vertical position of @a n matched text, may be @c NULL
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE when no matches were found or
 *         @a n is bigger than search results or on failure
 */
EAPI Eina_Bool    ewk_frame_text_matches_nth_pos_get(const Evas_Object *o, size_t n, int *x, int *y);

/**
 * Asks frame to stop loading.
 *
 * @param o frame object to stop loading
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_stop(Evas_Object *o);

/**
 * Asks frame to reload current document.
 *
 * @param o frame object to reload current document
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * @see ewk_frame_reload_full()
 */
EAPI Eina_Bool    ewk_frame_reload(Evas_Object *o);

/**
 * Asks frame to fully reload current document, using no previous caches.
 *
 * @param o frame object to reload current document
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_reload_full(Evas_Object *o);

/**
 * Asks the frame to navigate back in the history.
 *
 * @param o frame object to navigate back
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * @see ewk_frame_navigate()
 */
EAPI Eina_Bool    ewk_frame_back(Evas_Object *o);

/**
 * Asks frame to navigate forward in the history.
 *
 * @param o frame object to navigate forward
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 *
 * @see ewk_frame_navigate()
 */
EAPI Eina_Bool    ewk_frame_forward(Evas_Object *o);

/**
 * Navigates back or forward in the history.
 *
 * @param o frame object to navigate
 * @param steps if positive navigates that amount forwards, if negative
 *        does backwards
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_navigate(Evas_Object *o, int steps);

/**
 * Queries if it's possible to navigate backwards one item in the history.
 *
 * @param o frame object to query if backward navigation is possible
 *
 * @return @c EINA_TRUE if it's possible to navigate backwards one item in the history, @c EINA_FALSE otherwise
 *
 * @see ewk_frame_navigate_possible()
 */
EAPI Eina_Bool    ewk_frame_back_possible(Evas_Object *o);

/**
 * Queries if it's possible to navigate forwards one item in the history.
 *
 * @param o frame object to query if forward navigation is possible
 *
 * @return @c EINA_TRUE if it's possible to navigate forwards in the history, @c EINA_FALSE otherwise
 *
 * @see ewk_frame_navigate_possible()
 */
EAPI Eina_Bool    ewk_frame_forward_possible(Evas_Object *o);

/**
 * Queries if it's possible to navigate given @a steps in the history.
 *
 * @param o frame object to query if is possible to navigate @a steps in the history
 * @param steps positive value navigates that amount forwards, negative value 
 *        does backwards
 *
 * @return @c EINA_TRUE if it's possible to navigate @a steps in the history, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_navigate_possible(Evas_Object *o, int steps);

/**
 * Gets the current page zoom level used by this frame.
 *
 * @param o frame object to get zoom level
 *
 * @return page zoom level for the frame or @c -1.0 on failure
 *
 * @see ewk_frame_text_zoom_get()
 */
EAPI float        ewk_frame_page_zoom_get(const Evas_Object *o);

/**
 * Sets the current page zoom level used by this frame.
 *
 * @param o frame object to change zoom level
 * @param page_zoom_factor a new zoom level
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_frame_text_zoom_set()
 */
EAPI Eina_Bool    ewk_frame_page_zoom_set(Evas_Object *o, float page_zoom_factor);

/**
 * Gets the current text zoom level used by this frame.
 *
 * @param o frame object to get zoom level
 *
 * @return text zoom level for the frame or @c -1.0 on failure
 *
 * @see ewk_frame_page_zoom_get()
 */
EAPI float        ewk_frame_text_zoom_get(const Evas_Object *o);

/**
 * Sets the current text zoom level used by this frame.
 *
 * @param o frame object to change zoom level
 * @param textZoomFactor a new zoom level
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_frame_page_zoom_set()
 */
EAPI Eina_Bool    ewk_frame_text_zoom_set(Evas_Object *o, float text_zoom_factor);

/**
 * Frees hit test instance created by ewk_frame_hit_test_new().
 *
 * @param hit_test instance
 */
EAPI void          ewk_frame_hit_test_free(Ewk_Hit_Test *hit_test);

/**
 * Creates a new hit test for the given frame and point.
 *
 * The returned object should be freed by ewk_frame_hit_test_free().
 *
 * @param o frame object to do hit test on
 * @param x the horizontal position to query
 * @param y the vertical position to query
 *
 * @return a newly allocated hit test on success, @c NULL otherwise
 */
EAPI Ewk_Hit_Test *ewk_frame_hit_test_new(const Evas_Object *o, int x, int y);

/**
 * Sets a relative scroll of the given frame.
 *
 * This function does scroll @a dx and @a dy pixels
 * from the current position of scroll.
 *
 * @param o frame object to scroll
 * @param dx horizontal offset to scroll
 * @param dy vertical offset to scroll
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_scroll_add(Evas_Object *o, int dx, int dy);

/**
 * Sets an absolute scroll of the given frame.
 *
 * Both values are from zero to the contents size minus the viewport
 * size. See ewk_frame_scroll_size_get().
 *
 * @param o frame object to scroll
 * @param x horizontal position to scroll
 * @param y vertical position to scroll
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_scroll_set(Evas_Object *o, int x, int y);

/**
 * Gets the possible scroll size of the given frame.
 *
 * Possible scroll size is contents size minus the viewport
 * size. It's the last allowed value for ewk_frame_scroll_set()
 *
 * @param o frame object to get scroll size
 * @param w the pointer to store the horizontal size that is possible to scroll,
 *        may be @c NULL
 * @param h the pointer to store the vertical size that is possible to scroll,
 *        may be @c NULL
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise and
 *         values are zeroed
 */
EAPI Eina_Bool    ewk_frame_scroll_size_get(const Evas_Object *o, int *w, int *h);

/**
 * Gets the current scroll position of given frame.
 *
 * @param o frame object to get the current scroll position
 * @param x the pointer to store the horizontal position, may be @c NULL
 * @param y the pointer to store the vertical position. may be @c NULL
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise and
 *         values are zeroed.
 */
EAPI Eina_Bool    ewk_frame_scroll_pos_get(const Evas_Object *o, int *x, int *y);

/**
 * Gets the visible content geometry of the frame.
 *
 * @param o frame object to query visible content geometry
 * @param include_scrollbars whenever to include scrollbars size
 * @param x the pointer to store the horizontal position, may be @c NULL
 * @param y the pointer to store the vertical position, may be @c NULL
 * @param w the pointer to store width, may be @c NULL
 * @param h the pointer to store height, may be @c NULL
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise and
 *         values are zeroed
 */
EAPI Eina_Bool    ewk_frame_visible_content_geometry_get(const Evas_Object *o, Eina_Bool include_scrollbars, int *x, int *y, int *w, int *h);

/**
 * Queries if the frame should be repainted completely.
 *
 * Function tells if dirty areas should be repainted 
 * even if they are out of the screen.
 *
 * @param o frame object to query if the frame should be repainted completely
 *
 * @return @c EINA_TRUE if any dirty areas should be repainted, @c EINA_FALSE
 *         otherwise
 */
EAPI Eina_Bool    ewk_frame_paint_full_get(const Evas_Object *o);

/**
 * Sets if the frame should be repainted completely.
 *
 * Function sets if dirty areas should be repainted 
 * even if they are out of the screen.
 *
 * @param o frame object to set if the frame should be repainted completely
 * @param flag @c EINA_TRUE to repaint the frame completely,
 *             @c EINA_FALSE if not
 */
EAPI void         ewk_frame_paint_full_set(Evas_Object *o, Eina_Bool flag);

/**
 * Feeds the focus in event to the frame.
 *
 * @param o frame object to feed focus
 *
 * @return @c EINA_TRUE if the focus was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_focus_in(Evas_Object *o);

/**
 * Feeds the focus out event to the frame.
 *
 * @param o frame object to remove focus
 *
 * @return @c EINA_FALSE since the feature is not implemented
 */
EAPI Eina_Bool    ewk_frame_feed_focus_out(Evas_Object *o);

/**
 * Gets the geometry, relative to the frame, of the focused element in the
 * document.
 *
 * @param o frame object containing the focused element
 * @param x pointer where to store the X value of the geometry, may be @c NULL
 * @param x pointer where to store the Y value of the geometry, may be @c NULL
 * @param x pointer where to store width of the geometry, may be @c NULL
 * @param x pointer where to store height of the geometry, may be @c NULL
 *
 * @return @c EINA_TRUE if the frame contains the currently focused element and
 * its geometry was correctly fetched, @c EINA_FALSE in any other case
 */
EAPI Eina_Bool ewk_frame_focused_element_geometry_get(const Evas_Object *o, int *x, int *y, int *w, int *h);

/**
 * Feeds the mouse wheel event to the frame.
 *
 * @param o frame object to feed the mouse wheel event
 * @param ev the mouse wheel event
 *
 * @return @c EINA_TRUE if the mouse wheel event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_mouse_wheel(Evas_Object *o, const Evas_Event_Mouse_Wheel *ev);

/**
 * Feeds the mouse down event to the frame.
 *
 * @param o frame object to feed the mouse down event
 * @param ev the mouse down event
 *
 * @return @c EINA_TRUE if the mouse down event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_mouse_down(Evas_Object *o, const Evas_Event_Mouse_Down *ev);

/**
 * Feeds the mouse up event to the frame.
 *
 * @param o frame object to feed the mouse up event
 * @param ev the mouse up event
 *
 * @return @c EINA_TRUE if the mouse up event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_mouse_up(Evas_Object *o, const Evas_Event_Mouse_Up *ev);

/**
 * Feeds the mouse move event to the frame.
 *
 * @param o frame object to feed the mouse move event
 * @param ev the mouse move event
 *
 * @return @c EINA_TRUE if the mouse move event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_mouse_move(Evas_Object *o, const Evas_Event_Mouse_Move *ev);

/**
 * Feeds the touch event to the frame.
 *
 * @param o frame object to feed touch event
 * @param action the action of touch event
 * @param points a list of points (Ewk_Touch_Point) to process
 * @param metaState modifiers state of touch event. Users are expected to pass ORed values of the ECORE_EVENT_MODIFIER macros in Ecore_Input.h, such as ECORE_EVENT_MODIFIER_ALT or ECORE_EVENT_MODIFIER_SHIFT
 *
 * @return @c EINA_TRUE if touch event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_touch_event(Evas_Object *o, Ewk_Touch_Event_Type action, Eina_List *points, unsigned modifiers);

/**
 * Feeds the keyboard key down event to the frame.
 *
 * @param o frame object to feed event
 * @param ev keyboard key down event
 *
 * @return @c EINA_TRUE if the key down event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_key_down(Evas_Object *o, const Evas_Event_Key_Down *ev);

/**
 * Feeds the keyboard key up event to the frame.
 *
 * @param o frame object to feed event
 * @param ev keyboard key up event
 *
 * @return @c EINA_TRUE if the key up event was handled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool    ewk_frame_feed_key_up(Evas_Object *o, const Evas_Event_Key_Up *ev);

/**
 * Returns current text selection type.
 *
 * @param o a frame object to check selection type
 * @return current text selection type on success or no selection otherwise
 */
EAPI Ewk_Text_Selection_Type ewk_frame_text_selection_type_get(const Evas_Object *o);

/**
 * Gets the frame source.
 *
 * It's part of HTML saving feature. Currently only HTML documents are supported.
 *
 * @param o frame smart object to get the frame source
 * @param frame_source a pointer to store the source of frame,
 *        must @b not be @c NULL, this value @b should be freed after use
 *
 * @return @c length of @a frame_source on success, or @c -1 on failure
 *
 * @see ewk_frame_resources_location_get()
 */
EAPI ssize_t ewk_frame_source_get(const Evas_Object *o, char **frame_source);

/**
 * Gets the resource list of this frame.
 *
 * It's part of HTML saving feature. Currently only locations of images are supported.
 * An application might find these values in frame source and
 * replace them to the local paths. Values are not duplicated and they are decoded.
 *
 * @param o frame smart object to get the resources list
 * @return @c Eina_List with location of resources on success, or @c NULL on failure,
 *         the Eina_List should be freed after use
 *
 * @see ewk_frame_source_get()
 */
EAPI Eina_List *ewk_frame_resources_location_get(const Evas_Object *o);

/**
 * Retrieve the frame's contents in plain text.
 *
 * This function returns the contents of the given frame converted to plain text,
 * removing all the HTML formatting.
 *
 * The returned string @b should be freed by eina_stringshare_del() after use.
 *
 * @param ewkFrame Frame object whose contents to retrieve.
 *
 * @return A newly allocated string (which must be freed by the caller with @c free())
 *         or @c NULL in case of failure.
 */
EAPI const char *ewk_frame_plain_text_get(const Evas_Object *o);

/**
 * Returns whether the frame has displayed mixed content.
 *
 * When a frame has displayed mixed content, its current URI is an HTTPS one, but it has
 * loaded and displayed a resource (such as an image) from an insecure (non-HTTPS) URI.
 * Both the frame and the container view send a "mixedcontent,displayed" signal in this case.
 *
 * The status is reset only when a load event occurs (eg. the page is reloaded or a new page is loaded).
 *
 * @param o The frame to query.
 *
 * @sa ewk_view_mixed_content_displayed_get
 */
EAPI Eina_Bool ewk_frame_mixed_content_displayed_get(const Evas_Object *o);

/**
 * Returns whether the frame has run mixed content.
 *
 * When a frame has run mixed content, its current URI is an HTTPS one, but it has
 * loaded and run a resource (such as an image) from an insecure (non-HTTPS) URI.
 * Both the frame and the container view send a "mixedcontent,run" signal in this case.
 *
 * The status is reset only when a load event occurs (eg. the page is reloaded or a new page is loaded).
 *
 * @param o The frame to query.
 *
 * @sa ewk_view_mixed_content_run_get
 */
EAPI Eina_Bool ewk_frame_mixed_content_run_get(const Evas_Object *o);

/**
 * Returns the validity of the X.509 certificate related to the current frame.
 */
EAPI Ewk_Certificate_Status ewk_frame_certificate_status_get(Evas_Object *o);

#ifdef __cplusplus
}
#endif
#endif // ewk_frame_h
