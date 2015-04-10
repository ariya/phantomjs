/*
 * This file generated automatically from xfixes.xml by c_client.py.
 * Edit at your peril.
 */

#include <string.h>
#include <assert.h>
#include "xcbext.h"
#include "xfixes.h"
#include "xproto.h"
#include "render.h"
#include "shape.h"

xcb_extension_t xcb_xfixes_id = { "XFIXES", 0 };


/*****************************************************************************
 **
 ** xcb_xfixes_query_version_cookie_t xcb_xfixes_query_version
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          client_major_version
 ** @param uint32_t          client_minor_version
 ** @returns xcb_xfixes_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_query_version_cookie_t
xcb_xfixes_query_version (xcb_connection_t *c  /**< */,
                          uint32_t          client_major_version  /**< */,
                          uint32_t          client_minor_version  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_query_version_cookie_t xcb_ret;
    xcb_xfixes_query_version_request_t xcb_out;
    
    xcb_out.client_major_version = client_major_version;
    xcb_out.client_minor_version = client_minor_version;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_query_version_cookie_t xcb_xfixes_query_version_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          client_major_version
 ** @param uint32_t          client_minor_version
 ** @returns xcb_xfixes_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_query_version_cookie_t
xcb_xfixes_query_version_unchecked (xcb_connection_t *c  /**< */,
                                    uint32_t          client_major_version  /**< */,
                                    uint32_t          client_minor_version  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_query_version_cookie_t xcb_ret;
    xcb_xfixes_query_version_request_t xcb_out;
    
    xcb_out.client_major_version = client_major_version;
    xcb_out.client_minor_version = client_minor_version;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_query_version_reply_t * xcb_xfixes_query_version_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_xfixes_query_version_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_xfixes_query_version_reply_t *
 **
 *****************************************************************************/
 
xcb_xfixes_query_version_reply_t *
xcb_xfixes_query_version_reply (xcb_connection_t                   *c  /**< */,
                                xcb_xfixes_query_version_cookie_t   cookie  /**< */,
                                xcb_generic_error_t               **e  /**< */)
{
    return (xcb_xfixes_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_change_save_set_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint8_t           mode
 ** @param uint8_t           target
 ** @param uint8_t           map
 ** @param xcb_window_t      window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_change_save_set_checked (xcb_connection_t *c  /**< */,
                                    uint8_t           mode  /**< */,
                                    uint8_t           target  /**< */,
                                    uint8_t           map  /**< */,
                                    xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CHANGE_SAVE_SET,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_change_save_set_request_t xcb_out;
    
    xcb_out.mode = mode;
    xcb_out.target = target;
    xcb_out.map = map;
    xcb_out.pad0 = 0;
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_change_save_set
 ** 
 ** @param xcb_connection_t *c
 ** @param uint8_t           mode
 ** @param uint8_t           target
 ** @param uint8_t           map
 ** @param xcb_window_t      window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_change_save_set (xcb_connection_t *c  /**< */,
                            uint8_t           mode  /**< */,
                            uint8_t           target  /**< */,
                            uint8_t           map  /**< */,
                            xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CHANGE_SAVE_SET,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_change_save_set_request_t xcb_out;
    
    xcb_out.mode = mode;
    xcb_out.target = target;
    xcb_out.map = map;
    xcb_out.pad0 = 0;
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_select_selection_input_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param xcb_atom_t        selection
 ** @param uint32_t          event_mask
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_select_selection_input_checked (xcb_connection_t *c  /**< */,
                                           xcb_window_t      window  /**< */,
                                           xcb_atom_t        selection  /**< */,
                                           uint32_t          event_mask  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SELECT_SELECTION_INPUT,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_select_selection_input_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.selection = selection;
    xcb_out.event_mask = event_mask;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_select_selection_input
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param xcb_atom_t        selection
 ** @param uint32_t          event_mask
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_select_selection_input (xcb_connection_t *c  /**< */,
                                   xcb_window_t      window  /**< */,
                                   xcb_atom_t        selection  /**< */,
                                   uint32_t          event_mask  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SELECT_SELECTION_INPUT,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_select_selection_input_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.selection = selection;
    xcb_out.event_mask = event_mask;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_select_cursor_input_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint32_t          event_mask
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_select_cursor_input_checked (xcb_connection_t *c  /**< */,
                                        xcb_window_t      window  /**< */,
                                        uint32_t          event_mask  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SELECT_CURSOR_INPUT,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_select_cursor_input_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.event_mask = event_mask;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_select_cursor_input
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint32_t          event_mask
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_select_cursor_input (xcb_connection_t *c  /**< */,
                                xcb_window_t      window  /**< */,
                                uint32_t          event_mask  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SELECT_CURSOR_INPUT,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_select_cursor_input_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.event_mask = event_mask;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_cookie_t xcb_xfixes_get_cursor_image
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_cookie_t
xcb_xfixes_get_cursor_image (xcb_connection_t *c  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_GET_CURSOR_IMAGE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_get_cursor_image_cookie_t xcb_ret;
    xcb_xfixes_get_cursor_image_request_t xcb_out;
    
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_cookie_t xcb_xfixes_get_cursor_image_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_cookie_t
xcb_xfixes_get_cursor_image_unchecked (xcb_connection_t *c  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_GET_CURSOR_IMAGE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_get_cursor_image_cookie_t xcb_ret;
    xcb_xfixes_get_cursor_image_request_t xcb_out;
    
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** uint32_t * xcb_xfixes_get_cursor_image_cursor_image
 ** 
 ** @param const xcb_xfixes_get_cursor_image_reply_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_xfixes_get_cursor_image_cursor_image (const xcb_xfixes_get_cursor_image_reply_t *R  /**< */)
{
    return (uint32_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_image_cursor_image_length
 ** 
 ** @param const xcb_xfixes_get_cursor_image_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_image_cursor_image_length (const xcb_xfixes_get_cursor_image_reply_t *R  /**< */)
{
    return (R->width * R->height);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_image_cursor_image_end
 ** 
 ** @param const xcb_xfixes_get_cursor_image_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_image_cursor_image_end (const xcb_xfixes_get_cursor_image_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((uint32_t *) (R + 1)) + ((R->width * R->height));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_reply_t * xcb_xfixes_get_cursor_image_reply
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_xfixes_get_cursor_image_cookie_t   cookie
 ** @param xcb_generic_error_t                  **e
 ** @returns xcb_xfixes_get_cursor_image_reply_t *
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_reply_t *
xcb_xfixes_get_cursor_image_reply (xcb_connection_t                      *c  /**< */,
                                   xcb_xfixes_get_cursor_image_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e  /**< */)
{
    return (xcb_xfixes_get_cursor_image_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** void xcb_xfixes_region_next
 ** 
 ** @param xcb_xfixes_region_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xfixes_region_next (xcb_xfixes_region_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xfixes_region_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_region_end
 ** 
 ** @param xcb_xfixes_region_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_region_end (xcb_xfixes_region_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xfixes_region_t    region
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_checked (xcb_connection_t      *c  /**< */,
                                  xcb_xfixes_region_t    region  /**< */,
                                  uint32_t               rectangles_len  /**< */,
                                  const xcb_rectangle_t *rectangles  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) rectangles;
    xcb_parts[4].iov_len = rectangles_len * sizeof(xcb_rectangle_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xfixes_region_t    region
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region (xcb_connection_t      *c  /**< */,
                          xcb_xfixes_region_t    region  /**< */,
                          uint32_t               rectangles_len  /**< */,
                          const xcb_rectangle_t *rectangles  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) rectangles;
    xcb_parts[4].iov_len = rectangles_len * sizeof(xcb_rectangle_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_bitmap_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param xcb_pixmap_t         bitmap
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_bitmap_checked (xcb_connection_t    *c  /**< */,
                                              xcb_xfixes_region_t  region  /**< */,
                                              xcb_pixmap_t         bitmap  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_BITMAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_bitmap_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.bitmap = bitmap;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_bitmap
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param xcb_pixmap_t         bitmap
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_bitmap (xcb_connection_t    *c  /**< */,
                                      xcb_xfixes_region_t  region  /**< */,
                                      xcb_pixmap_t         bitmap  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_BITMAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_bitmap_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.bitmap = bitmap;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_window_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param xcb_window_t         window
 ** @param xcb_shape_kind_t     kind
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_window_checked (xcb_connection_t    *c  /**< */,
                                              xcb_xfixes_region_t  region  /**< */,
                                              xcb_window_t         window  /**< */,
                                              xcb_shape_kind_t     kind  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_WINDOW,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_window_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.window = window;
    xcb_out.kind = kind;
    memset(xcb_out.pad0, 0, 3);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_window
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param xcb_window_t         window
 ** @param xcb_shape_kind_t     kind
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_window (xcb_connection_t    *c  /**< */,
                                      xcb_xfixes_region_t  region  /**< */,
                                      xcb_window_t         window  /**< */,
                                      xcb_shape_kind_t     kind  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_WINDOW,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_window_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.window = window;
    xcb_out.kind = kind;
    memset(xcb_out.pad0, 0, 3);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_gc_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param xcb_gcontext_t       gc
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_gc_checked (xcb_connection_t    *c  /**< */,
                                          xcb_xfixes_region_t  region  /**< */,
                                          xcb_gcontext_t       gc  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_GC,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_gc_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.gc = gc;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_gc
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param xcb_gcontext_t       gc
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_gc (xcb_connection_t    *c  /**< */,
                                  xcb_xfixes_region_t  region  /**< */,
                                  xcb_gcontext_t       gc  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_GC,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_gc_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.gc = gc;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_picture_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_xfixes_region_t   region
 ** @param xcb_render_picture_t  picture
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_picture_checked (xcb_connection_t     *c  /**< */,
                                               xcb_xfixes_region_t   region  /**< */,
                                               xcb_render_picture_t  picture  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_PICTURE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_picture_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.picture = picture;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_create_region_from_picture
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_xfixes_region_t   region
 ** @param xcb_render_picture_t  picture
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_create_region_from_picture (xcb_connection_t     *c  /**< */,
                                       xcb_xfixes_region_t   region  /**< */,
                                       xcb_render_picture_t  picture  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CREATE_REGION_FROM_PICTURE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_create_region_from_picture_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.picture = picture;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_destroy_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_destroy_region_checked (xcb_connection_t    *c  /**< */,
                                   xcb_xfixes_region_t  region  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_DESTROY_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_destroy_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_destroy_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_destroy_region (xcb_connection_t    *c  /**< */,
                           xcb_xfixes_region_t  region  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_DESTROY_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_destroy_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_region_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xfixes_region_t    region
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_region_checked (xcb_connection_t      *c  /**< */,
                               xcb_xfixes_region_t    region  /**< */,
                               uint32_t               rectangles_len  /**< */,
                               const xcb_rectangle_t *rectangles  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) rectangles;
    xcb_parts[4].iov_len = rectangles_len * sizeof(xcb_rectangle_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_region
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xfixes_region_t    region
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_region (xcb_connection_t      *c  /**< */,
                       xcb_xfixes_region_t    region  /**< */,
                       uint32_t               rectangles_len  /**< */,
                       const xcb_rectangle_t *rectangles  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) rectangles;
    xcb_parts[4].iov_len = rectangles_len * sizeof(xcb_rectangle_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_copy_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_copy_region_checked (xcb_connection_t    *c  /**< */,
                                xcb_xfixes_region_t  source  /**< */,
                                xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_COPY_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_copy_region_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_copy_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_copy_region (xcb_connection_t    *c  /**< */,
                        xcb_xfixes_region_t  source  /**< */,
                        xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_COPY_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_copy_region_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_union_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source1
 ** @param xcb_xfixes_region_t  source2
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_union_region_checked (xcb_connection_t    *c  /**< */,
                                 xcb_xfixes_region_t  source1  /**< */,
                                 xcb_xfixes_region_t  source2  /**< */,
                                 xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_UNION_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_union_region_request_t xcb_out;
    
    xcb_out.source1 = source1;
    xcb_out.source2 = source2;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_union_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source1
 ** @param xcb_xfixes_region_t  source2
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_union_region (xcb_connection_t    *c  /**< */,
                         xcb_xfixes_region_t  source1  /**< */,
                         xcb_xfixes_region_t  source2  /**< */,
                         xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_UNION_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_union_region_request_t xcb_out;
    
    xcb_out.source1 = source1;
    xcb_out.source2 = source2;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_intersect_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source1
 ** @param xcb_xfixes_region_t  source2
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_intersect_region_checked (xcb_connection_t    *c  /**< */,
                                     xcb_xfixes_region_t  source1  /**< */,
                                     xcb_xfixes_region_t  source2  /**< */,
                                     xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_INTERSECT_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_intersect_region_request_t xcb_out;
    
    xcb_out.source1 = source1;
    xcb_out.source2 = source2;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_intersect_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source1
 ** @param xcb_xfixes_region_t  source2
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_intersect_region (xcb_connection_t    *c  /**< */,
                             xcb_xfixes_region_t  source1  /**< */,
                             xcb_xfixes_region_t  source2  /**< */,
                             xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_INTERSECT_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_intersect_region_request_t xcb_out;
    
    xcb_out.source1 = source1;
    xcb_out.source2 = source2;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_subtract_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source1
 ** @param xcb_xfixes_region_t  source2
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_subtract_region_checked (xcb_connection_t    *c  /**< */,
                                    xcb_xfixes_region_t  source1  /**< */,
                                    xcb_xfixes_region_t  source2  /**< */,
                                    xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SUBTRACT_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_subtract_region_request_t xcb_out;
    
    xcb_out.source1 = source1;
    xcb_out.source2 = source2;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_subtract_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source1
 ** @param xcb_xfixes_region_t  source2
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_subtract_region (xcb_connection_t    *c  /**< */,
                            xcb_xfixes_region_t  source1  /**< */,
                            xcb_xfixes_region_t  source2  /**< */,
                            xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SUBTRACT_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_subtract_region_request_t xcb_out;
    
    xcb_out.source1 = source1;
    xcb_out.source2 = source2;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_invert_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_rectangle_t      bounds
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_invert_region_checked (xcb_connection_t    *c  /**< */,
                                  xcb_xfixes_region_t  source  /**< */,
                                  xcb_rectangle_t      bounds  /**< */,
                                  xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_INVERT_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_invert_region_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.bounds = bounds;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_invert_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_rectangle_t      bounds
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_invert_region (xcb_connection_t    *c  /**< */,
                          xcb_xfixes_region_t  source  /**< */,
                          xcb_rectangle_t      bounds  /**< */,
                          xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_INVERT_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_invert_region_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.bounds = bounds;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_translate_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param int16_t              dx
 ** @param int16_t              dy
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_translate_region_checked (xcb_connection_t    *c  /**< */,
                                     xcb_xfixes_region_t  region  /**< */,
                                     int16_t              dx  /**< */,
                                     int16_t              dy  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_TRANSLATE_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_translate_region_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.dx = dx;
    xcb_out.dy = dy;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_translate_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @param int16_t              dx
 ** @param int16_t              dy
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_translate_region (xcb_connection_t    *c  /**< */,
                             xcb_xfixes_region_t  region  /**< */,
                             int16_t              dx  /**< */,
                             int16_t              dy  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_TRANSLATE_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_translate_region_request_t xcb_out;
    
    xcb_out.region = region;
    xcb_out.dx = dx;
    xcb_out.dy = dy;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_region_extents_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_region_extents_checked (xcb_connection_t    *c  /**< */,
                                   xcb_xfixes_region_t  source  /**< */,
                                   xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_REGION_EXTENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_region_extents_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_region_extents
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_xfixes_region_t  destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_region_extents (xcb_connection_t    *c  /**< */,
                           xcb_xfixes_region_t  source  /**< */,
                           xcb_xfixes_region_t  destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_REGION_EXTENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_region_extents_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_fetch_region_cookie_t xcb_xfixes_fetch_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @returns xcb_xfixes_fetch_region_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_fetch_region_cookie_t
xcb_xfixes_fetch_region (xcb_connection_t    *c  /**< */,
                         xcb_xfixes_region_t  region  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_FETCH_REGION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_fetch_region_cookie_t xcb_ret;
    xcb_xfixes_fetch_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_fetch_region_cookie_t xcb_xfixes_fetch_region_unchecked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  region
 ** @returns xcb_xfixes_fetch_region_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_fetch_region_cookie_t
xcb_xfixes_fetch_region_unchecked (xcb_connection_t    *c  /**< */,
                                   xcb_xfixes_region_t  region  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_FETCH_REGION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_fetch_region_cookie_t xcb_ret;
    xcb_xfixes_fetch_region_request_t xcb_out;
    
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_rectangle_t * xcb_xfixes_fetch_region_rectangles
 ** 
 ** @param const xcb_xfixes_fetch_region_reply_t *R
 ** @returns xcb_rectangle_t *
 **
 *****************************************************************************/
 
xcb_rectangle_t *
xcb_xfixes_fetch_region_rectangles (const xcb_xfixes_fetch_region_reply_t *R  /**< */)
{
    return (xcb_rectangle_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xfixes_fetch_region_rectangles_length
 ** 
 ** @param const xcb_xfixes_fetch_region_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_fetch_region_rectangles_length (const xcb_xfixes_fetch_region_reply_t *R  /**< */)
{
    return (R->length / 2);
}


/*****************************************************************************
 **
 ** xcb_rectangle_iterator_t xcb_xfixes_fetch_region_rectangles_iterator
 ** 
 ** @param const xcb_xfixes_fetch_region_reply_t *R
 ** @returns xcb_rectangle_iterator_t
 **
 *****************************************************************************/
 
xcb_rectangle_iterator_t
xcb_xfixes_fetch_region_rectangles_iterator (const xcb_xfixes_fetch_region_reply_t *R  /**< */)
{
    xcb_rectangle_iterator_t i;
    i.data = (xcb_rectangle_t *) (R + 1);
    i.rem = (R->length / 2);
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xfixes_fetch_region_reply_t * xcb_xfixes_fetch_region_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_xfixes_fetch_region_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_xfixes_fetch_region_reply_t *
 **
 *****************************************************************************/
 
xcb_xfixes_fetch_region_reply_t *
xcb_xfixes_fetch_region_reply (xcb_connection_t                  *c  /**< */,
                               xcb_xfixes_fetch_region_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */)
{
    return (xcb_xfixes_fetch_region_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_gc_clip_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_gcontext_t       gc
 ** @param xcb_xfixes_region_t  region
 ** @param int16_t              x_origin
 ** @param int16_t              y_origin
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_gc_clip_region_checked (xcb_connection_t    *c  /**< */,
                                       xcb_gcontext_t       gc  /**< */,
                                       xcb_xfixes_region_t  region  /**< */,
                                       int16_t              x_origin  /**< */,
                                       int16_t              y_origin  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_GC_CLIP_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_gc_clip_region_request_t xcb_out;
    
    xcb_out.gc = gc;
    xcb_out.region = region;
    xcb_out.x_origin = x_origin;
    xcb_out.y_origin = y_origin;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_gc_clip_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_gcontext_t       gc
 ** @param xcb_xfixes_region_t  region
 ** @param int16_t              x_origin
 ** @param int16_t              y_origin
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_gc_clip_region (xcb_connection_t    *c  /**< */,
                               xcb_gcontext_t       gc  /**< */,
                               xcb_xfixes_region_t  region  /**< */,
                               int16_t              x_origin  /**< */,
                               int16_t              y_origin  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_GC_CLIP_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_gc_clip_region_request_t xcb_out;
    
    xcb_out.gc = gc;
    xcb_out.region = region;
    xcb_out.x_origin = x_origin;
    xcb_out.y_origin = y_origin;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_window_shape_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_window_t         dest
 ** @param xcb_shape_kind_t     dest_kind
 ** @param int16_t              x_offset
 ** @param int16_t              y_offset
 ** @param xcb_xfixes_region_t  region
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_window_shape_region_checked (xcb_connection_t    *c  /**< */,
                                            xcb_window_t         dest  /**< */,
                                            xcb_shape_kind_t     dest_kind  /**< */,
                                            int16_t              x_offset  /**< */,
                                            int16_t              y_offset  /**< */,
                                            xcb_xfixes_region_t  region  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_WINDOW_SHAPE_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_window_shape_region_request_t xcb_out;
    
    xcb_out.dest = dest;
    xcb_out.dest_kind = dest_kind;
    memset(xcb_out.pad0, 0, 3);
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_window_shape_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_window_t         dest
 ** @param xcb_shape_kind_t     dest_kind
 ** @param int16_t              x_offset
 ** @param int16_t              y_offset
 ** @param xcb_xfixes_region_t  region
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_window_shape_region (xcb_connection_t    *c  /**< */,
                                    xcb_window_t         dest  /**< */,
                                    xcb_shape_kind_t     dest_kind  /**< */,
                                    int16_t              x_offset  /**< */,
                                    int16_t              y_offset  /**< */,
                                    xcb_xfixes_region_t  region  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_WINDOW_SHAPE_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_window_shape_region_request_t xcb_out;
    
    xcb_out.dest = dest;
    xcb_out.dest_kind = dest_kind;
    memset(xcb_out.pad0, 0, 3);
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    xcb_out.region = region;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_picture_clip_region_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @param xcb_xfixes_region_t   region
 ** @param int16_t               x_origin
 ** @param int16_t               y_origin
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_picture_clip_region_checked (xcb_connection_t     *c  /**< */,
                                            xcb_render_picture_t  picture  /**< */,
                                            xcb_xfixes_region_t   region  /**< */,
                                            int16_t               x_origin  /**< */,
                                            int16_t               y_origin  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_PICTURE_CLIP_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_picture_clip_region_request_t xcb_out;
    
    xcb_out.picture = picture;
    xcb_out.region = region;
    xcb_out.x_origin = x_origin;
    xcb_out.y_origin = y_origin;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_picture_clip_region
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @param xcb_xfixes_region_t   region
 ** @param int16_t               x_origin
 ** @param int16_t               y_origin
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_picture_clip_region (xcb_connection_t     *c  /**< */,
                                    xcb_render_picture_t  picture  /**< */,
                                    xcb_xfixes_region_t   region  /**< */,
                                    int16_t               x_origin  /**< */,
                                    int16_t               y_origin  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_PICTURE_CLIP_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_picture_clip_region_request_t xcb_out;
    
    xcb_out.picture = picture;
    xcb_out.region = region;
    xcb_out.x_origin = x_origin;
    xcb_out.y_origin = y_origin;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_cursor_name_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      cursor
 ** @param uint16_t          nbytes
 ** @param const char       *name
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_cursor_name_checked (xcb_connection_t *c  /**< */,
                                    xcb_cursor_t      cursor  /**< */,
                                    uint16_t          nbytes  /**< */,
                                    const char       *name  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_CURSOR_NAME,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_cursor_name_request_t xcb_out;
    
    xcb_out.cursor = cursor;
    xcb_out.nbytes = nbytes;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) name;
    xcb_parts[4].iov_len = nbytes * sizeof(char);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_set_cursor_name
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      cursor
 ** @param uint16_t          nbytes
 ** @param const char       *name
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_set_cursor_name (xcb_connection_t *c  /**< */,
                            xcb_cursor_t      cursor  /**< */,
                            uint16_t          nbytes  /**< */,
                            const char       *name  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SET_CURSOR_NAME,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_set_cursor_name_request_t xcb_out;
    
    xcb_out.cursor = cursor;
    xcb_out.nbytes = nbytes;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) name;
    xcb_parts[4].iov_len = nbytes * sizeof(char);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_name_cookie_t xcb_xfixes_get_cursor_name
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      cursor
 ** @returns xcb_xfixes_get_cursor_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_name_cookie_t
xcb_xfixes_get_cursor_name (xcb_connection_t *c  /**< */,
                            xcb_cursor_t      cursor  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_GET_CURSOR_NAME,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_get_cursor_name_cookie_t xcb_ret;
    xcb_xfixes_get_cursor_name_request_t xcb_out;
    
    xcb_out.cursor = cursor;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_name_cookie_t xcb_xfixes_get_cursor_name_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      cursor
 ** @returns xcb_xfixes_get_cursor_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_name_cookie_t
xcb_xfixes_get_cursor_name_unchecked (xcb_connection_t *c  /**< */,
                                      xcb_cursor_t      cursor  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_GET_CURSOR_NAME,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_get_cursor_name_cookie_t xcb_ret;
    xcb_xfixes_get_cursor_name_request_t xcb_out;
    
    xcb_out.cursor = cursor;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** char * xcb_xfixes_get_cursor_name_name
 ** 
 ** @param const xcb_xfixes_get_cursor_name_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_xfixes_get_cursor_name_name (const xcb_xfixes_get_cursor_name_reply_t *R  /**< */)
{
    return (char *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_name_name_length
 ** 
 ** @param const xcb_xfixes_get_cursor_name_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_name_name_length (const xcb_xfixes_get_cursor_name_reply_t *R  /**< */)
{
    return R->nbytes;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_name_name_end
 ** 
 ** @param const xcb_xfixes_get_cursor_name_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_name_name_end (const xcb_xfixes_get_cursor_name_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((char *) (R + 1)) + (R->nbytes);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_name_reply_t * xcb_xfixes_get_cursor_name_reply
 ** 
 ** @param xcb_connection_t                     *c
 ** @param xcb_xfixes_get_cursor_name_cookie_t   cookie
 ** @param xcb_generic_error_t                 **e
 ** @returns xcb_xfixes_get_cursor_name_reply_t *
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_name_reply_t *
xcb_xfixes_get_cursor_name_reply (xcb_connection_t                     *c  /**< */,
                                  xcb_xfixes_get_cursor_name_cookie_t   cookie  /**< */,
                                  xcb_generic_error_t                 **e  /**< */)
{
    return (xcb_xfixes_get_cursor_name_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_and_name_cookie_t xcb_xfixes_get_cursor_image_and_name
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_and_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_and_name_cookie_t
xcb_xfixes_get_cursor_image_and_name (xcb_connection_t *c  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_GET_CURSOR_IMAGE_AND_NAME,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_get_cursor_image_and_name_cookie_t xcb_ret;
    xcb_xfixes_get_cursor_image_and_name_request_t xcb_out;
    
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_and_name_cookie_t xcb_xfixes_get_cursor_image_and_name_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_and_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_and_name_cookie_t
xcb_xfixes_get_cursor_image_and_name_unchecked (xcb_connection_t *c  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_GET_CURSOR_IMAGE_AND_NAME,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xfixes_get_cursor_image_and_name_cookie_t xcb_ret;
    xcb_xfixes_get_cursor_image_and_name_request_t xcb_out;
    
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** char * xcb_xfixes_get_cursor_image_and_name_name
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_xfixes_get_cursor_image_and_name_name (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */)
{
    return (char *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_image_and_name_name_length
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_image_and_name_name_length (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */)
{
    return R->nbytes;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_image_and_name_name_end
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_image_and_name_name_end (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((char *) (R + 1)) + (R->nbytes);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** uint32_t * xcb_xfixes_get_cursor_image_and_name_cursor_image
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_xfixes_get_cursor_image_and_name_cursor_image (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xfixes_get_cursor_image_and_name_name_end(R);
    return (uint32_t *) ((char *) prev.data + XCB_TYPE_PAD(uint32_t, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_image_and_name_cursor_image_length
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_image_and_name_cursor_image_length (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */)
{
    return (R->width * R->height);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_image_and_name_cursor_image_end
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_image_and_name_cursor_image_end (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t child = xcb_xfixes_get_cursor_image_and_name_name_end(R);
    i.data = ((uint32_t *) child.data) + ((R->width * R->height));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_and_name_reply_t * xcb_xfixes_get_cursor_image_and_name_reply
 ** 
 ** @param xcb_connection_t                               *c
 ** @param xcb_xfixes_get_cursor_image_and_name_cookie_t   cookie
 ** @param xcb_generic_error_t                           **e
 ** @returns xcb_xfixes_get_cursor_image_and_name_reply_t *
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_and_name_reply_t *
xcb_xfixes_get_cursor_image_and_name_reply (xcb_connection_t                               *c  /**< */,
                                            xcb_xfixes_get_cursor_image_and_name_cookie_t   cookie  /**< */,
                                            xcb_generic_error_t                           **e  /**< */)
{
    return (xcb_xfixes_get_cursor_image_and_name_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_change_cursor_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      source
 ** @param xcb_cursor_t      destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_change_cursor_checked (xcb_connection_t *c  /**< */,
                                  xcb_cursor_t      source  /**< */,
                                  xcb_cursor_t      destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CHANGE_CURSOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_change_cursor_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_change_cursor
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      source
 ** @param xcb_cursor_t      destination
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_change_cursor (xcb_connection_t *c  /**< */,
                          xcb_cursor_t      source  /**< */,
                          xcb_cursor_t      destination  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CHANGE_CURSOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_change_cursor_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_change_cursor_by_name_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      src
 ** @param uint16_t          nbytes
 ** @param const char       *name
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_change_cursor_by_name_checked (xcb_connection_t *c  /**< */,
                                          xcb_cursor_t      src  /**< */,
                                          uint16_t          nbytes  /**< */,
                                          const char       *name  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CHANGE_CURSOR_BY_NAME,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_change_cursor_by_name_request_t xcb_out;
    
    xcb_out.src = src;
    xcb_out.nbytes = nbytes;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) name;
    xcb_parts[4].iov_len = nbytes * sizeof(char);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_change_cursor_by_name
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_cursor_t      src
 ** @param uint16_t          nbytes
 ** @param const char       *name
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_change_cursor_by_name (xcb_connection_t *c  /**< */,
                                  xcb_cursor_t      src  /**< */,
                                  uint16_t          nbytes  /**< */,
                                  const char       *name  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_CHANGE_CURSOR_BY_NAME,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_change_cursor_by_name_request_t xcb_out;
    
    xcb_out.src = src;
    xcb_out.nbytes = nbytes;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = (char *) name;
    xcb_parts[4].iov_len = nbytes * sizeof(char);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_expand_region_checked
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_xfixes_region_t  destination
 ** @param uint16_t             left
 ** @param uint16_t             right
 ** @param uint16_t             top
 ** @param uint16_t             bottom
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_expand_region_checked (xcb_connection_t    *c  /**< */,
                                  xcb_xfixes_region_t  source  /**< */,
                                  xcb_xfixes_region_t  destination  /**< */,
                                  uint16_t             left  /**< */,
                                  uint16_t             right  /**< */,
                                  uint16_t             top  /**< */,
                                  uint16_t             bottom  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_EXPAND_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_expand_region_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    xcb_out.left = left;
    xcb_out.right = right;
    xcb_out.top = top;
    xcb_out.bottom = bottom;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_expand_region
 ** 
 ** @param xcb_connection_t    *c
 ** @param xcb_xfixes_region_t  source
 ** @param xcb_xfixes_region_t  destination
 ** @param uint16_t             left
 ** @param uint16_t             right
 ** @param uint16_t             top
 ** @param uint16_t             bottom
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_expand_region (xcb_connection_t    *c  /**< */,
                          xcb_xfixes_region_t  source  /**< */,
                          xcb_xfixes_region_t  destination  /**< */,
                          uint16_t             left  /**< */,
                          uint16_t             right  /**< */,
                          uint16_t             top  /**< */,
                          uint16_t             bottom  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_EXPAND_REGION,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_expand_region_request_t xcb_out;
    
    xcb_out.source = source;
    xcb_out.destination = destination;
    xcb_out.left = left;
    xcb_out.right = right;
    xcb_out.top = top;
    xcb_out.bottom = bottom;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_hide_cursor_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_hide_cursor_checked (xcb_connection_t *c  /**< */,
                                xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_HIDE_CURSOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_hide_cursor_request_t xcb_out;
    
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_hide_cursor
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_hide_cursor (xcb_connection_t *c  /**< */,
                        xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_HIDE_CURSOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_hide_cursor_request_t xcb_out;
    
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_show_cursor_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_show_cursor_checked (xcb_connection_t *c  /**< */,
                                xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SHOW_CURSOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_show_cursor_request_t xcb_out;
    
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xfixes_show_cursor
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xfixes_show_cursor (xcb_connection_t *c  /**< */,
                        xcb_window_t      window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xfixes_id,
        /* opcode */ XCB_XFIXES_SHOW_CURSOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xfixes_show_cursor_request_t xcb_out;
    
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

