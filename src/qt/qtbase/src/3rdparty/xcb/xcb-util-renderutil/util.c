/* Copyright © 2000 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "xcb_renderutil.h"

xcb_render_pictvisual_t *
xcb_render_util_find_visual_format (const xcb_render_query_pict_formats_reply_t *formats,
			       const xcb_visualid_t visual)
{
    xcb_render_pictscreen_iterator_t screens;
    xcb_render_pictdepth_iterator_t depths;
    xcb_render_pictvisual_iterator_t visuals;
    if (!formats)
	return 0;
    for (screens = xcb_render_query_pict_formats_screens_iterator(formats); screens.rem; xcb_render_pictscreen_next(&screens))
	for (depths = xcb_render_pictscreen_depths_iterator(screens.data); depths.rem; xcb_render_pictdepth_next(&depths))
	    for (visuals = xcb_render_pictdepth_visuals_iterator(depths.data); visuals.rem; xcb_render_pictvisual_next(&visuals))
		if (visuals.data->visual == visual)
		    return visuals.data;
    return 0;
}

xcb_render_pictforminfo_t *
xcb_render_util_find_format (const xcb_render_query_pict_formats_reply_t	*formats,
			 unsigned long				mask,
			 const xcb_render_pictforminfo_t		*template,
			 int					count)
{
    xcb_render_pictforminfo_iterator_t i;
    if (!formats)
	return 0;
    for (i = xcb_render_query_pict_formats_formats_iterator(formats); i.rem; xcb_render_pictforminfo_next(&i))
    {
	if (mask & XCB_PICT_FORMAT_ID)
	    if (template->id != i.data->id)
		continue;
	if (mask & XCB_PICT_FORMAT_TYPE)
	    if (template->type != i.data->type)
		continue;
	if (mask & XCB_PICT_FORMAT_DEPTH)
	    if (template->depth != i.data->depth)
		continue;
	if (mask & XCB_PICT_FORMAT_RED)
	    if (template->direct.red_shift != i.data->direct.red_shift)
		continue;
	if (mask & XCB_PICT_FORMAT_RED_MASK)
	    if (template->direct.red_mask != i.data->direct.red_mask)
		continue;
	if (mask & XCB_PICT_FORMAT_GREEN)
	    if (template->direct.green_shift != i.data->direct.green_shift)
		continue;
	if (mask & XCB_PICT_FORMAT_GREEN_MASK)
	    if (template->direct.green_mask != i.data->direct.green_mask)
		continue;
	if (mask & XCB_PICT_FORMAT_BLUE)
	    if (template->direct.blue_shift != i.data->direct.blue_shift)
		continue;
	if (mask & XCB_PICT_FORMAT_BLUE_MASK)
	    if (template->direct.blue_mask != i.data->direct.blue_mask)
		continue;
	if (mask & XCB_PICT_FORMAT_ALPHA)
	    if (template->direct.alpha_shift != i.data->direct.alpha_shift)
		continue;
	if (mask & XCB_PICT_FORMAT_ALPHA_MASK)
	    if (template->direct.alpha_mask != i.data->direct.alpha_mask)
		continue;
	if (mask & XCB_PICT_FORMAT_COLORMAP)
	    if (template->colormap != i.data->colormap)
		continue;
	if (count-- == 0)
	    return i.data;
    }
    return 0;
}

xcb_render_pictforminfo_t *
xcb_render_util_find_standard_format (const xcb_render_query_pict_formats_reply_t	*formats,
				 xcb_pict_standard_t					format)
{
    static const struct {
	xcb_render_pictforminfo_t templ;
	unsigned long	      mask;
    } standardFormats[] = {
	/* XCB_PICT_STANDARD_ARGB_32 */
	{
	    {
		0,			    /* id */
		XCB_RENDER_PICT_TYPE_DIRECT,    /* type */
		32,			    /* depth */
		{ 0 },			    /* pad */
		{			    /* direct */
		    16,			    /* direct.red */
		    0xff,		    /* direct.red_mask */
		    8,			    /* direct.green */
		    0xff,		    /* direct.green_mask */
		    0,			    /* direct.blue */
		    0xff,		    /* direct.blue_mask */
		    24,			    /* direct.alpha */
		    0xff,		    /* direct.alpha_mask */
		},
		0,			    /* colormap */
	    },
	    XCB_PICT_FORMAT_TYPE | 
	    XCB_PICT_FORMAT_DEPTH |
	    XCB_PICT_FORMAT_RED |
	    XCB_PICT_FORMAT_RED_MASK |
	    XCB_PICT_FORMAT_GREEN |
	    XCB_PICT_FORMAT_GREEN_MASK |
	    XCB_PICT_FORMAT_BLUE |
	    XCB_PICT_FORMAT_BLUE_MASK |
	    XCB_PICT_FORMAT_ALPHA |
	    XCB_PICT_FORMAT_ALPHA_MASK,
	},
	/* XCB_PICT_STANDARD_RGB_24 */
	{
	    {
		0,			    /* id */
		XCB_RENDER_PICT_TYPE_DIRECT,    /* type */
		24,			    /* depth */
		{ 0 },			    /* pad */
		{			    /* direct */
		    16,			    /* direct.red */
		    0xff,		    /* direct.red_MASK */
		    8,			    /* direct.green */
		    0xff,		    /* direct.green_MASK */
		    0,			    /* direct.blue */
		    0xff,		    /* direct.blue_MASK */
		    0,			    /* direct.alpha */
		    0x00,		    /* direct.alpha_MASK */
		},
		0,			    /* colormap */
	    },
	    XCB_PICT_FORMAT_TYPE | 
	    XCB_PICT_FORMAT_DEPTH |
	    XCB_PICT_FORMAT_RED |
	    XCB_PICT_FORMAT_RED_MASK |
	    XCB_PICT_FORMAT_GREEN |
	    XCB_PICT_FORMAT_GREEN_MASK |
	    XCB_PICT_FORMAT_BLUE |
	    XCB_PICT_FORMAT_BLUE_MASK |
	    XCB_PICT_FORMAT_ALPHA_MASK,
	},
	/* XCB_PICT_STANDARD_A_8 */
	{
	    {
		0,			    /* id */
		XCB_RENDER_PICT_TYPE_DIRECT,    /* type */
		8,			    /* depth */
		{ 0 },			    /* pad */
		{			    /* direct */
		    0,			    /* direct.red */
		    0x00,		    /* direct.red_MASK */
		    0,			    /* direct.green */
		    0x00,		    /* direct.green_MASK */
		    0,			    /* direct.blue */
		    0x00,		    /* direct.blue_MASK */
		    0,			    /* direct.alpha */
		    0xff,		    /* direct.alpha_MASK */
		},
		0,			    /* colormap */
	    },
	    XCB_PICT_FORMAT_TYPE | 
	    XCB_PICT_FORMAT_DEPTH |
	    XCB_PICT_FORMAT_RED_MASK |
	    XCB_PICT_FORMAT_GREEN_MASK |
	    XCB_PICT_FORMAT_BLUE_MASK |
	    XCB_PICT_FORMAT_ALPHA |
	    XCB_PICT_FORMAT_ALPHA_MASK,
	},
	/* XCB_PICT_STANDARD_A_4 */
	{
	    {
		0,			    /* id */
		XCB_RENDER_PICT_TYPE_DIRECT,    /* type */
		4,			    /* depth */
		{ 0 },			    /* pad */
		{			    /* direct */
		    0,			    /* direct.red */
		    0x00,		    /* direct.red_MASK */
		    0,			    /* direct.green */
		    0x00,		    /* direct.green_MASK */
		    0,			    /* direct.blue */
		    0x00,		    /* direct.blue_MASK */
		    0,			    /* direct.alpha */
		    0x0f,		    /* direct.alpha_MASK */
		},
		0,			    /* colormap */
	    },
	    XCB_PICT_FORMAT_TYPE | 
	    XCB_PICT_FORMAT_DEPTH |
	    XCB_PICT_FORMAT_RED_MASK |
	    XCB_PICT_FORMAT_GREEN_MASK |
	    XCB_PICT_FORMAT_BLUE_MASK |
	    XCB_PICT_FORMAT_ALPHA |
	    XCB_PICT_FORMAT_ALPHA_MASK,
	},
	/* XCB_PICT_STANDARD_A_1 */
	{
	    {
		0,			    /* id */
		XCB_RENDER_PICT_TYPE_DIRECT,    /* type */
		1,			    /* depth */
		{ 0 },			    /* pad */
		{			    /* direct */
		    0,			    /* direct.red */
		    0x00,		    /* direct.red_MASK */
		    0,			    /* direct.green */
		    0x00,		    /* direct.green_MASK */
		    0,			    /* direct.blue */
		    0x00,		    /* direct.blue_MASK */
		    0,			    /* direct.alpha */
		    0x01,		    /* direct.alpha_MASK */
		},
		0,			    /* colormap */
	    },
	    XCB_PICT_FORMAT_TYPE | 
	    XCB_PICT_FORMAT_DEPTH |
	    XCB_PICT_FORMAT_RED_MASK |
	    XCB_PICT_FORMAT_GREEN_MASK |
	    XCB_PICT_FORMAT_BLUE_MASK |
	    XCB_PICT_FORMAT_ALPHA |
	    XCB_PICT_FORMAT_ALPHA_MASK,
	},
    };

    if (format < 0 || format >= sizeof(standardFormats) / sizeof(*standardFormats))
	return 0;

    return xcb_render_util_find_format (formats, 
				    standardFormats[format].mask,
				    &standardFormats[format].templ,
				    0);
}
