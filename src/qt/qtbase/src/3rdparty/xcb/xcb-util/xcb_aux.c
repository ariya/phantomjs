/*
 * Copyright © 2008 Bart Massey <bart@cs.pdx.edu>
 * Copyright © 2008 Ian Osgood <iano@quirkster.com>
 * Copyright © 2008 Jamey Sharp <jamey@minilop.net>
 * Copyright © 2008 Josh Triplett <josh@freedesktop.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>
#include "xcb_aux.h"

/* Connection related functions */

uint8_t
xcb_aux_get_depth (xcb_connection_t *c,
                   xcb_screen_t     *screen)
{
  xcb_drawable_t            drawable;
  xcb_get_geometry_reply_t *geom;
  int                       depth = 0;

  drawable = screen->root;
  geom = xcb_get_geometry_reply (c, xcb_get_geometry(c, drawable), 0);

  if (geom) {
    depth = geom->depth;
    free (geom);
  }

  return depth;
}

uint8_t
xcb_aux_get_depth_of_visual (xcb_screen_t *screen,
			     xcb_visualid_t id)
{
    xcb_depth_iterator_t i;
    xcb_visualtype_iterator_t j;
    for (i = xcb_screen_allowed_depths_iterator(screen);
	 i.rem; xcb_depth_next(&i))
	for (j = xcb_depth_visuals_iterator(i.data);
	     j.rem; xcb_visualtype_next(&j))
	    if (j.data->visual_id == id)
		return i.data->depth;
    return 0;
}

xcb_screen_t *
xcb_aux_get_screen (xcb_connection_t *c,
                    int               screen)
{
  xcb_screen_iterator_t i = xcb_setup_roots_iterator(xcb_get_setup(c));
  for (; i.rem; --screen, xcb_screen_next(&i))
    if (screen == 0)
      return i.data;
  return 0;
}

xcb_visualtype_t *
xcb_aux_get_visualtype (xcb_connection_t *c,
                        int               scr,
                        xcb_visualid_t    vid)
{
  xcb_screen_t        *screen;
  xcb_depth_t         *depth;
  xcb_visualtype_iterator_t iter;
  int               cur;

  screen = xcb_aux_get_screen (c, scr);
  if (!screen) return NULL;

  depth = xcb_screen_allowed_depths_iterator(screen).data;
  if (!depth) return NULL;
   
   iter = xcb_depth_visuals_iterator(depth);
   for (cur = 0 ; cur < iter.rem ; xcb_visualtype_next(&iter), ++cur)
      if (vid == iter.data->visual_id)
	 return iter.data;

   return NULL;
}

xcb_visualtype_t *
xcb_aux_find_visual_by_id (xcb_screen_t *screen,
			   xcb_visualid_t id)
{
    xcb_depth_iterator_t i;
    xcb_visualtype_iterator_t j;
    for (i = xcb_screen_allowed_depths_iterator(screen);
	 i.rem; xcb_depth_next(&i))
	for (j = xcb_depth_visuals_iterator(i.data);
	     j.rem; xcb_visualtype_next(&j))
	    if (j.data->visual_id == id)
		return j.data;
    return 0;
}

xcb_visualtype_t *
xcb_aux_find_visual_by_attrs (xcb_screen_t *screen,
			      int8_t class,
			      int8_t depth)
{
    xcb_depth_iterator_t i;
    xcb_visualtype_iterator_t j;
    for (i = xcb_screen_allowed_depths_iterator(screen);
	 i.rem; xcb_depth_next(&i)) {
	if (depth != -1 && i.data->depth != depth)
	    continue;
	for (j = xcb_depth_visuals_iterator(i.data);
	     j.rem; xcb_visualtype_next(&j))
	    if (class == -1 || j.data->_class == class)
		return j.data;
    }
    return 0;
}

void
xcb_aux_sync (xcb_connection_t *c)
{
    free(xcb_get_input_focus_reply(c, xcb_get_input_focus(c), NULL));
}

/* structs instead of value lists */
/* TODO: generate the struct types and functions from protocol masks and descriptions */

/* This generic implementation of pack_list depends on:
   a) structs packed to uint32_t size
   b) structs consist of just uint32_t/int32_t fields in the same order as bitmask
*/

static void
pack_list( uint32_t mask, const uint32_t *src, uint32_t *dest )
{
	for ( ; mask; mask >>= 1, src++)
		if (mask & 1)
			*dest++ = *src;
}

xcb_void_cookie_t
xcb_aux_create_window (xcb_connection_t       *c,
                       uint8_t                depth,
                       xcb_window_t           wid,
                       xcb_window_t           parent,
                       int16_t                x,
                       int16_t                y,
                       uint16_t               width,
                       uint16_t               height,
                       uint16_t               border_width,
                       uint16_t               _class,
                       xcb_visualid_t         visual,
                       uint32_t               mask,
                       const xcb_params_cw_t *params)
{
	uint32_t value_list[16];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_create_window(c, depth, wid, parent,
		x, y, width, height, border_width,
		_class, visual, mask, value_list);
}

xcb_void_cookie_t
xcb_aux_create_window_checked (xcb_connection_t       *c,
			       uint8_t                depth,
			       xcb_window_t           wid,
			       xcb_window_t           parent,
			       int16_t                x,
			       int16_t                y,
			       uint16_t               width,
			       uint16_t               height,
			       uint16_t               border_width,
			       uint16_t               _class,
			       xcb_visualid_t         visual,
			       uint32_t               mask,
			       const xcb_params_cw_t *params)
{
	uint32_t value_list[16];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_create_window_checked(c, depth, wid, parent,
					 x, y, width, height, border_width,
					 _class, visual, mask, value_list);
}

xcb_void_cookie_t
xcb_aux_change_window_attributes_checked (xcb_connection_t      *c,
                                          xcb_window_t           window,
                                          uint32_t               mask,
                                          const xcb_params_cw_t *params)
{
	uint32_t value_list[16];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_change_window_attributes_checked( c, window, mask, value_list );
}

xcb_void_cookie_t
xcb_aux_change_window_attributes (xcb_connection_t      *c,
                                  xcb_window_t           window,
                                  uint32_t               mask,
                                  const xcb_params_cw_t *params)
{
	uint32_t value_list[16];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_change_window_attributes( c, window, mask, value_list );
}

xcb_void_cookie_t
xcb_aux_configure_window (xcb_connection_t                    *c,
                          xcb_window_t                         window,
                          uint16_t                             mask,
                          const xcb_params_configure_window_t *params)
{
	uint32_t value_list[8];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_configure_window( c, window, mask, value_list );
}

xcb_void_cookie_t
xcb_aux_create_gc (xcb_connection_t      *c,
                   xcb_gcontext_t         gid,
                   xcb_drawable_t         drawable,
                   uint32_t               mask,
                   const xcb_params_gc_t *params)
{
	uint32_t value_list[32];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_create_gc( c, gid, drawable, mask, value_list );
}

xcb_void_cookie_t
xcb_aux_create_gc_checked (xcb_connection_t      *c,
			   xcb_gcontext_t         gid,
			   xcb_drawable_t         drawable,
			   uint32_t               mask,
			   const xcb_params_gc_t *params)
{
	uint32_t value_list[32];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_create_gc_checked( c, gid, drawable, mask, value_list);
}

xcb_void_cookie_t
xcb_aux_change_gc (xcb_connection_t     *c,
                   xcb_gcontext_t        gc,
                   uint32_t              mask,
                   const xcb_params_gc_t *params)
{
	uint32_t value_list[32];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_change_gc( c, gc, mask, value_list );
}

xcb_void_cookie_t
xcb_aux_change_gc_checked (xcb_connection_t     *c,
			   xcb_gcontext_t        gc,
			   uint32_t              mask,
			   const xcb_params_gc_t *params)
{
	uint32_t value_list[32];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_change_gc_checked( c, gc, mask, value_list );
}

xcb_void_cookie_t
xcb_aux_change_keyboard_control (xcb_connection_t            *c,
                                 uint32_t                     mask,
                                 const xcb_params_keyboard_t *params)
{
	uint32_t value_list[16];
	pack_list(mask, (const uint32_t *)params, value_list);
	return xcb_change_keyboard_control( c, mask, value_list );
}

/* Color related functions */

/* Return true if the given color name can be translated locally,
   in which case load the components.  Otherwise, a lookup_color request
   will be needed, so return false. */
int
xcb_aux_parse_color(char *color_name,
		    uint16_t *red,  uint16_t *green,  uint16_t *blue)
{
    int n, r, g, b, i;
    if (!color_name || *color_name != '#')
	return 0;
    /*
     * Excitingly weird RGB parsing code from Xlib.
     */
    n = strlen (color_name);
    color_name++;
    n--;
    if (n != 3 && n != 6 && n != 9 && n != 12)
	return 0;
    n /= 3;
    g = b = 0;
    do {
	r = g;
	g = b;
	b = 0;
	for (i = n; --i >= 0; ) {
	    char c = *color_name++;
	    b <<= 4;
	    if (c >= '0' && c <= '9')
		b |= c - '0';
	    else if (c >= 'A' && c <= 'F')
		b |= c - ('A' - 10);
	    else if (c >= 'a' && c <= 'f')
		b |= c - ('a' - 10);
	    else return 0;
	}
    } while (*color_name != '\0');
    n <<= 2;
    n = 16 - n;
    *red = r << n;
    *green = g << n;
    *blue = b << n;
    return 1;
}

/* Drawing related functions */

/* Adapted from Xlib */
xcb_void_cookie_t
xcb_aux_set_line_attributes_checked (xcb_connection_t *dpy,
				     xcb_gcontext_t gc,
				     uint16_t linewidth,
				     int32_t linestyle,
				     int32_t capstyle,
				     int32_t joinstyle)
{
    uint32_t mask = 0;
    xcb_params_gc_t gv;

    XCB_AUX_ADD_PARAM(&mask, &gv, line_width, linewidth);
    XCB_AUX_ADD_PARAM(&mask, &gv, line_style, linestyle);
    XCB_AUX_ADD_PARAM(&mask, &gv, cap_style, capstyle);
    XCB_AUX_ADD_PARAM(&mask, &gv, join_style, joinstyle);
    return xcb_aux_change_gc_checked(dpy, gc, mask, &gv);
}

/* Adapted from Xlib */
/* XXX It would be wiser for apps just to call
   clear_area() directly. */
xcb_void_cookie_t
xcb_aux_clear_window(xcb_connection_t *  dpy,
		     xcb_window_t        w)
{
    return xcb_clear_area(dpy, 0, w, 0, 0, 0, 0);
}
