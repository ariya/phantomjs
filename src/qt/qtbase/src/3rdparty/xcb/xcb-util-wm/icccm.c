/*
 * Copyright © 2008 Arnaud Fontaine <arnau@debian.org>
 * Copyright © 2007-2008 Vincent Torri <vtorri@univ-evry.fr>
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

#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include <xcb/xcb.h>

#include "xcb_icccm.h"

xcb_get_property_cookie_t
xcb_icccm_get_text_property(xcb_connection_t *c,
                            xcb_window_t window,
                            xcb_atom_t property)
{
  return xcb_get_property(c, 0, window, property, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);
}

xcb_get_property_cookie_t
xcb_icccm_get_text_property_unchecked(xcb_connection_t *c,
                                      xcb_window_t window,
                                      xcb_atom_t property)
{
  return xcb_get_property_unchecked(c, 0, window, property, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX);
}

uint8_t
xcb_icccm_get_text_property_reply(xcb_connection_t *c,
                                  xcb_get_property_cookie_t cookie,
                                  xcb_icccm_get_text_property_reply_t *prop,
                                  xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);

  if(!reply || reply->type == XCB_NONE) {
    free(reply);
    return 0;
  }

  prop->_reply = reply;
  prop->encoding = prop->_reply->type;
  prop->format = prop->_reply->format;
  prop->name_len = xcb_get_property_value_length(prop->_reply);
  prop->name = xcb_get_property_value(prop->_reply);

  return 1;
}

void
xcb_icccm_get_text_property_reply_wipe(xcb_icccm_get_text_property_reply_t *prop)
{
  free(prop->_reply);
}

/* WM_NAME */

xcb_void_cookie_t
xcb_icccm_set_wm_name_checked(xcb_connection_t *c, xcb_window_t window,
                              xcb_atom_t encoding, uint8_t format,
                              uint32_t name_len, const char *name)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     XCB_ATOM_WM_NAME, encoding, format,
                                     name_len, name);
}

xcb_void_cookie_t
xcb_icccm_set_wm_name(xcb_connection_t *c, xcb_window_t window, xcb_atom_t encoding,
                      uint8_t format, uint32_t name_len, const char *name)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
                             XCB_ATOM_WM_NAME, encoding, format, name_len,
                             name);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_name(xcb_connection_t *c,
                      xcb_window_t window)
{
  return xcb_icccm_get_text_property(c, window, XCB_ATOM_WM_NAME);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_name_unchecked(xcb_connection_t *c,
                                xcb_window_t window)
{
  return xcb_icccm_get_text_property_unchecked(c, window, XCB_ATOM_WM_NAME);
}

uint8_t
xcb_icccm_get_wm_name_reply(xcb_connection_t *c,
                            xcb_get_property_cookie_t cookie,
                            xcb_icccm_get_text_property_reply_t *prop,
                            xcb_generic_error_t **e)
{
  return xcb_icccm_get_text_property_reply(c, cookie, prop, e);
}

/* WM_ICON_NAME */

xcb_void_cookie_t
xcb_icccm_set_wm_icon_name_checked(xcb_connection_t *c, xcb_window_t window,
                                   xcb_atom_t encoding, uint8_t format,
                                   uint32_t name_len, const char *name)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     XCB_ATOM_WM_ICON_NAME, encoding, format,
                                     name_len, name);
}

xcb_void_cookie_t
xcb_icccm_set_wm_icon_name(xcb_connection_t *c, xcb_window_t window,
                           xcb_atom_t encoding, uint8_t format, uint32_t name_len,
                           const char *name)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
                             XCB_ATOM_WM_ICON_NAME, encoding, format,
                             name_len, name);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_icon_name(xcb_connection_t *c,
                           xcb_window_t window)
{
  return xcb_icccm_get_text_property(c, window, XCB_ATOM_WM_ICON_NAME);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_icon_name_unchecked(xcb_connection_t *c,
                                     xcb_window_t window)
{
  return xcb_icccm_get_text_property_unchecked(c, window, XCB_ATOM_WM_ICON_NAME);
}

uint8_t
xcb_icccm_get_wm_icon_name_reply(xcb_connection_t *c,
                                 xcb_get_property_cookie_t cookie,
                                 xcb_icccm_get_text_property_reply_t *prop,
                                 xcb_generic_error_t **e)
{
  return xcb_icccm_get_text_property_reply(c, cookie, prop, e);
}

/* WM_COLORMAP_WINDOWS */

xcb_void_cookie_t
xcb_icccm_set_wm_colormap_windows_checked(xcb_connection_t *c,
                                          xcb_window_t window,
                                          xcb_atom_t wm_colormap_windows_atom,
                                          uint32_t list_len,
                                          const xcb_window_t *list)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     wm_colormap_windows_atom, XCB_ATOM_WINDOW,
                                     32, list_len, list);
}

xcb_void_cookie_t
xcb_icccm_set_wm_colormap_windows(xcb_connection_t *c,
                                  xcb_window_t window,
                                  xcb_atom_t wm_colormap_windows_atom,
                                  uint32_t list_len,
                                  const xcb_atom_t *list)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
                             wm_colormap_windows_atom, XCB_ATOM_WINDOW, 32,
                             list_len, list);
}


xcb_get_property_cookie_t
xcb_icccm_get_wm_colormap_windows(xcb_connection_t *c,
                                  xcb_window_t window,
                                  xcb_atom_t wm_colormap_windows_atom )
{
  return xcb_get_property(c, 0, window, wm_colormap_windows_atom,
                          XCB_ATOM_WINDOW, 0, UINT_MAX);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_colormap_windows_unchecked(xcb_connection_t *c,
                                            xcb_window_t window,
                                            xcb_atom_t wm_colormap_windows_atom)
{
  return xcb_get_property_unchecked(c, 0, window, wm_colormap_windows_atom,
                                    XCB_ATOM_WINDOW, 0, UINT_MAX);
}

uint8_t
xcb_icccm_get_wm_colormap_windows_from_reply(xcb_get_property_reply_t *reply,
                                             xcb_icccm_get_wm_colormap_windows_reply_t *colormap_windows)
{
  if(!reply || reply->type != XCB_ATOM_WINDOW || reply->format != 32)
    return 0;

  colormap_windows->_reply = reply;
  colormap_windows->windows_len = xcb_get_property_value_length(colormap_windows->_reply) /  (reply->format / 8);
  colormap_windows->windows = (xcb_window_t *) xcb_get_property_value(colormap_windows->_reply);

  return 1;
}

uint8_t
xcb_icccm_get_wm_colormap_windows_reply(xcb_connection_t *c,
                                        xcb_get_property_cookie_t cookie,
                                        xcb_icccm_get_wm_colormap_windows_reply_t *colormap_windows,
                                        xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);
  uint8_t ret = xcb_icccm_get_wm_colormap_windows_from_reply(reply, colormap_windows);
  if(!ret)
    free(reply);
  return ret;
}

void
xcb_icccm_get_wm_colormap_windows_reply_wipe(xcb_icccm_get_wm_colormap_windows_reply_t *colormap_windows)
{
  free(colormap_windows->_reply);
}

/* WM_CLIENT_MACHINE */

xcb_void_cookie_t
xcb_icccm_set_wm_client_machine_checked(xcb_connection_t *c, xcb_window_t window,
                                        xcb_atom_t encoding, uint8_t format,
                                        uint32_t name_len, const char *name)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     XCB_ATOM_WM_CLIENT_MACHINE, encoding,
                                     format, name_len, name);
}

xcb_void_cookie_t
xcb_icccm_set_wm_client_machine(xcb_connection_t *c, xcb_window_t window,
                                xcb_atom_t encoding, uint8_t format,
                                uint32_t name_len, const char *name)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
                             XCB_ATOM_WM_CLIENT_MACHINE, encoding, format,
                             name_len, name);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_client_machine(xcb_connection_t *c,
                                xcb_window_t window)
{
  return xcb_icccm_get_text_property(c, window, XCB_ATOM_WM_CLIENT_MACHINE);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_client_machine_unchecked(xcb_connection_t *c,
                                          xcb_window_t window)
{
  return xcb_icccm_get_text_property_unchecked(c, window, XCB_ATOM_WM_CLIENT_MACHINE);
}

uint8_t
xcb_icccm_get_wm_client_machine_reply(xcb_connection_t *c,
                                      xcb_get_property_cookie_t cookie,
                                      xcb_icccm_get_text_property_reply_t *prop,
                                      xcb_generic_error_t **e)
{
  return xcb_icccm_get_text_property_reply(c, cookie, prop, e);
}

/* WM_CLASS */

xcb_void_cookie_t
xcb_icccm_set_wm_class_checked(xcb_connection_t *c,
                               xcb_window_t window,
                               uint32_t class_len,
                               const char *class)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
                                     class_len, class);
}

xcb_void_cookie_t
xcb_icccm_set_wm_class(xcb_connection_t *c,
                       xcb_window_t window,
                       uint32_t class_len,
                       const char *class)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
                             XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
                             class_len, class);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_class(xcb_connection_t *c, xcb_window_t window)
{
  return xcb_get_property(c, 0, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 0L, 2048L);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_class_unchecked(xcb_connection_t *c, xcb_window_t window)
{
  return xcb_get_property_unchecked(c, 0, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 0L, 2048L);
}

uint8_t
xcb_icccm_get_wm_class_from_reply(xcb_icccm_get_wm_class_reply_t *prop,
                                  xcb_get_property_reply_t *reply)
{
  if(!reply || reply->type != XCB_ATOM_STRING || reply->format != 8)
    return 0;

  prop->_reply = reply;
  prop->instance_name = (char *) xcb_get_property_value(prop->_reply);

  int len = xcb_get_property_value_length(prop->_reply);
  /* Ensure there's a C end-of-string at the end of the property.
     Truncate the property if necessary (the spec says there's already
     a 0 in the last position, so this only hurts invalid props). */
  if(len < reply->length * 4)
    prop->instance_name[len] = 0;
  else
    prop->instance_name[len-1] = 0;

  int name_len = strlen(prop->instance_name);
  if(name_len == len)
    name_len--;

  prop->class_name = prop->instance_name + name_len + 1;

  return 1;
}

uint8_t
xcb_icccm_get_wm_class_reply(xcb_connection_t *c, xcb_get_property_cookie_t cookie,
                             xcb_icccm_get_wm_class_reply_t *prop, xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);
  uint8_t ret = xcb_icccm_get_wm_class_from_reply(prop, reply);
  /* if reply parsing failed, free the reply to avoid mem leak */
  if(!ret)
    free(reply);
  return ret;
}

void
xcb_icccm_get_wm_class_reply_wipe(xcb_icccm_get_wm_class_reply_t *prop)
{
  free(prop->_reply);
}

/* WM_TRANSIENT_FOR */

xcb_void_cookie_t
xcb_icccm_set_wm_transient_for_checked(xcb_connection_t *c, xcb_window_t window,
                                       xcb_window_t transient_for_window)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     XCB_ATOM_WM_TRANSIENT_FOR,
                                     XCB_ATOM_WINDOW, 32, 1,
                                     &transient_for_window);
}

xcb_void_cookie_t
xcb_icccm_set_wm_transient_for(xcb_connection_t *c, xcb_window_t window,
                               xcb_window_t transient_for_window)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window,
                             XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,
                             1, &transient_for_window);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_transient_for(xcb_connection_t *c, xcb_window_t window)
{
  return xcb_get_property(c, 0, window, XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 0, 1);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_transient_for_unchecked(xcb_connection_t *c, xcb_window_t window)
{
  return xcb_get_property_unchecked(c, 0, window, XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 0, 1);
}

uint8_t
xcb_icccm_get_wm_transient_for_from_reply(xcb_window_t *prop,
                                          xcb_get_property_reply_t *reply)
{
  if(!reply || reply->type != XCB_ATOM_WINDOW || reply->format != 32 || !reply->length)
    return 0;

  *prop = *((xcb_window_t *) xcb_get_property_value(reply));

  return 1;
}

uint8_t
xcb_icccm_get_wm_transient_for_reply(xcb_connection_t *c,
                                     xcb_get_property_cookie_t cookie,
                                     xcb_window_t *prop,
                                     xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);
  uint8_t ret = xcb_icccm_get_wm_transient_for_from_reply(prop, reply);
  free(reply);
  return ret;
}

/* WM_SIZE_HINTS */

void
xcb_icccm_size_hints_set_position(xcb_size_hints_t *hints, int user_specified,
                                  int32_t x, int32_t y)
{
  hints->flags &= ~(XCB_ICCCM_SIZE_HINT_US_POSITION | XCB_ICCCM_SIZE_HINT_P_POSITION);
  if (user_specified)
    hints->flags |= XCB_ICCCM_SIZE_HINT_US_POSITION;
  else
    hints->flags |= XCB_ICCCM_SIZE_HINT_P_POSITION;
  hints->x = x;
  hints->y = y;
}

void
xcb_icccm_size_hints_set_size(xcb_size_hints_t *hints, int user_specified,
                              int32_t width, int32_t height)
{
  hints->flags &= ~(XCB_ICCCM_SIZE_HINT_US_SIZE | XCB_ICCCM_SIZE_HINT_P_SIZE);
  if (user_specified)
    hints->flags |= XCB_ICCCM_SIZE_HINT_US_SIZE;
  else
    hints->flags |= XCB_ICCCM_SIZE_HINT_P_SIZE;
  hints->width = width;
  hints->height = height;
}

void
xcb_icccm_size_hints_set_min_size(xcb_size_hints_t *hints, int32_t min_width,
                                  int32_t min_height)
{
  hints->flags |= XCB_ICCCM_SIZE_HINT_P_MIN_SIZE;
  hints->min_width = min_width;
  hints->min_height = min_height;
}

void
xcb_icccm_size_hints_set_max_size(xcb_size_hints_t *hints, int32_t max_width,
                                  int32_t max_height)
{
  hints->flags |= XCB_ICCCM_SIZE_HINT_P_MAX_SIZE;
  hints->max_width = max_width;
  hints->max_height = max_height;
}

void
xcb_icccm_size_hints_set_resize_inc(xcb_size_hints_t *hints, int32_t width_inc,
                                    int32_t height_inc)
{
  hints->flags |= XCB_ICCCM_SIZE_HINT_P_RESIZE_INC;
  hints->width_inc = width_inc;
  hints->height_inc = height_inc;
}

void
xcb_icccm_size_hints_set_aspect(xcb_size_hints_t *hints, int32_t min_aspect_num,
                                int32_t min_aspect_den, int32_t max_aspect_num,
                                int32_t max_aspect_den)
{
  hints->flags |= XCB_ICCCM_SIZE_HINT_P_ASPECT;
  hints->min_aspect_num = min_aspect_num;
  hints->min_aspect_den = min_aspect_den;
  hints->max_aspect_num = max_aspect_num;
  hints->max_aspect_den = max_aspect_den;
}

void
xcb_icccm_size_hints_set_base_size(xcb_size_hints_t *hints, int32_t base_width,
                                   int32_t base_height)
{
  hints->flags |= XCB_ICCCM_SIZE_HINT_BASE_SIZE;
  hints->base_width = base_width;
  hints->base_height = base_height;
}

void
xcb_icccm_size_hints_set_win_gravity(xcb_size_hints_t *hints, xcb_gravity_t win_gravity)
{
  hints->flags |= XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY;
  hints->win_gravity = win_gravity;
}

xcb_void_cookie_t
xcb_icccm_set_wm_size_hints_checked(xcb_connection_t *c, xcb_window_t window,
                                    xcb_atom_t property, xcb_size_hints_t *hints)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, property,
				     XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(*hints) >> 2,
				     hints);
}

xcb_void_cookie_t
xcb_icccm_set_wm_size_hints(xcb_connection_t *c, xcb_window_t window,
                            xcb_atom_t property, xcb_size_hints_t *hints)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, property,
			     XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(*hints) >> 2, hints);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_size_hints(xcb_connection_t *c, xcb_window_t window,
                            xcb_atom_t property)
{
  return xcb_get_property(c, 0, window, property, XCB_ATOM_WM_SIZE_HINTS, 0L, XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_size_hints_unchecked(xcb_connection_t *c, xcb_window_t window,
                                      xcb_atom_t property)
{
  return xcb_get_property_unchecked(c, 0, window, property, XCB_ATOM_WM_SIZE_HINTS,
                                    0L, XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS);
}

uint8_t
xcb_icccm_get_wm_size_hints_from_reply(xcb_size_hints_t *hints, xcb_get_property_reply_t *reply)
{
  uint32_t flags;
  int length;

  if(!reply)
    return 0;

  if (!(reply->type == XCB_ATOM_WM_SIZE_HINTS &&
        reply->format == 32))
    return 0;

  length = xcb_get_property_value_length(reply) / (reply->format / 8);

  if(length > XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS)
    length = XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS;

  memcpy(hints, (xcb_size_hints_t *) xcb_get_property_value (reply),
         length * (reply->format / 8));

  flags = (XCB_ICCCM_SIZE_HINT_US_POSITION | XCB_ICCCM_SIZE_HINT_US_SIZE |
           XCB_ICCCM_SIZE_HINT_P_POSITION | XCB_ICCCM_SIZE_HINT_P_SIZE |
           XCB_ICCCM_SIZE_HINT_P_MIN_SIZE | XCB_ICCCM_SIZE_HINT_P_MAX_SIZE |
           XCB_ICCCM_SIZE_HINT_P_RESIZE_INC | XCB_ICCCM_SIZE_HINT_P_ASPECT);

  /* NumPropSizeElements = 18 (ICCCM version 1) */
  if(length >= 18)
    flags |= (XCB_ICCCM_SIZE_HINT_BASE_SIZE | XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY);
  else
    {
      hints->base_width = 0;
      hints->base_height = 0;
      hints->win_gravity = 0;
    }
  /* get rid of unwanted bits */
  hints->flags &= flags;

  return 1;
}

uint8_t
xcb_icccm_get_wm_size_hints_reply(xcb_connection_t *c, xcb_get_property_cookie_t cookie,
                                  xcb_size_hints_t *hints, xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);
  uint8_t ret = xcb_icccm_get_wm_size_hints_from_reply(hints, reply);
  free(reply);
  return ret;
}

/* WM_NORMAL_HINTS */

xcb_void_cookie_t
xcb_icccm_set_wm_normal_hints_checked(xcb_connection_t *c, xcb_window_t window,
                                      xcb_size_hints_t *hints)
{
  return xcb_icccm_set_wm_size_hints_checked(c, window, XCB_ATOM_WM_NORMAL_HINTS, hints);
}

xcb_void_cookie_t
xcb_icccm_set_wm_normal_hints(xcb_connection_t *c, xcb_window_t window,
                              xcb_size_hints_t *hints)
{
  return xcb_icccm_set_wm_size_hints(c, window, XCB_ATOM_WM_NORMAL_HINTS, hints);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_normal_hints(xcb_connection_t *c, xcb_window_t window)
{
  return xcb_icccm_get_wm_size_hints(c, window, XCB_ATOM_WM_NORMAL_HINTS);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_normal_hints_unchecked(xcb_connection_t *c, xcb_window_t window)
{
  return xcb_icccm_get_wm_size_hints_unchecked(c, window, XCB_ATOM_WM_NORMAL_HINTS);
}

uint8_t
xcb_icccm_get_wm_normal_hints_reply(xcb_connection_t *c,
                                    xcb_get_property_cookie_t cookie,
                                    xcb_size_hints_t *hints,
                                    xcb_generic_error_t **e)
{
  return xcb_icccm_get_wm_size_hints_reply(c, cookie, hints, e);
}

/* WM_HINTS */

uint32_t
xcb_icccm_wm_hints_get_urgency(xcb_icccm_wm_hints_t *hints)
{
  return (hints->flags & XCB_ICCCM_WM_HINT_X_URGENCY);
}

void
xcb_icccm_wm_hints_set_input(xcb_icccm_wm_hints_t *hints, uint8_t input)
{
  hints->input = input;
  hints->flags |= XCB_ICCCM_WM_HINT_INPUT;
}

void
xcb_icccm_wm_hints_set_iconic(xcb_icccm_wm_hints_t *hints)
{
  hints->initial_state = XCB_ICCCM_WM_STATE_ICONIC;
  hints->flags |= XCB_ICCCM_WM_HINT_STATE;
}

void
xcb_icccm_wm_hints_set_normal(xcb_icccm_wm_hints_t *hints)
{
  hints->initial_state = XCB_ICCCM_WM_STATE_NORMAL;
  hints->flags |= XCB_ICCCM_WM_HINT_STATE;
}

void
xcb_icccm_wm_hints_set_withdrawn(xcb_icccm_wm_hints_t *hints)
{
  hints->initial_state = XCB_ICCCM_WM_STATE_WITHDRAWN;
  hints->flags |= XCB_ICCCM_WM_HINT_STATE;
}

void
xcb_icccm_wm_hints_set_none(xcb_icccm_wm_hints_t *hints)
{
  hints->flags &= ~XCB_ICCCM_WM_HINT_STATE;
}

void
xcb_icccm_wm_hints_set_icon_pixmap(xcb_icccm_wm_hints_t *hints, xcb_pixmap_t icon_pixmap)
{
  hints->icon_pixmap = icon_pixmap;
  hints->flags |= XCB_ICCCM_WM_HINT_ICON_PIXMAP;
}

void
xcb_icccm_wm_hints_set_icon_mask(xcb_icccm_wm_hints_t *hints, xcb_pixmap_t icon_mask)
{
  hints->icon_mask = icon_mask;
  hints->flags |= XCB_ICCCM_WM_HINT_ICON_MASK;
}

void
xcb_icccm_wm_hints_set_icon_window(xcb_icccm_wm_hints_t *hints, xcb_window_t icon_window)
{
  hints->icon_window = icon_window;
  hints->flags |= XCB_ICCCM_WM_HINT_ICON_WINDOW;
}

void
xcb_icccm_wm_hints_set_window_group(xcb_icccm_wm_hints_t *hints, xcb_window_t window_group)
{
  hints->window_group = window_group;
  hints->flags |= XCB_ICCCM_WM_HINT_WINDOW_GROUP;
}

void
xcb_icccm_wm_hints_set_urgency(xcb_icccm_wm_hints_t *hints)
{
  hints->flags |= XCB_ICCCM_WM_HINT_X_URGENCY;
}

xcb_void_cookie_t
xcb_icccm_set_wm_hints_checked(xcb_connection_t *c, xcb_window_t window,
                               xcb_icccm_wm_hints_t *hints)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_HINTS,
				     XCB_ATOM_WM_HINTS, 32, sizeof(*hints) >> 2, hints);
}

xcb_void_cookie_t
xcb_icccm_set_wm_hints(xcb_connection_t *c, xcb_window_t window,
                       xcb_icccm_wm_hints_t *hints)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_HINTS,
			     XCB_ATOM_WM_HINTS, 32, sizeof(*hints) >> 2, hints);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_hints(xcb_connection_t *c,
                       xcb_window_t window)
{
  return xcb_get_property(c, 0, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 0L,
                          XCB_ICCCM_NUM_WM_HINTS_ELEMENTS);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_hints_unchecked(xcb_connection_t *c,
                                 xcb_window_t window)
{
  return xcb_get_property_unchecked(c, 0, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 0L,
                                    XCB_ICCCM_NUM_WM_HINTS_ELEMENTS);
}

uint8_t
xcb_icccm_get_wm_hints_from_reply(xcb_icccm_wm_hints_t *hints,
                                  xcb_get_property_reply_t *reply)
{
  if(!reply || reply->type != XCB_ATOM_WM_HINTS || reply->format != 32)
    return 0;

  int length = xcb_get_property_value_length(reply);
  int num_elem = length / (reply->format / 8);

  if(num_elem < XCB_ICCCM_NUM_WM_HINTS_ELEMENTS - 1)
    return 0;

  if(length > sizeof(xcb_size_hints_t))
    length = sizeof(xcb_size_hints_t);

  memcpy(hints, (xcb_size_hints_t *) xcb_get_property_value(reply), length);

  if(num_elem < XCB_ICCCM_NUM_WM_HINTS_ELEMENTS)
    hints->window_group = XCB_NONE;

  return 1;
}

uint8_t
xcb_icccm_get_wm_hints_reply(xcb_connection_t *c,
                             xcb_get_property_cookie_t cookie,
                             xcb_icccm_wm_hints_t *hints,
                             xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);
  int ret = xcb_icccm_get_wm_hints_from_reply(hints, reply);
  free(reply);
  return ret;
}

/* WM_PROTOCOLS */

xcb_void_cookie_t
xcb_icccm_set_wm_protocols_checked(xcb_connection_t *c, xcb_window_t window,
                                   xcb_atom_t wm_protocols, uint32_t list_len,
                                   xcb_atom_t *list)
{
  return xcb_change_property_checked(c, XCB_PROP_MODE_REPLACE, window,
                                     wm_protocols, XCB_ATOM_ATOM, 32, list_len, list);
}

xcb_void_cookie_t
xcb_icccm_set_wm_protocols(xcb_connection_t *c, xcb_window_t window,
                           xcb_atom_t wm_protocols, uint32_t list_len, xcb_atom_t *list)
{
  return xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, wm_protocols,
                             XCB_ATOM_ATOM, 32, list_len, list);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_protocols(xcb_connection_t *c, xcb_window_t window,
                           xcb_atom_t wm_protocol_atom)
{
  return xcb_get_property(c, 0, window, wm_protocol_atom, XCB_ATOM_ATOM, 0, UINT_MAX);
}

xcb_get_property_cookie_t
xcb_icccm_get_wm_protocols_unchecked(xcb_connection_t *c,
                                     xcb_window_t window,
                                     xcb_atom_t wm_protocol_atom)
{
  return xcb_get_property_unchecked(c, 0, window, wm_protocol_atom, XCB_ATOM_ATOM, 0,
                                    UINT_MAX);
}

uint8_t
xcb_icccm_get_wm_protocols_from_reply(xcb_get_property_reply_t *reply, xcb_icccm_get_wm_protocols_reply_t *protocols)
{
  if(!reply || reply->type != XCB_ATOM_ATOM || reply->format != 32)
    return 0;

  protocols->_reply = reply;
  protocols->atoms_len = xcb_get_property_value_length(protocols->_reply) /  (reply->format / 8);
  protocols->atoms = (xcb_atom_t *) xcb_get_property_value(protocols->_reply);

  return 1;
}

uint8_t
xcb_icccm_get_wm_protocols_reply(xcb_connection_t *c,
                                 xcb_get_property_cookie_t cookie,
                                 xcb_icccm_get_wm_protocols_reply_t *protocols,
                                 xcb_generic_error_t **e)
{
  xcb_get_property_reply_t *reply = xcb_get_property_reply(c, cookie, e);
  uint8_t ret = xcb_icccm_get_wm_protocols_from_reply(reply, protocols);
  if(!ret)
    free(reply);
  return ret;
}

void
xcb_icccm_get_wm_protocols_reply_wipe(xcb_icccm_get_wm_protocols_reply_t *protocols)
{
  free(protocols->_reply);
}
