/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_navigation_policy_decision.h
 * @brief   Describes the Ewk navigation policy decision API.
 */

#ifndef ewk_navigation_policy_decision_h
#define ewk_navigation_policy_decision_h

#include "ewk_url_request.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Navigation_Policy_Decision as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Navigation_Policy_Decision;

/// Enum containing navigation types
typedef enum  {
    EWK_NAVIGATION_TYPE_LINK_ACTIVATED,
    EWK_NAVIGATION_TYPE_FORM_SUBMITTED,
    EWK_NAVIGATION_TYPE_BACK_FORWARD,
    EWK_NAVIGATION_TYPE_RELOAD,
    EWK_NAVIGATION_TYPE_FORM_RESUBMITTED,
    EWK_NAVIGATION_TYPE_OTHER
} Ewk_Navigation_Type;

/// Enum containing button types
typedef enum {
    EVENT_MOUSE_BUTTON_NONE = -1,
    EVENT_MOUSE_BUTTON_LEFT = 0,
    EVENT_MOUSE_BUTTON_MIDDLE = 1,
    EVENT_MOUSE_BUTTON_RIGHT = 2
} Event_Mouse_Button;

typedef enum {
    EVENT_MODIFIER_KEY_SHIFT = 1 << 0,
    EVENT_MODIFIER_KEY_CTRL = 1 << 1,
    EVENT_MODIFIER_KEY_ALT = 1 << 2,
    EVENT_MODIFIER_KEY_META = 1 << 3
} Event_Modifier_Keys;

/**
 * Query type for this navigation policy decision.
 *
 * @param decision navigation policy decision object to query.
 *
 * @return the type of navigation.
 */
EAPI Ewk_Navigation_Type ewk_navigation_policy_navigation_type_get(const Ewk_Navigation_Policy_Decision *decision);

/**
 * Query mouse button for this navigation policy decision.
 *
 * @param decision navigation policy decision object to query.
 *
 * @return the mouse button clicked to trigger the navigation.
 */
EAPI Event_Mouse_Button ewk_navigation_policy_mouse_button_get(const Ewk_Navigation_Policy_Decision *decision);

/**
 * Query modifier keys for this navigation policy decision.
 *
 * @param decision navigation policy decision object to query.
 *
 * @return the modifier keys used when triggering the navigation.
 */
EAPI Event_Modifier_Keys ewk_navigation_policy_modifiers_get(const Ewk_Navigation_Policy_Decision *decision);

/**
 * Query frame name for this navigation policy decision.
 *
 * The frame name is non-null for new window policy decisions only.
 *
 * @param decision navigation policy decision object to query.
 *
 * @return the frame name pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_navigation_policy_frame_name_get(const Ewk_Navigation_Policy_Decision *decision);

/**
 * Query URL request for this navigation policy decision.
 *
 * @param decision navigation policy decision object to query.
 *
 * @return The URL request pointer or @c NULL in case of error.
 */
EAPI Ewk_Url_Request *ewk_navigation_policy_request_get(const Ewk_Navigation_Policy_Decision *decision);

/**
 * Accepts the navigation request.
 *
 * The navigation will be accepted by default.
 *
 * @param decision navigation policy decision object to query.
 */
EAPI void ewk_navigation_policy_decision_accept(Ewk_Navigation_Policy_Decision *decision);

/**
 * Rejects the navigation request.
 *
 * @param decision navigation policy decision object to query.
 */
EAPI void ewk_navigation_policy_decision_reject(Ewk_Navigation_Policy_Decision *decision);

/**
 * Triggers a download instead of navigating to the url.
 *
 * @param decision navigation policy decision object to query.
 */
EAPI void ewk_navigation_policy_decision_download(Ewk_Navigation_Policy_Decision *decision);

#ifdef __cplusplus
}
#endif

#endif // ewk_navigation_policy_decision_h
