/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_private_h
#define ewk_private_h

#include "APICast.h"
#include <Evas.h>

// If defined, ewk will do type checking to ensure objects are of correct type
#define EWK_TYPE_CHECK 1
#define EWK_ARGB_BYTES_SIZE 4

// forward declarations
namespace WebCore {
#if USE(ACCELERATED_COMPOSITING)
class GraphicsContext3D;
class GraphicsLayer;
#endif
}

struct Ewk_Window_Object_Cleared_Event {
    JSContextRef context;
    JSObjectRef windowObject;
    Evas_Object* frame;
};

extern int _ewk_log_dom;

#define CRITICAL(...) EINA_LOG_DOM_CRIT(_ewk_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_ewk_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_ewk_log_dom, __VA_ARGS__)
#define INFO(...) EINA_LOG_DOM_INFO(_ewk_log_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(_ewk_log_dom, __VA_ARGS__)

#endif // ewk_private_h
