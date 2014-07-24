/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/***************************************************************************/
/*                                                                         */
/*  ftimage.h                                                              */
/*                                                                         */
/*    FreeType glyph image formats and default raster interface            */
/*    (specification).                                                     */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004 by                               */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Note: A `raster' is simply a scan-line converter, used to render      */
  /*       QT_FT_Outlines into QT_FT_Bitmaps.                                    */
  /*                                                                       */
  /*************************************************************************/


#ifndef __QT_FTIMAGE_H__
#define __QT_FTIMAGE_H__

/*
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
*/

QT_FT_BEGIN_HEADER

  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    basic_types                                                        */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    QT_FT_Pos                                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The type QT_FT_Pos is a 32-bit integer used to store vectorial        */
  /*    coordinates.  Depending on the context, these can represent        */
  /*    distances in integer font units, or 16,16, or 26.6 fixed float     */
  /*    pixel coordinates.                                                 */
  /*                                                                       */
  typedef signed int  QT_FT_Pos;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Vector                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to store a 2D vector; coordinates are of   */
  /*    the QT_FT_Pos type.                                                   */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    x :: The horizontal coordinate.                                    */
  /*    y :: The vertical coordinate.                                      */
  /*                                                                       */
  typedef struct  QT_FT_Vector_
  {
    QT_FT_Pos  x;
    QT_FT_Pos  y;

  } QT_FT_Vector;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_BBox                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to hold an outline's bounding box, i.e., the      */
  /*    coordinates of its extrema in the horizontal and vertical          */
  /*    directions.                                                        */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    xMin :: The horizontal minimum (left-most).                        */
  /*                                                                       */
  /*    yMin :: The vertical minimum (bottom-most).                        */
  /*                                                                       */
  /*    xMax :: The horizontal maximum (right-most).                       */
  /*                                                                       */
  /*    yMax :: The vertical maximum (top-most).                           */
  /*                                                                       */
  typedef struct  QT_FT_BBox_
  {
    QT_FT_Pos  xMin, yMin;
    QT_FT_Pos  xMax, yMax;

  } QT_FT_BBox;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    QT_FT_Pixel_Mode                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration type used to describe the format of pixels in a     */
  /*    given bitmap.  Note that additional formats may be added in the    */
  /*    future.                                                            */
  /*                                                                       */
  /* <Values>                                                              */
  /*    QT_FT_PIXEL_MODE_NONE ::                                              */
  /*      Value 0 is reserved.                                             */
  /*                                                                       */
  /*    QT_FT_PIXEL_MODE_MONO ::                                              */
  /*      A monochrome bitmap, using 1 bit per pixel.  Note that pixels    */
  /*      are stored in most-significant order (MSB), which means that     */
  /*      the left-most pixel in a byte has value 128.                     */
  /*                                                                       */
  /*    QT_FT_PIXEL_MODE_GRAY ::                                              */
  /*      An 8-bit bitmap, generally used to represent anti-aliased glyph  */
  /*      images.  Each pixel is stored in one byte.  Note that the number */
  /*      of value "gray" levels is stored in the `num_bytes' field of     */
  /*      the @QT_FT_Bitmap structure (it generally is 256).                  */
  /*                                                                       */
  /*    QT_FT_PIXEL_MODE_GRAY2 ::                                             */
  /*      A 2-bit/pixel bitmap, used to represent embedded anti-aliased    */
  /*      bitmaps in font files according to the OpenType specification.   */
  /*      We haven't found a single font using this format, however.       */
  /*                                                                       */
  /*    QT_FT_PIXEL_MODE_GRAY4 ::                                             */
  /*      A 4-bit/pixel bitmap, used to represent embedded anti-aliased    */
  /*      bitmaps in font files according to the OpenType specification.   */
  /*      We haven't found a single font using this format, however.       */
  /*                                                                       */
  /*    QT_FT_PIXEL_MODE_LCD ::                                               */
  /*      An 8-bit bitmap, used to represent RGB or BGR decimated glyph    */
  /*      images used for display on LCD displays; the bitmap's width is   */
  /*      three times wider than the original glyph image.  See also       */
  /*      @QT_FT_RENDER_MODE_LCD.                                             */
  /*                                                                       */
  /*    QT_FT_PIXEL_MODE_LCD_V ::                                             */
  /*      An 8-bit bitmap, used to represent RGB or BGR decimated glyph    */
  /*      images used for display on rotated LCD displays; the bitmap's    */
  /*      height is three times taller than the original glyph image.      */
  /*      See also @QT_FT_RENDER_MODE_LCD_V.                                  */
  /*                                                                       */
  typedef enum  QT_FT_Pixel_Mode_
  {
    QT_FT_PIXEL_MODE_NONE = 0,
    QT_FT_PIXEL_MODE_MONO,
    QT_FT_PIXEL_MODE_GRAY,
    QT_FT_PIXEL_MODE_GRAY2,
    QT_FT_PIXEL_MODE_GRAY4,
    QT_FT_PIXEL_MODE_LCD,
    QT_FT_PIXEL_MODE_LCD_V,

    QT_FT_PIXEL_MODE_MAX      /* do not remove */

  } QT_FT_Pixel_Mode;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    qt_ft_pixel_mode_xxx                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of deprecated constants.  Use the corresponding             */
  /*    @QT_FT_Pixel_Mode values instead.                                     */
  /*                                                                       */
  /* <Values>                                                              */
  /*    qt_ft_pixel_mode_none  :: see @QT_FT_PIXEL_MODE_NONE                     */
  /*    qt_ft_pixel_mode_mono  :: see @QT_FT_PIXEL_MODE_MONO                     */
  /*    qt_ft_pixel_mode_grays :: see @QT_FT_PIXEL_MODE_GRAY                     */
  /*    qt_ft_pixel_mode_pal2  :: see @QT_FT_PIXEL_MODE_GRAY2                    */
  /*    qt_ft_pixel_mode_pal4  :: see @QT_FT_PIXEL_MODE_GRAY4                    */
  /*                                                                       */
#define qt_ft_pixel_mode_none   QT_FT_PIXEL_MODE_NONE
#define qt_ft_pixel_mode_mono   QT_FT_PIXEL_MODE_MONO
#define qt_ft_pixel_mode_grays  QT_FT_PIXEL_MODE_GRAY
#define qt_ft_pixel_mode_pal2   QT_FT_PIXEL_MODE_GRAY2
#define qt_ft_pixel_mode_pal4   QT_FT_PIXEL_MODE_GRAY4

 /* */

#if 0

  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    QT_FT_Palette_Mode                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS TYPE IS DEPRECATED.  DO NOT USE IT!                           */
  /*                                                                       */
  /*    An enumeration type used to describe the format of a bitmap        */
  /*    palette, used with qt_ft_pixel_mode_pal4 and qt_ft_pixel_mode_pal8.      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    qt_ft_palette_mode_rgb  :: The palette is an array of 3-bytes RGB     */
  /*                            records.                                   */
  /*                                                                       */
  /*    qt_ft_palette_mode_rgba :: The palette is an array of 4-bytes RGBA    */
  /*                            records.                                   */
  /*                                                                       */
  /* <Note>                                                                */
  /*    As qt_ft_pixel_mode_pal2, pal4 and pal8 are currently unused by       */
  /*    FreeType, these types are not handled by the library itself.       */
  /*                                                                       */
  typedef enum  QT_FT_Palette_Mode_
  {
    qt_ft_palette_mode_rgb = 0,
    qt_ft_palette_mode_rgba,

    qt_ft_palettte_mode_max   /* do not remove */

  } QT_FT_Palette_Mode;

  /* */

#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Bitmap                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to describe a bitmap or pixmap to the raster.     */
  /*    Note that we now manage pixmaps of various depths through the      */
  /*    `pixel_mode' field.                                                */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    rows         :: The number of bitmap rows.                         */
  /*                                                                       */
  /*    width        :: The number of pixels in bitmap row.                */
  /*                                                                       */
  /*    pitch        :: The pitch's absolute value is the number of bytes  */
  /*                    taken by one bitmap row, including padding.        */
  /*                    However, the pitch is positive when the bitmap has */
  /*                    a `down' flow, and negative when it has an `up'    */
  /*                    flow.  In all cases, the pitch is an offset to add */
  /*                    to a bitmap pointer in order to go down one row.   */
  /*                                                                       */
  /*    buffer       :: A typeless pointer to the bitmap buffer.  This     */
  /*                    value should be aligned on 32-bit boundaries in    */
  /*                    most cases.                                        */
  /*                                                                       */
  /*    num_grays    :: This field is only used with                       */
  /*                    `QT_FT_PIXEL_MODE_GRAY'; it gives the number of gray  */
  /*                    levels used in the bitmap.                         */
  /*                                                                       */
  /*    pixel_mode   :: The pixel mode, i.e., how pixel bits are stored.   */
  /*                    See @QT_FT_Pixel_Mode for possible values.            */
  /*                                                                       */
  /*    palette_mode :: This field is only used with paletted pixel modes; */
  /*                    it indicates how the palette is stored.            */
  /*                                                                       */
  /*    palette      :: A typeless pointer to the bitmap palette; only     */
  /*                    used for paletted pixel modes.                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*   For now, the only pixel mode supported by FreeType are mono and     */
  /*   grays.  However, drivers might be added in the future to support    */
  /*   more `colorful' options.                                            */
  /*                                                                       */
  /*   When using pixel modes pal2, pal4 and pal8 with a void `palette'    */
  /*   field, a gray pixmap with respectively 4, 16, and 256 levels of     */
  /*   gray is assumed.  This, in order to be compatible with some         */
  /*   embedded bitmap formats defined in the TrueType specification.      */
  /*                                                                       */
  /*   Note that no font was found presenting such embedded bitmaps, so    */
  /*   this is currently completely unhandled by the library.              */
  /*                                                                       */
  typedef struct  QT_FT_Bitmap_
  {
    int             rows;
    int             width;
    int             pitch;
    unsigned char*  buffer;
    short           num_grays;
    char            pixel_mode;
    char            palette_mode;
    void*           palette;

  } QT_FT_Bitmap;


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    outline_processing                                                 */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Outline                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This structure is used to describe an outline to the scan-line     */
  /*    converter.                                                         */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    n_contours :: The number of contours in the outline.               */
  /*                                                                       */
  /*    n_points   :: The number of points in the outline.                 */
  /*                                                                       */
  /*    points     :: A pointer to an array of `n_points' QT_FT_Vector        */
  /*                  elements, giving the outline's point coordinates.    */
  /*                                                                       */
  /*    tags       :: A pointer to an array of `n_points' chars, giving    */
  /*                  each outline point's type.  If bit 0 is unset, the   */
  /*                  point is `off' the curve, i.e. a Bezier control      */
  /*                  point, while it is `on' when set.                    */
  /*                                                                       */
  /*                  Bit 1 is meaningful for `off' points only.  If set,  */
  /*                  it indicates a third-order Bezier arc control point; */
  /*                  and a second-order control point if unset.           */
  /*                                                                       */
  /*    contours   :: An array of `n_contours' shorts, giving the end      */
  /*                  point of each contour within the outline.  For       */
  /*                  example, the first contour is defined by the points  */
  /*                  `0' to `contours[0]', the second one is defined by   */
  /*                  the points `contours[0]+1' to `contours[1]', etc.    */
  /*                                                                       */
  /*    flags      :: A set of bit flags used to characterize the outline  */
  /*                  and give hints to the scan-converter and hinter on   */
  /*                  how to convert/grid-fit it.  See QT_FT_Outline_Flags.   */
  /*                                                                       */
  typedef struct  QT_FT_Outline_
  {
    int       n_contours;      /* number of contours in glyph        */
    int       n_points;        /* number of points in the glyph      */

    QT_FT_Vector*  points;          /* the outline's points               */
    char*       tags;            /* the points flags                   */
    int*      contours;        /* the contour end points             */

    int         flags;           /* outline masks                      */

  } QT_FT_Outline;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*   QT_FT_OUTLINE_FLAGS                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of bit-field constants use for the flags in an outline's    */
  /*    `flags' field.                                                     */
  /*                                                                       */
  /* <Values>                                                              */
  /*    QT_FT_OUTLINE_NONE           :: Value 0 is reserved.                  */
  /*                                                                       */
  /*    QT_FT_OUTLINE_OWNER          :: If set, this flag indicates that the  */
  /*                                 outline's field arrays (i.e.          */
  /*                                 `points', `flags' & `contours') are   */
  /*                                 `owned' by the outline object, and    */
  /*                                 should thus be freed when it is       */
  /*                                 destroyed.                            */
  /*                                                                       */
  /*   QT_FT_OUTLINE_EVEN_ODD_FILL   :: By default, outlines are filled using */
  /*                                 the non-zero winding rule.  If set to */
  /*                                 1, the outline will be filled using   */
  /*                                 the even-odd fill rule (only works    */
  /*                                 with the smooth raster).              */
  /*                                                                       */
  /*   QT_FT_OUTLINE_REVERSE_FILL    :: By default, outside contours of an    */
  /*                                 outline are oriented in clock-wise    */
  /*                                 direction, as defined in the TrueType */
  /*                                 specification.  This flag is set if   */
  /*                                 the outline uses the opposite         */
  /*                                 direction (typically for Type 1       */
  /*                                 fonts).  This flag is ignored by the  */
  /*                                 scan-converter.  However, it is very  */
  /*                                 important for the auto-hinter.        */
  /*                                                                       */
  /*   QT_FT_OUTLINE_IGNORE_DROPOUTS :: By default, the scan converter will   */
  /*                                 try to detect drop-outs in an outline */
  /*                                 and correct the glyph bitmap to       */
  /*                                 ensure consistent shape continuity.   */
  /*                                 If set, this flag hints the scan-line */
  /*                                 converter to ignore such cases.       */
  /*                                                                       */
  /*   QT_FT_OUTLINE_HIGH_PRECISION  :: This flag indicates that the          */
  /*                                 scan-line converter should try to     */
  /*                                 convert this outline to bitmaps with  */
  /*                                 the highest possible quality.  It is  */
  /*                                 typically set for small character     */
  /*                                 sizes.  Note that this is only a      */
  /*                                 hint, that might be completely        */
  /*                                 ignored by a given scan-converter.    */
  /*                                                                       */
  /*   QT_FT_OUTLINE_SINGLE_PASS     :: This flag is set to force a given     */
  /*                                 scan-converter to only use a single   */
  /*                                 pass over the outline to render a     */
  /*                                 bitmap glyph image.  Normally, it is  */
  /*                                 set for very large character sizes.   */
  /*                                 It is only a hint, that might be      */
  /*                                 completely ignored by a given         */
  /*                                 scan-converter.                       */
  /*                                                                       */
#define QT_FT_OUTLINE_NONE             0x0
#define QT_FT_OUTLINE_OWNER            0x1
#define QT_FT_OUTLINE_EVEN_ODD_FILL    0x2
#define QT_FT_OUTLINE_REVERSE_FILL     0x4
#define QT_FT_OUTLINE_IGNORE_DROPOUTS  0x8

#define QT_FT_OUTLINE_HIGH_PRECISION   0x100
#define QT_FT_OUTLINE_SINGLE_PASS      0x200


 /*************************************************************************
  *
  * @enum:
  *   qt_ft_outline_flags
  *
  * @description:
  *   These constants are deprecated.  Please use the corresponding
  *   @QT_FT_OUTLINE_FLAGS values.
  *
  * @values:
  *   qt_ft_outline_none            :: See @QT_FT_OUTLINE_NONE.
  *   qt_ft_outline_owner           :: See @QT_FT_OUTLINE_OWNER.
  *   qt_ft_outline_even_odd_fill   :: See @QT_FT_OUTLINE_EVEN_ODD_FILL.
  *   qt_ft_outline_reverse_fill    :: See @QT_FT_OUTLINE_REVERSE_FILL.
  *   qt_ft_outline_ignore_dropouts :: See @QT_FT_OUTLINE_IGNORE_DROPOUTS.
  *   qt_ft_outline_high_precision  :: See @QT_FT_OUTLINE_HIGH_PRECISION.
  *   qt_ft_outline_single_pass     :: See @QT_FT_OUTLINE_SINGLE_PASS.
  */
#define qt_ft_outline_none             QT_FT_OUTLINE_NONE
#define qt_ft_outline_owner            QT_FT_OUTLINE_OWNER
#define qt_ft_outline_even_odd_fill    QT_FT_OUTLINE_EVEN_ODD_FILL
#define qt_ft_outline_reverse_fill     QT_FT_OUTLINE_REVERSE_FILL
#define qt_ft_outline_ignore_dropouts  QT_FT_OUTLINE_IGNORE_DROPOUTS
#define qt_ft_outline_high_precision   QT_FT_OUTLINE_HIGH_PRECISION
#define qt_ft_outline_single_pass      QT_FT_OUTLINE_SINGLE_PASS

  /* */

#define QT_FT_CURVE_TAG( flag )  ( flag & 3 )

#define QT_FT_CURVE_TAG_ON           1
#define QT_FT_CURVE_TAG_CONIC        0
#define QT_FT_CURVE_TAG_CUBIC        2

#define QT_FT_CURVE_TAG_TOUCH_X      8  /* reserved for the TrueType hinter */
#define QT_FT_CURVE_TAG_TOUCH_Y     16  /* reserved for the TrueType hinter */

#define QT_FT_CURVE_TAG_TOUCH_BOTH  ( QT_FT_CURVE_TAG_TOUCH_X | \
                                   QT_FT_CURVE_TAG_TOUCH_Y )

#define  QT_FT_Curve_Tag_On       QT_FT_CURVE_TAG_ON
#define  QT_FT_Curve_Tag_Conic    QT_FT_CURVE_TAG_CONIC
#define  QT_FT_Curve_Tag_Cubic    QT_FT_CURVE_TAG_CUBIC
#define  QT_FT_Curve_Tag_Touch_X  QT_FT_CURVE_TAG_TOUCH_X
#define  QT_FT_Curve_Tag_Touch_Y  QT_FT_CURVE_TAG_TOUCH_Y

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Outline_MoveToFunc                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function pointer type used to describe the signature of a `move  */
  /*    to' function during outline walking/decomposition.                 */
  /*                                                                       */
  /*    A `move to' is emitted to start a new contour in an outline.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    to   :: A pointer to the target point of the `move to'.            */
  /*                                                                       */
  /*    user :: A typeless pointer which is passed from the caller of the  */
  /*            decomposition function.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  typedef int
  (*QT_FT_Outline_MoveToFunc)( QT_FT_Vector*  to,
                            void*       user );

#define QT_FT_Outline_MoveTo_Func  QT_FT_Outline_MoveToFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Outline_LineToFunc                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function pointer type used to describe the signature of a `line  */
  /*    to' function during outline walking/decomposition.                 */
  /*                                                                       */
  /*    A `line to' is emitted to indicate a segment in the outline.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    to   :: A pointer to the target point of the `line to'.            */
  /*                                                                       */
  /*    user :: A typeless pointer which is passed from the caller of the  */
  /*            decomposition function.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  typedef int
  (*QT_FT_Outline_LineToFunc)( QT_FT_Vector*  to,
                            void*       user );

#define  QT_FT_Outline_LineTo_Func  QT_FT_Outline_LineToFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Outline_ConicToFunc                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function pointer type use to describe the signature of a `conic  */
  /*    to' function during outline walking/decomposition.                 */
  /*                                                                       */
  /*    A `conic to' is emitted to indicate a second-order Bezier arc in   */
  /*    the outline.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    control :: An intermediate control point between the last position */
  /*               and the new target in `to'.                             */
  /*                                                                       */
  /*    to      :: A pointer to the target end point of the conic arc.     */
  /*                                                                       */
  /*    user    :: A typeless pointer which is passed from the caller of   */
  /*               the decomposition function.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  typedef int
  (*QT_FT_Outline_ConicToFunc)( QT_FT_Vector*  control,
                             QT_FT_Vector*  to,
                             void*       user );

#define  QT_FT_Outline_ConicTo_Func  QT_FT_Outline_ConicToFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Outline_CubicToFunc                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function pointer type used to describe the signature of a `cubic */
  /*    to' function during outline walking/decomposition.                 */
  /*                                                                       */
  /*    A `cubic to' is emitted to indicate a third-order Bezier arc.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    control1 :: A pointer to the first Bezier control point.           */
  /*                                                                       */
  /*    control2 :: A pointer to the second Bezier control point.          */
  /*                                                                       */
  /*    to       :: A pointer to the target end point.                     */
  /*                                                                       */
  /*    user     :: A typeless pointer which is passed from the caller of  */
  /*                the decomposition function.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  typedef int
  (*QT_FT_Outline_CubicToFunc)( QT_FT_Vector*  control1,
                             QT_FT_Vector*  control2,
                             QT_FT_Vector*  to,
                             void*       user );

#define  QT_FT_Outline_CubicTo_Func  QT_FT_Outline_CubicToFunc


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Outline_Funcs                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to hold various function pointers used during outline  */
  /*    decomposition in order to emit segments, conic, and cubic Beziers, */
  /*    as well as `move to' and `close to' operations.                    */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    move_to  :: The `move to' emitter.                                 */
  /*                                                                       */
  /*    line_to  :: The segment emitter.                                   */
  /*                                                                       */
  /*    conic_to :: The second-order Bezier arc emitter.                   */
  /*                                                                       */
  /*    cubic_to :: The third-order Bezier arc emitter.                    */
  /*                                                                       */
  /*    shift    :: The shift that is applied to coordinates before they   */
  /*                are sent to the emitter.                               */
  /*                                                                       */
  /*    delta    :: The delta that is applied to coordinates before they   */
  /*                are sent to the emitter, but after the shift.          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The point coordinates sent to the emitters are the transformed     */
  /*    version of the original coordinates (this is important for high    */
  /*    accuracy during scan-conversion).  The transformation is simple:   */
  /*                                                                       */
  /*      x' = (x << shift) - delta                                        */
  /*      y' = (x << shift) - delta                                        */
  /*                                                                       */
  /*    Set the value of `shift' and `delta' to 0 to get the original      */
  /*    point coordinates.                                                 */
  /*                                                                       */
  typedef struct  QT_FT_Outline_Funcs_
  {
    QT_FT_Outline_MoveToFunc   move_to;
    QT_FT_Outline_LineToFunc   line_to;
    QT_FT_Outline_ConicToFunc  conic_to;
    QT_FT_Outline_CubicToFunc  cubic_to;

    int                     shift;
    QT_FT_Pos                  delta;

  } QT_FT_Outline_Funcs;


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    basic_types                                                        */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    QT_FT_IMAGE_TAG                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro converts four letter tags into an unsigned long.        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Since many 16bit compilers don't like 32bit enumerations, you      */
  /*    should redefine this macro in case of problems to something like   */
  /*    this:                                                              */
  /*                                                                       */
  /*      #define QT_FT_IMAGE_TAG( value, _x1, _x2, _x3, _x4 )  value         */
  /*                                                                       */
  /*    to get a simple enumeration without assigning special numbers.     */
  /*                                                                       */
#ifndef QT_FT_IMAGE_TAG
#define QT_FT_IMAGE_TAG( value, _x1, _x2, _x3, _x4 )  \
          value = ( ( (unsigned long)_x1 << 24 ) | \
                    ( (unsigned long)_x2 << 16 ) | \
                    ( (unsigned long)_x3 << 8  ) | \
                      (unsigned long)_x4         )
#endif /* QT_FT_IMAGE_TAG */


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    QT_FT_Glyph_Format                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration type used to describe the format of a given glyph   */
  /*    image.  Note that this version of FreeType only supports two image */
  /*    formats, even though future font drivers will be able to register  */
  /*    their own format.                                                  */
  /*                                                                       */
  /* <Values>                                                              */
  /*    QT_FT_GLYPH_FORMAT_NONE ::                                            */
  /*      The value 0 is reserved and does describe a glyph format.        */
  /*                                                                       */
  /*    QT_FT_GLYPH_FORMAT_COMPOSITE ::                                       */
  /*      The glyph image is a composite of several other images.  This    */
  /*      format is _only_ used with @QT_FT_LOAD_NO_RECURSE, and is used to   */
  /*      report compound glyphs (like accented characters).               */
  /*                                                                       */
  /*    QT_FT_GLYPH_FORMAT_BITMAP ::                                          */
  /*      The glyph image is a bitmap, and can be described as an          */
  /*      @QT_FT_Bitmap.  You generally need to access the `bitmap' field of  */
  /*      the @QT_FT_GlyphSlotRec structure to read it.                       */
  /*                                                                       */
  /*    QT_FT_GLYPH_FORMAT_OUTLINE ::                                         */
  /*      The glyph image is a vertorial outline made of line segments     */
  /*      and Bezier arcs; it can be described as an @QT_FT_Outline; you      */
  /*      generally want to access the `outline' field of the              */
  /*      @QT_FT_GlyphSlotRec structure to read it.                           */
  /*                                                                       */
  /*    QT_FT_GLYPH_FORMAT_PLOTTER ::                                         */
  /*      The glyph image is a vectorial path with no inside/outside       */
  /*      contours.  Some Type 1 fonts, like those in the Hershey family,  */
  /*      contain glyphs in this format.  These are described as           */
  /*      @QT_FT_Outline, but FreeType isn't currently capable of rendering   */
  /*      them correctly.                                                  */
  /*                                                                       */
  typedef enum  QT_FT_Glyph_Format_
  {
    QT_FT_IMAGE_TAG( QT_FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),

    QT_FT_IMAGE_TAG( QT_FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
    QT_FT_IMAGE_TAG( QT_FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
    QT_FT_IMAGE_TAG( QT_FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
    QT_FT_IMAGE_TAG( QT_FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' )

  } QT_FT_Glyph_Format;


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    qt_ft_glyph_format_xxx                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of decprecated constants.  Use the corresponding            */
  /*    @QT_FT_Glyph_Format values instead.                                   */
  /*                                                                       */
  /* <Values>                                                              */
  /*    qt_ft_glyph_format_none      :: see @QT_FT_GLYPH_FORMAT_NONE             */
  /*    qt_ft_glyph_format_composite :: see @QT_FT_GLYPH_FORMAT_COMPOSITE        */
  /*    qt_ft_glyph_format_bitmap    :: see @QT_FT_GLYPH_FORMAT_BITMAP           */
  /*    qt_ft_glyph_format_outline   :: see @QT_FT_GLYPH_FORMAT_OUTLINE          */
  /*    qt_ft_glyph_format_plotter   :: see @QT_FT_GLYPH_FORMAT_PLOTTER          */
  /*                                                                       */
#define qt_ft_glyph_format_none       QT_FT_GLYPH_FORMAT_NONE
#define qt_ft_glyph_format_composite  QT_FT_GLYPH_FORMAT_COMPOSITE
#define qt_ft_glyph_format_bitmap     QT_FT_GLYPH_FORMAT_BITMAP
#define qt_ft_glyph_format_outline    QT_FT_GLYPH_FORMAT_OUTLINE
#define qt_ft_glyph_format_plotter    QT_FT_GLYPH_FORMAT_PLOTTER


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****            R A S T E R   D E F I N I T I O N S                *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* A raster is a scan converter, in charge of rendering an outline into  */
  /* a a bitmap.  This section contains the public API for rasters.        */
  /*                                                                       */
  /* Note that in FreeType 2, all rasters are now encapsulated within      */
  /* specific modules called `renderers'.  See `freetype/ftrender.h' for   */
  /* more details on renderers.                                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    raster                                                             */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Scanline converter                                                 */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    How vectorial outlines are converted into bitmaps and pixmaps.     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains technical definitions.                       */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    QT_FT_Raster                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle (pointer) to a raster object.  Each object can be used    */
  /*    independently to convert an outline into a bitmap or pixmap.       */
  /*                                                                       */
  typedef struct TRaster_ *QT_FT_Raster;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Span                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a single span of gray (or black) pixels  */
  /*    when rendering a monochrome or anti-aliased bitmap.                */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    x        :: The span's horizontal start position.                  */
  /*                                                                       */
  /*    len      :: The span's length in pixels.                           */
  /*                                                                       */
  /*    coverage :: The span color/coverage, ranging from 0 (background)   */
  /*                to 255 (foreground).  Only used for anti-aliased       */
  /*                rendering.                                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This structure is used by the span drawing callback type named     */
  /*    QT_FT_SpanFunc which takes the y-coordinate of the span as a          */
  /*    a parameter.                                                       */
  /*                                                                       */
  /*    The coverage value is always between 0 and 255, even if the number */
  /*    of gray levels have been set through QT_FT_Set_Gray_Levels().         */
  /*                                                                       */
  typedef struct  QT_FT_Span_
  {
    short           x;
    unsigned short  len;
    short y;
    unsigned char   coverage;
  } QT_FT_Span;


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_SpanFunc                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used as a call-back by the anti-aliased renderer in     */
  /*    order to let client applications draw themselves the gray pixel    */
  /*    spans on each scan line.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    y     :: The scanline's y-coordinate.                              */
  /*                                                                       */
  /*    count :: The number of spans to draw on this scanline.             */
  /*                                                                       */
  /*    spans :: A table of `count' spans to draw on the scanline.         */
  /*                                                                       */
  /*    user  :: User-supplied data that is passed to the callback.        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This callback allows client applications to directly render the    */
  /*    gray spans of the anti-aliased bitmap to any kind of surfaces.     */
  /*                                                                       */
  /*    This can be used to write anti-aliased outlines directly to a      */
  /*    given background bitmap, and even perform translucency.            */
  /*                                                                       */
  /*    Note that the `count' field cannot be greater than a fixed value   */
  /*    defined by the QT_FT_MAX_GRAY_SPANS configuration macro in            */
  /*    ftoption.h.  By default, this value is set to 32, which means that */
  /*    if there are more than 32 spans on a given scanline, the callback  */
  /*    will be called several times with the same `y' parameter in order  */
  /*    to draw all callbacks.                                             */
  /*                                                                       */
  /*    Otherwise, the callback is only called once per scan-line, and     */
  /*    only for those scanlines that do have `gray' pixels on them.       */
  /*                                                                       */
  typedef void
  (*QT_FT_SpanFunc)(int        count,
                    const QT_FT_Span*  spans,
                    void*     worker);

#define QT_FT_Raster_Span_Func   QT_FT_SpanFunc


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_BitTest_Func                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS TYPE IS DEPRECATED.  DO NOT USE IT.                           */
  /*                                                                       */
  /*    A function used as a call-back by the monochrome scan-converter    */
  /*    to test whether a given target pixel is already set to the drawing */
  /*    `color'.  These tests are crucial to implement drop-out control    */
  /*    per-se the TrueType spec.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    y     :: The pixel's y-coordinate.                                 */
  /*                                                                       */
  /*    x     :: The pixel's x-coordinate.                                 */
  /*                                                                       */
  /*    user  :: User-supplied data that is passed to the callback.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*   1 if the pixel is `set', 0 otherwise.                               */
  /*                                                                       */
  typedef int
  (*QT_FT_Raster_BitTest_Func)( int    y,
                             int    x,
                             void*  user );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_BitSet_Func                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS TYPE IS DEPRECATED.  DO NOT USE IT.                           */
  /*                                                                       */
  /*    A function used as a call-back by the monochrome scan-converter    */
  /*    to set an individual target pixel.  This is crucial to implement   */
  /*    drop-out control according to the TrueType specification.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    y     :: The pixel's y-coordinate.                                 */
  /*                                                                       */
  /*    x     :: The pixel's x-coordinate.                                 */
  /*                                                                       */
  /*    user  :: User-supplied data that is passed to the callback.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    1 if the pixel is `set', 0 otherwise.                              */
  /*                                                                       */
  typedef void
  (*QT_FT_Raster_BitSet_Func)( int    y,
                            int    x,
                            void*  user );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    QT_FT_RASTER_FLAG_XXX                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A list of bit flag constants as used in the `flags' field of a     */
  /*    @QT_FT_Raster_Params structure.                                       */
  /*                                                                       */
  /* <Values>                                                              */
  /*    QT_FT_RASTER_FLAG_DEFAULT :: This value is 0.                         */
  /*                                                                       */
  /*    QT_FT_RASTER_FLAG_AA      :: This flag is set to indicate that an     */
  /*                              anti-aliased glyph image should be       */
  /*                              generated.  Otherwise, it will be        */
  /*                              monochrome (1-bit).                      */
  /*                                                                       */
  /*    QT_FT_RASTER_FLAG_DIRECT  :: This flag is set to indicate direct      */
  /*                              rendering.  In this mode, client         */
  /*                              applications must provide their own span */
  /*                              callback.  This lets them directly       */
  /*                              draw or compose over an existing bitmap. */
  /*                              If this bit is not set, the target       */
  /*                              pixmap's buffer _must_ be zeroed before  */
  /*                              rendering.                               */
  /*                                                                       */
  /*                              Note that for now, direct rendering is   */
  /*                              only possible with anti-aliased glyphs.  */
  /*                                                                       */
  /*    QT_FT_RASTER_FLAG_CLIP    :: This flag is only used in direct         */
  /*                              rendering mode.  If set, the output will */
  /*                              be clipped to a box specified in the     */
  /*                              "clip_box" field of the QT_FT_Raster_Params */
  /*                              structure.                               */
  /*                                                                       */
  /*                              Note that by default, the glyph bitmap   */
  /*                              is clipped to the target pixmap, except  */
  /*                              in direct rendering mode where all spans */
  /*                              are generated if no clipping box is set. */
  /*                                                                       */
#define QT_FT_RASTER_FLAG_DEFAULT  0x0
#define QT_FT_RASTER_FLAG_AA       0x1
#define QT_FT_RASTER_FLAG_DIRECT   0x2
#define QT_FT_RASTER_FLAG_CLIP     0x4

  /* deprecated */
#define qt_ft_raster_flag_default  QT_FT_RASTER_FLAG_DEFAULT
#define qt_ft_raster_flag_aa       QT_FT_RASTER_FLAG_AA
#define qt_ft_raster_flag_direct   QT_FT_RASTER_FLAG_DIRECT
#define qt_ft_raster_flag_clip     QT_FT_RASTER_FLAG_CLIP


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Raster_Params                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to hold the arguments used by a raster's render        */
  /*    function.                                                          */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    target      :: The target bitmap.                                  */
  /*                                                                       */
  /*    source      :: A pointer to the source glyph image (e.g. an        */
  /*                   QT_FT_Outline).                                        */
  /*                                                                       */
  /*    flags       :: The rendering flags.                                */
  /*                                                                       */
  /*    gray_spans  :: The gray span drawing callback.                     */
  /*                                                                       */
  /*    black_spans :: The black span drawing callback.                    */
  /*                                                                       */
  /*    bit_test    :: The bit test callback.  UNIMPLEMENTED!              */
  /*                                                                       */
  /*    bit_set     :: The bit set callback.  UNIMPLEMENTED!               */
  /*                                                                       */
  /*    user        :: User-supplied data that is passed to each drawing   */
  /*                   callback.                                           */
  /*                                                                       */
  /*    clip_box    :: An optional clipping box.  It is only used in       */
  /*                   direct rendering mode.  Note that coordinates here  */
  /*                   should be expressed in _integer_ pixels (and not in */
  /*                   26.6 fixed-point units).                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An anti-aliased glyph bitmap is drawn if the QT_FT_RASTER_FLAG_AA bit */
  /*    flag is set in the `flags' field, otherwise a monochrome bitmap    */
  /*    will be generated.                                                 */
  /*                                                                       */
  /*    If the QT_FT_RASTER_FLAG_DIRECT bit flag is set in `flags', the       */
  /*    raster will call the `gray_spans' callback to draw gray pixel      */
  /*    spans, in the case of an aa glyph bitmap, it will call             */
  /*    `black_spans', and `bit_test' and `bit_set' in the case of a       */
  /*    monochrome bitmap.  This allows direct composition over a          */
  /*    pre-existing bitmap through user-provided callbacks to perform the */
  /*    span drawing/composition.                                          */
  /*                                                                       */
  /*    Note that the `bit_test' and `bit_set' callbacks are required when */
  /*    rendering a monochrome bitmap, as they are crucial to implement    */
  /*    correct drop-out control as defined in the TrueType specification. */
  /*                                                                       */
  typedef struct  QT_FT_Raster_Params_
  {
    QT_FT_Bitmap*              target;
    void*                   source;
    int                     flags;
    QT_FT_SpanFunc             gray_spans;
    QT_FT_SpanFunc             black_spans;
    QT_FT_Raster_BitTest_Func  bit_test;     /* doesn't work! */
    QT_FT_Raster_BitSet_Func   bit_set;      /* doesn't work! */
    void*                   user;
    QT_FT_BBox                 clip_box;
    int                     skip_spans;

  } QT_FT_Raster_Params;


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_NewFunc                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to create a new raster object.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to the memory allocator.                        */
  /*                                                                       */
  /* <Output>                                                              */
  /*    raster :: A handle to the new raster object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The `memory' parameter is a typeless pointer in order to avoid     */
  /*    un-wanted dependencies on the rest of the FreeType code.  In       */
  /*    practice, it is a QT_FT_Memory, i.e., a handle to the standard        */
  /*    FreeType memory allocator.  However, this field can be completely  */
  /*    ignored by a given raster implementation.                          */
  /*                                                                       */
  typedef int
  (*QT_FT_Raster_NewFunc)( QT_FT_Raster*  raster );

#define  QT_FT_Raster_New_Func    QT_FT_Raster_NewFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_DoneFunc                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to destroy a given raster object.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    raster :: A handle to the raster object.                           */
  /*                                                                       */
  typedef void
  (*QT_FT_Raster_DoneFunc)( QT_FT_Raster  raster );

#define  QT_FT_Raster_Done_Func   QT_FT_Raster_DoneFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_ResetFunc                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType provides an area of memory called the `render pool',      */
  /*    available to all registered rasters.  This pool can be freely used */
  /*    during a given scan-conversion but is shared by all rasters.  Its  */
  /*    content is thus transient.                                         */
  /*                                                                       */
  /*    This function is called each time the render pool changes, or just */
  /*    after a new raster object is created.                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    raster    :: A handle to the new raster object.                    */
  /*                                                                       */
  /*    pool_base :: The address in memory of the render pool.             */
  /*                                                                       */
  /*    pool_size :: The size in bytes of the render pool.                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Rasters can ignore the render pool and rely on dynamic memory      */
  /*    allocation if they want to (a handle to the memory allocator is    */
  /*    passed to the raster constructor).  However, this is not           */
  /*    recommended for efficiency purposes.                               */
  /*                                                                       */
  typedef void
  (*QT_FT_Raster_ResetFunc)( QT_FT_Raster       raster,
                          unsigned char*  pool_base,
                          unsigned long   pool_size );

#define  QT_FT_Raster_Reset_Func   QT_FT_Raster_ResetFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_SetModeFunc                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is a generic facility to change modes or attributes  */
  /*    in a given raster.  This can be used for debugging purposes, or    */
  /*    simply to allow implementation-specific `features' in a given      */
  /*    raster module.                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    raster :: A handle to the new raster object.                       */
  /*                                                                       */
  /*    mode   :: A 4-byte tag used to name the mode or property.          */
  /*                                                                       */
  /*    args   :: A pointer to the new mode/property to use.               */
  /*                                                                       */
  typedef int
  (*QT_FT_Raster_SetModeFunc)( QT_FT_Raster      raster,
                            unsigned long  mode,
                            void*          args );

#define  QT_FT_Raster_Set_Mode_Func  QT_FT_Raster_SetModeFunc

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    QT_FT_Raster_RenderFunc                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*   Invokes a given raster to scan-convert a given glyph image into a   */
  /*   target bitmap.                                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    raster :: A handle to the raster object.                           */
  /*                                                                       */
  /*    params :: A pointer to a QT_FT_Raster_Params structure used to store  */
  /*              the rendering parameters.                                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means success.                                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The exact format of the source image depends on the raster's glyph */
  /*    format defined in its QT_FT_Raster_Funcs structure.  It can be an     */
  /*    QT_FT_Outline or anything else in order to support a large array of   */
  /*    glyph formats.                                                     */
  /*                                                                       */
  /*    Note also that the render function can fail and return a           */
  /*    QT_FT_Err_Unimplemented_Feature error code if the raster used does    */
  /*    not support direct composition.                                    */
  /*                                                                       */
  /*    XXX: For now, the standard raster doesn't support direct           */
  /*         composition but this should change for the final release (see */
  /*         the files demos/src/ftgrays.c and demos/src/ftgrays2.c for    */
  /*         examples of distinct implementations which support direct     */
  /*         composition).                                                 */
  /*                                                                       */
  typedef int
  (*QT_FT_Raster_RenderFunc)( QT_FT_Raster          raster,
                           QT_FT_Raster_Params*  params );

#define  QT_FT_Raster_Render_Func    QT_FT_Raster_RenderFunc

  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    QT_FT_Raster_Funcs                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*   A structure used to describe a given raster class to the library.   */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    glyph_format  :: The supported glyph format for this raster.       */
  /*                                                                       */
  /*    raster_new    :: The raster constructor.                           */
  /*                                                                       */
  /*    raster_reset  :: Used to reset the render pool within the raster.  */
  /*                                                                       */
  /*    raster_render :: A function to render a glyph into a given bitmap. */
  /*                                                                       */
  /*    raster_done   :: The raster destructor.                            */
  /*                                                                       */
  typedef struct  QT_FT_Raster_Funcs_
  {
    QT_FT_Glyph_Format         glyph_format;
    QT_FT_Raster_NewFunc       raster_new;
    QT_FT_Raster_ResetFunc     raster_reset;
    QT_FT_Raster_SetModeFunc   raster_set_mode;
    QT_FT_Raster_RenderFunc    raster_render;
    QT_FT_Raster_DoneFunc      raster_done;

  } QT_FT_Raster_Funcs;


  /* */


QT_FT_END_HEADER

#endif /* __FTIMAGE_H__ */


/* END */
