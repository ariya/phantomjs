/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_custom_handler_private.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)
/**
 * @internal
 * Register a scheme handler.
 *
 * @param data Data of the handler including the protocol and the handler url.
 *
 * Emits signal: "protocolhandler,registration,requested" on View with a pointer to Ewk_Custom_Handler_Data.
 */
bool ewk_custom_handler_register_protocol_handler(Ewk_Custom_Handler_Data* data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(data->ewkView, false);
    evas_object_smart_callback_call(data->ewkView, "protocolhandler,registration,requested", data);
    return true;
}

#if ENABLE(CUSTOM_SCHEME_HANDLER)
/**
 * @internal
 * Query whether the handler is registered or not.
 *
 * @param data Data of the handler including the protocol and the handler url.
 *
 * Emits signal: "protocolhandler,isregistered" on View with a pointer to Ewk_Custom_Handler_Data.
 */
Ewk_Custom_Handlers_State ewk_custom_handler_is_protocol_handler_registered(Ewk_Custom_Handler_Data* data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(data->ewkView, EWK_CUSTOM_HANDLERS_DECLINED);
    evas_object_smart_callback_call(data->ewkView, "protocolhandler,isregistered", data);
    Ewk_Custom_Handlers_State result = data->result;
    return result;
}

/**
 * @internal
 * Remove the registered scheme handler.
 *
 * @param data Data of the handler including the protocol and the handler url.
 *
 * Emits signal: "protocolhandler,unregistration,requested" on View with a pointer to Ewk_Custom_Handler_Data.
 */
bool ewk_custom_handler_unregister_protocol_handler(Ewk_Custom_Handler_Data* data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(data->ewkView, false);
    evas_object_smart_callback_call(data->ewkView, "protocolhandler,unregistration,requested", data);
    return true;
}

#endif // ENABLE(CUSTOM_SCHEME_HANDLER)
#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)
