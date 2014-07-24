/*
 * This file generated automatically from render.xml by c_client.py.
 * Edit at your peril.
 */

/**
 * @defgroup XCB_Render_API XCB Render API
 * @brief Render XCB Protocol Implementation.
 * @{
 **/

#ifndef __RENDER_H
#define __RENDER_H

#include "xcb.h"
#include "xproto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XCB_RENDER_MAJOR_VERSION 0
#define XCB_RENDER_MINOR_VERSION 10
  
extern xcb_extension_t xcb_render_id;

typedef enum xcb_render_pict_type_t {
    XCB_RENDER_PICT_TYPE_INDEXED,
    XCB_RENDER_PICT_TYPE_DIRECT
} xcb_render_pict_type_t;

typedef enum xcb_render_picture_enum_t {
    XCB_RENDER_PICTURE_NONE
} xcb_render_picture_enum_t;

typedef enum xcb_render_pict_op_t {
    XCB_RENDER_PICT_OP_CLEAR,
    XCB_RENDER_PICT_OP_SRC,
    XCB_RENDER_PICT_OP_DST,
    XCB_RENDER_PICT_OP_OVER,
    XCB_RENDER_PICT_OP_OVER_REVERSE,
    XCB_RENDER_PICT_OP_IN,
    XCB_RENDER_PICT_OP_IN_REVERSE,
    XCB_RENDER_PICT_OP_OUT,
    XCB_RENDER_PICT_OP_OUT_REVERSE,
    XCB_RENDER_PICT_OP_ATOP,
    XCB_RENDER_PICT_OP_ATOP_REVERSE,
    XCB_RENDER_PICT_OP_XOR,
    XCB_RENDER_PICT_OP_ADD,
    XCB_RENDER_PICT_OP_SATURATE,
    XCB_RENDER_PICT_OP_DISJOINT_CLEAR = 16,
    XCB_RENDER_PICT_OP_DISJOINT_SRC,
    XCB_RENDER_PICT_OP_DISJOINT_DST,
    XCB_RENDER_PICT_OP_DISJOINT_OVER,
    XCB_RENDER_PICT_OP_DISJOINT_OVER_REVERSE,
    XCB_RENDER_PICT_OP_DISJOINT_IN,
    XCB_RENDER_PICT_OP_DISJOINT_IN_REVERSE,
    XCB_RENDER_PICT_OP_DISJOINT_OUT,
    XCB_RENDER_PICT_OP_DISJOINT_OUT_REVERSE,
    XCB_RENDER_PICT_OP_DISJOINT_ATOP,
    XCB_RENDER_PICT_OP_DISJOINT_ATOP_REVERSE,
    XCB_RENDER_PICT_OP_DISJOINT_XOR,
    XCB_RENDER_PICT_OP_CONJOINT_CLEAR = 32,
    XCB_RENDER_PICT_OP_CONJOINT_SRC,
    XCB_RENDER_PICT_OP_CONJOINT_DST,
    XCB_RENDER_PICT_OP_CONJOINT_OVER,
    XCB_RENDER_PICT_OP_CONJOINT_OVER_REVERSE,
    XCB_RENDER_PICT_OP_CONJOINT_IN,
    XCB_RENDER_PICT_OP_CONJOINT_IN_REVERSE,
    XCB_RENDER_PICT_OP_CONJOINT_OUT,
    XCB_RENDER_PICT_OP_CONJOINT_OUT_REVERSE,
    XCB_RENDER_PICT_OP_CONJOINT_ATOP,
    XCB_RENDER_PICT_OP_CONJOINT_ATOP_REVERSE,
    XCB_RENDER_PICT_OP_CONJOINT_XOR
} xcb_render_pict_op_t;

typedef enum xcb_render_poly_edge_t {
    XCB_RENDER_POLY_EDGE_SHARP,
    XCB_RENDER_POLY_EDGE_SMOOTH
} xcb_render_poly_edge_t;

typedef enum xcb_render_poly_mode_t {
    XCB_RENDER_POLY_MODE_PRECISE,
    XCB_RENDER_POLY_MODE_IMPRECISE
} xcb_render_poly_mode_t;

typedef enum xcb_render_cp_t {
    XCB_RENDER_CP_REPEAT = 1,
    XCB_RENDER_CP_ALPHA_MAP = 2,
    XCB_RENDER_CP_ALPHA_X_ORIGIN = 4,
    XCB_RENDER_CP_ALPHA_Y_ORIGIN = 8,
    XCB_RENDER_CP_CLIP_X_ORIGIN = 16,
    XCB_RENDER_CP_CLIP_Y_ORIGIN = 32,
    XCB_RENDER_CP_CLIP_MASK = 64,
    XCB_RENDER_CP_GRAPHICS_EXPOSURE = 128,
    XCB_RENDER_CP_SUBWINDOW_MODE = 256,
    XCB_RENDER_CP_POLY_EDGE = 512,
    XCB_RENDER_CP_POLY_MODE = 1024,
    XCB_RENDER_CP_DITHER = 2048,
    XCB_RENDER_CP_COMPONENT_ALPHA = 4096
} xcb_render_cp_t;

typedef enum xcb_render_sub_pixel_t {
    XCB_RENDER_SUB_PIXEL_UNKNOWN,
    XCB_RENDER_SUB_PIXEL_HORIZONTAL_RGB,
    XCB_RENDER_SUB_PIXEL_HORIZONTAL_BGR,
    XCB_RENDER_SUB_PIXEL_VERTICAL_RGB,
    XCB_RENDER_SUB_PIXEL_VERTICAL_BGR,
    XCB_RENDER_SUB_PIXEL_NONE
} xcb_render_sub_pixel_t;

typedef enum xcb_render_repeat_t {
    XCB_RENDER_REPEAT_NONE,
    XCB_RENDER_REPEAT_NORMAL,
    XCB_RENDER_REPEAT_PAD,
    XCB_RENDER_REPEAT_REFLECT
} xcb_render_repeat_t;

typedef uint32_t xcb_render_glyph_t;

/**
 * @brief xcb_render_glyph_iterator_t
 **/
typedef struct xcb_render_glyph_iterator_t {
    xcb_render_glyph_t *data; /**<  */
    int                 rem; /**<  */
    int                 index; /**<  */
} xcb_render_glyph_iterator_t;

typedef uint32_t xcb_render_glyphset_t;

/**
 * @brief xcb_render_glyphset_iterator_t
 **/
typedef struct xcb_render_glyphset_iterator_t {
    xcb_render_glyphset_t *data; /**<  */
    int                    rem; /**<  */
    int                    index; /**<  */
} xcb_render_glyphset_iterator_t;

typedef uint32_t xcb_render_picture_t;

/**
 * @brief xcb_render_picture_iterator_t
 **/
typedef struct xcb_render_picture_iterator_t {
    xcb_render_picture_t *data; /**<  */
    int                   rem; /**<  */
    int                   index; /**<  */
} xcb_render_picture_iterator_t;

typedef uint32_t xcb_render_pictformat_t;

/**
 * @brief xcb_render_pictformat_iterator_t
 **/
typedef struct xcb_render_pictformat_iterator_t {
    xcb_render_pictformat_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_render_pictformat_iterator_t;

typedef int32_t xcb_render_fixed_t;

/**
 * @brief xcb_render_fixed_iterator_t
 **/
typedef struct xcb_render_fixed_iterator_t {
    xcb_render_fixed_t *data; /**<  */
    int                 rem; /**<  */
    int                 index; /**<  */
} xcb_render_fixed_iterator_t;

/** Opcode for xcb_render_pict_format. */
#define XCB_RENDER_PICT_FORMAT 0

/**
 * @brief xcb_render_pict_format_error_t
 **/
typedef struct xcb_render_pict_format_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_render_pict_format_error_t;

/** Opcode for xcb_render_picture. */
#define XCB_RENDER_PICTURE 1

/**
 * @brief xcb_render_picture_error_t
 **/
typedef struct xcb_render_picture_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_render_picture_error_t;

/** Opcode for xcb_render_pict_op. */
#define XCB_RENDER_PICT_OP 2

/**
 * @brief xcb_render_pict_op_error_t
 **/
typedef struct xcb_render_pict_op_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_render_pict_op_error_t;

/** Opcode for xcb_render_glyph_set. */
#define XCB_RENDER_GLYPH_SET 3

/**
 * @brief xcb_render_glyph_set_error_t
 **/
typedef struct xcb_render_glyph_set_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_render_glyph_set_error_t;

/** Opcode for xcb_render_glyph. */
#define XCB_RENDER_GLYPH 4

/**
 * @brief xcb_render_glyph_error_t
 **/
typedef struct xcb_render_glyph_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
} xcb_render_glyph_error_t;

/**
 * @brief xcb_render_directformat_t
 **/
typedef struct xcb_render_directformat_t {
    uint16_t red_shift; /**<  */
    uint16_t red_mask; /**<  */
    uint16_t green_shift; /**<  */
    uint16_t green_mask; /**<  */
    uint16_t blue_shift; /**<  */
    uint16_t blue_mask; /**<  */
    uint16_t alpha_shift; /**<  */
    uint16_t alpha_mask; /**<  */
} xcb_render_directformat_t;

/**
 * @brief xcb_render_directformat_iterator_t
 **/
typedef struct xcb_render_directformat_iterator_t {
    xcb_render_directformat_t *data; /**<  */
    int                        rem; /**<  */
    int                        index; /**<  */
} xcb_render_directformat_iterator_t;

/**
 * @brief xcb_render_pictforminfo_t
 **/
typedef struct xcb_render_pictforminfo_t {
    xcb_render_pictformat_t   id; /**<  */
    uint8_t                   type; /**<  */
    uint8_t                   depth; /**<  */
    uint8_t                   pad0[2]; /**<  */
    xcb_render_directformat_t direct; /**<  */
    xcb_colormap_t            colormap; /**<  */
} xcb_render_pictforminfo_t;

/**
 * @brief xcb_render_pictforminfo_iterator_t
 **/
typedef struct xcb_render_pictforminfo_iterator_t {
    xcb_render_pictforminfo_t *data; /**<  */
    int                        rem; /**<  */
    int                        index; /**<  */
} xcb_render_pictforminfo_iterator_t;

/**
 * @brief xcb_render_pictvisual_t
 **/
typedef struct xcb_render_pictvisual_t {
    xcb_visualid_t          visual; /**<  */
    xcb_render_pictformat_t format; /**<  */
} xcb_render_pictvisual_t;

/**
 * @brief xcb_render_pictvisual_iterator_t
 **/
typedef struct xcb_render_pictvisual_iterator_t {
    xcb_render_pictvisual_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_render_pictvisual_iterator_t;

/**
 * @brief xcb_render_pictdepth_t
 **/
typedef struct xcb_render_pictdepth_t {
    uint8_t  depth; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t num_visuals; /**<  */
    uint8_t  pad1[4]; /**<  */
} xcb_render_pictdepth_t;

/**
 * @brief xcb_render_pictdepth_iterator_t
 **/
typedef struct xcb_render_pictdepth_iterator_t {
    xcb_render_pictdepth_t *data; /**<  */
    int                     rem; /**<  */
    int                     index; /**<  */
} xcb_render_pictdepth_iterator_t;

/**
 * @brief xcb_render_pictscreen_t
 **/
typedef struct xcb_render_pictscreen_t {
    uint32_t                num_depths; /**<  */
    xcb_render_pictformat_t fallback; /**<  */
} xcb_render_pictscreen_t;

/**
 * @brief xcb_render_pictscreen_iterator_t
 **/
typedef struct xcb_render_pictscreen_iterator_t {
    xcb_render_pictscreen_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_render_pictscreen_iterator_t;

/**
 * @brief xcb_render_indexvalue_t
 **/
typedef struct xcb_render_indexvalue_t {
    uint32_t pixel; /**<  */
    uint16_t red; /**<  */
    uint16_t green; /**<  */
    uint16_t blue; /**<  */
    uint16_t alpha; /**<  */
} xcb_render_indexvalue_t;

/**
 * @brief xcb_render_indexvalue_iterator_t
 **/
typedef struct xcb_render_indexvalue_iterator_t {
    xcb_render_indexvalue_t *data; /**<  */
    int                      rem; /**<  */
    int                      index; /**<  */
} xcb_render_indexvalue_iterator_t;

/**
 * @brief xcb_render_color_t
 **/
typedef struct xcb_render_color_t {
    uint16_t red; /**<  */
    uint16_t green; /**<  */
    uint16_t blue; /**<  */
    uint16_t alpha; /**<  */
} xcb_render_color_t;

/**
 * @brief xcb_render_color_iterator_t
 **/
typedef struct xcb_render_color_iterator_t {
    xcb_render_color_t *data; /**<  */
    int                 rem; /**<  */
    int                 index; /**<  */
} xcb_render_color_iterator_t;

/**
 * @brief xcb_render_pointfix_t
 **/
typedef struct xcb_render_pointfix_t {
    xcb_render_fixed_t x; /**<  */
    xcb_render_fixed_t y; /**<  */
} xcb_render_pointfix_t;

/**
 * @brief xcb_render_pointfix_iterator_t
 **/
typedef struct xcb_render_pointfix_iterator_t {
    xcb_render_pointfix_t *data; /**<  */
    int                    rem; /**<  */
    int                    index; /**<  */
} xcb_render_pointfix_iterator_t;

/**
 * @brief xcb_render_linefix_t
 **/
typedef struct xcb_render_linefix_t {
    xcb_render_pointfix_t p1; /**<  */
    xcb_render_pointfix_t p2; /**<  */
} xcb_render_linefix_t;

/**
 * @brief xcb_render_linefix_iterator_t
 **/
typedef struct xcb_render_linefix_iterator_t {
    xcb_render_linefix_t *data; /**<  */
    int                   rem; /**<  */
    int                   index; /**<  */
} xcb_render_linefix_iterator_t;

/**
 * @brief xcb_render_triangle_t
 **/
typedef struct xcb_render_triangle_t {
    xcb_render_pointfix_t p1; /**<  */
    xcb_render_pointfix_t p2; /**<  */
    xcb_render_pointfix_t p3; /**<  */
} xcb_render_triangle_t;

/**
 * @brief xcb_render_triangle_iterator_t
 **/
typedef struct xcb_render_triangle_iterator_t {
    xcb_render_triangle_t *data; /**<  */
    int                    rem; /**<  */
    int                    index; /**<  */
} xcb_render_triangle_iterator_t;

/**
 * @brief xcb_render_trapezoid_t
 **/
typedef struct xcb_render_trapezoid_t {
    xcb_render_fixed_t   top; /**<  */
    xcb_render_fixed_t   bottom; /**<  */
    xcb_render_linefix_t left; /**<  */
    xcb_render_linefix_t right; /**<  */
} xcb_render_trapezoid_t;

/**
 * @brief xcb_render_trapezoid_iterator_t
 **/
typedef struct xcb_render_trapezoid_iterator_t {
    xcb_render_trapezoid_t *data; /**<  */
    int                     rem; /**<  */
    int                     index; /**<  */
} xcb_render_trapezoid_iterator_t;

/**
 * @brief xcb_render_glyphinfo_t
 **/
typedef struct xcb_render_glyphinfo_t {
    uint16_t width; /**<  */
    uint16_t height; /**<  */
    int16_t  x; /**<  */
    int16_t  y; /**<  */
    int16_t  x_off; /**<  */
    int16_t  y_off; /**<  */
} xcb_render_glyphinfo_t;

/**
 * @brief xcb_render_glyphinfo_iterator_t
 **/
typedef struct xcb_render_glyphinfo_iterator_t {
    xcb_render_glyphinfo_t *data; /**<  */
    int                     rem; /**<  */
    int                     index; /**<  */
} xcb_render_glyphinfo_iterator_t;

/**
 * @brief xcb_render_query_version_cookie_t
 **/
typedef struct xcb_render_query_version_cookie_t {
    unsigned int sequence; /**<  */
} xcb_render_query_version_cookie_t;

/** Opcode for xcb_render_query_version. */
#define XCB_RENDER_QUERY_VERSION 0

/**
 * @brief xcb_render_query_version_request_t
 **/
typedef struct xcb_render_query_version_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint32_t client_major_version; /**<  */
    uint32_t client_minor_version; /**<  */
} xcb_render_query_version_request_t;

/**
 * @brief xcb_render_query_version_reply_t
 **/
typedef struct xcb_render_query_version_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t major_version; /**<  */
    uint32_t minor_version; /**<  */
    uint8_t  pad1[16]; /**<  */
} xcb_render_query_version_reply_t;

/**
 * @brief xcb_render_query_pict_formats_cookie_t
 **/
typedef struct xcb_render_query_pict_formats_cookie_t {
    unsigned int sequence; /**<  */
} xcb_render_query_pict_formats_cookie_t;

/** Opcode for xcb_render_query_pict_formats. */
#define XCB_RENDER_QUERY_PICT_FORMATS 1

/**
 * @brief xcb_render_query_pict_formats_request_t
 **/
typedef struct xcb_render_query_pict_formats_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
} xcb_render_query_pict_formats_request_t;

/**
 * @brief xcb_render_query_pict_formats_reply_t
 **/
typedef struct xcb_render_query_pict_formats_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t num_formats; /**<  */
    uint32_t num_screens; /**<  */
    uint32_t num_depths; /**<  */
    uint32_t num_visuals; /**<  */
    uint32_t num_subpixel; /**<  */
    uint8_t  pad1[4]; /**<  */
} xcb_render_query_pict_formats_reply_t;

/**
 * @brief xcb_render_query_pict_index_values_cookie_t
 **/
typedef struct xcb_render_query_pict_index_values_cookie_t {
    unsigned int sequence; /**<  */
} xcb_render_query_pict_index_values_cookie_t;

/** Opcode for xcb_render_query_pict_index_values. */
#define XCB_RENDER_QUERY_PICT_INDEX_VALUES 2

/**
 * @brief xcb_render_query_pict_index_values_request_t
 **/
typedef struct xcb_render_query_pict_index_values_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    xcb_render_pictformat_t format; /**<  */
} xcb_render_query_pict_index_values_request_t;

/**
 * @brief xcb_render_query_pict_index_values_reply_t
 **/
typedef struct xcb_render_query_pict_index_values_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t num_values; /**<  */
    uint8_t  pad1[20]; /**<  */
} xcb_render_query_pict_index_values_reply_t;

/** Opcode for xcb_render_create_picture. */
#define XCB_RENDER_CREATE_PICTURE 4

/**
 * @brief xcb_render_create_picture_request_t
 **/
typedef struct xcb_render_create_picture_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    xcb_render_picture_t    pid; /**<  */
    xcb_drawable_t          drawable; /**<  */
    xcb_render_pictformat_t format; /**<  */
    uint32_t                value_mask; /**<  */
} xcb_render_create_picture_request_t;

/** Opcode for xcb_render_change_picture. */
#define XCB_RENDER_CHANGE_PICTURE 5

/**
 * @brief xcb_render_change_picture_request_t
 **/
typedef struct xcb_render_change_picture_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
    uint32_t             value_mask; /**<  */
} xcb_render_change_picture_request_t;

/** Opcode for xcb_render_set_picture_clip_rectangles. */
#define XCB_RENDER_SET_PICTURE_CLIP_RECTANGLES 6

/**
 * @brief xcb_render_set_picture_clip_rectangles_request_t
 **/
typedef struct xcb_render_set_picture_clip_rectangles_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
    int16_t              clip_x_origin; /**<  */
    int16_t              clip_y_origin; /**<  */
} xcb_render_set_picture_clip_rectangles_request_t;

/** Opcode for xcb_render_free_picture. */
#define XCB_RENDER_FREE_PICTURE 7

/**
 * @brief xcb_render_free_picture_request_t
 **/
typedef struct xcb_render_free_picture_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
} xcb_render_free_picture_request_t;

/** Opcode for xcb_render_composite. */
#define XCB_RENDER_COMPOSITE 8

/**
 * @brief xcb_render_composite_request_t
 **/
typedef struct xcb_render_composite_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    uint8_t              op; /**<  */
    uint8_t              pad0[3]; /**<  */
    xcb_render_picture_t src; /**<  */
    xcb_render_picture_t mask; /**<  */
    xcb_render_picture_t dst; /**<  */
    int16_t              src_x; /**<  */
    int16_t              src_y; /**<  */
    int16_t              mask_x; /**<  */
    int16_t              mask_y; /**<  */
    int16_t              dst_x; /**<  */
    int16_t              dst_y; /**<  */
    uint16_t             width; /**<  */
    uint16_t             height; /**<  */
} xcb_render_composite_request_t;

/** Opcode for xcb_render_trapezoids. */
#define XCB_RENDER_TRAPEZOIDS 10

/**
 * @brief xcb_render_trapezoids_request_t
 **/
typedef struct xcb_render_trapezoids_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_trapezoids_request_t;

/** Opcode for xcb_render_triangles. */
#define XCB_RENDER_TRIANGLES 11

/**
 * @brief xcb_render_triangles_request_t
 **/
typedef struct xcb_render_triangles_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_triangles_request_t;

/** Opcode for xcb_render_tri_strip. */
#define XCB_RENDER_TRI_STRIP 12

/**
 * @brief xcb_render_tri_strip_request_t
 **/
typedef struct xcb_render_tri_strip_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_tri_strip_request_t;

/** Opcode for xcb_render_tri_fan. */
#define XCB_RENDER_TRI_FAN 13

/**
 * @brief xcb_render_tri_fan_request_t
 **/
typedef struct xcb_render_tri_fan_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_tri_fan_request_t;

/** Opcode for xcb_render_create_glyph_set. */
#define XCB_RENDER_CREATE_GLYPH_SET 17

/**
 * @brief xcb_render_create_glyph_set_request_t
 **/
typedef struct xcb_render_create_glyph_set_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    xcb_render_glyphset_t   gsid; /**<  */
    xcb_render_pictformat_t format; /**<  */
} xcb_render_create_glyph_set_request_t;

/** Opcode for xcb_render_reference_glyph_set. */
#define XCB_RENDER_REFERENCE_GLYPH_SET 18

/**
 * @brief xcb_render_reference_glyph_set_request_t
 **/
typedef struct xcb_render_reference_glyph_set_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_glyphset_t gsid; /**<  */
    xcb_render_glyphset_t existing; /**<  */
} xcb_render_reference_glyph_set_request_t;

/** Opcode for xcb_render_free_glyph_set. */
#define XCB_RENDER_FREE_GLYPH_SET 19

/**
 * @brief xcb_render_free_glyph_set_request_t
 **/
typedef struct xcb_render_free_glyph_set_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_glyphset_t glyphset; /**<  */
} xcb_render_free_glyph_set_request_t;

/** Opcode for xcb_render_add_glyphs. */
#define XCB_RENDER_ADD_GLYPHS 20

/**
 * @brief xcb_render_add_glyphs_request_t
 **/
typedef struct xcb_render_add_glyphs_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_glyphset_t glyphset; /**<  */
    uint32_t              glyphs_len; /**<  */
} xcb_render_add_glyphs_request_t;

/** Opcode for xcb_render_free_glyphs. */
#define XCB_RENDER_FREE_GLYPHS 22

/**
 * @brief xcb_render_free_glyphs_request_t
 **/
typedef struct xcb_render_free_glyphs_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_glyphset_t glyphset; /**<  */
} xcb_render_free_glyphs_request_t;

/** Opcode for xcb_render_composite_glyphs_8. */
#define XCB_RENDER_COMPOSITE_GLYPHS_8 23

/**
 * @brief xcb_render_composite_glyphs_8_request_t
 **/
typedef struct xcb_render_composite_glyphs_8_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    xcb_render_glyphset_t   glyphset; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_composite_glyphs_8_request_t;

/** Opcode for xcb_render_composite_glyphs_16. */
#define XCB_RENDER_COMPOSITE_GLYPHS_16 24

/**
 * @brief xcb_render_composite_glyphs_16_request_t
 **/
typedef struct xcb_render_composite_glyphs_16_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    xcb_render_glyphset_t   glyphset; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_composite_glyphs_16_request_t;

/** Opcode for xcb_render_composite_glyphs_32. */
#define XCB_RENDER_COMPOSITE_GLYPHS_32 25

/**
 * @brief xcb_render_composite_glyphs_32_request_t
 **/
typedef struct xcb_render_composite_glyphs_32_request_t {
    uint8_t                 major_opcode; /**<  */
    uint8_t                 minor_opcode; /**<  */
    uint16_t                length; /**<  */
    uint8_t                 op; /**<  */
    uint8_t                 pad0[3]; /**<  */
    xcb_render_picture_t    src; /**<  */
    xcb_render_picture_t    dst; /**<  */
    xcb_render_pictformat_t mask_format; /**<  */
    xcb_render_glyphset_t   glyphset; /**<  */
    int16_t                 src_x; /**<  */
    int16_t                 src_y; /**<  */
} xcb_render_composite_glyphs_32_request_t;

/** Opcode for xcb_render_fill_rectangles. */
#define XCB_RENDER_FILL_RECTANGLES 26

/**
 * @brief xcb_render_fill_rectangles_request_t
 **/
typedef struct xcb_render_fill_rectangles_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    uint8_t              op; /**<  */
    uint8_t              pad0[3]; /**<  */
    xcb_render_picture_t dst; /**<  */
    xcb_render_color_t   color; /**<  */
} xcb_render_fill_rectangles_request_t;

/** Opcode for xcb_render_create_cursor. */
#define XCB_RENDER_CREATE_CURSOR 27

/**
 * @brief xcb_render_create_cursor_request_t
 **/
typedef struct xcb_render_create_cursor_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_cursor_t         cid; /**<  */
    xcb_render_picture_t source; /**<  */
    uint16_t             x; /**<  */
    uint16_t             y; /**<  */
} xcb_render_create_cursor_request_t;

/**
 * @brief xcb_render_transform_t
 **/
typedef struct xcb_render_transform_t {
    xcb_render_fixed_t matrix11; /**<  */
    xcb_render_fixed_t matrix12; /**<  */
    xcb_render_fixed_t matrix13; /**<  */
    xcb_render_fixed_t matrix21; /**<  */
    xcb_render_fixed_t matrix22; /**<  */
    xcb_render_fixed_t matrix23; /**<  */
    xcb_render_fixed_t matrix31; /**<  */
    xcb_render_fixed_t matrix32; /**<  */
    xcb_render_fixed_t matrix33; /**<  */
} xcb_render_transform_t;

/**
 * @brief xcb_render_transform_iterator_t
 **/
typedef struct xcb_render_transform_iterator_t {
    xcb_render_transform_t *data; /**<  */
    int                     rem; /**<  */
    int                     index; /**<  */
} xcb_render_transform_iterator_t;

/** Opcode for xcb_render_set_picture_transform. */
#define XCB_RENDER_SET_PICTURE_TRANSFORM 28

/**
 * @brief xcb_render_set_picture_transform_request_t
 **/
typedef struct xcb_render_set_picture_transform_request_t {
    uint8_t                major_opcode; /**<  */
    uint8_t                minor_opcode; /**<  */
    uint16_t               length; /**<  */
    xcb_render_picture_t   picture; /**<  */
    xcb_render_transform_t transform; /**<  */
} xcb_render_set_picture_transform_request_t;

/**
 * @brief xcb_render_query_filters_cookie_t
 **/
typedef struct xcb_render_query_filters_cookie_t {
    unsigned int sequence; /**<  */
} xcb_render_query_filters_cookie_t;

/** Opcode for xcb_render_query_filters. */
#define XCB_RENDER_QUERY_FILTERS 29

/**
 * @brief xcb_render_query_filters_request_t
 **/
typedef struct xcb_render_query_filters_request_t {
    uint8_t        major_opcode; /**<  */
    uint8_t        minor_opcode; /**<  */
    uint16_t       length; /**<  */
    xcb_drawable_t drawable; /**<  */
} xcb_render_query_filters_request_t;

/**
 * @brief xcb_render_query_filters_reply_t
 **/
typedef struct xcb_render_query_filters_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t num_aliases; /**<  */
    uint32_t num_filters; /**<  */
    uint8_t  pad1[16]; /**<  */
} xcb_render_query_filters_reply_t;

/** Opcode for xcb_render_set_picture_filter. */
#define XCB_RENDER_SET_PICTURE_FILTER 30

/**
 * @brief xcb_render_set_picture_filter_request_t
 **/
typedef struct xcb_render_set_picture_filter_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
    uint16_t             filter_len; /**<  */
    uint8_t              pad0[2]; /**<  */
} xcb_render_set_picture_filter_request_t;

/**
 * @brief xcb_render_animcursorelt_t
 **/
typedef struct xcb_render_animcursorelt_t {
    xcb_cursor_t cursor; /**<  */
    uint32_t     delay; /**<  */
} xcb_render_animcursorelt_t;

/**
 * @brief xcb_render_animcursorelt_iterator_t
 **/
typedef struct xcb_render_animcursorelt_iterator_t {
    xcb_render_animcursorelt_t *data; /**<  */
    int                         rem; /**<  */
    int                         index; /**<  */
} xcb_render_animcursorelt_iterator_t;

/** Opcode for xcb_render_create_anim_cursor. */
#define XCB_RENDER_CREATE_ANIM_CURSOR 31

/**
 * @brief xcb_render_create_anim_cursor_request_t
 **/
typedef struct xcb_render_create_anim_cursor_request_t {
    uint8_t      major_opcode; /**<  */
    uint8_t      minor_opcode; /**<  */
    uint16_t     length; /**<  */
    xcb_cursor_t cid; /**<  */
} xcb_render_create_anim_cursor_request_t;

/**
 * @brief xcb_render_spanfix_t
 **/
typedef struct xcb_render_spanfix_t {
    xcb_render_fixed_t l; /**<  */
    xcb_render_fixed_t r; /**<  */
    xcb_render_fixed_t y; /**<  */
} xcb_render_spanfix_t;

/**
 * @brief xcb_render_spanfix_iterator_t
 **/
typedef struct xcb_render_spanfix_iterator_t {
    xcb_render_spanfix_t *data; /**<  */
    int                   rem; /**<  */
    int                   index; /**<  */
} xcb_render_spanfix_iterator_t;

/**
 * @brief xcb_render_trap_t
 **/
typedef struct xcb_render_trap_t {
    xcb_render_spanfix_t top; /**<  */
    xcb_render_spanfix_t bot; /**<  */
} xcb_render_trap_t;

/**
 * @brief xcb_render_trap_iterator_t
 **/
typedef struct xcb_render_trap_iterator_t {
    xcb_render_trap_t *data; /**<  */
    int                rem; /**<  */
    int                index; /**<  */
} xcb_render_trap_iterator_t;

/** Opcode for xcb_render_add_traps. */
#define XCB_RENDER_ADD_TRAPS 32

/**
 * @brief xcb_render_add_traps_request_t
 **/
typedef struct xcb_render_add_traps_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
    int16_t              x_off; /**<  */
    int16_t              y_off; /**<  */
} xcb_render_add_traps_request_t;

/** Opcode for xcb_render_create_solid_fill. */
#define XCB_RENDER_CREATE_SOLID_FILL 33

/**
 * @brief xcb_render_create_solid_fill_request_t
 **/
typedef struct xcb_render_create_solid_fill_request_t {
    uint8_t              major_opcode; /**<  */
    uint8_t              minor_opcode; /**<  */
    uint16_t             length; /**<  */
    xcb_render_picture_t picture; /**<  */
    xcb_render_color_t   color; /**<  */
} xcb_render_create_solid_fill_request_t;

/** Opcode for xcb_render_create_linear_gradient. */
#define XCB_RENDER_CREATE_LINEAR_GRADIENT 34

/**
 * @brief xcb_render_create_linear_gradient_request_t
 **/
typedef struct xcb_render_create_linear_gradient_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_picture_t  picture; /**<  */
    xcb_render_pointfix_t p1; /**<  */
    xcb_render_pointfix_t p2; /**<  */
    uint32_t              num_stops; /**<  */
} xcb_render_create_linear_gradient_request_t;

/** Opcode for xcb_render_create_radial_gradient. */
#define XCB_RENDER_CREATE_RADIAL_GRADIENT 35

/**
 * @brief xcb_render_create_radial_gradient_request_t
 **/
typedef struct xcb_render_create_radial_gradient_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_picture_t  picture; /**<  */
    xcb_render_pointfix_t inner; /**<  */
    xcb_render_pointfix_t outer; /**<  */
    xcb_render_fixed_t    inner_radius; /**<  */
    xcb_render_fixed_t    outer_radius; /**<  */
    uint32_t              num_stops; /**<  */
} xcb_render_create_radial_gradient_request_t;

/** Opcode for xcb_render_create_conical_gradient. */
#define XCB_RENDER_CREATE_CONICAL_GRADIENT 36

/**
 * @brief xcb_render_create_conical_gradient_request_t
 **/
typedef struct xcb_render_create_conical_gradient_request_t {
    uint8_t               major_opcode; /**<  */
    uint8_t               minor_opcode; /**<  */
    uint16_t              length; /**<  */
    xcb_render_picture_t  picture; /**<  */
    xcb_render_pointfix_t center; /**<  */
    xcb_render_fixed_t    angle; /**<  */
    uint32_t              num_stops; /**<  */
} xcb_render_create_conical_gradient_request_t;

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_glyph_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_glyph_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_glyph_next
 ** 
 ** @param xcb_render_glyph_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_glyph_next (xcb_render_glyph_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_glyph_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_glyph_end
 ** 
 ** @param xcb_render_glyph_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_glyph_end (xcb_render_glyph_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_glyphset_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_glyphset_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_glyphset_next
 ** 
 ** @param xcb_render_glyphset_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_glyphset_next (xcb_render_glyphset_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_glyphset_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_glyphset_end
 ** 
 ** @param xcb_render_glyphset_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_glyphset_end (xcb_render_glyphset_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_picture_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_picture_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_picture_next
 ** 
 ** @param xcb_render_picture_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_picture_next (xcb_render_picture_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_picture_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_picture_end
 ** 
 ** @param xcb_render_picture_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_picture_end (xcb_render_picture_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_pictformat_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_pictformat_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_pictformat_next
 ** 
 ** @param xcb_render_pictformat_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_pictformat_next (xcb_render_pictformat_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_pictformat_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_pictformat_end
 ** 
 ** @param xcb_render_pictformat_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_pictformat_end (xcb_render_pictformat_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_fixed_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_fixed_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_fixed_next
 ** 
 ** @param xcb_render_fixed_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_fixed_next (xcb_render_fixed_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_fixed_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_fixed_end
 ** 
 ** @param xcb_render_fixed_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_fixed_end (xcb_render_fixed_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_directformat_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_directformat_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_directformat_next
 ** 
 ** @param xcb_render_directformat_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_directformat_next (xcb_render_directformat_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_directformat_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_directformat_end
 ** 
 ** @param xcb_render_directformat_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_directformat_end (xcb_render_directformat_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_pictforminfo_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_pictforminfo_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_pictforminfo_next
 ** 
 ** @param xcb_render_pictforminfo_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_pictforminfo_next (xcb_render_pictforminfo_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_pictforminfo_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_pictforminfo_end
 ** 
 ** @param xcb_render_pictforminfo_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_pictforminfo_end (xcb_render_pictforminfo_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_pictvisual_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_pictvisual_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_pictvisual_next
 ** 
 ** @param xcb_render_pictvisual_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_pictvisual_next (xcb_render_pictvisual_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_pictvisual_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_pictvisual_end
 ** 
 ** @param xcb_render_pictvisual_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_pictvisual_end (xcb_render_pictvisual_iterator_t i  /**< */);


/*****************************************************************************
 **
 ** xcb_render_pictvisual_t * xcb_render_pictdepth_visuals
 ** 
 ** @param const xcb_render_pictdepth_t *R
 ** @returns xcb_render_pictvisual_t *
 **
 *****************************************************************************/
 
xcb_render_pictvisual_t *
xcb_render_pictdepth_visuals (const xcb_render_pictdepth_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_pictdepth_visuals_length
 ** 
 ** @param const xcb_render_pictdepth_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_pictdepth_visuals_length (const xcb_render_pictdepth_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_pictvisual_iterator_t xcb_render_pictdepth_visuals_iterator
 ** 
 ** @param const xcb_render_pictdepth_t *R
 ** @returns xcb_render_pictvisual_iterator_t
 **
 *****************************************************************************/
 
xcb_render_pictvisual_iterator_t
xcb_render_pictdepth_visuals_iterator (const xcb_render_pictdepth_t *R  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_pictdepth_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_pictdepth_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_pictdepth_next
 ** 
 ** @param xcb_render_pictdepth_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_pictdepth_next (xcb_render_pictdepth_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_pictdepth_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_pictdepth_end
 ** 
 ** @param xcb_render_pictdepth_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_pictdepth_end (xcb_render_pictdepth_iterator_t i  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_pictscreen_depths_length
 ** 
 ** @param const xcb_render_pictscreen_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_pictscreen_depths_length (const xcb_render_pictscreen_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_pictdepth_iterator_t xcb_render_pictscreen_depths_iterator
 ** 
 ** @param const xcb_render_pictscreen_t *R
 ** @returns xcb_render_pictdepth_iterator_t
 **
 *****************************************************************************/
 
xcb_render_pictdepth_iterator_t
xcb_render_pictscreen_depths_iterator (const xcb_render_pictscreen_t *R  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_pictscreen_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_pictscreen_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_pictscreen_next
 ** 
 ** @param xcb_render_pictscreen_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_pictscreen_next (xcb_render_pictscreen_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_pictscreen_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_pictscreen_end
 ** 
 ** @param xcb_render_pictscreen_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_pictscreen_end (xcb_render_pictscreen_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_indexvalue_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_indexvalue_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_indexvalue_next
 ** 
 ** @param xcb_render_indexvalue_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_indexvalue_next (xcb_render_indexvalue_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_indexvalue_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_indexvalue_end
 ** 
 ** @param xcb_render_indexvalue_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_indexvalue_end (xcb_render_indexvalue_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_color_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_color_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_color_next
 ** 
 ** @param xcb_render_color_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_color_next (xcb_render_color_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_color_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_color_end
 ** 
 ** @param xcb_render_color_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_color_end (xcb_render_color_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_pointfix_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_pointfix_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_pointfix_next
 ** 
 ** @param xcb_render_pointfix_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_pointfix_next (xcb_render_pointfix_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_pointfix_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_pointfix_end
 ** 
 ** @param xcb_render_pointfix_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_pointfix_end (xcb_render_pointfix_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_linefix_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_linefix_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_linefix_next
 ** 
 ** @param xcb_render_linefix_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_linefix_next (xcb_render_linefix_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_linefix_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_linefix_end
 ** 
 ** @param xcb_render_linefix_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_linefix_end (xcb_render_linefix_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_triangle_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_triangle_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_triangle_next
 ** 
 ** @param xcb_render_triangle_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_triangle_next (xcb_render_triangle_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_triangle_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_triangle_end
 ** 
 ** @param xcb_render_triangle_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_triangle_end (xcb_render_triangle_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_trapezoid_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_trapezoid_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_trapezoid_next
 ** 
 ** @param xcb_render_trapezoid_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_trapezoid_next (xcb_render_trapezoid_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_trapezoid_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_trapezoid_end
 ** 
 ** @param xcb_render_trapezoid_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_trapezoid_end (xcb_render_trapezoid_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_glyphinfo_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_glyphinfo_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_glyphinfo_next
 ** 
 ** @param xcb_render_glyphinfo_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_glyphinfo_next (xcb_render_glyphinfo_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_glyphinfo_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_glyphinfo_end
 ** 
 ** @param xcb_render_glyphinfo_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_glyphinfo_end (xcb_render_glyphinfo_iterator_t i  /**< */);

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
 ** xcb_render_query_version_cookie_t xcb_render_query_version
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          client_major_version
 ** @param uint32_t          client_minor_version
 ** @returns xcb_render_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_version_cookie_t
xcb_render_query_version (xcb_connection_t *c  /**< */,
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
 ** xcb_render_query_version_cookie_t xcb_render_query_version_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          client_major_version
 ** @param uint32_t          client_minor_version
 ** @returns xcb_render_query_version_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_version_cookie_t
xcb_render_query_version_unchecked (xcb_connection_t *c  /**< */,
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
 * xcb_render_query_version_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_render_query_version_reply_t * xcb_render_query_version_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_render_query_version_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_render_query_version_reply_t *
 **
 *****************************************************************************/
 
xcb_render_query_version_reply_t *
xcb_render_query_version_reply (xcb_connection_t                   *c  /**< */,
                                xcb_render_query_version_cookie_t   cookie  /**< */,
                                xcb_generic_error_t               **e  /**< */);

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
 ** xcb_render_query_pict_formats_cookie_t xcb_render_query_pict_formats
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_render_query_pict_formats_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_pict_formats_cookie_t
xcb_render_query_pict_formats (xcb_connection_t *c  /**< */);

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
 ** xcb_render_query_pict_formats_cookie_t xcb_render_query_pict_formats_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_render_query_pict_formats_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_pict_formats_cookie_t
xcb_render_query_pict_formats_unchecked (xcb_connection_t *c  /**< */);


/*****************************************************************************
 **
 ** xcb_render_pictforminfo_t * xcb_render_query_pict_formats_formats
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns xcb_render_pictforminfo_t *
 **
 *****************************************************************************/
 
xcb_render_pictforminfo_t *
xcb_render_query_pict_formats_formats (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_query_pict_formats_formats_length
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_query_pict_formats_formats_length (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_pictforminfo_iterator_t xcb_render_query_pict_formats_formats_iterator
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns xcb_render_pictforminfo_iterator_t
 **
 *****************************************************************************/
 
xcb_render_pictforminfo_iterator_t
xcb_render_query_pict_formats_formats_iterator (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_query_pict_formats_screens_length
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_query_pict_formats_screens_length (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_pictscreen_iterator_t xcb_render_query_pict_formats_screens_iterator
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns xcb_render_pictscreen_iterator_t
 **
 *****************************************************************************/
 
xcb_render_pictscreen_iterator_t
xcb_render_query_pict_formats_screens_iterator (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** uint32_t * xcb_render_query_pict_formats_subpixels
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns uint32_t *
 **
 *****************************************************************************/
 
uint32_t *
xcb_render_query_pict_formats_subpixels (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_query_pict_formats_subpixels_length
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_query_pict_formats_subpixels_length (const xcb_render_query_pict_formats_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_query_pict_formats_subpixels_end
 ** 
 ** @param const xcb_render_query_pict_formats_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_query_pict_formats_subpixels_end (const xcb_render_query_pict_formats_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_render_query_pict_formats_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_render_query_pict_formats_reply_t * xcb_render_query_pict_formats_reply
 ** 
 ** @param xcb_connection_t                        *c
 ** @param xcb_render_query_pict_formats_cookie_t   cookie
 ** @param xcb_generic_error_t                    **e
 ** @returns xcb_render_query_pict_formats_reply_t *
 **
 *****************************************************************************/
 
xcb_render_query_pict_formats_reply_t *
xcb_render_query_pict_formats_reply (xcb_connection_t                        *c  /**< */,
                                     xcb_render_query_pict_formats_cookie_t   cookie  /**< */,
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
 ** xcb_render_query_pict_index_values_cookie_t xcb_render_query_pict_index_values
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_pictformat_t  format
 ** @returns xcb_render_query_pict_index_values_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_pict_index_values_cookie_t
xcb_render_query_pict_index_values (xcb_connection_t        *c  /**< */,
                                    xcb_render_pictformat_t  format  /**< */);

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
 ** xcb_render_query_pict_index_values_cookie_t xcb_render_query_pict_index_values_unchecked
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_pictformat_t  format
 ** @returns xcb_render_query_pict_index_values_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_pict_index_values_cookie_t
xcb_render_query_pict_index_values_unchecked (xcb_connection_t        *c  /**< */,
                                              xcb_render_pictformat_t  format  /**< */);


/*****************************************************************************
 **
 ** xcb_render_indexvalue_t * xcb_render_query_pict_index_values_values
 ** 
 ** @param const xcb_render_query_pict_index_values_reply_t *R
 ** @returns xcb_render_indexvalue_t *
 **
 *****************************************************************************/
 
xcb_render_indexvalue_t *
xcb_render_query_pict_index_values_values (const xcb_render_query_pict_index_values_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_query_pict_index_values_values_length
 ** 
 ** @param const xcb_render_query_pict_index_values_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_query_pict_index_values_values_length (const xcb_render_query_pict_index_values_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_render_indexvalue_iterator_t xcb_render_query_pict_index_values_values_iterator
 ** 
 ** @param const xcb_render_query_pict_index_values_reply_t *R
 ** @returns xcb_render_indexvalue_iterator_t
 **
 *****************************************************************************/
 
xcb_render_indexvalue_iterator_t
xcb_render_query_pict_index_values_values_iterator (const xcb_render_query_pict_index_values_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_render_query_pict_index_values_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_render_query_pict_index_values_reply_t * xcb_render_query_pict_index_values_reply
 ** 
 ** @param xcb_connection_t                             *c
 ** @param xcb_render_query_pict_index_values_cookie_t   cookie
 ** @param xcb_generic_error_t                         **e
 ** @returns xcb_render_query_pict_index_values_reply_t *
 **
 *****************************************************************************/
 
xcb_render_query_pict_index_values_reply_t *
xcb_render_query_pict_index_values_reply (xcb_connection_t                             *c  /**< */,
                                          xcb_render_query_pict_index_values_cookie_t   cookie  /**< */,
                                          xcb_generic_error_t                         **e  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_picture_checked
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_picture_t     pid
 ** @param xcb_drawable_t           drawable
 ** @param xcb_render_pictformat_t  format
 ** @param uint32_t                 value_mask
 ** @param const uint32_t          *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_picture_checked (xcb_connection_t        *c  /**< */,
                                   xcb_render_picture_t     pid  /**< */,
                                   xcb_drawable_t           drawable  /**< */,
                                   xcb_render_pictformat_t  format  /**< */,
                                   uint32_t                 value_mask  /**< */,
                                   const uint32_t          *value_list  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_picture
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_picture_t     pid
 ** @param xcb_drawable_t           drawable
 ** @param xcb_render_pictformat_t  format
 ** @param uint32_t                 value_mask
 ** @param const uint32_t          *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_picture (xcb_connection_t        *c  /**< */,
                           xcb_render_picture_t     pid  /**< */,
                           xcb_drawable_t           drawable  /**< */,
                           xcb_render_pictformat_t  format  /**< */,
                           uint32_t                 value_mask  /**< */,
                           const uint32_t          *value_list  /**< */);

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
 ** xcb_void_cookie_t xcb_render_change_picture_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @param uint32_t              value_mask
 ** @param const uint32_t       *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_change_picture_checked (xcb_connection_t     *c  /**< */,
                                   xcb_render_picture_t  picture  /**< */,
                                   uint32_t              value_mask  /**< */,
                                   const uint32_t       *value_list  /**< */);

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
 ** xcb_void_cookie_t xcb_render_change_picture
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @param uint32_t              value_mask
 ** @param const uint32_t       *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_change_picture (xcb_connection_t     *c  /**< */,
                           xcb_render_picture_t  picture  /**< */,
                           uint32_t              value_mask  /**< */,
                           const uint32_t       *value_list  /**< */);

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
 ** xcb_void_cookie_t xcb_render_set_picture_clip_rectangles_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_render_picture_t   picture
 ** @param int16_t                clip_x_origin
 ** @param int16_t                clip_y_origin
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_set_picture_clip_rectangles_checked (xcb_connection_t      *c  /**< */,
                                                xcb_render_picture_t   picture  /**< */,
                                                int16_t                clip_x_origin  /**< */,
                                                int16_t                clip_y_origin  /**< */,
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
 ** xcb_void_cookie_t xcb_render_set_picture_clip_rectangles
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_render_picture_t   picture
 ** @param int16_t                clip_x_origin
 ** @param int16_t                clip_y_origin
 ** @param uint32_t               rectangles_len
 ** @param const xcb_rectangle_t *rectangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_set_picture_clip_rectangles (xcb_connection_t      *c  /**< */,
                                        xcb_render_picture_t   picture  /**< */,
                                        int16_t                clip_x_origin  /**< */,
                                        int16_t                clip_y_origin  /**< */,
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
 ** xcb_void_cookie_t xcb_render_free_picture_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_free_picture_checked (xcb_connection_t     *c  /**< */,
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
 ** xcb_void_cookie_t xcb_render_free_picture
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_free_picture (xcb_connection_t     *c  /**< */,
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
 ** xcb_void_cookie_t xcb_render_composite_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param uint8_t               op
 ** @param xcb_render_picture_t  src
 ** @param xcb_render_picture_t  mask
 ** @param xcb_render_picture_t  dst
 ** @param int16_t               src_x
 ** @param int16_t               src_y
 ** @param int16_t               mask_x
 ** @param int16_t               mask_y
 ** @param int16_t               dst_x
 ** @param int16_t               dst_y
 ** @param uint16_t              width
 ** @param uint16_t              height
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_checked (xcb_connection_t     *c  /**< */,
                              uint8_t               op  /**< */,
                              xcb_render_picture_t  src  /**< */,
                              xcb_render_picture_t  mask  /**< */,
                              xcb_render_picture_t  dst  /**< */,
                              int16_t               src_x  /**< */,
                              int16_t               src_y  /**< */,
                              int16_t               mask_x  /**< */,
                              int16_t               mask_y  /**< */,
                              int16_t               dst_x  /**< */,
                              int16_t               dst_y  /**< */,
                              uint16_t              width  /**< */,
                              uint16_t              height  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite
 ** 
 ** @param xcb_connection_t     *c
 ** @param uint8_t               op
 ** @param xcb_render_picture_t  src
 ** @param xcb_render_picture_t  mask
 ** @param xcb_render_picture_t  dst
 ** @param int16_t               src_x
 ** @param int16_t               src_y
 ** @param int16_t               mask_x
 ** @param int16_t               mask_y
 ** @param int16_t               dst_x
 ** @param int16_t               dst_y
 ** @param uint16_t              width
 ** @param uint16_t              height
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite (xcb_connection_t     *c  /**< */,
                      uint8_t               op  /**< */,
                      xcb_render_picture_t  src  /**< */,
                      xcb_render_picture_t  mask  /**< */,
                      xcb_render_picture_t  dst  /**< */,
                      int16_t               src_x  /**< */,
                      int16_t               src_y  /**< */,
                      int16_t               mask_x  /**< */,
                      int16_t               mask_y  /**< */,
                      int16_t               dst_x  /**< */,
                      int16_t               dst_y  /**< */,
                      uint16_t              width  /**< */,
                      uint16_t              height  /**< */);

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
 ** xcb_void_cookie_t xcb_render_trapezoids_checked
 ** 
 ** @param xcb_connection_t             *c
 ** @param uint8_t                       op
 ** @param xcb_render_picture_t          src
 ** @param xcb_render_picture_t          dst
 ** @param xcb_render_pictformat_t       mask_format
 ** @param int16_t                       src_x
 ** @param int16_t                       src_y
 ** @param uint32_t                      traps_len
 ** @param const xcb_render_trapezoid_t *traps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_trapezoids_checked (xcb_connection_t             *c  /**< */,
                               uint8_t                       op  /**< */,
                               xcb_render_picture_t          src  /**< */,
                               xcb_render_picture_t          dst  /**< */,
                               xcb_render_pictformat_t       mask_format  /**< */,
                               int16_t                       src_x  /**< */,
                               int16_t                       src_y  /**< */,
                               uint32_t                      traps_len  /**< */,
                               const xcb_render_trapezoid_t *traps  /**< */);

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
 ** xcb_void_cookie_t xcb_render_trapezoids
 ** 
 ** @param xcb_connection_t             *c
 ** @param uint8_t                       op
 ** @param xcb_render_picture_t          src
 ** @param xcb_render_picture_t          dst
 ** @param xcb_render_pictformat_t       mask_format
 ** @param int16_t                       src_x
 ** @param int16_t                       src_y
 ** @param uint32_t                      traps_len
 ** @param const xcb_render_trapezoid_t *traps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_trapezoids (xcb_connection_t             *c  /**< */,
                       uint8_t                       op  /**< */,
                       xcb_render_picture_t          src  /**< */,
                       xcb_render_picture_t          dst  /**< */,
                       xcb_render_pictformat_t       mask_format  /**< */,
                       int16_t                       src_x  /**< */,
                       int16_t                       src_y  /**< */,
                       uint32_t                      traps_len  /**< */,
                       const xcb_render_trapezoid_t *traps  /**< */);

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
 ** xcb_void_cookie_t xcb_render_triangles_checked
 ** 
 ** @param xcb_connection_t            *c
 ** @param uint8_t                      op
 ** @param xcb_render_picture_t         src
 ** @param xcb_render_picture_t         dst
 ** @param xcb_render_pictformat_t      mask_format
 ** @param int16_t                      src_x
 ** @param int16_t                      src_y
 ** @param uint32_t                     triangles_len
 ** @param const xcb_render_triangle_t *triangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_triangles_checked (xcb_connection_t            *c  /**< */,
                              uint8_t                      op  /**< */,
                              xcb_render_picture_t         src  /**< */,
                              xcb_render_picture_t         dst  /**< */,
                              xcb_render_pictformat_t      mask_format  /**< */,
                              int16_t                      src_x  /**< */,
                              int16_t                      src_y  /**< */,
                              uint32_t                     triangles_len  /**< */,
                              const xcb_render_triangle_t *triangles  /**< */);

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
 ** xcb_void_cookie_t xcb_render_triangles
 ** 
 ** @param xcb_connection_t            *c
 ** @param uint8_t                      op
 ** @param xcb_render_picture_t         src
 ** @param xcb_render_picture_t         dst
 ** @param xcb_render_pictformat_t      mask_format
 ** @param int16_t                      src_x
 ** @param int16_t                      src_y
 ** @param uint32_t                     triangles_len
 ** @param const xcb_render_triangle_t *triangles
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_triangles (xcb_connection_t            *c  /**< */,
                      uint8_t                      op  /**< */,
                      xcb_render_picture_t         src  /**< */,
                      xcb_render_picture_t         dst  /**< */,
                      xcb_render_pictformat_t      mask_format  /**< */,
                      int16_t                      src_x  /**< */,
                      int16_t                      src_y  /**< */,
                      uint32_t                     triangles_len  /**< */,
                      const xcb_render_triangle_t *triangles  /**< */);

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
 ** xcb_void_cookie_t xcb_render_tri_strip_checked
 ** 
 ** @param xcb_connection_t            *c
 ** @param uint8_t                      op
 ** @param xcb_render_picture_t         src
 ** @param xcb_render_picture_t         dst
 ** @param xcb_render_pictformat_t      mask_format
 ** @param int16_t                      src_x
 ** @param int16_t                      src_y
 ** @param uint32_t                     points_len
 ** @param const xcb_render_pointfix_t *points
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_tri_strip_checked (xcb_connection_t            *c  /**< */,
                              uint8_t                      op  /**< */,
                              xcb_render_picture_t         src  /**< */,
                              xcb_render_picture_t         dst  /**< */,
                              xcb_render_pictformat_t      mask_format  /**< */,
                              int16_t                      src_x  /**< */,
                              int16_t                      src_y  /**< */,
                              uint32_t                     points_len  /**< */,
                              const xcb_render_pointfix_t *points  /**< */);

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
 ** xcb_void_cookie_t xcb_render_tri_strip
 ** 
 ** @param xcb_connection_t            *c
 ** @param uint8_t                      op
 ** @param xcb_render_picture_t         src
 ** @param xcb_render_picture_t         dst
 ** @param xcb_render_pictformat_t      mask_format
 ** @param int16_t                      src_x
 ** @param int16_t                      src_y
 ** @param uint32_t                     points_len
 ** @param const xcb_render_pointfix_t *points
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_tri_strip (xcb_connection_t            *c  /**< */,
                      uint8_t                      op  /**< */,
                      xcb_render_picture_t         src  /**< */,
                      xcb_render_picture_t         dst  /**< */,
                      xcb_render_pictformat_t      mask_format  /**< */,
                      int16_t                      src_x  /**< */,
                      int16_t                      src_y  /**< */,
                      uint32_t                     points_len  /**< */,
                      const xcb_render_pointfix_t *points  /**< */);

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
 ** xcb_void_cookie_t xcb_render_tri_fan_checked
 ** 
 ** @param xcb_connection_t            *c
 ** @param uint8_t                      op
 ** @param xcb_render_picture_t         src
 ** @param xcb_render_picture_t         dst
 ** @param xcb_render_pictformat_t      mask_format
 ** @param int16_t                      src_x
 ** @param int16_t                      src_y
 ** @param uint32_t                     points_len
 ** @param const xcb_render_pointfix_t *points
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_tri_fan_checked (xcb_connection_t            *c  /**< */,
                            uint8_t                      op  /**< */,
                            xcb_render_picture_t         src  /**< */,
                            xcb_render_picture_t         dst  /**< */,
                            xcb_render_pictformat_t      mask_format  /**< */,
                            int16_t                      src_x  /**< */,
                            int16_t                      src_y  /**< */,
                            uint32_t                     points_len  /**< */,
                            const xcb_render_pointfix_t *points  /**< */);

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
 ** xcb_void_cookie_t xcb_render_tri_fan
 ** 
 ** @param xcb_connection_t            *c
 ** @param uint8_t                      op
 ** @param xcb_render_picture_t         src
 ** @param xcb_render_picture_t         dst
 ** @param xcb_render_pictformat_t      mask_format
 ** @param int16_t                      src_x
 ** @param int16_t                      src_y
 ** @param uint32_t                     points_len
 ** @param const xcb_render_pointfix_t *points
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_tri_fan (xcb_connection_t            *c  /**< */,
                    uint8_t                      op  /**< */,
                    xcb_render_picture_t         src  /**< */,
                    xcb_render_picture_t         dst  /**< */,
                    xcb_render_pictformat_t      mask_format  /**< */,
                    int16_t                      src_x  /**< */,
                    int16_t                      src_y  /**< */,
                    uint32_t                     points_len  /**< */,
                    const xcb_render_pointfix_t *points  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_glyph_set_checked
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_glyphset_t    gsid
 ** @param xcb_render_pictformat_t  format
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_glyph_set_checked (xcb_connection_t        *c  /**< */,
                                     xcb_render_glyphset_t    gsid  /**< */,
                                     xcb_render_pictformat_t  format  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_glyph_set
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_glyphset_t    gsid
 ** @param xcb_render_pictformat_t  format
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_glyph_set (xcb_connection_t        *c  /**< */,
                             xcb_render_glyphset_t    gsid  /**< */,
                             xcb_render_pictformat_t  format  /**< */);

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
 ** xcb_void_cookie_t xcb_render_reference_glyph_set_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_render_glyphset_t  gsid
 ** @param xcb_render_glyphset_t  existing
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_reference_glyph_set_checked (xcb_connection_t      *c  /**< */,
                                        xcb_render_glyphset_t  gsid  /**< */,
                                        xcb_render_glyphset_t  existing  /**< */);

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
 ** xcb_void_cookie_t xcb_render_reference_glyph_set
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_render_glyphset_t  gsid
 ** @param xcb_render_glyphset_t  existing
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_reference_glyph_set (xcb_connection_t      *c  /**< */,
                                xcb_render_glyphset_t  gsid  /**< */,
                                xcb_render_glyphset_t  existing  /**< */);

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
 ** xcb_void_cookie_t xcb_render_free_glyph_set_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_render_glyphset_t  glyphset
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_free_glyph_set_checked (xcb_connection_t      *c  /**< */,
                                   xcb_render_glyphset_t  glyphset  /**< */);

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
 ** xcb_void_cookie_t xcb_render_free_glyph_set
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_render_glyphset_t  glyphset
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_free_glyph_set (xcb_connection_t      *c  /**< */,
                           xcb_render_glyphset_t  glyphset  /**< */);

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
 ** xcb_void_cookie_t xcb_render_add_glyphs_checked
 ** 
 ** @param xcb_connection_t             *c
 ** @param xcb_render_glyphset_t         glyphset
 ** @param uint32_t                      glyphs_len
 ** @param const uint32_t               *glyphids
 ** @param const xcb_render_glyphinfo_t *glyphs
 ** @param uint32_t                      data_len
 ** @param const uint8_t                *data
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_add_glyphs_checked (xcb_connection_t             *c  /**< */,
                               xcb_render_glyphset_t         glyphset  /**< */,
                               uint32_t                      glyphs_len  /**< */,
                               const uint32_t               *glyphids  /**< */,
                               const xcb_render_glyphinfo_t *glyphs  /**< */,
                               uint32_t                      data_len  /**< */,
                               const uint8_t                *data  /**< */);

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
 ** xcb_void_cookie_t xcb_render_add_glyphs
 ** 
 ** @param xcb_connection_t             *c
 ** @param xcb_render_glyphset_t         glyphset
 ** @param uint32_t                      glyphs_len
 ** @param const uint32_t               *glyphids
 ** @param const xcb_render_glyphinfo_t *glyphs
 ** @param uint32_t                      data_len
 ** @param const uint8_t                *data
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_add_glyphs (xcb_connection_t             *c  /**< */,
                       xcb_render_glyphset_t         glyphset  /**< */,
                       uint32_t                      glyphs_len  /**< */,
                       const uint32_t               *glyphids  /**< */,
                       const xcb_render_glyphinfo_t *glyphs  /**< */,
                       uint32_t                      data_len  /**< */,
                       const uint8_t                *data  /**< */);

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
 ** xcb_void_cookie_t xcb_render_free_glyphs_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_glyphset_t     glyphset
 ** @param uint32_t                  glyphs_len
 ** @param const xcb_render_glyph_t *glyphs
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_free_glyphs_checked (xcb_connection_t         *c  /**< */,
                                xcb_render_glyphset_t     glyphset  /**< */,
                                uint32_t                  glyphs_len  /**< */,
                                const xcb_render_glyph_t *glyphs  /**< */);

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
 ** xcb_void_cookie_t xcb_render_free_glyphs
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_glyphset_t     glyphset
 ** @param uint32_t                  glyphs_len
 ** @param const xcb_render_glyph_t *glyphs
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_free_glyphs (xcb_connection_t         *c  /**< */,
                        xcb_render_glyphset_t     glyphset  /**< */,
                        uint32_t                  glyphs_len  /**< */,
                        const xcb_render_glyph_t *glyphs  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite_glyphs_8_checked
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint8_t                  op
 ** @param xcb_render_picture_t     src
 ** @param xcb_render_picture_t     dst
 ** @param xcb_render_pictformat_t  mask_format
 ** @param xcb_render_glyphset_t    glyphset
 ** @param int16_t                  src_x
 ** @param int16_t                  src_y
 ** @param uint32_t                 glyphcmds_len
 ** @param const uint8_t           *glyphcmds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_glyphs_8_checked (xcb_connection_t        *c  /**< */,
                                       uint8_t                  op  /**< */,
                                       xcb_render_picture_t     src  /**< */,
                                       xcb_render_picture_t     dst  /**< */,
                                       xcb_render_pictformat_t  mask_format  /**< */,
                                       xcb_render_glyphset_t    glyphset  /**< */,
                                       int16_t                  src_x  /**< */,
                                       int16_t                  src_y  /**< */,
                                       uint32_t                 glyphcmds_len  /**< */,
                                       const uint8_t           *glyphcmds  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite_glyphs_8
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint8_t                  op
 ** @param xcb_render_picture_t     src
 ** @param xcb_render_picture_t     dst
 ** @param xcb_render_pictformat_t  mask_format
 ** @param xcb_render_glyphset_t    glyphset
 ** @param int16_t                  src_x
 ** @param int16_t                  src_y
 ** @param uint32_t                 glyphcmds_len
 ** @param const uint8_t           *glyphcmds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_glyphs_8 (xcb_connection_t        *c  /**< */,
                               uint8_t                  op  /**< */,
                               xcb_render_picture_t     src  /**< */,
                               xcb_render_picture_t     dst  /**< */,
                               xcb_render_pictformat_t  mask_format  /**< */,
                               xcb_render_glyphset_t    glyphset  /**< */,
                               int16_t                  src_x  /**< */,
                               int16_t                  src_y  /**< */,
                               uint32_t                 glyphcmds_len  /**< */,
                               const uint8_t           *glyphcmds  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite_glyphs_16_checked
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint8_t                  op
 ** @param xcb_render_picture_t     src
 ** @param xcb_render_picture_t     dst
 ** @param xcb_render_pictformat_t  mask_format
 ** @param xcb_render_glyphset_t    glyphset
 ** @param int16_t                  src_x
 ** @param int16_t                  src_y
 ** @param uint32_t                 glyphcmds_len
 ** @param const uint8_t           *glyphcmds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_glyphs_16_checked (xcb_connection_t        *c  /**< */,
                                        uint8_t                  op  /**< */,
                                        xcb_render_picture_t     src  /**< */,
                                        xcb_render_picture_t     dst  /**< */,
                                        xcb_render_pictformat_t  mask_format  /**< */,
                                        xcb_render_glyphset_t    glyphset  /**< */,
                                        int16_t                  src_x  /**< */,
                                        int16_t                  src_y  /**< */,
                                        uint32_t                 glyphcmds_len  /**< */,
                                        const uint8_t           *glyphcmds  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite_glyphs_16
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint8_t                  op
 ** @param xcb_render_picture_t     src
 ** @param xcb_render_picture_t     dst
 ** @param xcb_render_pictformat_t  mask_format
 ** @param xcb_render_glyphset_t    glyphset
 ** @param int16_t                  src_x
 ** @param int16_t                  src_y
 ** @param uint32_t                 glyphcmds_len
 ** @param const uint8_t           *glyphcmds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_glyphs_16 (xcb_connection_t        *c  /**< */,
                                uint8_t                  op  /**< */,
                                xcb_render_picture_t     src  /**< */,
                                xcb_render_picture_t     dst  /**< */,
                                xcb_render_pictformat_t  mask_format  /**< */,
                                xcb_render_glyphset_t    glyphset  /**< */,
                                int16_t                  src_x  /**< */,
                                int16_t                  src_y  /**< */,
                                uint32_t                 glyphcmds_len  /**< */,
                                const uint8_t           *glyphcmds  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite_glyphs_32_checked
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint8_t                  op
 ** @param xcb_render_picture_t     src
 ** @param xcb_render_picture_t     dst
 ** @param xcb_render_pictformat_t  mask_format
 ** @param xcb_render_glyphset_t    glyphset
 ** @param int16_t                  src_x
 ** @param int16_t                  src_y
 ** @param uint32_t                 glyphcmds_len
 ** @param const uint8_t           *glyphcmds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_glyphs_32_checked (xcb_connection_t        *c  /**< */,
                                        uint8_t                  op  /**< */,
                                        xcb_render_picture_t     src  /**< */,
                                        xcb_render_picture_t     dst  /**< */,
                                        xcb_render_pictformat_t  mask_format  /**< */,
                                        xcb_render_glyphset_t    glyphset  /**< */,
                                        int16_t                  src_x  /**< */,
                                        int16_t                  src_y  /**< */,
                                        uint32_t                 glyphcmds_len  /**< */,
                                        const uint8_t           *glyphcmds  /**< */);

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
 ** xcb_void_cookie_t xcb_render_composite_glyphs_32
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint8_t                  op
 ** @param xcb_render_picture_t     src
 ** @param xcb_render_picture_t     dst
 ** @param xcb_render_pictformat_t  mask_format
 ** @param xcb_render_glyphset_t    glyphset
 ** @param int16_t                  src_x
 ** @param int16_t                  src_y
 ** @param uint32_t                 glyphcmds_len
 ** @param const uint8_t           *glyphcmds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_composite_glyphs_32 (xcb_connection_t        *c  /**< */,
                                uint8_t                  op  /**< */,
                                xcb_render_picture_t     src  /**< */,
                                xcb_render_picture_t     dst  /**< */,
                                xcb_render_pictformat_t  mask_format  /**< */,
                                xcb_render_glyphset_t    glyphset  /**< */,
                                int16_t                  src_x  /**< */,
                                int16_t                  src_y  /**< */,
                                uint32_t                 glyphcmds_len  /**< */,
                                const uint8_t           *glyphcmds  /**< */);

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
 ** xcb_void_cookie_t xcb_render_fill_rectangles_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param uint8_t                op
 ** @param xcb_render_picture_t   dst
 ** @param xcb_render_color_t     color
 ** @param uint32_t               rects_len
 ** @param const xcb_rectangle_t *rects
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_fill_rectangles_checked (xcb_connection_t      *c  /**< */,
                                    uint8_t                op  /**< */,
                                    xcb_render_picture_t   dst  /**< */,
                                    xcb_render_color_t     color  /**< */,
                                    uint32_t               rects_len  /**< */,
                                    const xcb_rectangle_t *rects  /**< */);

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
 ** xcb_void_cookie_t xcb_render_fill_rectangles
 ** 
 ** @param xcb_connection_t      *c
 ** @param uint8_t                op
 ** @param xcb_render_picture_t   dst
 ** @param xcb_render_color_t     color
 ** @param uint32_t               rects_len
 ** @param const xcb_rectangle_t *rects
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_fill_rectangles (xcb_connection_t      *c  /**< */,
                            uint8_t                op  /**< */,
                            xcb_render_picture_t   dst  /**< */,
                            xcb_render_color_t     color  /**< */,
                            uint32_t               rects_len  /**< */,
                            const xcb_rectangle_t *rects  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_cursor_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_cursor_t          cid
 ** @param xcb_render_picture_t  source
 ** @param uint16_t              x
 ** @param uint16_t              y
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_cursor_checked (xcb_connection_t     *c  /**< */,
                                  xcb_cursor_t          cid  /**< */,
                                  xcb_render_picture_t  source  /**< */,
                                  uint16_t              x  /**< */,
                                  uint16_t              y  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_cursor
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_cursor_t          cid
 ** @param xcb_render_picture_t  source
 ** @param uint16_t              x
 ** @param uint16_t              y
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_cursor (xcb_connection_t     *c  /**< */,
                          xcb_cursor_t          cid  /**< */,
                          xcb_render_picture_t  source  /**< */,
                          uint16_t              x  /**< */,
                          uint16_t              y  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_transform_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_transform_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_transform_next
 ** 
 ** @param xcb_render_transform_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_transform_next (xcb_render_transform_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_transform_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_transform_end
 ** 
 ** @param xcb_render_transform_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_transform_end (xcb_render_transform_iterator_t i  /**< */);

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
 ** xcb_void_cookie_t xcb_render_set_picture_transform_checked
 ** 
 ** @param xcb_connection_t       *c
 ** @param xcb_render_picture_t    picture
 ** @param xcb_render_transform_t  transform
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_set_picture_transform_checked (xcb_connection_t       *c  /**< */,
                                          xcb_render_picture_t    picture  /**< */,
                                          xcb_render_transform_t  transform  /**< */);

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
 ** xcb_void_cookie_t xcb_render_set_picture_transform
 ** 
 ** @param xcb_connection_t       *c
 ** @param xcb_render_picture_t    picture
 ** @param xcb_render_transform_t  transform
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_set_picture_transform (xcb_connection_t       *c  /**< */,
                                  xcb_render_picture_t    picture  /**< */,
                                  xcb_render_transform_t  transform  /**< */);

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
 ** xcb_render_query_filters_cookie_t xcb_render_query_filters
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @returns xcb_render_query_filters_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_filters_cookie_t
xcb_render_query_filters (xcb_connection_t *c  /**< */,
                          xcb_drawable_t    drawable  /**< */);

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
 ** xcb_render_query_filters_cookie_t xcb_render_query_filters_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_drawable_t    drawable
 ** @returns xcb_render_query_filters_cookie_t
 **
 *****************************************************************************/
 
xcb_render_query_filters_cookie_t
xcb_render_query_filters_unchecked (xcb_connection_t *c  /**< */,
                                    xcb_drawable_t    drawable  /**< */);


/*****************************************************************************
 **
 ** uint16_t * xcb_render_query_filters_aliases
 ** 
 ** @param const xcb_render_query_filters_reply_t *R
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_render_query_filters_aliases (const xcb_render_query_filters_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_query_filters_aliases_length
 ** 
 ** @param const xcb_render_query_filters_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_query_filters_aliases_length (const xcb_render_query_filters_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_query_filters_aliases_end
 ** 
 ** @param const xcb_render_query_filters_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_query_filters_aliases_end (const xcb_render_query_filters_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_render_query_filters_filters_length
 ** 
 ** @param const xcb_render_query_filters_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_render_query_filters_filters_length (const xcb_render_query_filters_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_str_iterator_t xcb_render_query_filters_filters_iterator
 ** 
 ** @param const xcb_render_query_filters_reply_t *R
 ** @returns xcb_str_iterator_t
 **
 *****************************************************************************/
 
xcb_str_iterator_t
xcb_render_query_filters_filters_iterator (const xcb_render_query_filters_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_render_query_filters_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_render_query_filters_reply_t * xcb_render_query_filters_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_render_query_filters_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_render_query_filters_reply_t *
 **
 *****************************************************************************/
 
xcb_render_query_filters_reply_t *
xcb_render_query_filters_reply (xcb_connection_t                   *c  /**< */,
                                xcb_render_query_filters_cookie_t   cookie  /**< */,
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
 ** xcb_void_cookie_t xcb_render_set_picture_filter_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param uint16_t                  filter_len
 ** @param const char               *filter
 ** @param uint32_t                  values_len
 ** @param const xcb_render_fixed_t *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_set_picture_filter_checked (xcb_connection_t         *c  /**< */,
                                       xcb_render_picture_t      picture  /**< */,
                                       uint16_t                  filter_len  /**< */,
                                       const char               *filter  /**< */,
                                       uint32_t                  values_len  /**< */,
                                       const xcb_render_fixed_t *values  /**< */);

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
 ** xcb_void_cookie_t xcb_render_set_picture_filter
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param uint16_t                  filter_len
 ** @param const char               *filter
 ** @param uint32_t                  values_len
 ** @param const xcb_render_fixed_t *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_set_picture_filter (xcb_connection_t         *c  /**< */,
                               xcb_render_picture_t      picture  /**< */,
                               uint16_t                  filter_len  /**< */,
                               const char               *filter  /**< */,
                               uint32_t                  values_len  /**< */,
                               const xcb_render_fixed_t *values  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_animcursorelt_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_animcursorelt_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_animcursorelt_next
 ** 
 ** @param xcb_render_animcursorelt_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_animcursorelt_next (xcb_render_animcursorelt_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_animcursorelt_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_animcursorelt_end
 ** 
 ** @param xcb_render_animcursorelt_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_animcursorelt_end (xcb_render_animcursorelt_iterator_t i  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_anim_cursor_checked
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_cursor_t                      cid
 ** @param uint32_t                          cursors_len
 ** @param const xcb_render_animcursorelt_t *cursors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_anim_cursor_checked (xcb_connection_t                 *c  /**< */,
                                       xcb_cursor_t                      cid  /**< */,
                                       uint32_t                          cursors_len  /**< */,
                                       const xcb_render_animcursorelt_t *cursors  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_anim_cursor
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_cursor_t                      cid
 ** @param uint32_t                          cursors_len
 ** @param const xcb_render_animcursorelt_t *cursors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_anim_cursor (xcb_connection_t                 *c  /**< */,
                               xcb_cursor_t                      cid  /**< */,
                               uint32_t                          cursors_len  /**< */,
                               const xcb_render_animcursorelt_t *cursors  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_spanfix_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_spanfix_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_spanfix_next
 ** 
 ** @param xcb_render_spanfix_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_spanfix_next (xcb_render_spanfix_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_spanfix_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_spanfix_end
 ** 
 ** @param xcb_render_spanfix_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_spanfix_end (xcb_render_spanfix_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_render_trap_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_render_trap_t)
 */

/*****************************************************************************
 **
 ** void xcb_render_trap_next
 ** 
 ** @param xcb_render_trap_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_render_trap_next (xcb_render_trap_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_render_trap_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_render_trap_end
 ** 
 ** @param xcb_render_trap_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_render_trap_end (xcb_render_trap_iterator_t i  /**< */);

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
 ** xcb_void_cookie_t xcb_render_add_traps_checked
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_picture_t     picture
 ** @param int16_t                  x_off
 ** @param int16_t                  y_off
 ** @param uint32_t                 traps_len
 ** @param const xcb_render_trap_t *traps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_add_traps_checked (xcb_connection_t        *c  /**< */,
                              xcb_render_picture_t     picture  /**< */,
                              int16_t                  x_off  /**< */,
                              int16_t                  y_off  /**< */,
                              uint32_t                 traps_len  /**< */,
                              const xcb_render_trap_t *traps  /**< */);

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
 ** xcb_void_cookie_t xcb_render_add_traps
 ** 
 ** @param xcb_connection_t        *c
 ** @param xcb_render_picture_t     picture
 ** @param int16_t                  x_off
 ** @param int16_t                  y_off
 ** @param uint32_t                 traps_len
 ** @param const xcb_render_trap_t *traps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_add_traps (xcb_connection_t        *c  /**< */,
                      xcb_render_picture_t     picture  /**< */,
                      int16_t                  x_off  /**< */,
                      int16_t                  y_off  /**< */,
                      uint32_t                 traps_len  /**< */,
                      const xcb_render_trap_t *traps  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_solid_fill_checked
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @param xcb_render_color_t    color
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_solid_fill_checked (xcb_connection_t     *c  /**< */,
                                      xcb_render_picture_t  picture  /**< */,
                                      xcb_render_color_t    color  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_solid_fill
 ** 
 ** @param xcb_connection_t     *c
 ** @param xcb_render_picture_t  picture
 ** @param xcb_render_color_t    color
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_solid_fill (xcb_connection_t     *c  /**< */,
                              xcb_render_picture_t  picture  /**< */,
                              xcb_render_color_t    color  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_linear_gradient_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param xcb_render_pointfix_t     p1
 ** @param xcb_render_pointfix_t     p2
 ** @param uint32_t                  num_stops
 ** @param const xcb_render_fixed_t *stops
 ** @param const xcb_render_color_t *colors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_linear_gradient_checked (xcb_connection_t         *c  /**< */,
                                           xcb_render_picture_t      picture  /**< */,
                                           xcb_render_pointfix_t     p1  /**< */,
                                           xcb_render_pointfix_t     p2  /**< */,
                                           uint32_t                  num_stops  /**< */,
                                           const xcb_render_fixed_t *stops  /**< */,
                                           const xcb_render_color_t *colors  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_linear_gradient
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param xcb_render_pointfix_t     p1
 ** @param xcb_render_pointfix_t     p2
 ** @param uint32_t                  num_stops
 ** @param const xcb_render_fixed_t *stops
 ** @param const xcb_render_color_t *colors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_linear_gradient (xcb_connection_t         *c  /**< */,
                                   xcb_render_picture_t      picture  /**< */,
                                   xcb_render_pointfix_t     p1  /**< */,
                                   xcb_render_pointfix_t     p2  /**< */,
                                   uint32_t                  num_stops  /**< */,
                                   const xcb_render_fixed_t *stops  /**< */,
                                   const xcb_render_color_t *colors  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_radial_gradient_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param xcb_render_pointfix_t     inner
 ** @param xcb_render_pointfix_t     outer
 ** @param xcb_render_fixed_t        inner_radius
 ** @param xcb_render_fixed_t        outer_radius
 ** @param uint32_t                  num_stops
 ** @param const xcb_render_fixed_t *stops
 ** @param const xcb_render_color_t *colors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_radial_gradient_checked (xcb_connection_t         *c  /**< */,
                                           xcb_render_picture_t      picture  /**< */,
                                           xcb_render_pointfix_t     inner  /**< */,
                                           xcb_render_pointfix_t     outer  /**< */,
                                           xcb_render_fixed_t        inner_radius  /**< */,
                                           xcb_render_fixed_t        outer_radius  /**< */,
                                           uint32_t                  num_stops  /**< */,
                                           const xcb_render_fixed_t *stops  /**< */,
                                           const xcb_render_color_t *colors  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_radial_gradient
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param xcb_render_pointfix_t     inner
 ** @param xcb_render_pointfix_t     outer
 ** @param xcb_render_fixed_t        inner_radius
 ** @param xcb_render_fixed_t        outer_radius
 ** @param uint32_t                  num_stops
 ** @param const xcb_render_fixed_t *stops
 ** @param const xcb_render_color_t *colors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_radial_gradient (xcb_connection_t         *c  /**< */,
                                   xcb_render_picture_t      picture  /**< */,
                                   xcb_render_pointfix_t     inner  /**< */,
                                   xcb_render_pointfix_t     outer  /**< */,
                                   xcb_render_fixed_t        inner_radius  /**< */,
                                   xcb_render_fixed_t        outer_radius  /**< */,
                                   uint32_t                  num_stops  /**< */,
                                   const xcb_render_fixed_t *stops  /**< */,
                                   const xcb_render_color_t *colors  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_conical_gradient_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param xcb_render_pointfix_t     center
 ** @param xcb_render_fixed_t        angle
 ** @param uint32_t                  num_stops
 ** @param const xcb_render_fixed_t *stops
 ** @param const xcb_render_color_t *colors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_conical_gradient_checked (xcb_connection_t         *c  /**< */,
                                            xcb_render_picture_t      picture  /**< */,
                                            xcb_render_pointfix_t     center  /**< */,
                                            xcb_render_fixed_t        angle  /**< */,
                                            uint32_t                  num_stops  /**< */,
                                            const xcb_render_fixed_t *stops  /**< */,
                                            const xcb_render_color_t *colors  /**< */);

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
 ** xcb_void_cookie_t xcb_render_create_conical_gradient
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_render_picture_t      picture
 ** @param xcb_render_pointfix_t     center
 ** @param xcb_render_fixed_t        angle
 ** @param uint32_t                  num_stops
 ** @param const xcb_render_fixed_t *stops
 ** @param const xcb_render_color_t *colors
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_render_create_conical_gradient (xcb_connection_t         *c  /**< */,
                                    xcb_render_picture_t      picture  /**< */,
                                    xcb_render_pointfix_t     center  /**< */,
                                    xcb_render_fixed_t        angle  /**< */,
                                    uint32_t                  num_stops  /**< */,
                                    const xcb_render_fixed_t *stops  /**< */,
                                    const xcb_render_color_t *colors  /**< */);


#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
