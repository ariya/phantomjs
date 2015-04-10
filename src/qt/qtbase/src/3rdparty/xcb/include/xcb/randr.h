/*
 * This file generated automatically from randr.xml by c_client.py.
 * Edit at your peril.
 */

/**
 * @defgroup XCB_RandR_API XCB RandR API
 * @brief RandR XCB Protocol Implementation.
 * @{
 **/

#ifndef __RANDR_H
#define __RANDR_H

#include "xcb.h"
#include "xproto.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XCB_RANDR_MAJOR_VERSION 1
#define XCB_RANDR_MINOR_VERSION 3
  
extern xcb_extension_t xcb_randr_id;

typedef uint32_t xcb_randr_mode_t;

/**
 * @brief xcb_randr_mode_iterator_t
 **/
typedef struct xcb_randr_mode_iterator_t {
    xcb_randr_mode_t *data; /**<  */
    int               rem; /**<  */
    int               index; /**<  */
} xcb_randr_mode_iterator_t;

typedef uint32_t xcb_randr_crtc_t;

/**
 * @brief xcb_randr_crtc_iterator_t
 **/
typedef struct xcb_randr_crtc_iterator_t {
    xcb_randr_crtc_t *data; /**<  */
    int               rem; /**<  */
    int               index; /**<  */
} xcb_randr_crtc_iterator_t;

typedef uint32_t xcb_randr_output_t;

/**
 * @brief xcb_randr_output_iterator_t
 **/
typedef struct xcb_randr_output_iterator_t {
    xcb_randr_output_t *data; /**<  */
    int                 rem; /**<  */
    int                 index; /**<  */
} xcb_randr_output_iterator_t;

/** Opcode for xcb_randr_bad_output. */
#define XCB_RANDR_BAD_OUTPUT 0

/**
 * @brief xcb_randr_bad_output_error_t
 **/
typedef struct xcb_randr_bad_output_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_randr_bad_output_error_t;

/** Opcode for xcb_randr_bad_crtc. */
#define XCB_RANDR_BAD_CRTC 1

/**
 * @brief xcb_randr_bad_crtc_error_t
 **/
typedef struct xcb_randr_bad_crtc_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_randr_bad_crtc_error_t;

/** Opcode for xcb_randr_bad_mode. */
#define XCB_RANDR_BAD_MODE 2

/**
 * @brief xcb_randr_bad_mode_error_t
 **/
typedef struct xcb_randr_bad_mode_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_randr_bad_mode_error_t;

typedef enum xcb_randr_rotation_t {
    XCB_RANDR_ROTATION_ROTATE_0 = 1,
    XCB_RANDR_ROTATION_ROTATE_90 = 2,
    XCB_RANDR_ROTATION_ROTATE_180 = 4,
    XCB_RANDR_ROTATION_ROTATE_270 = 8,
    XCB_RANDR_ROTATION_REFLECT_X = 16,
    XCB_RANDR_ROTATION_REFLECT_Y = 32
} xcb_randr_rotation_t;

/**
 * @brief xcb_randr_screen_size_t
 **/
typedef struct xcb_randr_screen_size_t {
    uint16_t width; /**<  */
    uint16_t height; /**<  */
    uint16_t mwidth; /**<  */
    uint16_t mheight; /**<  */
} xcb_randr_screen_size_t;

/**
 * @brief xcb_randr_screen_size_iterator_t
 **/
typedef struct xcb_randr_screen_size_iterator_t {
    xcb_randr_screen_size_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_randr_screen_size_iterator_t;

/**
 * @brief xcb_randr_refresh_rates_t
 **/
typedef struct xcb_randr_refresh_rates_t {
    uint16_t nRates; /**<  */
} xcb_randr_refresh_rates_t;

/**
 * @brief xcb_randr_refresh_rates_iterator_t
 **/
typedef struct xcb_randr_refresh_rates_iterator_t {
    xcb_randr_refresh_rates_t *data; /**<  */
    int                        rem; /**<  */
    int                        index; /**<  */
} xcb_randr_refresh_rates_iterator_t;

/**
 * @brief xcb_randr_query_version_cookie_t
 **/
typedef struct xcb_randr_query_version_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_query_version_cookie_t;

/** Opcode for xcb_randr_query_version. */
#define XCB_RANDR_QUERY_VERSION 0

/**
 * @brief xcb_randr_query_version_request_t
 **/
typedef struct xcb_randr_query_version_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint32_t major_version; /**<  */
    uint32_t minor_version; /**<  */
} xcb_randr_query_version_request_t;

/**
 * @brief xcb_randr_query_version_reply_t
 **/
typedef struct xcb_randr_query_version_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t major_version; /**<  */
    uint32_t minor_version; /**<  */
    uint8_t  pad1[16]; /**<  */
} xcb_randr_query_version_reply_t;

typedef enum xcb_randr_set_config_t {
    XCB_RANDR_SET_CONFIG_SUCCESS = 0,
    XCB_RANDR_SET_CONFIG_INVALID_CONFIG_TIME = 1,
    XCB_RANDR_SET_CONFIG_INVALID_TIME = 2,
    XCB_RANDR_SET_CONFIG_FAILED = 3
} xcb_randr_set_config_t;

/**
 * @brief xcb_randr_set_screen_config_cookie_t
 **/
typedef struct xcb_randr_set_screen_config_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_set_screen_config_cookie_t;

/** Opcode for xcb_randr_set_screen_config. */
#define XCB_RANDR_SET_SCREEN_CONFIG 2

/**
 * @brief xcb_randr_set_screen_config_request_t
 **/
typedef struct xcb_randr_set_screen_config_request_t {
    uint8_t         major_opcode; /**<  */
    uint8_t         minor_opcode; /**<  */
    uint16_t        length; /**<  */
    xcb_window_t    window; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_timestamp_t config_timestamp; /**<  */
    uint16_t        sizeID; /**<  */
    uint16_t        rotation; /**<  */
    uint16_t        rate; /**<  */
    uint8_t         pad0[2]; /**<  */
} xcb_randr_set_screen_config_request_t;

/**
 * @brief xcb_randr_set_screen_config_reply_t
 **/
typedef struct xcb_randr_set_screen_config_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         status; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_timestamp_t new_timestamp; /**<  */
    xcb_timestamp_t config_timestamp; /**<  */
    xcb_window_t    root; /**<  */
    uint16_t        subpixel_order; /**<  */
    uint8_t         pad0[10]; /**<  */
} xcb_randr_set_screen_config_reply_t;

typedef enum xcb_randr_notify_mask_t {
    XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE = 1,
    XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE = 2,
    XCB_RANDR_NOTIFY_MASK_OUTPUT_CHANGE = 4,
    XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY = 8
} xcb_randr_notify_mask_t;

/** Opcode for xcb_randr_select_input. */
#define XCB_RANDR_SELECT_INPUT 4

/**
 * @brief xcb_randr_select_input_request_t
 **/
typedef struct xcb_randr_select_input_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
    uint16_t     enable; /**<  */
    uint8_t      pad0[2]; /**<  */
} xcb_randr_select_input_request_t;

/**
 * @brief xcb_randr_get_screen_info_cookie_t
 **/
typedef struct xcb_randr_get_screen_info_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_screen_info_cookie_t;

/** Opcode for xcb_randr_get_screen_info. */
#define XCB_RANDR_GET_SCREEN_INFO 5

/**
 * @brief xcb_randr_get_screen_info_request_t
 **/
typedef struct xcb_randr_get_screen_info_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_randr_get_screen_info_request_t;

/**
 * @brief xcb_randr_get_screen_info_reply_t
 **/
typedef struct xcb_randr_get_screen_info_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         rotations; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_window_t    root; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_timestamp_t config_timestamp; /**<  */
    uint16_t        nSizes; /**<  */
    uint16_t        sizeID; /**<  */
    uint16_t        rotation; /**<  */
    uint16_t        rate; /**<  */
    uint16_t        nInfo; /**<  */
    uint8_t         pad0[2]; /**<  */
} xcb_randr_get_screen_info_reply_t;

/**
 * @brief xcb_randr_get_screen_size_range_cookie_t
 **/
typedef struct xcb_randr_get_screen_size_range_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_screen_size_range_cookie_t;

/** Opcode for xcb_randr_get_screen_size_range. */
#define XCB_RANDR_GET_SCREEN_SIZE_RANGE 6

/**
 * @brief xcb_randr_get_screen_size_range_request_t
 **/
typedef struct xcb_randr_get_screen_size_range_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_randr_get_screen_size_range_request_t;

/**
 * @brief xcb_randr_get_screen_size_range_reply_t
 **/
typedef struct xcb_randr_get_screen_size_range_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint16_t min_width; /**<  */
    uint16_t min_height; /**<  */
    uint16_t max_width; /**<  */
    uint16_t max_height; /**<  */
    uint8_t  pad1[16]; /**<  */
} xcb_randr_get_screen_size_range_reply_t;

/** Opcode for xcb_randr_set_screen_size. */
#define XCB_RANDR_SET_SCREEN_SIZE 7

/**
 * @brief xcb_randr_set_screen_size_request_t
 **/
typedef struct xcb_randr_set_screen_size_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
    uint16_t     width; /**<  */
    uint16_t     height; /**<  */
    uint32_t     mm_width; /**<  */
    uint32_t     mm_height; /**<  */
} xcb_randr_set_screen_size_request_t;

typedef enum xcb_randr_mode_flag_t {
    XCB_RANDR_MODE_FLAG_HSYNC_POSITIVE = 1,
    XCB_RANDR_MODE_FLAG_HSYNC_NEGATIVE = 2,
    XCB_RANDR_MODE_FLAG_VSYNC_POSITIVE = 4,
    XCB_RANDR_MODE_FLAG_VSYNC_NEGATIVE = 8,
    XCB_RANDR_MODE_FLAG_INTERLACE = 16,
    XCB_RANDR_MODE_FLAG_DOUBLE_SCAN = 32,
    XCB_RANDR_MODE_FLAG_CSYNC = 64,
    XCB_RANDR_MODE_FLAG_CSYNC_POSITIVE = 128,
    XCB_RANDR_MODE_FLAG_CSYNC_NEGATIVE = 256,
    XCB_RANDR_MODE_FLAG_HSKEW_PRESENT = 512,
    XCB_RANDR_MODE_FLAG_BCAST = 1024,
    XCB_RANDR_MODE_FLAG_PIXEL_MULTIPLEX = 2048,
    XCB_RANDR_MODE_FLAG_DOUBLE_CLOCK = 4096,
    XCB_RANDR_MODE_FLAG_HALVE_CLOCK = 8192
} xcb_randr_mode_flag_t;

/**
 * @brief xcb_randr_mode_info_t
 **/
typedef struct xcb_randr_mode_info_t {
    uint32_t id; /**<  */
    uint16_t width; /**<  */
    uint16_t height; /**<  */
    uint32_t dot_clock; /**<  */
    uint16_t hsync_start; /**<  */
    uint16_t hsync_end; /**<  */
    uint16_t htotal; /**<  */
    uint16_t hskew; /**<  */
    uint16_t vsync_start; /**<  */
    uint16_t vsync_end; /**<  */
    uint16_t vtotal; /**<  */
    uint16_t name_len; /**<  */
    uint32_t mode_flags; /**<  */
} xcb_randr_mode_info_t;

/**
 * @brief xcb_randr_mode_info_iterator_t
 **/
typedef struct xcb_randr_mode_info_iterator_t {
    xcb_randr_mode_info_t *data; /**<  */
    int                    rem; /**<  */
    int                    index; /**<  */
} xcb_randr_mode_info_iterator_t;

/**
 * @brief xcb_randr_get_screen_resources_cookie_t
 **/
typedef struct xcb_randr_get_screen_resources_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_screen_resources_cookie_t;

/** Opcode for xcb_randr_get_screen_resources. */
#define XCB_RANDR_GET_SCREEN_RESOURCES 8

/**
 * @brief xcb_randr_get_screen_resources_request_t
 **/
typedef struct xcb_randr_get_screen_resources_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_randr_get_screen_resources_request_t;

/**
 * @brief xcb_randr_get_screen_resources_reply_t
 **/
typedef struct xcb_randr_get_screen_resources_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         pad0; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_timestamp_t config_timestamp; /**<  */
    uint16_t        num_crtcs; /**<  */
    uint16_t        num_outputs; /**<  */
    uint16_t        num_modes; /**<  */
    uint16_t        names_len; /**<  */
    uint8_t         pad1[8]; /**<  */
} xcb_randr_get_screen_resources_reply_t;

typedef enum xcb_randr_connection_t {
    XCB_RANDR_CONNECTION_CONNECTED,
    XCB_RANDR_CONNECTION_DISCONNECTED,
    XCB_RANDR_CONNECTION_UNKNOWN
} xcb_randr_connection_t;

/**
 * @brief xcb_randr_get_output_info_cookie_t
 **/
typedef struct xcb_randr_get_output_info_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_output_info_cookie_t;

/** Opcode for xcb_randr_get_output_info. */
#define XCB_RANDR_GET_OUTPUT_INFO 9

/**
 * @brief xcb_randr_get_output_info_request_t
 **/
typedef struct xcb_randr_get_output_info_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_timestamp_t    config_timestamp; /**<  */
} xcb_randr_get_output_info_request_t;

/**
 * @brief xcb_randr_get_output_info_reply_t
 **/
typedef struct xcb_randr_get_output_info_reply_t {
    uint8_t          response_type; /**<  */
    uint8_t          status; /**<  */
    uint16_t         sequence; /**<  */
    uint32_t         length; /**<  */
    xcb_timestamp_t  timestamp; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
    uint32_t         mm_width; /**<  */
    uint32_t         mm_height; /**<  */
    uint8_t          connection; /**<  */
    uint8_t          subpixel_order; /**<  */
    uint16_t         num_crtcs; /**<  */
    uint16_t         num_modes; /**<  */
    uint16_t         num_preferred; /**<  */
    uint16_t         num_clones; /**<  */
    uint16_t         name_len; /**<  */
} xcb_randr_get_output_info_reply_t;

/**
 * @brief xcb_randr_list_output_properties_cookie_t
 **/
typedef struct xcb_randr_list_output_properties_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_list_output_properties_cookie_t;

/** Opcode for xcb_randr_list_output_properties. */
#define XCB_RANDR_LIST_OUTPUT_PROPERTIES 10

/**
 * @brief xcb_randr_list_output_properties_request_t
 **/
typedef struct xcb_randr_list_output_properties_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
} xcb_randr_list_output_properties_request_t;

/**
 * @brief xcb_randr_list_output_properties_reply_t
 **/
typedef struct xcb_randr_list_output_properties_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint16_t num_atoms; /**<  */
    uint8_t  pad1[22]; /**<  */
} xcb_randr_list_output_properties_reply_t;

/**
 * @brief xcb_randr_query_output_property_cookie_t
 **/
typedef struct xcb_randr_query_output_property_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_query_output_property_cookie_t;

/** Opcode for xcb_randr_query_output_property. */
#define XCB_RANDR_QUERY_OUTPUT_PROPERTY 11

/**
 * @brief xcb_randr_query_output_property_request_t
 **/
typedef struct xcb_randr_query_output_property_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_atom_t         property; /**<  */
} xcb_randr_query_output_property_request_t;

/**
 * @brief xcb_randr_query_output_property_reply_t
 **/
typedef struct xcb_randr_query_output_property_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint8_t  pending; /**<  */
    uint8_t  range; /**<  */
    uint8_t  immutable; /**<  */
    uint8_t  pad1[21]; /**<  */
} xcb_randr_query_output_property_reply_t;

/** Opcode for xcb_randr_configure_output_property. */
#define XCB_RANDR_CONFIGURE_OUTPUT_PROPERTY 12

/**
 * @brief xcb_randr_configure_output_property_request_t
 **/
typedef struct xcb_randr_configure_output_property_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_atom_t         property; /**<  */
    uint8_t            pending; /**<  */
    uint8_t            range; /**<  */
    uint8_t            pad0[2]; /**<  */
} xcb_randr_configure_output_property_request_t;

/** Opcode for xcb_randr_change_output_property. */
#define XCB_RANDR_CHANGE_OUTPUT_PROPERTY 13

/**
 * @brief xcb_randr_change_output_property_request_t
 **/
typedef struct xcb_randr_change_output_property_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_atom_t         property; /**<  */
    xcb_atom_t         type; /**<  */
    uint8_t            format; /**<  */
    uint8_t            mode; /**<  */
    uint8_t            pad0[2]; /**<  */
    uint32_t           num_units; /**<  */
} xcb_randr_change_output_property_request_t;

/** Opcode for xcb_randr_delete_output_property. */
#define XCB_RANDR_DELETE_OUTPUT_PROPERTY 14

/**
 * @brief xcb_randr_delete_output_property_request_t
 **/
typedef struct xcb_randr_delete_output_property_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_atom_t         property; /**<  */
} xcb_randr_delete_output_property_request_t;

/**
 * @brief xcb_randr_get_output_property_cookie_t
 **/
typedef struct xcb_randr_get_output_property_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_output_property_cookie_t;

/** Opcode for xcb_randr_get_output_property. */
#define XCB_RANDR_GET_OUTPUT_PROPERTY 15

/**
 * @brief xcb_randr_get_output_property_request_t
 **/
typedef struct xcb_randr_get_output_property_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_atom_t         property; /**<  */
    xcb_atom_t         type; /**<  */
    uint32_t           long_offset; /**<  */
    uint32_t           long_length; /**<  */
    uint8_t            _delete; /**<  */
    uint8_t            pending; /**<  */
    uint8_t            pad0[2]; /**<  */
} xcb_randr_get_output_property_request_t;

/**
 * @brief xcb_randr_get_output_property_reply_t
 **/
typedef struct xcb_randr_get_output_property_reply_t {
    uint8_t    response_type; /**<  */
    uint8_t    format; /**<  */
    uint16_t   sequence; /**<  */
    uint32_t   length; /**<  */
    xcb_atom_t type; /**<  */
    uint32_t   bytes_after; /**<  */
    uint32_t   num_items; /**<  */
    uint8_t    pad0[12]; /**<  */
} xcb_randr_get_output_property_reply_t;

/**
 * @brief xcb_randr_create_mode_cookie_t
 **/
typedef struct xcb_randr_create_mode_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_create_mode_cookie_t;

/** Opcode for xcb_randr_create_mode. */
#define XCB_RANDR_CREATE_MODE 16

/**
 * @brief xcb_randr_create_mode_request_t
 **/
typedef struct xcb_randr_create_mode_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_window_t          window; /**<  */
    xcb_randr_mode_info_t mode_info; /**<  */
} xcb_randr_create_mode_request_t;

/**
 * @brief xcb_randr_create_mode_reply_t
 **/
typedef struct xcb_randr_create_mode_reply_t {
    uint8_t          response_type; /**<  */
    uint8_t          pad0; /**<  */
    uint16_t         sequence; /**<  */
    uint32_t         length; /**<  */
    xcb_randr_mode_t mode; /**<  */
    uint8_t          pad1[20]; /**<  */
} xcb_randr_create_mode_reply_t;

/** Opcode for xcb_randr_destroy_mode. */
#define XCB_RANDR_DESTROY_MODE 17

/**
 * @brief xcb_randr_destroy_mode_request_t
 **/
typedef struct xcb_randr_destroy_mode_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_mode_t mode; /**<  */
} xcb_randr_destroy_mode_request_t;

/** Opcode for xcb_randr_add_output_mode. */
#define XCB_RANDR_ADD_OUTPUT_MODE 18

/**
 * @brief xcb_randr_add_output_mode_request_t
 **/
typedef struct xcb_randr_add_output_mode_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_randr_mode_t   mode; /**<  */
} xcb_randr_add_output_mode_request_t;

/** Opcode for xcb_randr_delete_output_mode. */
#define XCB_RANDR_DELETE_OUTPUT_MODE 19

/**
 * @brief xcb_randr_delete_output_mode_request_t
 **/
typedef struct xcb_randr_delete_output_mode_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_randr_mode_t   mode; /**<  */
} xcb_randr_delete_output_mode_request_t;

/**
 * @brief xcb_randr_get_crtc_info_cookie_t
 **/
typedef struct xcb_randr_get_crtc_info_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_crtc_info_cookie_t;

/** Opcode for xcb_randr_get_crtc_info. */
#define XCB_RANDR_GET_CRTC_INFO 20

/**
 * @brief xcb_randr_get_crtc_info_request_t
 **/
typedef struct xcb_randr_get_crtc_info_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
    xcb_timestamp_t  config_timestamp; /**<  */
} xcb_randr_get_crtc_info_request_t;

/**
 * @brief xcb_randr_get_crtc_info_reply_t
 **/
typedef struct xcb_randr_get_crtc_info_reply_t {
    uint8_t          response_type; /**<  */
    uint8_t          status; /**<  */
    uint16_t         sequence; /**<  */
    uint32_t         length; /**<  */
    xcb_timestamp_t  timestamp; /**<  */
    int16_t          x; /**<  */
    int16_t          y; /**<  */
    uint16_t         width; /**<  */
    uint16_t         height; /**<  */
    xcb_randr_mode_t mode; /**<  */
    uint16_t         rotation; /**<  */
    uint16_t         rotations; /**<  */
    uint16_t         num_outputs; /**<  */
    uint16_t         num_possible_outputs; /**<  */
} xcb_randr_get_crtc_info_reply_t;

/**
 * @brief xcb_randr_set_crtc_config_cookie_t
 **/
typedef struct xcb_randr_set_crtc_config_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_set_crtc_config_cookie_t;

/** Opcode for xcb_randr_set_crtc_config. */
#define XCB_RANDR_SET_CRTC_CONFIG 21

/**
 * @brief xcb_randr_set_crtc_config_request_t
 **/
typedef struct xcb_randr_set_crtc_config_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
    xcb_timestamp_t  timestamp; /**<  */
    xcb_timestamp_t  config_timestamp; /**<  */
    int16_t          x; /**<  */
    int16_t          y; /**<  */
    xcb_randr_mode_t mode; /**<  */
    uint16_t         rotation; /**<  */
    uint8_t          pad0[2]; /**<  */
} xcb_randr_set_crtc_config_request_t;

/**
 * @brief xcb_randr_set_crtc_config_reply_t
 **/
typedef struct xcb_randr_set_crtc_config_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         status; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    uint8_t         pad0[20]; /**<  */
} xcb_randr_set_crtc_config_reply_t;

/**
 * @brief xcb_randr_get_crtc_gamma_size_cookie_t
 **/
typedef struct xcb_randr_get_crtc_gamma_size_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_crtc_gamma_size_cookie_t;

/** Opcode for xcb_randr_get_crtc_gamma_size. */
#define XCB_RANDR_GET_CRTC_GAMMA_SIZE 22

/**
 * @brief xcb_randr_get_crtc_gamma_size_request_t
 **/
typedef struct xcb_randr_get_crtc_gamma_size_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
} xcb_randr_get_crtc_gamma_size_request_t;

/**
 * @brief xcb_randr_get_crtc_gamma_size_reply_t
 **/
typedef struct xcb_randr_get_crtc_gamma_size_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint16_t size; /**<  */
    uint8_t  pad1[22]; /**<  */
} xcb_randr_get_crtc_gamma_size_reply_t;

/**
 * @brief xcb_randr_get_crtc_gamma_cookie_t
 **/
typedef struct xcb_randr_get_crtc_gamma_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_crtc_gamma_cookie_t;

/** Opcode for xcb_randr_get_crtc_gamma. */
#define XCB_RANDR_GET_CRTC_GAMMA 23

/**
 * @brief xcb_randr_get_crtc_gamma_request_t
 **/
typedef struct xcb_randr_get_crtc_gamma_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
} xcb_randr_get_crtc_gamma_request_t;

/**
 * @brief xcb_randr_get_crtc_gamma_reply_t
 **/
typedef struct xcb_randr_get_crtc_gamma_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint16_t size; /**<  */
    uint8_t  pad1[22]; /**<  */
} xcb_randr_get_crtc_gamma_reply_t;

/** Opcode for xcb_randr_set_crtc_gamma. */
#define XCB_RANDR_SET_CRTC_GAMMA 24

/**
 * @brief xcb_randr_set_crtc_gamma_request_t
 **/
typedef struct xcb_randr_set_crtc_gamma_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
    uint16_t         size; /**<  */
    uint8_t          pad0[2]; /**<  */
} xcb_randr_set_crtc_gamma_request_t;

/**
 * @brief xcb_randr_get_screen_resources_current_cookie_t
 **/
typedef struct xcb_randr_get_screen_resources_current_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_screen_resources_current_cookie_t;

/** Opcode for xcb_randr_get_screen_resources_current. */
#define XCB_RANDR_GET_SCREEN_RESOURCES_CURRENT 25

/**
 * @brief xcb_randr_get_screen_resources_current_request_t
 **/
typedef struct xcb_randr_get_screen_resources_current_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_randr_get_screen_resources_current_request_t;

/**
 * @brief xcb_randr_get_screen_resources_current_reply_t
 **/
typedef struct xcb_randr_get_screen_resources_current_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         pad0; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_timestamp_t config_timestamp; /**<  */
    uint16_t        num_crtcs; /**<  */
    uint16_t        num_outputs; /**<  */
    uint16_t        num_modes; /**<  */
    uint16_t        names_len; /**<  */
    uint8_t         pad1[8]; /**<  */
} xcb_randr_get_screen_resources_current_reply_t;

/** Opcode for xcb_randr_set_crtc_transform. */
#define XCB_RANDR_SET_CRTC_TRANSFORM 26

/**
 * @brief xcb_randr_set_crtc_transform_request_t
 **/
typedef struct xcb_randr_set_crtc_transform_request_t {
    uint8_t                major_opcode; /**<  */
    uint8_t                minor_opcode; /**<  */
    uint16_t               length; /**<  */
    xcb_randr_crtc_t       crtc; /**<  */
    xcb_render_transform_t transform; /**<  */
    uint16_t               filter_len; /**<  */
    uint8_t                pad0[2]; /**<  */
} xcb_randr_set_crtc_transform_request_t;

/**
 * @brief xcb_randr_get_crtc_transform_cookie_t
 **/
typedef struct xcb_randr_get_crtc_transform_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_crtc_transform_cookie_t;

/** Opcode for xcb_randr_get_crtc_transform. */
#define XCB_RANDR_GET_CRTC_TRANSFORM 27

/**
 * @brief xcb_randr_get_crtc_transform_request_t
 **/
typedef struct xcb_randr_get_crtc_transform_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
} xcb_randr_get_crtc_transform_request_t;

/**
 * @brief xcb_randr_get_crtc_transform_reply_t
 **/
typedef struct xcb_randr_get_crtc_transform_reply_t {
    uint8_t                response_type; /**<  */
    uint8_t                pad0; /**<  */
    uint16_t               sequence; /**<  */
    uint32_t               length; /**<  */
    xcb_render_transform_t pending_transform; /**<  */
    uint8_t                has_transforms; /**<  */
    uint8_t                pad1[3]; /**<  */
    xcb_render_transform_t current_transform; /**<  */
    uint8_t                pad2[4]; /**<  */
    uint16_t               pending_len; /**<  */
    uint16_t               pending_nparams; /**<  */
    uint16_t               current_len; /**<  */
    uint16_t               current_nparams; /**<  */
} xcb_randr_get_crtc_transform_reply_t;

/**
 * @brief xcb_randr_get_panning_cookie_t
 **/
typedef struct xcb_randr_get_panning_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_panning_cookie_t;

/** Opcode for xcb_randr_get_panning. */
#define XCB_RANDR_GET_PANNING 28

/**
 * @brief xcb_randr_get_panning_request_t
 **/
typedef struct xcb_randr_get_panning_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
} xcb_randr_get_panning_request_t;

/**
 * @brief xcb_randr_get_panning_reply_t
 **/
typedef struct xcb_randr_get_panning_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         status; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    uint16_t        left; /**<  */
    uint16_t        top; /**<  */
    uint16_t        width; /**<  */
    uint16_t        height; /**<  */
    uint16_t        track_left; /**<  */
    uint16_t        track_top; /**<  */
    uint16_t        track_width; /**<  */
    uint16_t        track_height; /**<  */
    int16_t         border_left; /**<  */
    int16_t         border_top; /**<  */
    int16_t         border_right; /**<  */
    int16_t         border_bottom; /**<  */
} xcb_randr_get_panning_reply_t;

/**
 * @brief xcb_randr_set_panning_cookie_t
 **/
typedef struct xcb_randr_set_panning_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_set_panning_cookie_t;

/** Opcode for xcb_randr_set_panning. */
#define XCB_RANDR_SET_PANNING 29

/**
 * @brief xcb_randr_set_panning_request_t
 **/
typedef struct xcb_randr_set_panning_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
    xcb_timestamp_t  timestamp; /**<  */
    uint16_t         left; /**<  */
    uint16_t         top; /**<  */
    uint16_t         width; /**<  */
    uint16_t         height; /**<  */
    uint16_t         track_left; /**<  */
    uint16_t         track_top; /**<  */
    uint16_t         track_width; /**<  */
    uint16_t         track_height; /**<  */
    int16_t          border_left; /**<  */
    int16_t          border_top; /**<  */
    int16_t          border_right; /**<  */
    int16_t          border_bottom; /**<  */
} xcb_randr_set_panning_request_t;

/**
 * @brief xcb_randr_set_panning_reply_t
 **/
typedef struct xcb_randr_set_panning_reply_t {
    uint8_t         response_type; /**<  */
    uint8_t         status; /**<  */
    uint16_t        sequence; /**<  */
    uint32_t        length; /**<  */
    xcb_timestamp_t timestamp; /**<  */
} xcb_randr_set_panning_reply_t;

/** Opcode for xcb_randr_set_output_primary. */
#define XCB_RANDR_SET_OUTPUT_PRIMARY 30

/**
 * @brief xcb_randr_set_output_primary_request_t
 **/
typedef struct xcb_randr_set_output_primary_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_window_t       window; /**<  */
    xcb_randr_output_t output; /**<  */
} xcb_randr_set_output_primary_request_t;

/**
 * @brief xcb_randr_get_output_primary_cookie_t
 **/
typedef struct xcb_randr_get_output_primary_cookie_t {
    unsigned int sequence; /**<  */
} xcb_randr_get_output_primary_cookie_t;

/** Opcode for xcb_randr_get_output_primary. */
#define XCB_RANDR_GET_OUTPUT_PRIMARY 31

/**
 * @brief xcb_randr_get_output_primary_request_t
 **/
typedef struct xcb_randr_get_output_primary_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_window_t window; /**<  */
} xcb_randr_get_output_primary_request_t;

/**
 * @brief xcb_randr_get_output_primary_reply_t
 **/
typedef struct xcb_randr_get_output_primary_reply_t {
    uint8_t            response_type; /**<  */
    uint8_t            pad0; /**<  */
    uint16_t           sequence; /**<  */
    uint32_t           length; /**<  */
    xcb_randr_output_t output; /**<  */
} xcb_randr_get_output_primary_reply_t;

/** Opcode for xcb_randr_screen_change_notify. */
#define XCB_RANDR_SCREEN_CHANGE_NOTIFY 0

/**
 * @brief xcb_randr_screen_change_notify_event_t
 **/
typedef struct xcb_randr_screen_change_notify_event_t {
    uint8_t         response_type; /**<  */
    uint8_t         rotation; /**<  */
    uint16_t        sequence; /**<  */
    xcb_timestamp_t timestamp; /**<  */
    xcb_timestamp_t config_timestamp; /**<  */
    xcb_window_t    root; /**<  */
    xcb_window_t    request_window; /**<  */
    uint16_t        sizeID; /**<  */
    uint16_t        subpixel_order; /**<  */
    uint16_t        width; /**<  */
    uint16_t        height; /**<  */
    uint16_t        mwidth; /**<  */
    uint16_t        mheight; /**<  */
} xcb_randr_screen_change_notify_event_t;

typedef enum xcb_randr_notify_t {
    XCB_RANDR_NOTIFY_CRTC_CHANGE = 0,
    XCB_RANDR_NOTIFY_OUTPUT_CHANGE = 1,
    XCB_RANDR_NOTIFY_OUTPUT_PROPERTY = 2
} xcb_randr_notify_t;

/**
 * @brief xcb_randr_crtc_change_t
 **/
typedef struct xcb_randr_crtc_change_t {
    xcb_timestamp_t  timestamp; /**<  */
    xcb_window_t     window; /**<  */
    xcb_randr_crtc_t crtc; /**<  */
    xcb_randr_mode_t mode; /**<  */
    uint16_t         rotation; /**<  */
    uint8_t          pad0[2]; /**<  */
    int16_t          x; /**<  */
    int16_t          y; /**<  */
    uint16_t         width; /**<  */
    uint16_t         height; /**<  */
} xcb_randr_crtc_change_t;

/**
 * @brief xcb_randr_crtc_change_iterator_t
 **/
typedef struct xcb_randr_crtc_change_iterator_t {
    xcb_randr_crtc_change_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_randr_crtc_change_iterator_t;

/**
 * @brief xcb_randr_output_change_t
 **/
typedef struct xcb_randr_output_change_t {
    xcb_timestamp_t    timestamp; /**<  */
    xcb_timestamp_t    config_timestamp; /**<  */
    xcb_window_t       window; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_randr_crtc_t   crtc; /**<  */
    xcb_randr_mode_t   mode; /**<  */
    uint16_t           rotation; /**<  */
    uint8_t            connection; /**<  */
    uint8_t            subpixel_order; /**<  */
} xcb_randr_output_change_t;

/**
 * @brief xcb_randr_output_change_iterator_t
 **/
typedef struct xcb_randr_output_change_iterator_t {
    xcb_randr_output_change_t *data; /**<  */
    int                        rem; /**<  */
    int                        index; /**<  */
} xcb_randr_output_change_iterator_t;

/**
 * @brief xcb_randr_output_property_t
 **/
typedef struct xcb_randr_output_property_t {
    xcb_window_t       window; /**<  */
    xcb_randr_output_t output; /**<  */
    xcb_atom_t         atom; /**<  */
    xcb_timestamp_t    timestamp; /**<  */
    uint8_t            status; /**<  */
    uint8_t            pad0[11]; /**<  */
} xcb_randr_output_property_t;

/**
 * @brief xcb_randr_output_property_iterator_t
 **/
typedef struct xcb_randr_output_property_iterator_t {
    xcb_randr_output_property_t *data; /**<  */
    int                          rem; /**<  */
    int                          index; /**<  */
} xcb_randr_output_property_iterator_t;

/**
 * @brief xcb_randr_notify_data_t
 **/
typedef union xcb_randr_notify_data_t {
    xcb_randr_crtc_change_t     cc; /**<  */
    xcb_randr_output_change_t   oc; /**<  */
    xcb_randr_output_property_t op; /**<  */
} xcb_randr_notify_data_t;

/**
 * @brief xcb_randr_notify_data_iterator_t
 **/
typedef struct xcb_randr_notify_data_iterator_t {
    xcb_randr_notify_data_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_randr_notify_data_iterator_t;

/** Opcode for xcb_randr_notify. */
#define XCB_RANDR_NOTIFY 1

/**
 * @brief xcb_randr_notify_event_t
 **/
typedef struct xcb_randr_notify_event_t {
    uint8_t                 response_type; /**<  */
    uint8_t                 subCode; /**<  */
    uint16_t                sequence; /**<  */
    xcb_randr_notify_data_t u; /**<  */
} xcb_randr_notify_event_t;

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_mode_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_mode_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_mode_next
 ** 
 ** @param xcb_randr_mode_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_mode_next (xcb_randr_mode_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_mode_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_mode_end
 ** 
 ** @param xcb_randr_mode_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_mode_end (xcb_randr_mode_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_crtc_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_crtc_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_crtc_next
 ** 
 ** @param xcb_randr_crtc_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_crtc_next (xcb_randr_crtc_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_crtc_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_crtc_end
 ** 
 ** @param xcb_randr_crtc_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_crtc_end (xcb_randr_crtc_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_output_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_output_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_output_next
 ** 
 ** @param xcb_randr_output_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_output_next (xcb_randr_output_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_output_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_output_end
 ** 
 ** @param xcb_randr_output_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_output_end (xcb_randr_output_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_screen_size_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_screen_size_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_screen_size_next
 ** 
 ** @param xcb_randr_screen_size_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_screen_size_next (xcb_randr_screen_size_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_screen_size_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_screen_size_end
 ** 
 ** @param xcb_randr_screen_size_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_screen_size_end (xcb_randr_screen_size_iterator_t i  /**< */);


/*****************************************************************************
 **
 ** uint16_t * xcb_randr_refresh_rates_rates
 ** 
 ** @param const xcb_randr_refresh_rates_t *R
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_randr_refresh_rates_rates (const xcb_randr_refresh_rates_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_refresh_rates_rates_length
 ** 
 ** @param const xcb_randr_refresh_rates_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_refresh_rates_rates_length (const xcb_randr_refresh_rates_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_refresh_rates_rates_end
 ** 
 ** @param const xcb_randr_refresh_rates_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_refresh_rates_rates_end (const xcb_randr_refresh_rates_t *R  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_refresh_rates_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_refresh_rates_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_refresh_rates_next
 ** 
 ** @param xcb_randr_refresh_rates_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_refresh_rates_next (xcb_randr_refresh_rates_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_refresh_rates_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_refresh_rates_end
 ** 
 ** @param xcb_randr_refresh_rates_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_refresh_rates_end (xcb_randr_refresh_rates_iterator_t i  /**< */);

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
 ** xcb_randr_query_version_cookie_t xcb_randr_query_version
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          major_version
 ** @param uint32_t          minor_version
 ** @returns xcb_randr_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_query_version_cookie_t
xcb_randr_query_version (xcb_connection_t *c  /**< */,
                         uint32_t          major_version  /**< */,
                         uint32_t          minor_version  /**< */);

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
 ** xcb_randr_query_version_cookie_t xcb_randr_query_version_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          major_version
 ** @param uint32_t          minor_version
 ** @returns xcb_randr_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_query_version_cookie_t
xcb_randr_query_version_unchecked (xcb_connection_t *c  /**< */,
                                   uint32_t          major_version  /**< */,
                                   uint32_t          minor_version  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_query_version_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_query_version_reply_t * xcb_randr_query_version_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_randr_query_version_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_randr_query_version_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_query_version_reply_t *
xcb_randr_query_version_reply (xcb_connection_t                  *c  /**< */,
                               xcb_randr_query_version_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */);

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
 ** xcb_randr_set_screen_config_cookie_t xcb_randr_set_screen_config
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param xcb_timestamp_t   timestamp
 ** @param xcb_timestamp_t   config_timestamp
 ** @param uint16_t          sizeID
 ** @param uint16_t          rotation
 ** @param uint16_t          rate
 ** @returns xcb_randr_set_screen_config_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_set_screen_config_cookie_t
xcb_randr_set_screen_config (xcb_connection_t *c  /**< */,
                             xcb_window_t      window  /**< */,
                             xcb_timestamp_t   timestamp  /**< */,
                             xcb_timestamp_t   config_timestamp  /**< */,
                             uint16_t          sizeID  /**< */,
                             uint16_t          rotation  /**< */,
                             uint16_t          rate  /**< */);

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
 ** xcb_randr_set_screen_config_cookie_t xcb_randr_set_screen_config_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param xcb_timestamp_t   timestamp
 ** @param xcb_timestamp_t   config_timestamp
 ** @param uint16_t          sizeID
 ** @param uint16_t          rotation
 ** @param uint16_t          rate
 ** @returns xcb_randr_set_screen_config_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_set_screen_config_cookie_t
xcb_randr_set_screen_config_unchecked (xcb_connection_t *c  /**< */,
                                       xcb_window_t      window  /**< */,
                                       xcb_timestamp_t   timestamp  /**< */,
                                       xcb_timestamp_t   config_timestamp  /**< */,
                                       uint16_t          sizeID  /**< */,
                                       uint16_t          rotation  /**< */,
                                       uint16_t          rate  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_set_screen_config_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_set_screen_config_reply_t * xcb_randr_set_screen_config_reply
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_randr_set_screen_config_cookie_t   cookie
 ** @param xcb_generic_error_t                  **e
 ** @returns xcb_randr_set_screen_config_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_set_screen_config_reply_t *
xcb_randr_set_screen_config_reply (xcb_connection_t                      *c  /**< */,
                                   xcb_randr_set_screen_config_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_select_input_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint16_t          enable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_select_input_checked (xcb_connection_t *c  /**< */,
                                xcb_window_t      window  /**< */,
                                uint16_t          enable  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_select_input
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint16_t          enable
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_select_input (xcb_connection_t *c  /**< */,
                        xcb_window_t      window  /**< */,
                        uint16_t          enable  /**< */);

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
 ** xcb_randr_get_screen_info_cookie_t xcb_randr_get_screen_info
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_info_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_info_cookie_t
xcb_randr_get_screen_info (xcb_connection_t *c  /**< */,
                           xcb_window_t      window  /**< */);

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
 ** xcb_randr_get_screen_info_cookie_t xcb_randr_get_screen_info_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_info_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_info_cookie_t
xcb_randr_get_screen_info_unchecked (xcb_connection_t *c  /**< */,
                                     xcb_window_t      window  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_screen_size_t * xcb_randr_get_screen_info_sizes
 ** 
 ** @param const xcb_randr_get_screen_info_reply_t *R
 ** @returns xcb_randr_screen_size_t *
 **
 *****************************************************************************/
 
xcb_randr_screen_size_t *
xcb_randr_get_screen_info_sizes (const xcb_randr_get_screen_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_info_sizes_length
 ** 
 ** @param const xcb_randr_get_screen_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_info_sizes_length (const xcb_randr_get_screen_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_screen_size_iterator_t xcb_randr_get_screen_info_sizes_iterator
 ** 
 ** @param const xcb_randr_get_screen_info_reply_t *R
 ** @returns xcb_randr_screen_size_iterator_t
 **
 *****************************************************************************/
 
xcb_randr_screen_size_iterator_t
xcb_randr_get_screen_info_sizes_iterator (const xcb_randr_get_screen_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_info_rates_length
 ** 
 ** @param const xcb_randr_get_screen_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_info_rates_length (const xcb_randr_get_screen_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_refresh_rates_iterator_t xcb_randr_get_screen_info_rates_iterator
 ** 
 ** @param const xcb_randr_get_screen_info_reply_t *R
 ** @returns xcb_randr_refresh_rates_iterator_t
 **
 *****************************************************************************/
 
xcb_randr_refresh_rates_iterator_t
xcb_randr_get_screen_info_rates_iterator (const xcb_randr_get_screen_info_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_screen_info_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_screen_info_reply_t * xcb_randr_get_screen_info_reply
 ** 
 ** @param xcb_connection_t                    *c
 ** @param xcb_randr_get_screen_info_cookie_t   cookie
 ** @param xcb_generic_error_t                **e
 ** @returns xcb_randr_get_screen_info_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_screen_info_reply_t *
xcb_randr_get_screen_info_reply (xcb_connection_t                    *c  /**< */,
                                 xcb_randr_get_screen_info_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e  /**< */);

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
 ** xcb_randr_get_screen_size_range_cookie_t xcb_randr_get_screen_size_range
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_size_range_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_size_range_cookie_t
xcb_randr_get_screen_size_range (xcb_connection_t *c  /**< */,
                                 xcb_window_t      window  /**< */);

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
 ** xcb_randr_get_screen_size_range_cookie_t xcb_randr_get_screen_size_range_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_size_range_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_size_range_cookie_t
xcb_randr_get_screen_size_range_unchecked (xcb_connection_t *c  /**< */,
                                           xcb_window_t      window  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_screen_size_range_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_screen_size_range_reply_t * xcb_randr_get_screen_size_range_reply
 ** 
 ** @param xcb_connection_t                          *c
 ** @param xcb_randr_get_screen_size_range_cookie_t   cookie
 ** @param xcb_generic_error_t                      **e
 ** @returns xcb_randr_get_screen_size_range_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_screen_size_range_reply_t *
xcb_randr_get_screen_size_range_reply (xcb_connection_t                          *c  /**< */,
                                       xcb_randr_get_screen_size_range_cookie_t   cookie  /**< */,
                                       xcb_generic_error_t                      **e  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_screen_size_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint16_t          width
 ** @param uint16_t          height
 ** @param uint32_t          mm_width
 ** @param uint32_t          mm_height
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_screen_size_checked (xcb_connection_t *c  /**< */,
                                   xcb_window_t      window  /**< */,
                                   uint16_t          width  /**< */,
                                   uint16_t          height  /**< */,
                                   uint32_t          mm_width  /**< */,
                                   uint32_t          mm_height  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_screen_size
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @param uint16_t          width
 ** @param uint16_t          height
 ** @param uint32_t          mm_width
 ** @param uint32_t          mm_height
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_screen_size (xcb_connection_t *c  /**< */,
                           xcb_window_t      window  /**< */,
                           uint16_t          width  /**< */,
                           uint16_t          height  /**< */,
                           uint32_t          mm_width  /**< */,
                           uint32_t          mm_height  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_mode_info_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_mode_info_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_mode_info_next
 ** 
 ** @param xcb_randr_mode_info_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_mode_info_next (xcb_randr_mode_info_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_mode_info_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_mode_info_end
 ** 
 ** @param xcb_randr_mode_info_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_mode_info_end (xcb_randr_mode_info_iterator_t i  /**< */);

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
 ** xcb_randr_get_screen_resources_cookie_t xcb_randr_get_screen_resources
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_resources_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_resources_cookie_t
xcb_randr_get_screen_resources (xcb_connection_t *c  /**< */,
                                xcb_window_t      window  /**< */);

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
 ** xcb_randr_get_screen_resources_cookie_t xcb_randr_get_screen_resources_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_resources_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_resources_cookie_t
xcb_randr_get_screen_resources_unchecked (xcb_connection_t *c  /**< */,
                                          xcb_window_t      window  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_crtc_t * xcb_randr_get_screen_resources_crtcs
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_randr_crtc_t *
 **
 *****************************************************************************/
 
xcb_randr_crtc_t *
xcb_randr_get_screen_resources_crtcs (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_crtcs_length
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_crtcs_length (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_screen_resources_crtcs_end
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_screen_resources_crtcs_end (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_output_t * xcb_randr_get_screen_resources_outputs
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_randr_output_t *
 **
 *****************************************************************************/
 
xcb_randr_output_t *
xcb_randr_get_screen_resources_outputs (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_outputs_length
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_outputs_length (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_screen_resources_outputs_end
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_screen_resources_outputs_end (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_mode_info_t * xcb_randr_get_screen_resources_modes
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_randr_mode_info_t *
 **
 *****************************************************************************/
 
xcb_randr_mode_info_t *
xcb_randr_get_screen_resources_modes (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_modes_length
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_modes_length (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_mode_info_iterator_t xcb_randr_get_screen_resources_modes_iterator
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_randr_mode_info_iterator_t
 **
 *****************************************************************************/
 
xcb_randr_mode_info_iterator_t
xcb_randr_get_screen_resources_modes_iterator (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint8_t * xcb_randr_get_screen_resources_names
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_randr_get_screen_resources_names (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_names_length
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_names_length (const xcb_randr_get_screen_resources_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_screen_resources_names_end
 ** 
 ** @param const xcb_randr_get_screen_resources_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_screen_resources_names_end (const xcb_randr_get_screen_resources_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_screen_resources_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_screen_resources_reply_t * xcb_randr_get_screen_resources_reply
 ** 
 ** @param xcb_connection_t                         *c
 ** @param xcb_randr_get_screen_resources_cookie_t   cookie
 ** @param xcb_generic_error_t                     **e
 ** @returns xcb_randr_get_screen_resources_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_screen_resources_reply_t *
xcb_randr_get_screen_resources_reply (xcb_connection_t                         *c  /**< */,
                                      xcb_randr_get_screen_resources_cookie_t   cookie  /**< */,
                                      xcb_generic_error_t                     **e  /**< */);

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
 ** xcb_randr_get_output_info_cookie_t xcb_randr_get_output_info
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_timestamp_t     config_timestamp
 ** @returns xcb_randr_get_output_info_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_output_info_cookie_t
xcb_randr_get_output_info (xcb_connection_t   *c  /**< */,
                           xcb_randr_output_t  output  /**< */,
                           xcb_timestamp_t     config_timestamp  /**< */);

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
 ** xcb_randr_get_output_info_cookie_t xcb_randr_get_output_info_unchecked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_timestamp_t     config_timestamp
 ** @returns xcb_randr_get_output_info_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_output_info_cookie_t
xcb_randr_get_output_info_unchecked (xcb_connection_t   *c  /**< */,
                                     xcb_randr_output_t  output  /**< */,
                                     xcb_timestamp_t     config_timestamp  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_crtc_t * xcb_randr_get_output_info_crtcs
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_randr_crtc_t *
 **
 *****************************************************************************/
 
xcb_randr_crtc_t *
xcb_randr_get_output_info_crtcs (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_output_info_crtcs_length
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_output_info_crtcs_length (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_output_info_crtcs_end
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_output_info_crtcs_end (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_mode_t * xcb_randr_get_output_info_modes
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_randr_mode_t *
 **
 *****************************************************************************/
 
xcb_randr_mode_t *
xcb_randr_get_output_info_modes (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_output_info_modes_length
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_output_info_modes_length (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_output_info_modes_end
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_output_info_modes_end (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_output_t * xcb_randr_get_output_info_clones
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_randr_output_t *
 **
 *****************************************************************************/
 
xcb_randr_output_t *
xcb_randr_get_output_info_clones (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_output_info_clones_length
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_output_info_clones_length (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_output_info_clones_end
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_output_info_clones_end (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint8_t * xcb_randr_get_output_info_name
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_randr_get_output_info_name (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_output_info_name_length
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_output_info_name_length (const xcb_randr_get_output_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_output_info_name_end
 ** 
 ** @param const xcb_randr_get_output_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_output_info_name_end (const xcb_randr_get_output_info_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_output_info_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_output_info_reply_t * xcb_randr_get_output_info_reply
 ** 
 ** @param xcb_connection_t                    *c
 ** @param xcb_randr_get_output_info_cookie_t   cookie
 ** @param xcb_generic_error_t                **e
 ** @returns xcb_randr_get_output_info_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_output_info_reply_t *
xcb_randr_get_output_info_reply (xcb_connection_t                    *c  /**< */,
                                 xcb_randr_get_output_info_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e  /**< */);

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
 ** xcb_randr_list_output_properties_cookie_t xcb_randr_list_output_properties
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @returns xcb_randr_list_output_properties_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_list_output_properties_cookie_t
xcb_randr_list_output_properties (xcb_connection_t   *c  /**< */,
                                  xcb_randr_output_t  output  /**< */);

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
 ** xcb_randr_list_output_properties_cookie_t xcb_randr_list_output_properties_unchecked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @returns xcb_randr_list_output_properties_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_list_output_properties_cookie_t
xcb_randr_list_output_properties_unchecked (xcb_connection_t   *c  /**< */,
                                            xcb_randr_output_t  output  /**< */);


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_randr_list_output_properties_atoms
 ** 
 ** @param const xcb_randr_list_output_properties_reply_t *R
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_randr_list_output_properties_atoms (const xcb_randr_list_output_properties_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_list_output_properties_atoms_length
 ** 
 ** @param const xcb_randr_list_output_properties_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_list_output_properties_atoms_length (const xcb_randr_list_output_properties_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_list_output_properties_atoms_end
 ** 
 ** @param const xcb_randr_list_output_properties_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_list_output_properties_atoms_end (const xcb_randr_list_output_properties_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_list_output_properties_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_list_output_properties_reply_t * xcb_randr_list_output_properties_reply
 ** 
 ** @param xcb_connection_t                           *c
 ** @param xcb_randr_list_output_properties_cookie_t   cookie
 ** @param xcb_generic_error_t                       **e
 ** @returns xcb_randr_list_output_properties_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_list_output_properties_reply_t *
xcb_randr_list_output_properties_reply (xcb_connection_t                           *c  /**< */,
                                        xcb_randr_list_output_properties_cookie_t   cookie  /**< */,
                                        xcb_generic_error_t                       **e  /**< */);

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
 ** xcb_randr_query_output_property_cookie_t xcb_randr_query_output_property
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @returns xcb_randr_query_output_property_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_query_output_property_cookie_t
xcb_randr_query_output_property (xcb_connection_t   *c  /**< */,
                                 xcb_randr_output_t  output  /**< */,
                                 xcb_atom_t          property  /**< */);

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
 ** xcb_randr_query_output_property_cookie_t xcb_randr_query_output_property_unchecked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @returns xcb_randr_query_output_property_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_query_output_property_cookie_t
xcb_randr_query_output_property_unchecked (xcb_connection_t   *c  /**< */,
                                           xcb_randr_output_t  output  /**< */,
                                           xcb_atom_t          property  /**< */);


/*****************************************************************************
 **
 ** int32_t * xcb_randr_query_output_property_valid_values
 ** 
 ** @param const xcb_randr_query_output_property_reply_t *R
 ** @returns int32_t *
 **
 *****************************************************************************/
 
int32_t *
xcb_randr_query_output_property_valid_values (const xcb_randr_query_output_property_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_query_output_property_valid_values_length
 ** 
 ** @param const xcb_randr_query_output_property_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_query_output_property_valid_values_length (const xcb_randr_query_output_property_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_query_output_property_valid_values_end
 ** 
 ** @param const xcb_randr_query_output_property_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_query_output_property_valid_values_end (const xcb_randr_query_output_property_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_query_output_property_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_query_output_property_reply_t * xcb_randr_query_output_property_reply
 ** 
 ** @param xcb_connection_t                          *c
 ** @param xcb_randr_query_output_property_cookie_t   cookie
 ** @param xcb_generic_error_t                      **e
 ** @returns xcb_randr_query_output_property_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_query_output_property_reply_t *
xcb_randr_query_output_property_reply (xcb_connection_t                          *c  /**< */,
                                       xcb_randr_query_output_property_cookie_t   cookie  /**< */,
                                       xcb_generic_error_t                      **e  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_configure_output_property_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @param uint8_t             pending
 ** @param uint8_t             range
 ** @param uint32_t            values_len
 ** @param const int32_t      *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_configure_output_property_checked (xcb_connection_t   *c  /**< */,
                                             xcb_randr_output_t  output  /**< */,
                                             xcb_atom_t          property  /**< */,
                                             uint8_t             pending  /**< */,
                                             uint8_t             range  /**< */,
                                             uint32_t            values_len  /**< */,
                                             const int32_t      *values  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_configure_output_property
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @param uint8_t             pending
 ** @param uint8_t             range
 ** @param uint32_t            values_len
 ** @param const int32_t      *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_configure_output_property (xcb_connection_t   *c  /**< */,
                                     xcb_randr_output_t  output  /**< */,
                                     xcb_atom_t          property  /**< */,
                                     uint8_t             pending  /**< */,
                                     uint8_t             range  /**< */,
                                     uint32_t            values_len  /**< */,
                                     const int32_t      *values  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_change_output_property_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @param xcb_atom_t          type
 ** @param uint8_t             format
 ** @param uint8_t             mode
 ** @param uint32_t            num_units
 ** @param const void         *data
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_change_output_property_checked (xcb_connection_t   *c  /**< */,
                                          xcb_randr_output_t  output  /**< */,
                                          xcb_atom_t          property  /**< */,
                                          xcb_atom_t          type  /**< */,
                                          uint8_t             format  /**< */,
                                          uint8_t             mode  /**< */,
                                          uint32_t            num_units  /**< */,
                                          const void         *data  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_change_output_property
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @param xcb_atom_t          type
 ** @param uint8_t             format
 ** @param uint8_t             mode
 ** @param uint32_t            num_units
 ** @param const void         *data
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_change_output_property (xcb_connection_t   *c  /**< */,
                                  xcb_randr_output_t  output  /**< */,
                                  xcb_atom_t          property  /**< */,
                                  xcb_atom_t          type  /**< */,
                                  uint8_t             format  /**< */,
                                  uint8_t             mode  /**< */,
                                  uint32_t            num_units  /**< */,
                                  const void         *data  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_delete_output_property_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_delete_output_property_checked (xcb_connection_t   *c  /**< */,
                                          xcb_randr_output_t  output  /**< */,
                                          xcb_atom_t          property  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_delete_output_property
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_delete_output_property (xcb_connection_t   *c  /**< */,
                                  xcb_randr_output_t  output  /**< */,
                                  xcb_atom_t          property  /**< */);

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
 ** xcb_randr_get_output_property_cookie_t xcb_randr_get_output_property
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @param xcb_atom_t          type
 ** @param uint32_t            long_offset
 ** @param uint32_t            long_length
 ** @param uint8_t             _delete
 ** @param uint8_t             pending
 ** @returns xcb_randr_get_output_property_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_output_property_cookie_t
xcb_randr_get_output_property (xcb_connection_t   *c  /**< */,
                               xcb_randr_output_t  output  /**< */,
                               xcb_atom_t          property  /**< */,
                               xcb_atom_t          type  /**< */,
                               uint32_t            long_offset  /**< */,
                               uint32_t            long_length  /**< */,
                               uint8_t             _delete  /**< */,
                               uint8_t             pending  /**< */);

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
 ** xcb_randr_get_output_property_cookie_t xcb_randr_get_output_property_unchecked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_atom_t          property
 ** @param xcb_atom_t          type
 ** @param uint32_t            long_offset
 ** @param uint32_t            long_length
 ** @param uint8_t             _delete
 ** @param uint8_t             pending
 ** @returns xcb_randr_get_output_property_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_output_property_cookie_t
xcb_randr_get_output_property_unchecked (xcb_connection_t   *c  /**< */,
                                         xcb_randr_output_t  output  /**< */,
                                         xcb_atom_t          property  /**< */,
                                         xcb_atom_t          type  /**< */,
                                         uint32_t            long_offset  /**< */,
                                         uint32_t            long_length  /**< */,
                                         uint8_t             _delete  /**< */,
                                         uint8_t             pending  /**< */);


/*****************************************************************************
 **
 ** uint8_t * xcb_randr_get_output_property_data
 ** 
 ** @param const xcb_randr_get_output_property_reply_t *R
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_randr_get_output_property_data (const xcb_randr_get_output_property_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_output_property_data_length
 ** 
 ** @param const xcb_randr_get_output_property_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_output_property_data_length (const xcb_randr_get_output_property_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_output_property_data_end
 ** 
 ** @param const xcb_randr_get_output_property_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_output_property_data_end (const xcb_randr_get_output_property_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_output_property_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_output_property_reply_t * xcb_randr_get_output_property_reply
 ** 
 ** @param xcb_connection_t                        *c
 ** @param xcb_randr_get_output_property_cookie_t   cookie
 ** @param xcb_generic_error_t                    **e
 ** @returns xcb_randr_get_output_property_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_output_property_reply_t *
xcb_randr_get_output_property_reply (xcb_connection_t                        *c  /**< */,
                                     xcb_randr_get_output_property_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e  /**< */);

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
 ** xcb_randr_create_mode_cookie_t xcb_randr_create_mode
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_window_t           window
 ** @param xcb_randr_mode_info_t  mode_info
 ** @param uint32_t               name_len
 ** @param const char            *name
 ** @returns xcb_randr_create_mode_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_create_mode_cookie_t
xcb_randr_create_mode (xcb_connection_t      *c  /**< */,
                       xcb_window_t           window  /**< */,
                       xcb_randr_mode_info_t  mode_info  /**< */,
                       uint32_t               name_len  /**< */,
                       const char            *name  /**< */);

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
 ** xcb_randr_create_mode_cookie_t xcb_randr_create_mode_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_window_t           window
 ** @param xcb_randr_mode_info_t  mode_info
 ** @param uint32_t               name_len
 ** @param const char            *name
 ** @returns xcb_randr_create_mode_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_create_mode_cookie_t
xcb_randr_create_mode_unchecked (xcb_connection_t      *c  /**< */,
                                 xcb_window_t           window  /**< */,
                                 xcb_randr_mode_info_t  mode_info  /**< */,
                                 uint32_t               name_len  /**< */,
                                 const char            *name  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_create_mode_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_create_mode_reply_t * xcb_randr_create_mode_reply
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_randr_create_mode_cookie_t   cookie
 ** @param xcb_generic_error_t            **e
 ** @returns xcb_randr_create_mode_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_create_mode_reply_t *
xcb_randr_create_mode_reply (xcb_connection_t                *c  /**< */,
                             xcb_randr_create_mode_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_destroy_mode_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_mode_t  mode
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_destroy_mode_checked (xcb_connection_t *c  /**< */,
                                xcb_randr_mode_t  mode  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_destroy_mode
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_mode_t  mode
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_destroy_mode (xcb_connection_t *c  /**< */,
                        xcb_randr_mode_t  mode  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_add_output_mode_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_randr_mode_t    mode
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_add_output_mode_checked (xcb_connection_t   *c  /**< */,
                                   xcb_randr_output_t  output  /**< */,
                                   xcb_randr_mode_t    mode  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_add_output_mode
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_randr_mode_t    mode
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_add_output_mode (xcb_connection_t   *c  /**< */,
                           xcb_randr_output_t  output  /**< */,
                           xcb_randr_mode_t    mode  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_delete_output_mode_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_randr_mode_t    mode
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_delete_output_mode_checked (xcb_connection_t   *c  /**< */,
                                      xcb_randr_output_t  output  /**< */,
                                      xcb_randr_mode_t    mode  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_delete_output_mode
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_randr_output_t  output
 ** @param xcb_randr_mode_t    mode
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_delete_output_mode (xcb_connection_t   *c  /**< */,
                              xcb_randr_output_t  output  /**< */,
                              xcb_randr_mode_t    mode  /**< */);

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
 ** xcb_randr_get_crtc_info_cookie_t xcb_randr_get_crtc_info
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @param xcb_timestamp_t   config_timestamp
 ** @returns xcb_randr_get_crtc_info_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_info_cookie_t
xcb_randr_get_crtc_info (xcb_connection_t *c  /**< */,
                         xcb_randr_crtc_t  crtc  /**< */,
                         xcb_timestamp_t   config_timestamp  /**< */);

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
 ** xcb_randr_get_crtc_info_cookie_t xcb_randr_get_crtc_info_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @param xcb_timestamp_t   config_timestamp
 ** @returns xcb_randr_get_crtc_info_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_info_cookie_t
xcb_randr_get_crtc_info_unchecked (xcb_connection_t *c  /**< */,
                                   xcb_randr_crtc_t  crtc  /**< */,
                                   xcb_timestamp_t   config_timestamp  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_output_t * xcb_randr_get_crtc_info_outputs
 ** 
 ** @param const xcb_randr_get_crtc_info_reply_t *R
 ** @returns xcb_randr_output_t *
 **
 *****************************************************************************/
 
xcb_randr_output_t *
xcb_randr_get_crtc_info_outputs (const xcb_randr_get_crtc_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_info_outputs_length
 ** 
 ** @param const xcb_randr_get_crtc_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_info_outputs_length (const xcb_randr_get_crtc_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_info_outputs_end
 ** 
 ** @param const xcb_randr_get_crtc_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_info_outputs_end (const xcb_randr_get_crtc_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_output_t * xcb_randr_get_crtc_info_possible
 ** 
 ** @param const xcb_randr_get_crtc_info_reply_t *R
 ** @returns xcb_randr_output_t *
 **
 *****************************************************************************/
 
xcb_randr_output_t *
xcb_randr_get_crtc_info_possible (const xcb_randr_get_crtc_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_info_possible_length
 ** 
 ** @param const xcb_randr_get_crtc_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_info_possible_length (const xcb_randr_get_crtc_info_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_info_possible_end
 ** 
 ** @param const xcb_randr_get_crtc_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_info_possible_end (const xcb_randr_get_crtc_info_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_crtc_info_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_crtc_info_reply_t * xcb_randr_get_crtc_info_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_randr_get_crtc_info_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_randr_get_crtc_info_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_info_reply_t *
xcb_randr_get_crtc_info_reply (xcb_connection_t                  *c  /**< */,
                               xcb_randr_get_crtc_info_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */);

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
 ** xcb_randr_set_crtc_config_cookie_t xcb_randr_set_crtc_config
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_randr_crtc_t          crtc
 ** @param xcb_timestamp_t           timestamp
 ** @param xcb_timestamp_t           config_timestamp
 ** @param int16_t                   x
 ** @param int16_t                   y
 ** @param xcb_randr_mode_t          mode
 ** @param uint16_t                  rotation
 ** @param uint32_t                  outputs_len
 ** @param const xcb_randr_output_t *outputs
 ** @returns xcb_randr_set_crtc_config_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_set_crtc_config_cookie_t
xcb_randr_set_crtc_config (xcb_connection_t         *c  /**< */,
                           xcb_randr_crtc_t          crtc  /**< */,
                           xcb_timestamp_t           timestamp  /**< */,
                           xcb_timestamp_t           config_timestamp  /**< */,
                           int16_t                   x  /**< */,
                           int16_t                   y  /**< */,
                           xcb_randr_mode_t          mode  /**< */,
                           uint16_t                  rotation  /**< */,
                           uint32_t                  outputs_len  /**< */,
                           const xcb_randr_output_t *outputs  /**< */);

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
 ** xcb_randr_set_crtc_config_cookie_t xcb_randr_set_crtc_config_unchecked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_randr_crtc_t          crtc
 ** @param xcb_timestamp_t           timestamp
 ** @param xcb_timestamp_t           config_timestamp
 ** @param int16_t                   x
 ** @param int16_t                   y
 ** @param xcb_randr_mode_t          mode
 ** @param uint16_t                  rotation
 ** @param uint32_t                  outputs_len
 ** @param const xcb_randr_output_t *outputs
 ** @returns xcb_randr_set_crtc_config_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_set_crtc_config_cookie_t
xcb_randr_set_crtc_config_unchecked (xcb_connection_t         *c  /**< */,
                                     xcb_randr_crtc_t          crtc  /**< */,
                                     xcb_timestamp_t           timestamp  /**< */,
                                     xcb_timestamp_t           config_timestamp  /**< */,
                                     int16_t                   x  /**< */,
                                     int16_t                   y  /**< */,
                                     xcb_randr_mode_t          mode  /**< */,
                                     uint16_t                  rotation  /**< */,
                                     uint32_t                  outputs_len  /**< */,
                                     const xcb_randr_output_t *outputs  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_set_crtc_config_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_set_crtc_config_reply_t * xcb_randr_set_crtc_config_reply
 ** 
 ** @param xcb_connection_t                    *c
 ** @param xcb_randr_set_crtc_config_cookie_t   cookie
 ** @param xcb_generic_error_t                **e
 ** @returns xcb_randr_set_crtc_config_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_set_crtc_config_reply_t *
xcb_randr_set_crtc_config_reply (xcb_connection_t                    *c  /**< */,
                                 xcb_randr_set_crtc_config_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e  /**< */);

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
 ** xcb_randr_get_crtc_gamma_size_cookie_t xcb_randr_get_crtc_gamma_size
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_crtc_gamma_size_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_gamma_size_cookie_t
xcb_randr_get_crtc_gamma_size (xcb_connection_t *c  /**< */,
                               xcb_randr_crtc_t  crtc  /**< */);

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
 ** xcb_randr_get_crtc_gamma_size_cookie_t xcb_randr_get_crtc_gamma_size_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_crtc_gamma_size_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_gamma_size_cookie_t
xcb_randr_get_crtc_gamma_size_unchecked (xcb_connection_t *c  /**< */,
                                         xcb_randr_crtc_t  crtc  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_crtc_gamma_size_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_crtc_gamma_size_reply_t * xcb_randr_get_crtc_gamma_size_reply
 ** 
 ** @param xcb_connection_t                        *c
 ** @param xcb_randr_get_crtc_gamma_size_cookie_t   cookie
 ** @param xcb_generic_error_t                    **e
 ** @returns xcb_randr_get_crtc_gamma_size_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_gamma_size_reply_t *
xcb_randr_get_crtc_gamma_size_reply (xcb_connection_t                        *c  /**< */,
                                     xcb_randr_get_crtc_gamma_size_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e  /**< */);

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
 ** xcb_randr_get_crtc_gamma_cookie_t xcb_randr_get_crtc_gamma
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_crtc_gamma_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_gamma_cookie_t
xcb_randr_get_crtc_gamma (xcb_connection_t *c  /**< */,
                          xcb_randr_crtc_t  crtc  /**< */);

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
 ** xcb_randr_get_crtc_gamma_cookie_t xcb_randr_get_crtc_gamma_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_crtc_gamma_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_gamma_cookie_t
xcb_randr_get_crtc_gamma_unchecked (xcb_connection_t *c  /**< */,
                                    xcb_randr_crtc_t  crtc  /**< */);


/*****************************************************************************
 **
 ** uint16_t * xcb_randr_get_crtc_gamma_red
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_randr_get_crtc_gamma_red (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_gamma_red_length
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_gamma_red_length (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_gamma_red_end
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_gamma_red_end (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint16_t * xcb_randr_get_crtc_gamma_green
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_randr_get_crtc_gamma_green (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_gamma_green_length
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_gamma_green_length (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_gamma_green_end
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_gamma_green_end (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint16_t * xcb_randr_get_crtc_gamma_blue
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_randr_get_crtc_gamma_blue (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_gamma_blue_length
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_gamma_blue_length (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_gamma_blue_end
 ** 
 ** @param const xcb_randr_get_crtc_gamma_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_gamma_blue_end (const xcb_randr_get_crtc_gamma_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_crtc_gamma_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_crtc_gamma_reply_t * xcb_randr_get_crtc_gamma_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_randr_get_crtc_gamma_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_randr_get_crtc_gamma_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_gamma_reply_t *
xcb_randr_get_crtc_gamma_reply (xcb_connection_t                   *c  /**< */,
                                xcb_randr_get_crtc_gamma_cookie_t   cookie  /**< */,
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
 ** xcb_void_cookie_t xcb_randr_set_crtc_gamma_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @param uint16_t          size
 ** @param const uint16_t   *red
 ** @param const uint16_t   *green
 ** @param const uint16_t   *blue
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_crtc_gamma_checked (xcb_connection_t *c  /**< */,
                                  xcb_randr_crtc_t  crtc  /**< */,
                                  uint16_t          size  /**< */,
                                  const uint16_t   *red  /**< */,
                                  const uint16_t   *green  /**< */,
                                  const uint16_t   *blue  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_crtc_gamma
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @param uint16_t          size
 ** @param const uint16_t   *red
 ** @param const uint16_t   *green
 ** @param const uint16_t   *blue
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_crtc_gamma (xcb_connection_t *c  /**< */,
                          xcb_randr_crtc_t  crtc  /**< */,
                          uint16_t          size  /**< */,
                          const uint16_t   *red  /**< */,
                          const uint16_t   *green  /**< */,
                          const uint16_t   *blue  /**< */);

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
 ** xcb_randr_get_screen_resources_current_cookie_t xcb_randr_get_screen_resources_current
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_resources_current_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_resources_current_cookie_t
xcb_randr_get_screen_resources_current (xcb_connection_t *c  /**< */,
                                        xcb_window_t      window  /**< */);

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
 ** xcb_randr_get_screen_resources_current_cookie_t xcb_randr_get_screen_resources_current_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_screen_resources_current_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_screen_resources_current_cookie_t
xcb_randr_get_screen_resources_current_unchecked (xcb_connection_t *c  /**< */,
                                                  xcb_window_t      window  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_crtc_t * xcb_randr_get_screen_resources_current_crtcs
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_randr_crtc_t *
 **
 *****************************************************************************/
 
xcb_randr_crtc_t *
xcb_randr_get_screen_resources_current_crtcs (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_current_crtcs_length
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_current_crtcs_length (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_screen_resources_current_crtcs_end
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_screen_resources_current_crtcs_end (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_output_t * xcb_randr_get_screen_resources_current_outputs
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_randr_output_t *
 **
 *****************************************************************************/
 
xcb_randr_output_t *
xcb_randr_get_screen_resources_current_outputs (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_current_outputs_length
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_current_outputs_length (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_screen_resources_current_outputs_end
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_screen_resources_current_outputs_end (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_mode_info_t * xcb_randr_get_screen_resources_current_modes
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_randr_mode_info_t *
 **
 *****************************************************************************/
 
xcb_randr_mode_info_t *
xcb_randr_get_screen_resources_current_modes (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_current_modes_length
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_current_modes_length (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_randr_mode_info_iterator_t xcb_randr_get_screen_resources_current_modes_iterator
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_randr_mode_info_iterator_t
 **
 *****************************************************************************/
 
xcb_randr_mode_info_iterator_t
xcb_randr_get_screen_resources_current_modes_iterator (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint8_t * xcb_randr_get_screen_resources_current_names
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_randr_get_screen_resources_current_names (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_screen_resources_current_names_length
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_screen_resources_current_names_length (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_screen_resources_current_names_end
 ** 
 ** @param const xcb_randr_get_screen_resources_current_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_screen_resources_current_names_end (const xcb_randr_get_screen_resources_current_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_screen_resources_current_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_screen_resources_current_reply_t * xcb_randr_get_screen_resources_current_reply
 ** 
 ** @param xcb_connection_t                                 *c
 ** @param xcb_randr_get_screen_resources_current_cookie_t   cookie
 ** @param xcb_generic_error_t                             **e
 ** @returns xcb_randr_get_screen_resources_current_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_screen_resources_current_reply_t *
xcb_randr_get_screen_resources_current_reply (xcb_connection_t                                 *c  /**< */,
                                              xcb_randr_get_screen_resources_current_cookie_t   cookie  /**< */,
                                              xcb_generic_error_t                             **e  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_crtc_transform_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_randr_crtc_t          crtc
 ** @param xcb_render_transform_t    transform
 ** @param uint16_t                  filter_len
 ** @param const char               *filter_name
 ** @param uint32_t                  filter_params_len
 ** @param const xcb_render_fixed_t *filter_params
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_crtc_transform_checked (xcb_connection_t         *c  /**< */,
                                      xcb_randr_crtc_t          crtc  /**< */,
                                      xcb_render_transform_t    transform  /**< */,
                                      uint16_t                  filter_len  /**< */,
                                      const char               *filter_name  /**< */,
                                      uint32_t                  filter_params_len  /**< */,
                                      const xcb_render_fixed_t *filter_params  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_crtc_transform
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_randr_crtc_t          crtc
 ** @param xcb_render_transform_t    transform
 ** @param uint16_t                  filter_len
 ** @param const char               *filter_name
 ** @param uint32_t                  filter_params_len
 ** @param const xcb_render_fixed_t *filter_params
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_crtc_transform (xcb_connection_t         *c  /**< */,
                              xcb_randr_crtc_t          crtc  /**< */,
                              xcb_render_transform_t    transform  /**< */,
                              uint16_t                  filter_len  /**< */,
                              const char               *filter_name  /**< */,
                              uint32_t                  filter_params_len  /**< */,
                              const xcb_render_fixed_t *filter_params  /**< */);

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
 ** xcb_randr_get_crtc_transform_cookie_t xcb_randr_get_crtc_transform
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_crtc_transform_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_transform_cookie_t
xcb_randr_get_crtc_transform (xcb_connection_t *c  /**< */,
                              xcb_randr_crtc_t  crtc  /**< */);

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
 ** xcb_randr_get_crtc_transform_cookie_t xcb_randr_get_crtc_transform_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_crtc_transform_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_transform_cookie_t
xcb_randr_get_crtc_transform_unchecked (xcb_connection_t *c  /**< */,
                                        xcb_randr_crtc_t  crtc  /**< */);


/*****************************************************************************
 **
 ** char * xcb_randr_get_crtc_transform_pending_filter_name
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_randr_get_crtc_transform_pending_filter_name (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_transform_pending_filter_name_length
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_transform_pending_filter_name_length (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_transform_pending_filter_name_end
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_transform_pending_filter_name_end (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_fixed_t * xcb_randr_get_crtc_transform_pending_params
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns xcb_render_fixed_t *
 **
 *****************************************************************************/
 
xcb_render_fixed_t *
xcb_randr_get_crtc_transform_pending_params (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_transform_pending_params_length
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_transform_pending_params_length (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_transform_pending_params_end
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_transform_pending_params_end (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** char * xcb_randr_get_crtc_transform_current_filter_name
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_randr_get_crtc_transform_current_filter_name (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_transform_current_filter_name_length
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_transform_current_filter_name_length (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_transform_current_filter_name_end
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_transform_current_filter_name_end (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_fixed_t * xcb_randr_get_crtc_transform_current_params
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns xcb_render_fixed_t *
 **
 *****************************************************************************/
 
xcb_render_fixed_t *
xcb_randr_get_crtc_transform_current_params (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_randr_get_crtc_transform_current_params_length
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_randr_get_crtc_transform_current_params_length (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_get_crtc_transform_current_params_end
 ** 
 ** @param const xcb_randr_get_crtc_transform_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_get_crtc_transform_current_params_end (const xcb_randr_get_crtc_transform_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_crtc_transform_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_crtc_transform_reply_t * xcb_randr_get_crtc_transform_reply
 ** 
 ** @param xcb_connection_t                       *c
 ** @param xcb_randr_get_crtc_transform_cookie_t   cookie
 ** @param xcb_generic_error_t                   **e
 ** @returns xcb_randr_get_crtc_transform_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_crtc_transform_reply_t *
xcb_randr_get_crtc_transform_reply (xcb_connection_t                       *c  /**< */,
                                    xcb_randr_get_crtc_transform_cookie_t   cookie  /**< */,
                                    xcb_generic_error_t                   **e  /**< */);

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
 ** xcb_randr_get_panning_cookie_t xcb_randr_get_panning
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_panning_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_panning_cookie_t
xcb_randr_get_panning (xcb_connection_t *c  /**< */,
                       xcb_randr_crtc_t  crtc  /**< */);

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
 ** xcb_randr_get_panning_cookie_t xcb_randr_get_panning_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @returns xcb_randr_get_panning_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_panning_cookie_t
xcb_randr_get_panning_unchecked (xcb_connection_t *c  /**< */,
                                 xcb_randr_crtc_t  crtc  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_panning_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_panning_reply_t * xcb_randr_get_panning_reply
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_randr_get_panning_cookie_t   cookie
 ** @param xcb_generic_error_t            **e
 ** @returns xcb_randr_get_panning_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_panning_reply_t *
xcb_randr_get_panning_reply (xcb_connection_t                *c  /**< */,
                             xcb_randr_get_panning_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e  /**< */);

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
 ** xcb_randr_set_panning_cookie_t xcb_randr_set_panning
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @param xcb_timestamp_t   timestamp
 ** @param uint16_t          left
 ** @param uint16_t          top
 ** @param uint16_t          width
 ** @param uint16_t          height
 ** @param uint16_t          track_left
 ** @param uint16_t          track_top
 ** @param uint16_t          track_width
 ** @param uint16_t          track_height
 ** @param int16_t           border_left
 ** @param int16_t           border_top
 ** @param int16_t           border_right
 ** @param int16_t           border_bottom
 ** @returns xcb_randr_set_panning_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_set_panning_cookie_t
xcb_randr_set_panning (xcb_connection_t *c  /**< */,
                       xcb_randr_crtc_t  crtc  /**< */,
                       xcb_timestamp_t   timestamp  /**< */,
                       uint16_t          left  /**< */,
                       uint16_t          top  /**< */,
                       uint16_t          width  /**< */,
                       uint16_t          height  /**< */,
                       uint16_t          track_left  /**< */,
                       uint16_t          track_top  /**< */,
                       uint16_t          track_width  /**< */,
                       uint16_t          track_height  /**< */,
                       int16_t           border_left  /**< */,
                       int16_t           border_top  /**< */,
                       int16_t           border_right  /**< */,
                       int16_t           border_bottom  /**< */);

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
 ** xcb_randr_set_panning_cookie_t xcb_randr_set_panning_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_randr_crtc_t  crtc
 ** @param xcb_timestamp_t   timestamp
 ** @param uint16_t          left
 ** @param uint16_t          top
 ** @param uint16_t          width
 ** @param uint16_t          height
 ** @param uint16_t          track_left
 ** @param uint16_t          track_top
 ** @param uint16_t          track_width
 ** @param uint16_t          track_height
 ** @param int16_t           border_left
 ** @param int16_t           border_top
 ** @param int16_t           border_right
 ** @param int16_t           border_bottom
 ** @returns xcb_randr_set_panning_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_set_panning_cookie_t
xcb_randr_set_panning_unchecked (xcb_connection_t *c  /**< */,
                                 xcb_randr_crtc_t  crtc  /**< */,
                                 xcb_timestamp_t   timestamp  /**< */,
                                 uint16_t          left  /**< */,
                                 uint16_t          top  /**< */,
                                 uint16_t          width  /**< */,
                                 uint16_t          height  /**< */,
                                 uint16_t          track_left  /**< */,
                                 uint16_t          track_top  /**< */,
                                 uint16_t          track_width  /**< */,
                                 uint16_t          track_height  /**< */,
                                 int16_t           border_left  /**< */,
                                 int16_t           border_top  /**< */,
                                 int16_t           border_right  /**< */,
                                 int16_t           border_bottom  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_set_panning_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_set_panning_reply_t * xcb_randr_set_panning_reply
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_randr_set_panning_cookie_t   cookie
 ** @param xcb_generic_error_t            **e
 ** @returns xcb_randr_set_panning_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_set_panning_reply_t *
xcb_randr_set_panning_reply (xcb_connection_t                *c  /**< */,
                             xcb_randr_set_panning_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_output_primary_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_window_t        window
 ** @param xcb_randr_output_t  output
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_output_primary_checked (xcb_connection_t   *c  /**< */,
                                      xcb_window_t        window  /**< */,
                                      xcb_randr_output_t  output  /**< */);

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
 ** xcb_void_cookie_t xcb_randr_set_output_primary
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_window_t        window
 ** @param xcb_randr_output_t  output
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_randr_set_output_primary (xcb_connection_t   *c  /**< */,
                              xcb_window_t        window  /**< */,
                              xcb_randr_output_t  output  /**< */);

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
 ** xcb_randr_get_output_primary_cookie_t xcb_randr_get_output_primary
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_output_primary_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_output_primary_cookie_t
xcb_randr_get_output_primary (xcb_connection_t *c  /**< */,
                              xcb_window_t      window  /**< */);

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
 ** xcb_randr_get_output_primary_cookie_t xcb_randr_get_output_primary_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_window_t      window
 ** @returns xcb_randr_get_output_primary_cookie_t
 **
 *****************************************************************************/
 
xcb_randr_get_output_primary_cookie_t
xcb_randr_get_output_primary_unchecked (xcb_connection_t *c  /**< */,
                                        xcb_window_t      window  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_randr_get_output_primary_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_randr_get_output_primary_reply_t * xcb_randr_get_output_primary_reply
 ** 
 ** @param xcb_connection_t                       *c
 ** @param xcb_randr_get_output_primary_cookie_t   cookie
 ** @param xcb_generic_error_t                   **e
 ** @returns xcb_randr_get_output_primary_reply_t *
 **
 *****************************************************************************/
 
xcb_randr_get_output_primary_reply_t *
xcb_randr_get_output_primary_reply (xcb_connection_t                       *c  /**< */,
                                    xcb_randr_get_output_primary_cookie_t   cookie  /**< */,
                                    xcb_generic_error_t                   **e  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_crtc_change_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_crtc_change_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_crtc_change_next
 ** 
 ** @param xcb_randr_crtc_change_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_crtc_change_next (xcb_randr_crtc_change_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_crtc_change_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_crtc_change_end
 ** 
 ** @param xcb_randr_crtc_change_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_crtc_change_end (xcb_randr_crtc_change_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_output_change_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_output_change_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_output_change_next
 ** 
 ** @param xcb_randr_output_change_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_output_change_next (xcb_randr_output_change_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_output_change_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_output_change_end
 ** 
 ** @param xcb_randr_output_change_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_output_change_end (xcb_randr_output_change_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_output_property_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_output_property_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_output_property_next
 ** 
 ** @param xcb_randr_output_property_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_output_property_next (xcb_randr_output_property_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_output_property_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_output_property_end
 ** 
 ** @param xcb_randr_output_property_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_output_property_end (xcb_randr_output_property_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_randr_notify_data_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_randr_notify_data_t)
 */

/*****************************************************************************
 **
 ** void xcb_randr_notify_data_next
 ** 
 ** @param xcb_randr_notify_data_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_randr_notify_data_next (xcb_randr_notify_data_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_randr_notify_data_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_randr_notify_data_end
 ** 
 ** @param xcb_randr_notify_data_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_randr_notify_data_end (xcb_randr_notify_data_iterator_t i  /**< */);


#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
