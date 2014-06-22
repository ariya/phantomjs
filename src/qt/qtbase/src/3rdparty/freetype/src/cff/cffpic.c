/***************************************************************************/
/*                                                                         */
/*  cffpic.c                                                               */
/*                                                                         */
/*    The FreeType position independent code services for cff module.      */
/*                                                                         */
/*  Copyright 2009 by                                                      */
/*  Oran Agra and Mickey Gabel.                                            */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "cffpic.h"

#ifdef FT_CONFIG_OPTION_PIC

  /* forward declaration of PIC init functions from cffdrivr.c */
  FT_Error FT_Create_Class_cff_services( FT_Library, FT_ServiceDescRec**);
  void FT_Destroy_Class_cff_services( FT_Library, FT_ServiceDescRec*);
  void FT_Init_Class_cff_service_ps_info( FT_Library, FT_Service_PsInfoRec*);
  void FT_Init_Class_cff_service_glyph_dict( FT_Library, FT_Service_GlyphDictRec*);
  void FT_Init_Class_cff_service_ps_name( FT_Library, FT_Service_PsFontNameRec*);
  void FT_Init_Class_cff_service_get_cmap_info( FT_Library, FT_Service_TTCMapsRec*);
  void FT_Init_Class_cff_service_cid_info( FT_Library, FT_Service_CIDRec*);

  /* forward declaration of PIC init functions from cffparse.c */
  FT_Error FT_Create_Class_cff_field_handlers( FT_Library, CFF_Field_Handler**);
  void FT_Destroy_Class_cff_field_handlers( FT_Library, CFF_Field_Handler*);

  /* forward declaration of PIC init functions from cffcmap.c */
  void FT_Init_Class_cff_cmap_encoding_class_rec( FT_Library, FT_CMap_ClassRec*);
  void FT_Init_Class_cff_cmap_unicode_class_rec( FT_Library, FT_CMap_ClassRec*);

  void
  cff_driver_class_pic_free(  FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Memory memory = library->memory;
    if ( pic_container->cff )
    {
      CffModulePIC* container = (CffModulePIC*)pic_container->cff;
      if(container->cff_services)
        FT_Destroy_Class_cff_services(library, container->cff_services);
      container->cff_services = NULL;
      if(container->cff_field_handlers)
        FT_Destroy_Class_cff_field_handlers(library, container->cff_field_handlers);
      container->cff_field_handlers = NULL;
      FT_FREE( container );
      pic_container->cff = NULL;
    }
  }

  FT_Error
  cff_driver_class_pic_init(  FT_Library library )
  {
    FT_PIC_Container* pic_container = &library->pic_container;
    FT_Error        error = FT_Err_Ok;
    CffModulePIC* container;
    FT_Memory memory = library->memory;

    /* allocate pointer, clear and set global container pointer */
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof(*container) );
    pic_container->cff = container;

    /* initialize pointer table - this is how the module usually expects this data */
    error = FT_Create_Class_cff_services(library, &container->cff_services);
    if(error) 
      goto Exit;
    error = FT_Create_Class_cff_field_handlers(library, &container->cff_field_handlers);
    if(error) 
      goto Exit;
    FT_Init_Class_cff_service_ps_info(library, &container->cff_service_ps_info);
    FT_Init_Class_cff_service_glyph_dict(library, &container->cff_service_glyph_dict);
    FT_Init_Class_cff_service_ps_name(library, &container->cff_service_ps_name);
    FT_Init_Class_cff_service_get_cmap_info(library, &container->cff_service_get_cmap_info);
    FT_Init_Class_cff_service_cid_info(library, &container->cff_service_cid_info);
    FT_Init_Class_cff_cmap_encoding_class_rec(library, &container->cff_cmap_encoding_class_rec);
    FT_Init_Class_cff_cmap_unicode_class_rec(library, &container->cff_cmap_unicode_class_rec);
Exit:
    if(error)
      cff_driver_class_pic_free(library);
    return error;
  }

#endif /* FT_CONFIG_OPTION_PIC */


/* END */
