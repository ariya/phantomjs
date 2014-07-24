/*
    Copyright (C) 2012 Samsung Electronics

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

#ifndef ewk_custom_handler_private_h
#define ewk_custom_handler_private_h

#include "ewk_view.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)
bool ewk_custom_handler_register_protocol_handler(Ewk_Custom_Handler_Data* data);

#if ENABLE(CUSTOM_SCHEME_HANDLER)
Ewk_Custom_Handlers_State ewk_custom_handler_is_protocol_handler_registered(Ewk_Custom_Handler_Data* data);
bool ewk_custom_handler_unregister_protocol_handler(Ewk_Custom_Handler_Data* data);

#endif // ENABLE(CUSTOM_SCHEME_HANDLER)
#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)
#endif // ewk_custom_handler_private_h
