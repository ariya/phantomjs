/*
 * This file generated automatically from xfixes.xml by c_client.py.
 * Edit at your peril.
 */

/**
 * @defgroup XCB_XFixes_API XCB XFixes API
 * @brief XFixes XCB Protocol Implementation.
 * @{
 **/

#ifndef __XFIXES_H
#define __XFIXES_H

#include "xcb.h"
#include "xproto.h"
#include "render.h"
#include "shape.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XCB_XFIXES_MAJOR_VERSION 4
#define XCB_XFIXES_MINOR_VERSION 0
  
extern xcb_extension_t xcb_xfixes_id;

/**
 * @brief xcb_xfixes_query_version_cookie_t
 **/
typedef struct xcb_xfixes_query_version_cookie_t {
    unsigned int sequence; /**<  */
} xcb_xfixes_query_version_cookie_t;

/** Opcode for xcb_xfixes_query_version. */
#define XCB_XFIXES_QUERY_VERSION 0

/**
 * @brief xcb_xfixes_query_version_request_t
 **/
typedef struct xcb_xfixes_query_version_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint32_t client_major_version; /**<  */
    uint32_t client_minor_version; /**<  */
} xcb_xfixes_query_version_request_t;

/**
 * @brief xcb_xfixes_query_version_reply_t
 **/
typedef struct xcb_xfixes_query_version_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t major_version; /**<  */
    uint32_t minor_version; /**<  */
    uint8_t  pad1[16]; /**<  */
} xcb_xfixes_query_version_reply_t;

typedef enum xcb_xfixes_save_set_mode_t {
    XCB_XFIXES_SAVE_SET_MODE_INSERT,
    XCB_XFIXES_SAVE_SET_MODE_DELETE
} xcb_xfixes_save_set_mode_t;

typedef enum xcb_xfixes_save_set_target_t {
    XCB_XFIXES_SAVE_SET_TARGET_NEAREST,
    XCB_XFIXES_SAVE_SET_TARGET_ROOT
} xcb_xfixes_save_set_target_t;

typedef enum xcb_xfixes_save_set_mapping_t {
    XCB_XFIXES_SAVE_SET_MAPPING_MAP,
    XCB_XFIXES_SAVE_SET_MAPPING_UNMAP
} xcb_xfixes_save_set_mapping_t;

/** Opcode for xcb_xfixes_change_save_set. */
#define XCB_XFIXES_CHANGE_SAVE_SET 1

/**
 * @brief xcb_xfixes_change_save_set_request_t
 **/
typedef struct xcb_xfixes_change_save_set_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    uint8_t      mode; /**<  */
    uint8_t      target; /**<  */
    uint8_t      map; /**<  */
    uint8_t      pad0; /**<  */
    xcb_window_t window; /**<  */
} xcb_xfixes_change_save_set_request_t;

typedef enum xcb_xfixes_selection_event_t {
    XCB_XFIXES_SELECTION_EVENT_SET_SELECTION_OWNER,
    XCB_XFIXES_SELECTION_EVENT_SELECTION_WINDOW_DESTROY,
    XCB_XFIXES_SELECTION_EVENT_SELECTION_CLIENT_CLOSE
} xcb_xfixes_selection_event_t;

typedef enum xcb_xfixes_selection_event_mask_t {
    XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER = 1,
    XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY = 2,
    XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE = 4
} xcb_xfixes_selection_event_mask_t;

/** Opcode for xcb_xfixes_selection_notify. */
#define XCB_XFIXES_SELECTION_NOTIFY 0

/**
 * @brief xcb_xfixes_selection_notify_event_t
 **/
typedef struct xcb_xfixes_selection_notify_event_t {
    uint8_t         response_type; /**<  */
    uint8_t         subtype; /**<  */
    uint16_t        sequence; /**<  */
    xcb_window_t    window; /**<  */
    xcb_window_t    owner; /**<  */
    xcb_atom_t      selection; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_timestamp_t selection_timestamp; /**<  */
    uint8_t         pad0[8]; /**<  */
} xcb_xfixes_selection_notify_event_t;

/** Opcode for xcb_xfixes_select_selection_input. */
#define XCB_XFIXES_SELECT_SELECTION_INPUT 2

/**
 * @brief xcb_xfixes_select_selection_input_request_t
 **/
typedef struct xcb_xfixes_select_selection_input_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
    xcb_atom_t   selection; /**<  */
    uint32_t     event_mask; /**<  */
} xcb_xfixes_select_selection_input_request_t;

typedef enum xcb_xfixes_cursor_notify_t {
    XCB_XFIXES_CURSOR_NOTIFY_DISPLAY_CURSOR
} xcb_xfixes_cursor_notify_t;

typedef enum xcb_xfixes_cursor_notify_mask_t {
    XCB_XFIXES_CURSOR_NOTIFY_MASK_DISPLAY_CURSOR = 1
} xcb_xfixes_cursor_notify_mask_t;

/** Opcode for xcb_xfixes_cursor_notify. */
#define XCB_XFIXES_CURSOR_NOTIFY 1

/**
 * @brief xcb_xfixes_cursor_notify_event_t
 **/
typedef struct xcb_xfixes_cursor_notify_event_t {
    uint8_t         response_type; /**<  */
    uint8_t         subtype; /**<  */
    uint16_t        sequence; /**<  */
    xcb_window_t    window; /**<  */
    uint32_t        cursor_serial; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_atom_t      name; /**<  */
    uint8_t         pad0[12]; /**<  */
} xcb_xfixes_cursor_notify_event_t;

/** Opcode for xcb_xfixes_select_cursor_input. */
#define XCB_XFIXES_SELECT_CURSOR_INPUT 3

/**
 * @brief xcb_xfixes_select_cursor_input_request_t
 **/
typedef struct xcb_xfixes_select_cursor_input_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
    uint32_t     event_mask; /**<  */
} xcb_xfixes_select_cursor_input_request_t;

/**
 * @brief xcb_xfixes_get_cursor_image_cookie_t
 **/
typedef struct xcb_xfixes_get_cursor_image_cookie_t {
    unsigned int sequence; /**<  */
} xcb_xfixes_get_cursor_image_cookie_t;

/** Opcode for xcb_xfixes_get_cursor_image. */
#define XCB_XFIXES_GET_CURSOR_IMAGE 4

/**
 * @brief xcb_xfixes_get_cursor_image_request_t
 **/
typedef struct xcb_xfixes_get_cursor_image_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
} xcb_xfixes_get_cursor_image_request_t;

/**
 * @brief xcb_xfixes_get_cursor_image_reply_t
 **/
typedef struct xcb_xfixes_get_cursor_image_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    int16_t  x; /**<  */
    int16_t  y; /**<  */
    uint16_t width; /**<  */
    uint16_t height; /**<  */
    uint16_t xhot; /**<  */
    uint16_t yhot; /**<  */
    uint32_t cursor_serial; /**<  */
    uint8_t  pad1[8]; /**<  */
} xcb_xfixes_get_cursor_image_reply_t;

typedef uint32_t xcb_xfixes_region_t;

/**
 * @brief xcb_xfixes_region_iterator_t
 **/
typedef struct xcb_xfixes_region_iterator_t {
    xcb_xfixes_region_t *data; /**<  */
    int                  rem; /**<  */
    int                  index; /**<  */
} xcb_xfixes_region_iterator_t;

/** Opcode for xcb_xfixes_bad_region. */
#define XCB_XFIXES_BAD_REGION 0

/**
 * @brief xcb_xfixes_bad_region_error_t
 **/
typedef struct xcb_xfixes_bad_region_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_xfixes_bad_region_error_t;

typedef enum xcb_xfixes_region_enum_t {
    XCB_XFIXES_REGION_NONE
} xcb_xfixes_region_enum_t;

/** Opcode for xcb_xfixes_create_region. */
#define XCB_XFIXES_CREATE_REGION 5

/**
 * @brief xcb_xfixes_create_region_request_t
 **/
typedef struct xcb_xfixes_create_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
} xcb_xfixes_create_region_request_t;

/** Opcode for xcb_xfixes_create_region_from_bitmap. */
#define XCB_XFIXES_CREATE_REGION_FROM_BITMAP 6

/**
 * @brief xcb_xfixes_create_region_from_bitmap_request_t
 **/
typedef struct xcb_xfixes_create_region_from_bitmap_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
    xcb_pixmap_t        bitmap; /**<  */
} xcb_xfixes_create_region_from_bitmap_request_t;

/** Opcode for xcb_xfixes_create_region_from_window. */
#define XCB_XFIXES_CREATE_REGION_FROM_WINDOW 7

/**
 * @brief xcb_xfixes_create_region_from_window_request_t
 **/
typedef struct xcb_xfixes_create_region_from_window_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
    xcb_window_t        window; /**<  */
    xcb_shape_kind_t    kind; /**<  */
    uint8_t             pad0[3]; /**<  */
} xcb_xfixes_create_region_from_window_request_t;

/** Opcode for xcb_xfixes_create_region_from_gc. */
#define XCB_XFIXES_CREATE_REGION_FROM_GC 8

/**
 * @brief xcb_xfixes_create_region_from_gc_request_t
 **/
typedef struct xcb_xfixes_create_region_from_gc_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
    xcb_gcontext_t      gc; /**<  */
} xcb_xfixes_create_region_from_gc_request_t;

/** Opcode for xcb_xfixes_create_region_from_picture. */
#define XCB_XFIXES_CREATE_REGION_FROM_PICTURE 9

/**
 * @brief xcb_xfixes_create_region_from_picture_request_t
 **/
typedef struct xcb_xfixes_create_region_from_picture_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_xfixes_region_t  region; /**<  */
    xcb_render_picture_t picture; /**<  */
} xcb_xfixes_create_region_from_picture_request_t;

/** Opcode for xcb_xfixes_destroy_region. */
#define XCB_XFIXES_DESTROY_REGION 10

/**
 * @brief xcb_xfixes_destroy_region_request_t
 **/
typedef struct xcb_xfixes_destroy_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
} xcb_xfixes_destroy_region_request_t;

/** Opcode for xcb_xfixes_set_region. */
#define XCB_XFIXES_SET_REGION 11

/**
 * @brief xcb_xfixes_set_region_request_t
 **/
typedef struct xcb_xfixes_set_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
} xcb_xfixes_set_region_request_t;

/** Opcode for xcb_xfixes_copy_region. */
#define XCB_XFIXES_COPY_REGION 12

/**
 * @brief xcb_xfixes_copy_region_request_t
 **/
typedef struct xcb_xfixes_copy_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source; /**<  */
    xcb_xfixes_region_t destination; /**<  */
} xcb_xfixes_copy_region_request_t;

/** Opcode for xcb_xfixes_union_region. */
#define XCB_XFIXES_UNION_REGION 13

/**
 * @brief xcb_xfixes_union_region_request_t
 **/
typedef struct xcb_xfixes_union_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source1; /**<  */
    xcb_xfixes_region_t source2; /**<  */
    xcb_xfixes_region_t destination; /**<  */
} xcb_xfixes_union_region_request_t;

/** Opcode for xcb_xfixes_intersect_region. */
#define XCB_XFIXES_INTERSECT_REGION 14

/**
 * @brief xcb_xfixes_intersect_region_request_t
 **/
typedef struct xcb_xfixes_intersect_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source1; /**<  */
    xcb_xfixes_region_t source2; /**<  */
    xcb_xfixes_region_t destination; /**<  */
} xcb_xfixes_intersect_region_request_t;

/** Opcode for xcb_xfixes_subtract_region. */
#define XCB_XFIXES_SUBTRACT_REGION 15

/**
 * @brief xcb_xfixes_subtract_region_request_t
 **/
typedef struct xcb_xfixes_subtract_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source1; /**<  */
    xcb_xfixes_region_t source2; /**<  */
    xcb_xfixes_region_t destination; /**<  */
} xcb_xfixes_subtract_region_request_t;

/** Opcode for xcb_xfixes_invert_region. */
#define XCB_XFIXES_INVERT_REGION 16

/**
 * @brief xcb_xfixes_invert_region_request_t
 **/
typedef struct xcb_xfixes_invert_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source; /**<  */
    xcb_rectangle_t     bounds; /**<  */
    xcb_xfixes_region_t destination; /**<  */
} xcb_xfixes_invert_region_request_t;

/** Opcode for xcb_xfixes_translate_region. */
#define XCB_XFIXES_TRANSLATE_REGION 17

/**
 * @brief xcb_xfixes_translate_region_request_t
 **/
typedef struct xcb_xfixes_translate_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
    int16_t             dx; /**<  */
    int16_t             dy; /**<  */
} xcb_xfixes_translate_region_request_t;

/** Opcode for xcb_xfixes_region_extents. */
#define XCB_XFIXES_REGION_EXTENTS 18

/**
 * @brief xcb_xfixes_region_extents_request_t
 **/
typedef struct xcb_xfixes_region_extents_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source; /**<  */
    xcb_xfixes_region_t destination; /**<  */
} xcb_xfixes_region_extents_request_t;

/**
 * @brief xcb_xfixes_fetch_region_cookie_t
 **/
typedef struct xcb_xfixes_fetch_region_cookie_t {
    unsigned int sequence; /**<  */
} xcb_xfixes_fetch_region_cookie_t;

/** Opcode for xcb_xfixes_fetch_region. */
#define XCB_XFIXES_FETCH_REGION 19

/**
 * @brief xcb_xfixes_fetch_region_request_t
 **/
typedef struct xcb_xfixes_fetch_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t region; /**<  */
} xcb_xfixes_fetch_region_request_t;

/**
 * @brief xcb_xfixes_fetch_region_reply_t
 **/
typedef struct xcb_xfixes_fetch_region_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         pad0; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_rectangle_t extents; /**<  */
    uint8_t         pad1[16]; /**<  */
} xcb_xfixes_fetch_region_reply_t;

/** Opcode for xcb_xfixes_set_gc_clip_region. */
#define XCB_XFIXES_SET_GC_CLIP_REGION 20

/**
 * @brief xcb_xfixes_set_gc_clip_region_request_t
 **/
typedef struct xcb_xfixes_set_gc_clip_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_gcontext_t      gc; /**<  */
    xcb_xfixes_region_t region; /**<  */
    int16_t             x_origin; /**<  */
    int16_t             y_origin; /**<  */
} xcb_xfixes_set_gc_clip_region_request_t;

/** Opcode for xcb_xfixes_set_window_shape_region. */
#define XCB_XFIXES_SET_WINDOW_SHAPE_REGION 21

/**
 * @brief xcb_xfixes_set_window_shape_region_request_t
 **/
typedef struct xcb_xfixes_set_window_shape_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_window_t        dest; /**<  */
    xcb_shape_kind_t    dest_kind; /**<  */
    uint8_t             pad0[3]; /**<  */
    int16_t             x_offset; /**<  */
    int16_t             y_offset; /**<  */
    xcb_xfixes_region_t region; /**<  */
} xcb_xfixes_set_window_shape_region_request_t;

/** Opcode for xcb_xfixes_set_picture_clip_region. */
#define XCB_XFIXES_SET_PICTURE_CLIP_REGION 22

/**
 * @brief xcb_xfixes_set_picture_clip_region_request_t
 **/
typedef struct xcb_xfixes_set_picture_clip_region_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
    xcb_xfixes_region_t  region; /**<  */
    int16_t              x_origin; /**<  */
    int16_t              y_origin; /**<  */
} xcb_xfixes_set_picture_clip_region_request_t;

/** Opcode for xcb_xfixes_set_cursor_name. */
#define XCB_XFIXES_SET_CURSOR_NAME 23

/**
 * @brief xcb_xfixes_set_cursor_name_request_t
 **/
typedef struct xcb_xfixes_set_cursor_name_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_cursor_t cursor; /**<  */
    uint16_t     nbytes; /**<  */
    uint8_t      pad0[2]; /**<  */
} xcb_xfixes_set_cursor_name_request_t;

/**
 * @brief xcb_xfixes_get_cursor_name_cookie_t
 **/
typedef struct xcb_xfixes_get_cursor_name_cookie_t {
    unsigned int sequence; /**<  */
} xcb_xfixes_get_cursor_name_cookie_t;

/** Opcode for xcb_xfixes_get_cursor_name. */
#define XCB_XFIXES_GET_CURSOR_NAME 24

/**
 * @brief xcb_xfixes_get_cursor_name_request_t
 **/
typedef struct xcb_xfixes_get_cursor_name_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_cursor_t cursor; /**<  */
} xcb_xfixes_get_cursor_name_request_t;

/**
 * @brief xcb_xfixes_get_cursor_name_reply_t
 **/
typedef struct xcb_xfixes_get_cursor_name_reply_t {
    uint8_t    response_type; /**<  */
    uint8_t    pad0; /**<  */
    uint16_t   sequence; /**<  */
    uint32_t   length; /**<  */
    xcb_atom_t atom; /**<  */
    uint16_t   nbytes; /**<  */
    uint8_t    pad1[18]; /**<  */
} xcb_xfixes_get_cursor_name_reply_t;

/**
 * @brief xcb_xfixes_get_cursor_image_and_name_cookie_t
 **/
typedef struct xcb_xfixes_get_cursor_image_and_name_cookie_t {
    unsigned int sequence; /**<  */
} xcb_xfixes_get_cursor_image_and_name_cookie_t;

/** Opcode for xcb_xfixes_get_cursor_image_and_name. */
#define XCB_XFIXES_GET_CURSOR_IMAGE_AND_NAME 25

/**
 * @brief xcb_xfixes_get_cursor_image_and_name_request_t
 **/
typedef struct xcb_xfixes_get_cursor_image_and_name_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
} xcb_xfixes_get_cursor_image_and_name_request_t;

/**
 * @brief xcb_xfixes_get_cursor_image_and_name_reply_t
 **/
typedef struct xcb_xfixes_get_cursor_image_and_name_reply_t {
    uint8_t    response_type; /**<  */
    uint8_t    pad0; /**<  */
    uint16_t   sequence; /**<  */
    uint32_t   length; /**<  */
    int16_t    x; /**<  */
    int16_t    y; /**<  */
    uint16_t   width; /**<  */
    uint16_t   height; /**<  */
    uint16_t   xhot; /**<  */
    uint16_t   yhot; /**<  */
    uint32_t   cursor_serial; /**<  */
    xcb_atom_t cursor_atom; /**<  */
    uint16_t   nbytes; /**<  */
    uint8_t    pad1[2]; /**<  */
} xcb_xfixes_get_cursor_image_and_name_reply_t;

/** Opcode for xcb_xfixes_change_cursor. */
#define XCB_XFIXES_CHANGE_CURSOR 26

/**
 * @brief xcb_xfixes_change_cursor_request_t
 **/
typedef struct xcb_xfixes_change_cursor_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_cursor_t source; /**<  */
    xcb_cursor_t destination; /**<  */
} xcb_xfixes_change_cursor_request_t;

/** Opcode for xcb_xfixes_change_cursor_by_name. */
#define XCB_XFIXES_CHANGE_CURSOR_BY_NAME 27

/**
 * @brief xcb_xfixes_change_cursor_by_name_request_t
 **/
typedef struct xcb_xfixes_change_cursor_by_name_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_cursor_t src; /**<  */
    uint16_t     nbytes; /**<  */
    uint8_t      pad0[2]; /**<  */
} xcb_xfixes_change_cursor_by_name_request_t;

/** Opcode for xcb_xfixes_expand_region. */
#define XCB_XFIXES_EXPAND_REGION 28

/**
 * @brief xcb_xfixes_expand_region_request_t
 **/
typedef struct xcb_xfixes_expand_region_request_t {
    uint8_t             major_opcode; /**<  */
    uint8_t             minor_opcode; /**<  */
    uint16_t            length; /**<  */
    xcb_xfixes_region_t source; /**<  */
    xcb_xfixes_region_t destination; /**<  */
    uint16_t            left; /**<  */
    uint16_t            right; /**<  */
    uint16_t            top; /**<  */
    uint16_t            bottom; /**<  */
} xcb_xfixes_expand_region_request_t;

/** Opcode for xcb_xfixes_hide_cursor. */
#define XCB_XFIXES_HIDE_CURSOR 29

/**
 * @brief xcb_xfixes_hide_cursor_request_t
 **/
typedef struct xcb_xfixes_hide_cursor_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_xfixes_hide_cursor_request_t;

/** Opcode for xcb_xfixes_show_cursor. */
#define XCB_XFIXES_SHOW_CURSOR 30

/**
 * @brief xcb_xfixes_show_cursor_request_t
 **/
typedef struct xcb_xfixes_show_cursor_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_xfixes_show_cursor_request_t;

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                          uint32_t          client_minor_version  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

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
                                    uint32_t          client_minor_version  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_xfixes_query_version_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

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
                                xcb_generic_error_t               **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                    xcb_window_t      window  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                            xcb_window_t      window  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                           uint32_t          event_mask  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                   uint32_t          event_mask  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                        uint32_t          event_mask  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                uint32_t          event_mask  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_cookie_t xcb_xfixes_get_cursor_image
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_cookie_t
xcb_xfixes_get_cursor_image (xcb_connection_t *c  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_cookie_t xcb_xfixes_get_cursor_image_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_cookie_t
xcb_xfixes_get_cursor_image_unchecked (xcb_connection_t *c  /**< */);


/*****************************************************************************
 **
 ** uint32_t * xcb_xfixes_get_cursor_image_cursor_image
 ** 
 ** @param const xcb_xfixes_get_cursor_image_reply_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_xfixes_get_cursor_image_cursor_image (const xcb_xfixes_get_cursor_image_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_image_cursor_image_length
 ** 
 ** @param const xcb_xfixes_get_cursor_image_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_image_cursor_image_length (const xcb_xfixes_get_cursor_image_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_image_cursor_image_end
 ** 
 ** @param const xcb_xfixes_get_cursor_image_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_image_cursor_image_end (const xcb_xfixes_get_cursor_image_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_xfixes_get_cursor_image_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

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
                                   xcb_generic_error_t                  **e  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_xfixes_region_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_xfixes_region_t)
 */

/*****************************************************************************
 **
 ** void xcb_xfixes_region_next
 ** 
 ** @param xcb_xfixes_region_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xfixes_region_next (xcb_xfixes_region_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_xfixes_region_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_region_end
 ** 
 ** @param xcb_xfixes_region_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_region_end (xcb_xfixes_region_iterator_t i  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                  const xcb_rectangle_t *rectangles  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                          const xcb_rectangle_t *rectangles  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                              xcb_pixmap_t         bitmap  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                      xcb_pixmap_t         bitmap  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                              xcb_shape_kind_t     kind  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                      xcb_shape_kind_t     kind  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                          xcb_gcontext_t       gc  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                  xcb_gcontext_t       gc  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                               xcb_render_picture_t  picture  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                       xcb_render_picture_t  picture  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                   xcb_xfixes_region_t  region  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                           xcb_xfixes_region_t  region  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                               const xcb_rectangle_t *rectangles  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                       const xcb_rectangle_t *rectangles  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                        xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                 xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                         xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                     xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                             xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                    xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                            xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                  xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                          xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                     int16_t              dy  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                             int16_t              dy  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                   xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                           xcb_xfixes_region_t  destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                         xcb_xfixes_region_t  region  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

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
                                   xcb_xfixes_region_t  region  /**< */);


/*****************************************************************************
 **
 ** xcb_rectangle_t * xcb_xfixes_fetch_region_rectangles
 ** 
 ** @param const xcb_xfixes_fetch_region_reply_t *R
 ** @returns xcb_rectangle_t *
 **
 *****************************************************************************/
 
xcb_rectangle_t *
xcb_xfixes_fetch_region_rectangles (const xcb_xfixes_fetch_region_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_xfixes_fetch_region_rectangles_length
 ** 
 ** @param const xcb_xfixes_fetch_region_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_fetch_region_rectangles_length (const xcb_xfixes_fetch_region_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_rectangle_iterator_t xcb_xfixes_fetch_region_rectangles_iterator
 ** 
 ** @param const xcb_xfixes_fetch_region_reply_t *R
 ** @returns xcb_rectangle_iterator_t
 **
 *****************************************************************************/
 
xcb_rectangle_iterator_t
xcb_xfixes_fetch_region_rectangles_iterator (const xcb_xfixes_fetch_region_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_xfixes_fetch_region_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

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
                               xcb_generic_error_t              **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                       int16_t              y_origin  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                               int16_t              y_origin  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                            xcb_xfixes_region_t  region  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                    xcb_xfixes_region_t  region  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                            int16_t               y_origin  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                    int16_t               y_origin  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                    const char       *name  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                            const char       *name  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                            xcb_cursor_t      cursor  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

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
                                      xcb_cursor_t      cursor  /**< */);


/*****************************************************************************
 **
 ** char * xcb_xfixes_get_cursor_name_name
 ** 
 ** @param const xcb_xfixes_get_cursor_name_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_xfixes_get_cursor_name_name (const xcb_xfixes_get_cursor_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_name_name_length
 ** 
 ** @param const xcb_xfixes_get_cursor_name_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_name_name_length (const xcb_xfixes_get_cursor_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_name_name_end
 ** 
 ** @param const xcb_xfixes_get_cursor_name_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_name_name_end (const xcb_xfixes_get_cursor_name_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_xfixes_get_cursor_name_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

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
                                  xcb_generic_error_t                 **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_and_name_cookie_t xcb_xfixes_get_cursor_image_and_name
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_and_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_and_name_cookie_t
xcb_xfixes_get_cursor_image_and_name (xcb_connection_t *c  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_xfixes_get_cursor_image_and_name_cookie_t xcb_xfixes_get_cursor_image_and_name_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_xfixes_get_cursor_image_and_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xfixes_get_cursor_image_and_name_cookie_t
xcb_xfixes_get_cursor_image_and_name_unchecked (xcb_connection_t *c  /**< */);


/*****************************************************************************
 **
 ** char * xcb_xfixes_get_cursor_image_and_name_name
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_xfixes_get_cursor_image_and_name_name (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_image_and_name_name_length
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_image_and_name_name_length (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_image_and_name_name_end
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_image_and_name_name_end (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint32_t * xcb_xfixes_get_cursor_image_and_name_cursor_image
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_xfixes_get_cursor_image_and_name_cursor_image (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_xfixes_get_cursor_image_and_name_cursor_image_length
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xfixes_get_cursor_image_and_name_cursor_image_length (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xfixes_get_cursor_image_and_name_cursor_image_end
 ** 
 ** @param const xcb_xfixes_get_cursor_image_and_name_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xfixes_get_cursor_image_and_name_cursor_image_end (const xcb_xfixes_get_cursor_image_and_name_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_xfixes_get_cursor_image_and_name_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

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
                                            xcb_generic_error_t                           **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                  xcb_cursor_t      destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                          xcb_cursor_t      destination  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                          const char       *name  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                                  const char       *name  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                  uint16_t             bottom  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                          uint16_t             bottom  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                xcb_window_t      window  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                        xcb_window_t      window  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

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
                                xcb_window_t      window  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

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
                        xcb_window_t      window  /**< */);


#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
