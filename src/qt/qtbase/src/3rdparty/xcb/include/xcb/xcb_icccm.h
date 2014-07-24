#ifndef __XCB_ICCCM_H__
#define __XCB_ICCCM_H__

/*
 * Copyright (C) 2008 Arnaud Fontaine <arnau@debian.org>
 * Copyright (C) 2007-2008 Vincent Torri <vtorri@univ-evry.fr>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or
 * their institutions shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization from the authors.
 */

/**
 * @defgroup xcb__icccm_t XCB ICCCM Functions
 *
 * These functions allow easy handling of the protocol described in the
 * Inter-Client Communication Conventions Manual.
 *
 * @{
 */

#include <xcb/xcb.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief TextProperty reply structure.
 */
typedef struct {
/** Store reply to avoid memory allocation, should normally not be
    used directly */
xcb_get_property_reply_t *_reply;
/** Encoding used */
xcb_atom_t encoding;
/** Length of the name field above */
uint32_t name_len;
/** Property value */
char *name;
/** Format, may be 8, 16 or 32 */
uint8_t format;
} xcb_icccm_get_text_property_reply_t;

/**
 * @brief Deliver a GetProperty request to the X server.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param property Property atom to get.
 * @return The request cookie.
 *
 * Allow to get a window property, in most case you might want to use
 * above functions to get an ICCCM property for a given window.
 */
xcb_get_property_cookie_t xcb_icccm_get_text_property(xcb_connection_t *c,
                                                        xcb_window_t window,
                                                        xcb_atom_t property);

/**
 * @see xcb_icccm_get_text_property()
 */
xcb_get_property_cookie_t xcb_icccm_get_text_property_unchecked(xcb_connection_t *c,
                                                                  xcb_window_t window,
                                                                  xcb_atom_t property);

/**
 * @brief Fill given structure with the property value of a window.
 * @param c The connection to the X server.
 * @param cookie TextProperty request cookie.
 * @param prop TextProperty reply which is to be filled.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * If the function return 0 (failure), the content of prop is unmodified and
 * therefore the structure must not be wiped.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_text_property_unchecked() is used.  Otherwise, it stores
 * the error if any.  prop structure members should be freed by
 * xcb_icccm_get_text_property_reply_wipe().
 */
uint8_t xcb_icccm_get_text_property_reply(xcb_connection_t *c,
                                            xcb_get_property_cookie_t cookie,
                                            xcb_icccm_get_text_property_reply_t *prop,
                                            xcb_generic_error_t **e);

/**
 * @brief Wipe prop structure members previously allocated by
 *        xcb_icccm_get_text_property_reply().
 * @param prop prop structure whose members is going to be freed.
 */
void xcb_icccm_get_text_property_reply_wipe(xcb_icccm_get_text_property_reply_t *prop);

/* WM_NAME */

/**
 * @brief Deliver a SetProperty request to set WM_NAME property value.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param encoding Encoding used for the data passed in the name parameter, the set property will also have this encoding as its type.
 * @param format Encoding format.
 * @param name_len Length of name value to set.
 * @param name Name value to set.
 */
xcb_void_cookie_t xcb_icccm_set_wm_name_checked(xcb_connection_t *c,
                                                  xcb_window_t window,
                                                  xcb_atom_t encoding,
                                                  uint8_t format,
                                                  uint32_t name_len,
                                                  const char *name);

/**
 * @see xcb_icccm_set_wm_name_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_name(xcb_connection_t *c, xcb_window_t window,
                                          xcb_atom_t encoding, uint8_t format,
                                          uint32_t name_len, const char *name);

/**
 * @brief Deliver a GetProperty request to the X server for WM_NAME.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_name(xcb_connection_t *c,
                                                  xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_name()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_name_unchecked(xcb_connection_t *c,
                                                            xcb_window_t window);

/**
 * @brief Fill given structure with the WM_NAME property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param prop WM_NAME property value.
 * @param e Error if any.
 * @see xcb_icccm_get_text_property_reply()
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t xcb_icccm_get_wm_name_reply(xcb_connection_t *c,
                                      xcb_get_property_cookie_t cookie,
                                      xcb_icccm_get_text_property_reply_t *prop,
                                      xcb_generic_error_t **e);

/* WM_ICON_NAME */

/**
 * @brief Deliver a SetProperty request to set WM_ICON_NAME property value.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param encoding Encoding used for the data passed in the name parameter, the set property will also have this encoding as its type.
 * @param format Encoding format.
 * @param name_len Length of name value to set.
 * @param name Name value to set.
 */
xcb_void_cookie_t xcb_icccm_set_wm_icon_name_checked(xcb_connection_t *c,
                                                       xcb_window_t window,
                                                       xcb_atom_t encoding,
                                                       uint8_t format,
                                                       uint32_t name_len,
                                                       const char *name);

/**
 * @see xcb_icccm_set_wm_icon_name_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_icon_name(xcb_connection_t *c,
                                               xcb_window_t window,
                                               xcb_atom_t encoding,
                                               uint8_t format,
                                               uint32_t name_len,
                                               const char *name);

/**
 * @brief Send request to get WM_ICON_NAME property of a window.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_icon_name(xcb_connection_t *c,
                                                       xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_icon_name()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_icon_name_unchecked(xcb_connection_t *c,
                                                                 xcb_window_t window);

/**
 * @brief Fill given structure with the WM_ICON_NAME property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param prop WM_ICON_NAME property value.
 * @param e Error if any.
 * @see xcb_icccm_get_text_property_reply()
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t xcb_icccm_get_wm_icon_name_reply(xcb_connection_t *c,
                                           xcb_get_property_cookie_t cookie,
                                           xcb_icccm_get_text_property_reply_t *prop,
                                           xcb_generic_error_t **e);

/* WM_COLORMAP_WINDOWS */

/**
 * @brief Deliver a ChangeProperty request to set WM_COLORMAP_WINDOWS property value.
 * @param c The connection to the X server.
 * @param wm_colormap_windows The WM_COLORMAP_WINDOWS atom
 * @param window Window X identifier.
 * @param list_len Windows list len.
 * @param list Windows list.
 * @return The request cookie.
 */
xcb_void_cookie_t xcb_icccm_set_wm_colormap_windows_checked(xcb_connection_t *c,
                                                              xcb_window_t window,
                                                              xcb_atom_t wm_colormap_windows_atom,
                                                              uint32_t list_len,
                                                              const xcb_window_t *list);

/**
 * @see xcb_icccm_set_wm_colormap_windows_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_colormap_windows(xcb_connection_t *c,
                                                      xcb_window_t window,
                                                      xcb_atom_t wm_colormap_windows_atom,
                                                      uint32_t list_len,
                                                      const xcb_window_t *list);

/**
 * @brief WM_COLORMAP_WINDOWS structure.
 */
typedef struct {
/** Length of the windows list */
uint32_t windows_len;
/** Windows list */
xcb_window_t *windows;
/** Store reply to avoid memory allocation, should normally not be
    used directly */
xcb_get_property_reply_t *_reply;
} xcb_icccm_get_wm_colormap_windows_reply_t;

/**
 * @brief Send request to get WM_COLORMAP_WINDOWS property of a given window.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_colormap_windows(xcb_connection_t *c,
                                                              xcb_window_t window,
                                                              xcb_atom_t wm_colormap_windows_atom);

/**
 * @see xcb_icccm_get_wm_colormap_windows()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_colormap_windows_unchecked(xcb_connection_t *c,
                                                                        xcb_window_t window,
                                                                        xcb_atom_t wm_colormap_windows_atom);

/**
 * @brief Fill the given structure with the WM_COLORMAP_WINDOWS property of a window.
 * @param reply The reply of the GetProperty request.
 * @param colormap_windows WM_COLORMAP property value.
 * @return Return 1 on success, 0 otherwise.
 *
 * protocols structure members should be freed by
 * xcb_icccm_get_wm_protocols_reply_wipe().
 */
uint8_t xcb_icccm_get_wm_colormap_windows_from_reply(xcb_get_property_reply_t *reply,
                                                       xcb_icccm_get_wm_colormap_windows_reply_t *colormap_windows);
/**
 * @brief Fill the given structure with the WM_COLORMAP_WINDOWS property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param protocols WM_COLORMAP_WINDOWS property value.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_colormap_windows_unchecked() is used.  Otherwise, it
 * stores the error if any. protocols structure members should be
 * freed by xcb_icccm_get_wm_colormap_windows_reply_wipe().
 */
uint8_t xcb_icccm_get_wm_colormap_windows_reply(xcb_connection_t *c,
                                                  xcb_get_property_cookie_t cookie,
                                                  xcb_icccm_get_wm_colormap_windows_reply_t *windows,
                                                  xcb_generic_error_t **e);

/**
 * @brief Wipe protocols structure members previously allocated by
 *        xcb_icccm_get_wm_colormap_windows_reply().
 * @param windows windows structure whose members is going to be freed.
 */
void xcb_icccm_get_wm_colormap_windows_reply_wipe(xcb_icccm_get_wm_colormap_windows_reply_t *windows);

/* WM_CLIENT_MACHINE */

/**
 * @brief Deliver a SetProperty request to set WM_CLIENT_MACHINE property value.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param encoding Encoding used for the data passed in the name parameter, the set property will also have this encoding as its type.
 * @param format Encoding format.
 * @param name_len Length of name value to set.
 * @param name Name value to set.
 */
xcb_void_cookie_t xcb_icccm_set_wm_client_machine_checked(xcb_connection_t *c,
                                                            xcb_window_t window,
                                                            xcb_atom_t encoding,
                                                            uint8_t format,
                                                            uint32_t name_len,
                                                            const char *name);

/**
 * @see xcb_icccm_set_wm_client_machine_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_client_machine(xcb_connection_t *c,
                                                    xcb_window_t window,
                                                    xcb_atom_t encoding,
                                                    uint8_t format,
                                                    uint32_t name_len,
                                                    const char *name);

/**
 * @brief Send request to get WM_CLIENT_MACHINE property of a window.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_client_machine(xcb_connection_t *c,
                                                            xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_client_machine()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_client_machine_unchecked(xcb_connection_t *c,
                                                                      xcb_window_t window);

/**
 * @brief Fill given structure with the WM_CLIENT_MACHINE property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param prop WM_CLIENT_MACHINE property value.
 * @param e Error if any.
 * @see xcb_icccm_get_text_property_reply()
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t xcb_icccm_get_wm_client_machine_reply(xcb_connection_t *c,
                                                xcb_get_property_cookie_t cookie,
                                                xcb_icccm_get_text_property_reply_t *prop,
                                                xcb_generic_error_t **e);

/* WM_CLASS */

/**
 * @brief WM_CLASS hint structure
 */

/**
 * @brief Deliver a SetProperty request to set WM_CLASS property value.
 *
 * WM_CLASS string is a concatenation of the instance and class name
 * strings respectively (including null character).
 *
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param class_len Length of WM_CLASS string.
 * @param class_name WM_CLASS string.
 * @return The request cookie.
 */
xcb_void_cookie_t xcb_icccm_set_wm_class_checked(xcb_connection_t *c,
                                                   xcb_window_t window,
                                                   uint32_t class_len,
                                                   const char *class_name);

/**
 * @see xcb_icccm_set_wm_class_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_class(xcb_connection_t *c,
                                           xcb_window_t window,
                                           uint32_t class_len,
                                           const char *class_name);

typedef struct {
/** Instance name */
char *instance_name;
/** Class of application */
char *class_name;
/** Store reply to avoid memory allocation, should normally not be
    used directly */
xcb_get_property_reply_t *_reply;
} xcb_icccm_get_wm_class_reply_t;

/**
 * @brief Deliver a GetProperty request to the X server for WM_CLASS.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_class(xcb_connection_t *c,
                                                   xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_class()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_class_unchecked(xcb_connection_t *c,
                                                             xcb_window_t window);


/**
 * @brief Fill give structure with the WM_CLASS property of a window.
 * @param prop The property structure to fill.
 * @param reply The property request reply.
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t
xcb_icccm_get_wm_class_from_reply(xcb_icccm_get_wm_class_reply_t *prop,
                                    xcb_get_property_reply_t *reply);

/**
 * @brief Fill given structure with the WM_CLASS property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param prop WM_CLASS property value.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_class_unchecked() is used.  Otherwise, it stores the
 * error if any. prop structure members should be freed by
 * xcb_icccm_get_wm_class_reply_wipe().
 */
uint8_t xcb_icccm_get_wm_class_reply(xcb_connection_t *c,
                                       xcb_get_property_cookie_t cookie,
                                       xcb_icccm_get_wm_class_reply_t *prop,
                                       xcb_generic_error_t **e);

/**
 * @brief Wipe prop structure members previously allocated by
 *        xcb_icccm_get_wm_class_reply().
 * @param prop prop structure whose members is going to be freed.
 */
void xcb_icccm_get_wm_class_reply_wipe(xcb_icccm_get_wm_class_reply_t *prop);

/* WM_TRANSIENT_FOR */

/**
 * @brief Deliver a SetProperty request to set WM_TRANSIENT_FOR property value.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param transient_for_window The WM_TRANSIENT_FOR window X identifier.
 * @return The request cookie.
 */
xcb_void_cookie_t xcb_icccm_set_wm_transient_for_checked(xcb_connection_t *c,
                                                           xcb_window_t window,
                                                           xcb_window_t transient_for_window);

/**
 * @see xcb_icccm_set_wm_transient_for
 */
xcb_void_cookie_t xcb_icccm_set_wm_transient_for(xcb_connection_t *c,
                                                   xcb_window_t window,
                                                   xcb_window_t transient_for_window);

/**
 * @brief Send request to get WM_TRANSIENT_FOR property of a window.
 * @param c The connection to the X server
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_transient_for(xcb_connection_t *c,
                                                           xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_transient_for_unchecked()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_transient_for_unchecked(xcb_connection_t *c,
                                                                     xcb_window_t window);

/**
 * @brief Fill given window pointer with the WM_TRANSIENT_FOR property of a window.
 * @param prop WM_TRANSIENT_FOR property value.
 * @param reply The get property request reply.
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t
xcb_icccm_get_wm_transient_for_from_reply(xcb_window_t *prop,
                                            xcb_get_property_reply_t *reply);
/**
 * @brief Fill given structure with the WM_TRANSIENT_FOR property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param prop WM_TRANSIENT_FOR property value.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_transient_for_unchecked() is used.  Otherwise, it stores
 * the error if any.
 */
uint8_t xcb_icccm_get_wm_transient_for_reply(xcb_connection_t *c,
                                               xcb_get_property_cookie_t cookie,
                                               xcb_window_t *prop,
                                               xcb_generic_error_t **e);

/* WM_SIZE_HINTS */

typedef enum {
XCB_ICCCM_SIZE_HINT_US_POSITION = 1 << 0,
  XCB_ICCCM_SIZE_HINT_US_SIZE = 1 << 1,
  XCB_ICCCM_SIZE_HINT_P_POSITION = 1 << 2,
  XCB_ICCCM_SIZE_HINT_P_SIZE = 1 << 3,
  XCB_ICCCM_SIZE_HINT_P_MIN_SIZE = 1 << 4,
  XCB_ICCCM_SIZE_HINT_P_MAX_SIZE = 1 << 5,
  XCB_ICCCM_SIZE_HINT_P_RESIZE_INC = 1 << 6,
  XCB_ICCCM_SIZE_HINT_P_ASPECT = 1 << 7,
  XCB_ICCCM_SIZE_HINT_BASE_SIZE = 1 << 8,
  XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY = 1 << 9
  } xcb_icccm_size_hints_flags_t;

/**
 * @brief Size hints structure.
 */
typedef struct {
/** User specified flags */
uint32_t flags;
/** User-specified position */
int32_t x, y;
/** User-specified size */
int32_t width, height;
/** Program-specified minimum size */
int32_t min_width, min_height;
/** Program-specified maximum size */
int32_t max_width, max_height;
/** Program-specified resize increments */
int32_t width_inc, height_inc;
/** Program-specified minimum aspect ratios */
int32_t min_aspect_num, min_aspect_den;
/** Program-specified maximum aspect ratios */
int32_t max_aspect_num, max_aspect_den;
/** Program-specified base size */
int32_t base_width, base_height;
/** Program-specified window gravity */
uint32_t win_gravity;
} xcb_size_hints_t;

/** Number of elements in this structure */
#define XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS 18

/**
 * @brief Set size hints to a given position.
 * @param hints SIZE_HINTS structure.
 * @param user_specified Is the size user-specified?
 * @param x The X position.
 * @param y The Y position.
 */
void xcb_icccm_size_hints_set_position(xcb_size_hints_t *hints, int user_specified,
                                         int32_t x, int32_t y);

/**
 * @brief Set size hints to a given size.
 * @param hints SIZE_HINTS structure.
 * @param user_specified is the size user-specified?
 * @param width The width.
 * @param height The height.
 */
void xcb_icccm_size_hints_set_size(xcb_size_hints_t *hints, int user_specified,
                                     int32_t width, int32_t height);

/**
 * @brief Set size hints to a given minimum size.
 * @param hints SIZE_HINTS structure.
 * @param width The minimum width.
 * @param height The minimum height.
 */
void xcb_icccm_size_hints_set_min_size(xcb_size_hints_t *hints, int32_t min_width,
                                         int32_t min_height);

/**
 * @brief Set size hints to a given maximum size.
 * @param hints SIZE_HINTS structure.
 * @param width The maximum width.
 * @param height The maximum height.
 */
void xcb_icccm_size_hints_set_max_size(xcb_size_hints_t *hints, int32_t max_width,
                                         int32_t max_height);

/**
 * @brief Set size hints to a given resize increments.
 * @param hints SIZE_HINTS structure.
 * @param width The resize increments width.
 * @param height The resize increments height.
 */
void xcb_icccm_size_hints_set_resize_inc(xcb_size_hints_t *hints, int32_t width_inc,
                                           int32_t height_inc);

/**
 * @brief Set size hints to a given aspect ratios.
 * @param hints SIZE_HINTS structure.
 * @param min_aspect_num The minimum aspect ratios for the width.
 * @param min_aspect_den The minimum aspect ratios for the height.
 * @param max_aspect_num The maximum aspect ratios for the width.
 * @param max_aspect_den The maximum aspect ratios for the height.
 */
void xcb_icccm_size_hints_set_aspect(xcb_size_hints_t *hints, int32_t min_aspect_num,
                                       int32_t min_aspect_den, int32_t max_aspect_num,
                                     int32_t max_aspect_den);

/**
 * @brief Set size hints to a given base size.
 * @param hints SIZE_HINTS structure.
 * @param base_width Base width.
 * @param base_height Base height.
 */
void xcb_icccm_size_hints_set_base_size(xcb_size_hints_t *hints, int32_t base_width,
                                          int32_t base_height);

/**
 * @brief Set size hints to a given window gravity.
 * @param hints SIZE_HINTS structure.
 * @param win_gravity Window gravity value.
 */
void xcb_icccm_size_hints_set_win_gravity(xcb_size_hints_t *hints,
                                            xcb_gravity_t win_gravity);

/**
 * @brief Deliver a ChangeProperty request to set a value to a given property.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param property Property to set value for.
 * @param hints Hints value to set.
 */
xcb_void_cookie_t xcb_icccm_set_wm_size_hints_checked(xcb_connection_t *c,
                                                        xcb_window_t window,
                                                      xcb_atom_t property,
                                                      xcb_size_hints_t *hints);

/**
 * @see xcb_icccm_set_wm_size_hints_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_size_hints(xcb_connection_t *c,
                                                xcb_window_t window,
                                              xcb_atom_t property,
                                              xcb_size_hints_t *hints);

/**
 * @brief Send request to get size hints structure for the named property.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param property Specify the property name.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_size_hints(xcb_connection_t *c,
                                                        xcb_window_t window,
                                                      xcb_atom_t property);

/**
 * @see xcb_icccm_get_wm_size_hints()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_size_hints_unchecked(xcb_connection_t *c,
                                                                  xcb_window_t window,
                                                                xcb_atom_t property);

/**
 * @brief Fill given structure with the size hints of the named property.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param hints Size hints structure.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_size_hints_unchecked() is used.  Otherwise, it stores
 * the error if any. The returned pointer should be freed.
 */
uint8_t xcb_icccm_get_wm_size_hints_reply(xcb_connection_t *c,
                                            xcb_get_property_cookie_t cookie,
                                          xcb_size_hints_t *hints,
                                          xcb_generic_error_t **e);

/* WM_NORMAL_HINTS */

/**
 * @brief Deliver a ChangeProperty request to set WM_NORMAL_HINTS property value.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param hints Hints value to set.
 */
xcb_void_cookie_t xcb_icccm_set_wm_normal_hints_checked(xcb_connection_t *c,
                                                          xcb_window_t window,
                                                        xcb_size_hints_t *hints);

/**
 * @see xcb_icccm_set_wm_normal_hints_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_normal_hints(xcb_connection_t *c,
                                                  xcb_window_t window,
                                                xcb_size_hints_t *hints);

/**
 * @brief Send request to get WM_NORMAL_HINTS property of a window.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_normal_hints(xcb_connection_t *c,
                                                          xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_normal_hints()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_normal_hints_unchecked(xcb_connection_t *c,
                                                                    xcb_window_t window);

/**
 * @brief Fill given structure with the WM_NORMAL_HINTS property of a window.
 * @param hints WM_NORMAL_HINTS property value.
 * @param reply The get property request reply.
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t
xcb_icccm_get_wm_size_hints_from_reply(xcb_size_hints_t *hints,
                                         xcb_get_property_reply_t *reply);

/**
 * @brief Fill given structure with the WM_NORMAL_HINTS property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param hints WM_NORMAL_HINTS property value.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_normal_hints_unchecked() is used.  Otherwise, it stores
 * the error if any. The returned pointer should be freed.
 */
uint8_t xcb_icccm_get_wm_normal_hints_reply(xcb_connection_t *c,
                                              xcb_get_property_cookie_t cookie,
                                            xcb_size_hints_t *hints,
                                            xcb_generic_error_t **e);

/* WM_HINTS */

/**
 * @brief WM hints structure (may be extended in the future).
 */
typedef struct {
/** Marks which fields in this structure are defined */
int32_t flags;
/** Does this application rely on the window manager to get keyboard
    input? */
  uint32_t input;
  /** See below */
  int32_t initial_state;
  /** Pixmap to be used as icon */
  xcb_pixmap_t icon_pixmap;
  /** Window to be used as icon */
  xcb_window_t icon_window;
  /** Initial position of icon */
  int32_t icon_x, icon_y;
  /** Icon mask bitmap */
  xcb_pixmap_t icon_mask;
  /* Identifier of related window group */
  xcb_window_t window_group;
} xcb_icccm_wm_hints_t;

/** Number of elements in this structure */
#define XCB_ICCCM_NUM_WM_HINTS_ELEMENTS 9

/**
 * @brief WM_HINTS window states.
 */
typedef enum {
  XCB_ICCCM_WM_STATE_WITHDRAWN = 0,
  XCB_ICCCM_WM_STATE_NORMAL = 1,
  XCB_ICCCM_WM_STATE_ICONIC = 3
} xcb_icccm_wm_state_t;

typedef enum {
  XCB_ICCCM_WM_HINT_INPUT = (1L << 0),
  XCB_ICCCM_WM_HINT_STATE = (1L << 1),
  XCB_ICCCM_WM_HINT_ICON_PIXMAP = (1L << 2),
  XCB_ICCCM_WM_HINT_ICON_WINDOW = (1L << 3),
  XCB_ICCCM_WM_HINT_ICON_POSITION = (1L << 4),
  XCB_ICCCM_WM_HINT_ICON_MASK = (1L << 5),
  XCB_ICCCM_WM_HINT_WINDOW_GROUP = (1L << 6),
  XCB_ICCCM_WM_HINT_X_URGENCY = (1L << 8)
} xcb_icccm_wm_t;

#define XCB_ICCCM_WM_ALL_HINTS (XCB_ICCCM_WM_HINT_INPUT | XCB_ICCCM_WM_HINT_STATE | \
                                XCB_ICCCM_WM_HINT_ICON_PIXMAP | XCB_ICCCM_WM_HINT_ICON_WINDOW | \
                                XCB_ICCCM_WM_HINT_ICON_POSITION | XCB_ICCCM_WM_HINT_ICON_MASK | \
                                XCB_ICCCM_WM_HINT_WINDOW_GROUP)

/**
 * @brief Get urgency hint.
 * @param hints WM_HINTS structure.
 * @return Urgency hint value.
 */
uint32_t xcb_icccm_wm_hints_get_urgency(xcb_icccm_wm_hints_t *hints);

/**
 * @brief Set input focus.
 * @param hints WM_HINTS structure.
 * @param input Input focus.
 */
void xcb_icccm_wm_hints_set_input(xcb_icccm_wm_hints_t *hints, uint8_t input);

/**
 * @brief Set hints state to 'iconic'.
 * @param hints WM_HINTS structure.
 */
void xcb_icccm_wm_hints_set_iconic(xcb_icccm_wm_hints_t *hints);

/**
 * @brief Set hints state to 'normal'.
 * @param hints WM_HINTS structure.
 */
void xcb_icccm_wm_hints_set_normal(xcb_icccm_wm_hints_t *hints);

/**
 * @brief Set hints state to 'withdrawn'.
 * @param hints WM_HINTS structure.
 */
void xcb_icccm_wm_hints_set_withdrawn(xcb_icccm_wm_hints_t *hints);

/**
 * @brief Set hints state to none.
 * @param hints WM_HINTS structure.
 */
void xcb_icccm_wm_hints_set_none(xcb_icccm_wm_hints_t *hints);

/**
 * @brief Set pixmap to be used as icon.
 * @param hints WM_HINTS structure.
 * @param icon_pixmap Pixmap.
 */
void xcb_icccm_wm_hints_set_icon_pixmap(xcb_icccm_wm_hints_t *hints,
                                        xcb_pixmap_t icon_pixmap);

/**
 * @brief Set icon mask bitmap.
 * @param hints WM_HINTS structure.
 * @param icon_mask Pixmap.
 */
void xcb_icccm_wm_hints_set_icon_mask(xcb_icccm_wm_hints_t *hints, xcb_pixmap_t icon_mask);

/**
 * @brief Set window identifier to be used as icon.
 * @param hints WM_HINTS structure.
 * @param icon_window Window X identifier.
 */
void xcb_icccm_wm_hints_set_icon_window(xcb_icccm_wm_hints_t *hints,
                                        xcb_window_t icon_window);

/**
 * @brief Set identifier of related window group.
 * @param hints WM_HINTS structure.
 * @param window_group Window X identifier.
 */
void xcb_icccm_wm_hints_set_window_group(xcb_icccm_wm_hints_t *hints,
                                         xcb_window_t window_group);

/**
 * @brief Set urgency hints flag.
 * @param hints WM_HINTS structure.
 */
void xcb_icccm_wm_hints_set_urgency(xcb_icccm_wm_hints_t *hints);

/**
 * @brief Deliver a SetProperty request to set WM_HINTS property value.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @param hints Hints value to set.
 */
xcb_void_cookie_t xcb_icccm_set_wm_hints_checked(xcb_connection_t *c,
                                                 xcb_window_t window,
                                                 xcb_icccm_wm_hints_t *hints);

/**
 * @see xcb_icccm_set_wm_hints_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_hints(xcb_connection_t *c,
                                         xcb_window_t window,
                                         xcb_icccm_wm_hints_t *hints);

/**
 * @brief Send request to get WM_HINTS property of a window.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_hints(xcb_connection_t *c,
                                                 xcb_window_t window);

/**
 * @see xcb_icccm_get_wm_hints()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_hints_unchecked(xcb_connection_t *c,
                                                           xcb_window_t window);

/**
 * @brief Fill given structure with the WM_HINTS property of a window.
 * @param hints WM_HINTS property value.
 * @param reply The get property request reply.
 * @return Return 1 on success, 0 otherwise.
 */
uint8_t
xcb_icccm_get_wm_hints_from_reply(xcb_icccm_wm_hints_t *hints,
                                  xcb_get_property_reply_t *reply);

/**
 * @brief Fill given structure with the WM_HINTS property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param hints WM_HINTS property value.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_hints_unchecked() is used.  Otherwise, it stores the
 * error if any. The returned pointer should be freed.
 */
uint8_t xcb_icccm_get_wm_hints_reply(xcb_connection_t *c,
                                     xcb_get_property_cookie_t cookie,
                                     xcb_icccm_wm_hints_t *hints,
                                     xcb_generic_error_t **e);

/* WM_PROTOCOLS */

/**
 * @brief Deliver a SetProperty request to set WM_PROTOCOLS property value.
 * @param c The connection to the X server.
 * @param wm_protocols The WM_PROTOCOLS atom.
 * @param window Window X identifier.
 * @param list_len Atom list len.
 * @param list Atom list.
 */
xcb_void_cookie_t xcb_icccm_set_wm_protocols_checked(xcb_connection_t *c,
                                                     xcb_window_t window,
                                                     xcb_atom_t wm_protocols,
                                                     uint32_t list_len,
                                                     xcb_atom_t *list);

/**
 * @see xcb_icccm_set_wm_protocols_checked()
 */
xcb_void_cookie_t xcb_icccm_set_wm_protocols(xcb_connection_t *c,
                                             xcb_window_t window,
                                             xcb_atom_t wm_protocols,
                                             uint32_t list_len,
                                             xcb_atom_t *list);

/**
 * @brief WM_PROTOCOLS structure.
 */
typedef struct {
  /** Length of the atoms list */
  uint32_t atoms_len;
  /** Atoms list */
  xcb_atom_t *atoms;
  /** Store reply to avoid memory allocation, should normally not be
      used directly */
  xcb_get_property_reply_t *_reply;
} xcb_icccm_get_wm_protocols_reply_t;

/**
 * @brief Send request to get WM_PROTOCOLS property of a given window.
 * @param c The connection to the X server.
 * @param window Window X identifier.
 * @return The request cookie.
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_protocols(xcb_connection_t *c,
                                                     xcb_window_t window,
                                                     xcb_atom_t wm_protocol_atom);

/**
 * @see xcb_icccm_get_wm_protocols()
 */
xcb_get_property_cookie_t xcb_icccm_get_wm_protocols_unchecked(xcb_connection_t *c,
                                                               xcb_window_t window,
                                                               xcb_atom_t wm_protocol_atom);

/**
 * @brief Fill the given structure with the WM_PROTOCOLS property of a window.
 * @param reply The reply of the GetProperty request.
 * @param protocols WM_PROTOCOLS property value.
 * @return Return 1 on success, 0 otherwise.
 *
 * protocols structure members should be freed by
 * xcb_icccm_get_wm_protocols_reply_wipe().
 */
uint8_t xcb_icccm_get_wm_protocols_from_reply(xcb_get_property_reply_t *reply,
                                              xcb_icccm_get_wm_protocols_reply_t *protocols);
/**
 * @brief Fill the given structure with the WM_PROTOCOLS property of a window.
 * @param c The connection to the X server.
 * @param cookie Request cookie.
 * @param protocols WM_PROTOCOLS property value.
 * @param e Error if any.
 * @return Return 1 on success, 0 otherwise.
 *
 * The parameter e supplied to this function must be NULL if
 * xcb_icccm_get_wm_protocols_unchecked() is used.  Otherwise, it stores the
 * error if any. protocols structure members should be freed by
 * xcb_icccm_get_wm_protocols_reply_wipe().
 */
uint8_t xcb_icccm_get_wm_protocols_reply(xcb_connection_t *c,
                                         xcb_get_property_cookie_t cookie,
                                         xcb_icccm_get_wm_protocols_reply_t *protocols,
                                         xcb_generic_error_t **e);

/**
 * @brief Wipe protocols structure members previously allocated by
 *        xcb_icccm_get_wm_protocols_reply().
 * @param protocols protocols structure whose members is going to be freed.
 */
void xcb_icccm_get_wm_protocols_reply_wipe(xcb_icccm_get_wm_protocols_reply_t *protocols);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __XCB_ICCCM_H__ */
