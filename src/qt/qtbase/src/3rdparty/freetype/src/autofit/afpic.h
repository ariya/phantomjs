/***************************************************************************/
/*                                                                         */
/*  afpic.h                                                                */
/*                                                                         */
/*    The FreeType position independent code services for autofit module.  */
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


#ifndef __AFPIC_H__
#define __AFPIC_H__

  
FT_BEGIN_HEADER

#include FT_INTERNAL_PIC_H

#ifndef FT_CONFIG_OPTION_PIC

#define AF_SCRIPT_CLASSES_GET         af_script_classes
#define AF_AF_AUTOFITTER_SERVICE_GET  af_autofitter_service

#else /* FT_CONFIG_OPTION_PIC */

#include "aftypes.h"

/* increase these when you add new scripts, and update autofit_module_class_pic_init */
#ifdef FT_OPTION_AUTOFIT2
  #define AF_SCRIPT_CLASSES_COUNT     6
#else
  #define AF_SCRIPT_CLASSES_COUNT     5  
#endif
#define AF_SCRIPT_CLASSES_REC_COUNT  (AF_SCRIPT_CLASSES_COUNT-1)    

  typedef struct AFModulePIC_
  {
    AF_ScriptClass    af_script_classes[AF_SCRIPT_CLASSES_COUNT];
    AF_ScriptClassRec af_script_classes_rec[AF_SCRIPT_CLASSES_REC_COUNT];
    FT_AutoHinter_ServiceRec af_autofitter_service;
  } AFModulePIC;

#define GET_PIC(lib)                  ((AFModulePIC*)((lib)->pic_container.autofit))
#define AF_SCRIPT_CLASSES_GET         (GET_PIC(FT_FACE_LIBRARY(globals->face))->af_script_classes)
#define AF_AF_AUTOFITTER_SERVICE_GET  (GET_PIC(library)->af_autofitter_service)

#endif /* FT_CONFIG_OPTION_PIC */

 /* */

FT_END_HEADER

#endif /* __AFPIC_H__ */


/* END */
