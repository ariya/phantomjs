/*
 * This file generated automatically from shape.xml by c_client.py.
 * Edit at your peril.
 */

#include <string.h>
#include <assert.h>
#include "xcbext.h"
#include "shape.h"
#include "xproto.h"

xcb_extension_t xcb_shape_id = { "SHAPE", 0 };


/*****************************************************************************
 **
 ** void xcb_shape_op_next
 ** 
 ** @param xcb_shape_op_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_shape_op_next (xcb_shape_op_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_shape_op_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_shape_op_end
 ** 
 ** @param xcb_shape_op_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_shape_op_end (xcb_shape_op_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_shape_kind_next
 ** 
 ** @param xcb_shape_kind_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_shape_kind_next (xcb_shape_kind_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_shape_kind_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_shape_kind_end
 ** 
 ** @param xcb_shape_kind_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_shape_kind_end (xcb_shape_kind_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** xcb_shape_query_version_cookie_t xcb_shape_query_version
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_shape_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_query_version_cookie_t
xcb_shape_query_version (xcb_connection_t *c  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_query_version_cookie_t xcb_ret;
    xcb_shape_query_version_request_t xcb_out;
    
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_query_version_cookie_t xcb_shape_query_version_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_shape_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_query_version_cookie_t
xcb_shape_query_version_unchecked (xcb_connection_t *c  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_QUERY_VERSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_query_version_cookie_t xcb_ret;
    xcb_shape_query_version_request_t xcb_out;
    
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_query_version_reply_t * xcb_shape_query_version_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_shape_query_version_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_shape_query_version_reply_t *
 **
 *****************************************************************************/
 
xcb_shape_query_version_reply_t *
xcb_shape_query_version_reply (xcb_connection_t                  *c  /**< */,
                               xcb_shape_query_version_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */)
{
    return (xcb_shape_query_version_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_rectangles_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_shape_op_t         operation
 ** @param xcb_shape_kind_t       destination_kind
 ** @param uint8_t                ordering
 ** @param xcb_window_t           destination_window
 ** @param int16_t                x_offset
 ** @param int16_t                y_offset
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_rectangles_checked (xcb_connection_t      *c  /**< */,
                              xcb_shape_op_t         operation  /**< */,
                              xcb_shape_kind_t       destination_kind  /**< */,
                              uint8_t                ordering  /**< */,
                              xcb_window_t           destination_window  /**< */,
                              int16_t                x_offset  /**< */,
                              int16_t                y_offset  /**< */,
                              uint32_t               rectangles_len  /**< */,
                              const xcb_rectangle_t *rectangles  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_RECTANGLES,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_rectangles_request_t xcb_out;
    
    xcb_out.operation = operation;
    xcb_out.destination_kind = destination_kind;
    xcb_out.ordering = ordering;
    xcb_out.pad0 = 0;
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    
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
 ** xcb_void_cookie_t xcb_shape_rectangles
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_shape_op_t         operation
 ** @param xcb_shape_kind_t       destination_kind
 ** @param uint8_t                ordering
 ** @param xcb_window_t           destination_window
 ** @param int16_t                x_offset
 ** @param int16_t                y_offset
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_rectangles (xcb_connection_t      *c  /**< */,
                      xcb_shape_op_t         operation  /**< */,
                      xcb_shape_kind_t       destination_kind  /**< */,
                      uint8_t                ordering  /**< */,
                      xcb_window_t           destination_window  /**< */,
                      int16_t                x_offset  /**< */,
                      int16_t                y_offset  /**< */,
                      uint32_t               rectangles_len  /**< */,
                      const xcb_rectangle_t *rectangles  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_RECTANGLES,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_rectangles_request_t xcb_out;
    
    xcb_out.operation = operation;
    xcb_out.destination_kind = destination_kind;
    xcb_out.ordering = ordering;
    xcb_out.pad0 = 0;
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    
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
 ** xcb_void_cookie_t xcb_shape_mask_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_shape_op_t    operation
 ** @param xcb_shape_kind_t  destination_kind
 ** @param xcb_window_t      destination_window
 ** @param int16_t           x_offset
 ** @param int16_t           y_offset
 ** @param xcb_pixmap_t      source_bitmap
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_mask_checked (xcb_connection_t *c  /**< */,
                        xcb_shape_op_t    operation  /**< */,
                        xcb_shape_kind_t  destination_kind  /**< */,
                        xcb_window_t      destination_window  /**< */,
                        int16_t           x_offset  /**< */,
                        int16_t           y_offset  /**< */,
                        xcb_pixmap_t      source_bitmap  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_MASK,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_mask_request_t xcb_out;
    
    xcb_out.operation = operation;
    xcb_out.destination_kind = destination_kind;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    xcb_out.source_bitmap = source_bitmap;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_mask
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_shape_op_t    operation
 ** @param xcb_shape_kind_t  destination_kind
 ** @param xcb_window_t      destination_window
 ** @param int16_t           x_offset
 ** @param int16_t           y_offset
 ** @param xcb_pixmap_t      source_bitmap
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_mask (xcb_connection_t *c  /**< */,
                xcb_shape_op_t    operation  /**< */,
                xcb_shape_kind_t  destination_kind  /**< */,
                xcb_window_t      destination_window  /**< */,
                int16_t           x_offset  /**< */,
                int16_t           y_offset  /**< */,
                xcb_pixmap_t      source_bitmap  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_MASK,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_mask_request_t xcb_out;
    
    xcb_out.operation = operation;
    xcb_out.destination_kind = destination_kind;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    xcb_out.source_bitmap = source_bitmap;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_combine_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_shape_op_t    operation
 ** @param xcb_shape_kind_t  destination_kind
 ** @param xcb_shape_kind_t  source_kind
 ** @param xcb_window_t      destination_window
 ** @param int16_t           x_offset
 ** @param int16_t           y_offset
 ** @param xcb_window_t      source_window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_combine_checked (xcb_connection_t *c  /**< */,
                           xcb_shape_op_t    operation  /**< */,
                           xcb_shape_kind_t  destination_kind  /**< */,
                           xcb_shape_kind_t  source_kind  /**< */,
                           xcb_window_t      destination_window  /**< */,
                           int16_t           x_offset  /**< */,
                           int16_t           y_offset  /**< */,
                           xcb_window_t      source_window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_COMBINE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_combine_request_t xcb_out;
    
    xcb_out.operation = operation;
    xcb_out.destination_kind = destination_kind;
    xcb_out.source_kind = source_kind;
    xcb_out.pad0 = 0;
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    xcb_out.source_window = source_window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_combine
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_shape_op_t    operation
 ** @param xcb_shape_kind_t  destination_kind
 ** @param xcb_shape_kind_t  source_kind
 ** @param xcb_window_t      destination_window
 ** @param int16_t           x_offset
 ** @param int16_t           y_offset
 ** @param xcb_window_t      source_window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_combine (xcb_connection_t *c  /**< */,
                   xcb_shape_op_t    operation  /**< */,
                   xcb_shape_kind_t  destination_kind  /**< */,
                   xcb_shape_kind_t  source_kind  /**< */,
                   xcb_window_t      destination_window  /**< */,
                   int16_t           x_offset  /**< */,
                   int16_t           y_offset  /**< */,
                   xcb_window_t      source_window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_COMBINE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_combine_request_t xcb_out;
    
    xcb_out.operation = operation;
    xcb_out.destination_kind = destination_kind;
    xcb_out.source_kind = source_kind;
    xcb_out.pad0 = 0;
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    xcb_out.source_window = source_window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_offset_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_shape_kind_t  destination_kind
 ** @param xcb_window_t      destination_window
 ** @param int16_t           x_offset
 ** @param int16_t           y_offset
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_offset_checked (xcb_connection_t *c  /**< */,
                          xcb_shape_kind_t  destination_kind  /**< */,
                          xcb_window_t      destination_window  /**< */,
                          int16_t           x_offset  /**< */,
                          int16_t           y_offset  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_OFFSET,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_offset_request_t xcb_out;
    
    xcb_out.destination_kind = destination_kind;
    memset(xcb_out.pad0, 0, 3);
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_offset
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_shape_kind_t  destination_kind
 ** @param xcb_window_t      destination_window
 ** @param int16_t           x_offset
 ** @param int16_t           y_offset
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_offset (xcb_connection_t *c  /**< */,
                  xcb_shape_kind_t  destination_kind  /**< */,
                  xcb_window_t      destination_window  /**< */,
                  int16_t           x_offset  /**< */,
                  int16_t           y_offset  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_OFFSET,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_offset_request_t xcb_out;
    
    xcb_out.destination_kind = destination_kind;
    memset(xcb_out.pad0, 0, 3);
    xcb_out.destination_window = destination_window;
    xcb_out.x_offset = x_offset;
    xcb_out.y_offset = y_offset;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_query_extents_cookie_t xcb_shape_query_extents
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      destination_window
 ** @returns xcb_shape_query_extents_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_query_extents_cookie_t
xcb_shape_query_extents (xcb_connection_t *c  /**< */,
                         xcb_window_t      destination_window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_QUERY_EXTENTS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_query_extents_cookie_t xcb_ret;
    xcb_shape_query_extents_request_t xcb_out;
    
    xcb_out.destination_window = destination_window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_query_extents_cookie_t xcb_shape_query_extents_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      destination_window
 ** @returns xcb_shape_query_extents_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_query_extents_cookie_t
xcb_shape_query_extents_unchecked (xcb_connection_t *c  /**< */,
                                   xcb_window_t      destination_window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_QUERY_EXTENTS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_query_extents_cookie_t xcb_ret;
    xcb_shape_query_extents_request_t xcb_out;
    
    xcb_out.destination_window = destination_window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_query_extents_reply_t * xcb_shape_query_extents_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_shape_query_extents_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_shape_query_extents_reply_t *
 **
 *****************************************************************************/
 
xcb_shape_query_extents_reply_t *
xcb_shape_query_extents_reply (xcb_connection_t                  *c  /**< */,
                               xcb_shape_query_extents_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */)
{
    return (xcb_shape_query_extents_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_shape_select_input_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      destination_window
 ** @param uint8_t           enable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_select_input_checked (xcb_connection_t *c  /**< */,
                                xcb_window_t      destination_window  /**< */,
                                uint8_t           enable  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_SELECT_INPUT,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_select_input_request_t xcb_out;
    
    xcb_out.destination_window = destination_window;
    xcb_out.enable = enable;
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
 ** xcb_void_cookie_t xcb_shape_select_input
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      destination_window
 ** @param uint8_t           enable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_shape_select_input (xcb_connection_t *c  /**< */,
                        xcb_window_t      destination_window  /**< */,
                        uint8_t           enable  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_SELECT_INPUT,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_shape_select_input_request_t xcb_out;
    
    xcb_out.destination_window = destination_window;
    xcb_out.enable = enable;
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
 ** xcb_shape_input_selected_cookie_t xcb_shape_input_selected
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      destination_window
 ** @returns xcb_shape_input_selected_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_input_selected_cookie_t
xcb_shape_input_selected (xcb_connection_t *c  /**< */,
                          xcb_window_t      destination_window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_INPUT_SELECTED,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_input_selected_cookie_t xcb_ret;
    xcb_shape_input_selected_request_t xcb_out;
    
    xcb_out.destination_window = destination_window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_input_selected_cookie_t xcb_shape_input_selected_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      destination_window
 ** @returns xcb_shape_input_selected_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_input_selected_cookie_t
xcb_shape_input_selected_unchecked (xcb_connection_t *c  /**< */,
                                    xcb_window_t      destination_window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_INPUT_SELECTED,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_input_selected_cookie_t xcb_ret;
    xcb_shape_input_selected_request_t xcb_out;
    
    xcb_out.destination_window = destination_window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_shape_input_selected_reply_t * xcb_shape_input_selected_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_shape_input_selected_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_shape_input_selected_reply_t *
 **
 *****************************************************************************/
 
xcb_shape_input_selected_reply_t *
xcb_shape_input_selected_reply (xcb_connection_t                   *c  /**< */,
                                xcb_shape_input_selected_cookie_t   cookie  /**< */,
                                xcb_generic_error_t               **e  /**< */)
{
    return (xcb_shape_input_selected_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_shape_get_rectangles_cookie_t xcb_shape_get_rectangles
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param xcb_shape_kind_t  source_kind
 ** @returns xcb_shape_get_rectangles_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_get_rectangles_cookie_t
xcb_shape_get_rectangles (xcb_connection_t *c  /**< */,
                          xcb_window_t      window  /**< */,
                          xcb_shape_kind_t  source_kind  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_GET_RECTANGLES,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_get_rectangles_cookie_t xcb_ret;
    xcb_shape_get_rectangles_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.source_kind = source_kind;
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
 ** xcb_shape_get_rectangles_cookie_t xcb_shape_get_rectangles_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param xcb_shape_kind_t  source_kind
 ** @returns xcb_shape_get_rectangles_cookie_t
 **
 *****************************************************************************/
 
xcb_shape_get_rectangles_cookie_t
xcb_shape_get_rectangles_unchecked (xcb_connection_t *c  /**< */,
                                    xcb_window_t      window  /**< */,
                                    xcb_shape_kind_t  source_kind  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_shape_id,
        /* opcode */ XCB_SHAPE_GET_RECTANGLES,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_shape_get_rectangles_cookie_t xcb_ret;
    xcb_shape_get_rectangles_request_t xcb_out;
    
    xcb_out.window = window;
    xcb_out.source_kind = source_kind;
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
 ** xcb_rectangle_t * xcb_shape_get_rectangles_rectangles
 ** 
 ** @param const xcb_shape_get_rectangles_reply_t *R
 ** @returns xcb_rectangle_t *
 **
 *****************************************************************************/
 
xcb_rectangle_t *
xcb_shape_get_rectangles_rectangles (const xcb_shape_get_rectangles_reply_t *R  /**< */)
{
    return (xcb_rectangle_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_shape_get_rectangles_rectangles_length
 ** 
 ** @param const xcb_shape_get_rectangles_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_shape_get_rectangles_rectangles_length (const xcb_shape_get_rectangles_reply_t *R  /**< */)
{
    return R->rectangles_len;
}


/*****************************************************************************
 **
 ** xcb_rectangle_iterator_t xcb_shape_get_rectangles_rectangles_iterator
 ** 
 ** @param const xcb_shape_get_rectangles_reply_t *R
 ** @returns xcb_rectangle_iterator_t
 **
 *****************************************************************************/
 
xcb_rectangle_iterator_t
xcb_shape_get_rectangles_rectangles_iterator (const xcb_shape_get_rectangles_reply_t *R  /**< */)
{
    xcb_rectangle_iterator_t i;
    i.data = (xcb_rectangle_t *) (R + 1);
    i.rem = R->rectangles_len;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_shape_get_rectangles_reply_t * xcb_shape_get_rectangles_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_shape_get_rectangles_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_shape_get_rectangles_reply_t *
 **
 *****************************************************************************/
 
xcb_shape_get_rectangles_reply_t *
xcb_shape_get_rectangles_reply (xcb_connection_t                   *c  /**< */,
                                xcb_shape_get_rectangles_cookie_t   cookie  /**< */,
                                xcb_generic_error_t               **e  /**< */)
{
    return (xcb_shape_get_rectangles_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

