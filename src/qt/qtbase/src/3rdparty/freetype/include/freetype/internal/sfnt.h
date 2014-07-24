/***************************************************************************/
/*                                                                         */
/*  sfnt.h                                                                 */
/*                                                                         */
/*    High-level `sfnt' driver interface (specification).                  */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004, 2005, 2006 by                   */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __SFNT_H__
#define __SFNT_H__


#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Init_Face_Func                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    First part of the SFNT face object initialization.  This finds     */
  /*    the face in a SFNT file or collection, and load its format tag in  */
  /*    face->format_tag.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    face_index :: The index of the TrueType font, if we are opening a  */
  /*                  collection.                                          */
  /*                                                                       */
  /*    num_params :: The number of additional parameters.                 */
  /*                                                                       */
  /*    params     :: Optional additional parameters.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  /*    This function recognizes fonts embedded in a `TrueType             */
  /*    collection'.                                                       */
  /*                                                                       */
  /*    Once the format tag has been validated by the font driver, it      */
  /*    should then call the TT_Load_Face_Func() callback to read the rest */
  /*    of the SFNT tables in the object.                                  */
  /*                                                                       */
  typedef FT_Error
  (*TT_Init_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Face_Func                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Second part of the SFNT face object initialization.  This loads    */
  /*    the common SFNT tables (head, OS/2, maxp, metrics, etc.) in the    */
  /*    face object.                                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    face_index :: The index of the TrueType font, if we are opening a  */
  /*                  collection.                                          */
  /*                                                                       */
  /*    num_params :: The number of additional parameters.                 */
  /*                                                                       */
  /*    params     :: Optional additional parameters.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function must be called after TT_Init_Face_Func().            */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Done_Face_Func                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A callback used to delete the common SFNT data from a face.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT destroy the face object.                    */
  /*                                                                       */
  typedef void
  (*TT_Done_Face_Func)( TT_Face  face );


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_SFNT_HeaderRec_Func                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the header of a SFNT font file.  Supports collections.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face_index :: The index of the TrueType font, if we are opening a  */
  /*                  collection.                                          */
  /*                                                                       */
  /* <Output>                                                              */
  /*    sfnt       :: The SFNT header.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  /*    This function recognizes fonts embedded in a `TrueType             */
  /*    collection'.                                                       */
  /*                                                                       */
  /*    This function checks that the header is valid by looking at the    */
  /*    values of `search_range', `entry_selector', and `range_shift'.     */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_SFNT_HeaderRec_Func)( TT_Face      face,
                                  FT_Stream    stream,
                                  FT_Long      face_index,
                                  SFNT_Header  sfnt );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Directory_Func                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the table directory into a face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /*    sfnt   :: The SFNT header.                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be on the first byte after the 4-byte font  */
  /*    format tag.  This is the case just after a call to                 */
  /*    TT_Load_Format_Tag().                                              */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Directory_Func)( TT_Face      face,
                             FT_Stream    stream,
                             SFNT_Header  sfnt );

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Any_Func                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load any font table into client memory.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: The face object to look for.                             */
  /*                                                                       */
  /*    tag    :: The tag of table to load.  Use the value 0 if you want   */
  /*              to access the whole font file, else set this parameter   */
  /*              to a valid TrueType table tag that you can forge with    */
  /*              the MAKE_TT_TAG macro.                                   */
  /*                                                                       */
  /*    offset :: The starting offset in the table (or the file if         */
  /*              tag == 0).                                               */
  /*                                                                       */
  /*    length :: The address of the decision variable:                    */
  /*                                                                       */
  /*                If length == NULL:                                     */
  /*                  Loads the whole table.  Returns an error if          */
  /*                  `offset' == 0!                                       */
  /*                                                                       */
  /*                If *length == 0:                                       */
  /*                  Exits immediately; returning the length of the given */
  /*                  table or of the font file, depending on the value of */
  /*                  `tag'.                                               */
  /*                                                                       */
  /*                If *length != 0:                                       */
  /*                  Loads the next `length' bytes of table or font,      */
  /*                  starting at offset `offset' (in table or font too).  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    buffer :: The address of target buffer.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    TrueType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Any_Func)( TT_Face    face,
                       FT_ULong   tag,
                       FT_Long    offset,
                       FT_Byte   *buffer,
                       FT_ULong*  length );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Find_SBit_Image_Func                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Check whether an embedded bitmap (an `sbit') exists for a given    */
  /*    glyph, at a given strike.                                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    glyph_index   :: The glyph index.                                  */
  /*                                                                       */
  /*    strike_index  :: The current strike index.                         */
  /*                                                                       */
  /* <Output>                                                              */
  /*    arange        :: The SBit range containing the glyph index.        */
  /*                                                                       */
  /*    astrike       :: The SBit strike containing the glyph index.       */
  /*                                                                       */
  /*    aglyph_offset :: The offset of the glyph data in `EBDT' table.     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns                    */
  /*    SFNT_Err_Invalid_Argument if no sbit exists for the requested      */
  /*    glyph.                                                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Find_SBit_Image_Func)( TT_Face          face,
                              FT_UInt          glyph_index,
                              FT_ULong         strike_index,
                              TT_SBit_Range   *arange,
                              TT_SBit_Strike  *astrike,
                              FT_ULong        *aglyph_offset );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_SBit_Metrics_Func                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Get the big metrics for a given embedded bitmap.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream      :: The input stream.                                   */
  /*                                                                       */
  /*    range       :: The SBit range containing the glyph.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    big_metrics :: A big SBit metrics structure for the glyph.         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be positioned at the glyph's offset within  */
  /*    the `EBDT' table before the call.                                  */
  /*                                                                       */
  /*    If the image format uses variable metrics, the stream cursor is    */
  /*    positioned just after the metrics header in the `EBDT' table on    */
  /*    function exit.                                                     */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_SBit_Metrics_Func)( FT_Stream        stream,
                                TT_SBit_Range    range,
                                TT_SBit_Metrics  metrics );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_SBit_Image_Func                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load a given glyph sbit image from the font resource.  This also   */
  /*    returns its metrics.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face ::                                                            */
  /*      The target face object.                                          */
  /*                                                                       */
  /*    strike_index ::                                                    */
  /*      The strike index.                                                */
  /*                                                                       */
  /*    glyph_index ::                                                     */
  /*      The current glyph index.                                         */
  /*                                                                       */
  /*    load_flags ::                                                      */
  /*      The current load flags.                                          */
  /*                                                                       */
  /*    stream ::                                                          */
  /*      The input stream.                                                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amap ::                                                            */
  /*      The target pixmap.                                               */
  /*                                                                       */
  /*    ametrics ::                                                        */
  /*      A big sbit metrics structure for the glyph image.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    glyph sbit exists for the index.                                   */
  /*                                                                       */
  /*  <Note>                                                               */
  /*    The `map.buffer' field is always freed before the glyph is loaded. */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_SBit_Image_Func)( TT_Face              face,
                              FT_ULong             strike_index,
                              FT_UInt              glyph_index,
                              FT_UInt              load_flags,
                              FT_Stream            stream,
                              FT_Bitmap           *amap,
                              TT_SBit_MetricsRec  *ametrics );


#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Set_SBit_Strike_OldFunc                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Select an sbit strike for a given size request.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    req           :: The size request.                                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    astrike_index :: The index of the sbit strike.                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    sbit strike exists for the selected ppem values.                   */
  /*                                                                       */
  typedef FT_Error
  (*TT_Set_SBit_Strike_OldFunc)( TT_Face    face,
                                 FT_UInt    x_ppem,
                                 FT_UInt    y_ppem,
                                 FT_ULong*  astrike_index );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_CharMap_Load_Func                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given TrueType character map into memory.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the parent face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the current stream object.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    cmap   :: A pointer to a cmap object.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function assumes that the stream is already in use (i.e.,      */
  /*    opened).  In case of error, all partially allocated tables are     */
  /*    released.                                                          */
  /*                                                                       */
  typedef FT_Error
  (*TT_CharMap_Load_Func)( TT_Face    face,
                           void*      cmap,
                           FT_Stream  input );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_CharMap_Free_Func                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a character mapping table.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the parent face object.                        */
  /*                                                                       */
  /*    cmap :: A handle to a cmap object.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_CharMap_Free_Func)( TT_Face       face,
                           void*         cmap );

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Set_SBit_Strike_Func                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Select an sbit strike for a given size request.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    req           :: The size request.                                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    astrike_index :: The index of the sbit strike.                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    sbit strike exists for the selected ppem values.                   */
  /*                                                                       */
  typedef FT_Error
  (*TT_Set_SBit_Strike_Func)( TT_Face          face,
                              FT_Size_Request  req,
                              FT_ULong*        astrike_index );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Strike_Metrics_Func                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the metrics of a given strike.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    strike_index  :: The strike index.                                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    metrics       :: the metrics of the strike.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    such sbit strike exists.                                           */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Strike_Metrics_Func)( TT_Face           face,
                                  FT_ULong          strike_index,
                                  FT_Size_Metrics*  metrics );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Get_PS_Name_Func                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Get the PostScript glyph name of a glyph.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    idx  :: The glyph index.                                           */
  /*                                                                       */
  /*    PSname :: The address of a string pointer.  Will be NULL in case   */
  /*              of error, otherwise it is a pointer to the glyph name.   */
  /*                                                                       */
  /*              You must not modify the returned string!                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Get_PS_Name_Func)( TT_Face      face,
                          FT_UInt      idx,
                          FT_String**  PSname );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Metrics_Func                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load a metrics table, which is a table with a horizontal and a     */
  /*    vertical version.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load the vertical one.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Metrics_Func)( TT_Face    face,
                           FT_Stream  stream,
                           FT_Bool    vertical );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Get_Metrics_Func                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load the horizontal or vertical header in a face object.           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load vertical metrics.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Get_Metrics_Func)( TT_Face     face,
                          FT_Bool     vertical,
                          FT_UInt     gindex,
                          FT_Short*   abearing,
                          FT_UShort*  aadvance );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Table_Func                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Load a given TrueType table.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function uses `face->goto_table' to seek the stream to the     */
  /*    start of the table, except while loading the font directory.       */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Table_Func)( TT_Face    face,
                         FT_Stream  stream );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Free_Table_Func                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Free a given TrueType table.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  typedef void
  (*TT_Free_Table_Func)( TT_Face  face );


  /*
   * @functype:
   *    TT_Face_GetKerningFunc
   *
   * @description:
   *    Return the horizontal kerning value between two glyphs.
   *
   * @input:
   *    face        :: A handle to the source face object.
   *    left_glyph  :: The left glyph index.
   *    right_glyph :: The right glyph index.
   *
   * @return:
   *    The kerning value in font units.
   */
  typedef FT_Int
  (*TT_Face_GetKerningFunc)( TT_Face  face,
                             FT_UInt  left_glyph,
                             FT_UInt  right_glyph );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    SFNT_Interface                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This structure holds pointers to the functions used to load and    */
  /*    free the basic tables that are required in a `sfnt' font file.     */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    Check the various xxx_Func() descriptions for details.             */
  /*                                                                       */
  typedef struct  SFNT_Interface_
  {
    TT_Loader_GotoTableFunc      goto_table;

    TT_Init_Face_Func            init_face;
    TT_Load_Face_Func            load_face;
    TT_Done_Face_Func            done_face;
    FT_Module_Requester          get_interface;

    TT_Load_Any_Func             load_any;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_Load_SFNT_HeaderRec_Func  load_sfnt_header;
    TT_Load_Directory_Func       load_directory;
#endif

    /* these functions are called by `load_face' but they can also  */
    /* be called from external modules, if there is a need to do so */
    TT_Load_Table_Func           load_head;
    TT_Load_Metrics_Func         load_hhea;
    TT_Load_Table_Func           load_cmap;
    TT_Load_Table_Func           load_maxp;
    TT_Load_Table_Func           load_os2;
    TT_Load_Table_Func           load_post;

    TT_Load_Table_Func           load_name;
    TT_Free_Table_Func           free_name;

    /* optional tables */
#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_Load_Table_Func           load_hdmx_stub;
    TT_Free_Table_Func           free_hdmx_stub;
#endif

    /* this field was called `load_kerning' up to version 2.1.10 */
    TT_Load_Table_Func           load_kern;

    TT_Load_Table_Func           load_gasp;
    TT_Load_Table_Func           load_pclt;

    /* see `ttload.h'; this field was called `load_bitmap_header' up to */
    /* version 2.1.10                                                   */
    TT_Load_Table_Func           load_bhed;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS

    /* see `ttsbit.h' */
    TT_Set_SBit_Strike_OldFunc   set_sbit_strike_stub;
    TT_Load_Table_Func           load_sbits_stub;

    /*
     *  The following two fields appeared in version 2.1.8, and were placed
     *  between `load_sbits' and `load_sbit_image'.  We support them as a
     *  special exception since they are used by Xfont library within the
     *  X.Org xserver, and because the probability that other rogue clients
     *  use the other version 2.1.7 fields below is _extremely_ low.
     *
     *  Note that this forces us to disable an interesting memory-saving
     *  optimization though...
     */

    TT_Find_SBit_Image_Func      find_sbit_image;
    TT_Load_SBit_Metrics_Func    load_sbit_metrics;

#endif

    TT_Load_SBit_Image_Func      load_sbit_image;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_Free_Table_Func           free_sbits_stub;
#endif

    /* see `ttpost.h' */
    TT_Get_PS_Name_Func          get_psname;
    TT_Free_Table_Func           free_psnames;

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
    TT_CharMap_Load_Func         load_charmap_stub;
    TT_CharMap_Free_Func         free_charmap_stub;
#endif

    /* starting here, the structure differs from version 2.1.7 */

    /* this field was introduced in version 2.1.8, named `get_psname' */
    TT_Face_GetKerningFunc       get_kerning;

    /* new elements introduced after version 2.1.10 */

    /* load the font directory, i.e., the offset table and */
    /* the table directory                                 */
    TT_Load_Table_Func           load_font_dir;
    TT_Load_Metrics_Func         load_hmtx;

    TT_Load_Table_Func           load_eblc;
    TT_Free_Table_Func           free_eblc;

    TT_Set_SBit_Strike_Func      set_sbit_strike;
    TT_Load_Strike_Metrics_Func  load_strike_metrics;

    TT_Get_Metrics_Func          get_metrics;

  } SFNT_Interface;


  /* transitional */
  typedef SFNT_Interface*   SFNT_Service;

#ifndef FT_CONFIG_OPTION_PIC

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
#define FT_DEFINE_DRIVERS_OLD_INTERNAL(a) \
  a, 
#else
  #define FT_DEFINE_DRIVERS_OLD_INTERNAL(a)
#endif
#define FT_INTERNAL(a) \
  a, 

#define FT_DEFINE_SFNT_INTERFACE(class_,                                     \
    goto_table_, init_face_, load_face_, done_face_, get_interface_,         \
    load_any_, load_sfnt_header_, load_directory_, load_head_,               \
    load_hhea_, load_cmap_, load_maxp_, load_os2_, load_post_,               \
    load_name_, free_name_, load_hdmx_stub_, free_hdmx_stub_,                \
    load_kern_, load_gasp_, load_pclt_, load_bhed_,                          \
    set_sbit_strike_stub_, load_sbits_stub_, find_sbit_image_,               \
    load_sbit_metrics_, load_sbit_image_, free_sbits_stub_,                  \
    get_psname_, free_psnames_, load_charmap_stub_, free_charmap_stub_,      \
    get_kerning_, load_font_dir_, load_hmtx_, load_eblc_, free_eblc_,        \
    set_sbit_strike_, load_strike_metrics_, get_metrics_ )                   \
  static const SFNT_Interface class_ =                                       \
  {                                                                          \
    FT_INTERNAL(goto_table_) \
    FT_INTERNAL(init_face_) \
    FT_INTERNAL(load_face_) \
    FT_INTERNAL(done_face_) \
    FT_INTERNAL(get_interface_) \
    FT_INTERNAL(load_any_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sfnt_header_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_directory_) \
    FT_INTERNAL(load_head_) \
    FT_INTERNAL(load_hhea_) \
    FT_INTERNAL(load_cmap_) \
    FT_INTERNAL(load_maxp_) \
    FT_INTERNAL(load_os2_) \
    FT_INTERNAL(load_post_) \
    FT_INTERNAL(load_name_) \
    FT_INTERNAL(free_name_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_hdmx_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_hdmx_stub_) \
    FT_INTERNAL(load_kern_) \
    FT_INTERNAL(load_gasp_) \
    FT_INTERNAL(load_pclt_) \
    FT_INTERNAL(load_bhed_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(set_sbit_strike_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbits_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(find_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbit_metrics_) \
    FT_INTERNAL(load_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_sbits_stub_) \
    FT_INTERNAL(get_psname_) \
    FT_INTERNAL(free_psnames_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_charmap_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_charmap_stub_) \
    FT_INTERNAL(get_kerning_) \
    FT_INTERNAL(load_font_dir_) \
    FT_INTERNAL(load_hmtx_) \
    FT_INTERNAL(load_eblc_) \
    FT_INTERNAL(free_eblc_) \
    FT_INTERNAL(set_sbit_strike_) \
    FT_INTERNAL(load_strike_metrics_) \
    FT_INTERNAL(get_metrics_) \
  };

#else /* FT_CONFIG_OPTION_PIC */ 

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
#define FT_DEFINE_DRIVERS_OLD_INTERNAL(a, a_) \
  clazz->a = a_;
#else
  #define FT_DEFINE_DRIVERS_OLD_INTERNAL(a, a_)
#endif
#define FT_INTERNAL(a, a_) \
  clazz->a = a_;

#define FT_DEFINE_SFNT_INTERFACE(class_,                                     \
    goto_table_, init_face_, load_face_, done_face_, get_interface_,         \
    load_any_, load_sfnt_header_, load_directory_, load_head_,               \
    load_hhea_, load_cmap_, load_maxp_, load_os2_, load_post_,               \
    load_name_, free_name_, load_hdmx_stub_, free_hdmx_stub_,                \
    load_kern_, load_gasp_, load_pclt_, load_bhed_,                          \
    set_sbit_strike_stub_, load_sbits_stub_, find_sbit_image_,               \
    load_sbit_metrics_, load_sbit_image_, free_sbits_stub_,                  \
    get_psname_, free_psnames_, load_charmap_stub_, free_charmap_stub_,      \
    get_kerning_, load_font_dir_, load_hmtx_, load_eblc_, free_eblc_,        \
    set_sbit_strike_, load_strike_metrics_, get_metrics_ )                   \
  void                                                                       \
  FT_Init_Class_##class_( FT_Library library, SFNT_Interface*  clazz )       \
  {                                                                          \
    FT_UNUSED(library);                                                      \
    FT_INTERNAL(goto_table,goto_table_) \
    FT_INTERNAL(init_face,init_face_) \
    FT_INTERNAL(load_face,load_face_) \
    FT_INTERNAL(done_face,done_face_) \
    FT_INTERNAL(get_interface,get_interface_) \
    FT_INTERNAL(load_any,load_any_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sfnt_header,load_sfnt_header_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_directory,load_directory_) \
    FT_INTERNAL(load_head,load_head_) \
    FT_INTERNAL(load_hhea,load_hhea_) \
    FT_INTERNAL(load_cmap,load_cmap_) \
    FT_INTERNAL(load_maxp,load_maxp_) \
    FT_INTERNAL(load_os2,load_os2_) \
    FT_INTERNAL(load_post,load_post_) \
    FT_INTERNAL(load_name,load_name_) \
    FT_INTERNAL(free_name,free_name_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_hdmx_stub,load_hdmx_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_hdmx_stub,free_hdmx_stub_) \
    FT_INTERNAL(load_kern,load_kern_) \
    FT_INTERNAL(load_gasp,load_gasp_) \
    FT_INTERNAL(load_pclt,load_pclt_) \
    FT_INTERNAL(load_bhed,load_bhed_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(set_sbit_strike_stub,set_sbit_strike_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbits_stub,load_sbits_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(find_sbit_image,find_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_sbit_metrics,load_sbit_metrics_) \
    FT_INTERNAL(load_sbit_image,load_sbit_image_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_sbits_stub,free_sbits_stub_) \
    FT_INTERNAL(get_psname,get_psname_) \
    FT_INTERNAL(free_psnames,free_psnames_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(load_charmap_stub,load_charmap_stub_) \
    FT_DEFINE_DRIVERS_OLD_INTERNAL(free_charmap_stub,free_charmap_stub_) \
    FT_INTERNAL(get_kerning,get_kerning_) \
    FT_INTERNAL(load_font_dir,load_font_dir_) \
    FT_INTERNAL(load_hmtx,load_hmtx_) \
    FT_INTERNAL(load_eblc,load_eblc_) \
    FT_INTERNAL(free_eblc,free_eblc_) \
    FT_INTERNAL(set_sbit_strike,set_sbit_strike_) \
    FT_INTERNAL(load_strike_metrics,load_strike_metrics_) \
    FT_INTERNAL(get_metrics,get_metrics_) \
  } 

#endif /* FT_CONFIG_OPTION_PIC */ 

FT_END_HEADER

#endif /* __SFNT_H__ */


/* END */
