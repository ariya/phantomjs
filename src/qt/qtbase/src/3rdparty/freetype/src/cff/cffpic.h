/***************************************************************************/
/*                                                                         */
/*  cffpic.h                                                               */
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


#ifndef __CFFPIC_H__
#define __CFFPIC_H__

  
FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC
#define FT_CFF_SERVICE_PS_INFO_GET         cff_service_ps_info
#define FT_CFF_SERVICE_GLYPH_DICT_GET      cff_service_glyph_dict
#define FT_CFF_SERVICE_PS_NAME_GET         cff_service_ps_name
#define FT_CFF_SERVICE_GET_CMAP_INFO_GET   cff_service_get_cmap_info
#define FT_CFF_SERVICE_CID_INFO_GET        cff_service_cid_info
#define FT_CFF_SERVICES_GET                cff_services
#define FT_CFF_CMAP_ENCODING_CLASS_REC_GET cff_cmap_encoding_class_rec
#define FT_CFF_CMAP_UNICODE_CLASS_REC_GET  cff_cmap_unicode_class_rec
#define FT_CFF_FIELD_HANDLERS_GET          cff_field_handlers

#else /* FT_CONFIG_OPTION_PIC */

#include FT_SERVICE_GLYPH_DICT_H
#include "cffparse.h"
#include FT_SERVICE_POSTSCRIPT_INFO_H
#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_TT_CMAP_H
#include FT_SERVICE_CID_H

  typedef struct CffModulePIC_
  {
    FT_ServiceDescRec* cff_services;
    CFF_Field_Handler* cff_field_handlers;
    FT_Service_PsInfoRec cff_service_ps_info;
    FT_Service_GlyphDictRec cff_service_glyph_dict;
    FT_Service_PsFontNameRec cff_service_ps_name;
    FT_Service_TTCMapsRec  cff_service_get_cmap_info;
    FT_Service_CIDRec  cff_service_cid_info;
    FT_CMap_ClassRec cff_cmap_encoding_class_rec;
    FT_CMap_ClassRec cff_cmap_unicode_class_rec;
  } CffModulePIC;

#define GET_PIC(lib)                       ((CffModulePIC*)((lib)->pic_container.cff))
#define FT_CFF_SERVICE_PS_INFO_GET         (GET_PIC(library)->cff_service_ps_info)
#define FT_CFF_SERVICE_GLYPH_DICT_GET      (GET_PIC(library)->cff_service_glyph_dict)
#define FT_CFF_SERVICE_PS_NAME_GET         (GET_PIC(library)->cff_service_ps_name)
#define FT_CFF_SERVICE_GET_CMAP_INFO_GET   (GET_PIC(library)->cff_service_get_cmap_info)
#define FT_CFF_SERVICE_CID_INFO_GET        (GET_PIC(library)->cff_service_cid_info)
#define FT_CFF_SERVICES_GET                (GET_PIC(library)->cff_services)
#define FT_CFF_CMAP_ENCODING_CLASS_REC_GET (GET_PIC(library)->cff_cmap_encoding_class_rec)
#define FT_CFF_CMAP_UNICODE_CLASS_REC_GET  (GET_PIC(library)->cff_cmap_unicode_class_rec)
#define FT_CFF_FIELD_HANDLERS_GET          (GET_PIC(library)->cff_field_handlers)

#endif /* FT_CONFIG_OPTION_PIC */

 /* */

FT_END_HEADER

#endif /* __CFFPIC_H__ */


/* END */
