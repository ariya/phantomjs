/*
 *  Copyright (C) 2011 Collabora Ltd.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Use this file to assert that various WebKit API enum values continue
// matching WebCore defined enum values.

#include "config.h"

#include "DumpRenderTreeSupportGtk.h"
#include "EditingBehaviorTypes.h"
#include "ErrorsGtk.h"
#include "FindOptions.h"
#include "FrameLoaderTypes.h"
#include "PasteboardHelper.h"
#include "webkiterror.h"
#include "webkitwebnavigationaction.h"
#include "webkitwebsettings.h"
#include "webkitwebview.h"
#include <wtf/Assertions.h>

#define COMPILE_ASSERT_MATCHING_ENUM(webkit_name, webcore_name) \
    COMPILE_ASSERT(int(webkit_name) == int(WebCore::webcore_name), mismatching_enums)

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_EDITING_BEHAVIOR_MAC, EditingMacBehavior);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_EDITING_BEHAVIOR_WINDOWS, EditingWindowsBehavior);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_EDITING_BEHAVIOR_UNIX, EditingUnixBehavior);

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED, NavigationTypeLinkClicked);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_NAVIGATION_REASON_FORM_SUBMITTED, NavigationTypeFormSubmitted);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_NAVIGATION_REASON_BACK_FORWARD, NavigationTypeBackForward);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_NAVIGATION_REASON_RELOAD, NavigationTypeReload);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_NAVIGATION_REASON_FORM_RESUBMITTED, NavigationTypeFormResubmitted);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_NAVIGATION_REASON_OTHER, NavigationTypeOther);

COMPILE_ASSERT_MATCHING_ENUM(WebKit::WebFindOptionsAtWordStarts, AtWordStarts);
COMPILE_ASSERT_MATCHING_ENUM(WebKit::WebFindOptionsTreatMedialCapitalAsWordStart, TreatMedialCapitalAsWordStart);
COMPILE_ASSERT_MATCHING_ENUM(WebKit::WebFindOptionsBackwards, Backwards);
COMPILE_ASSERT_MATCHING_ENUM(WebKit::WebFindOptionsWrapAround, WrapAround);
COMPILE_ASSERT_MATCHING_ENUM(WebKit::WebFindOptionsStartInSelection, StartInSelection);

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_VIEW_TARGET_INFO_HTML, PasteboardHelper::TargetTypeMarkup);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_VIEW_TARGET_INFO_TEXT, PasteboardHelper::TargetTypeText);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_VIEW_TARGET_INFO_IMAGE, PasteboardHelper::TargetTypeImage);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_VIEW_TARGET_INFO_URI_LIST, PasteboardHelper::TargetTypeURIList);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_WEB_VIEW_TARGET_INFO_NETSCAPE_URL, PasteboardHelper::TargetTypeNetscapeURL);

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_FAILED, NetworkErrorFailed);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_TRANSPORT, NetworkErrorTransport);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_UNKNOWN_PROTOCOL, NetworkErrorUnknownProtocol);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_CANCELLED, NetworkErrorCancelled);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_NETWORK_ERROR_FILE_DOES_NOT_EXIST, NetworkErrorFileDoesNotExist);

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_FAILED, PolicyErrorFailed);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_CANNOT_SHOW_MIME_TYPE, PolicyErrorCannotShowMimeType);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_CANNOT_SHOW_URL, PolicyErrorCannotShowURL);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_FRAME_LOAD_INTERRUPTED_BY_POLICY_CHANGE, PolicyErrorFrameLoadInterruptedByPolicyChange);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_POLICY_ERROR_CANNOT_USE_RESTRICTED_PORT, PolicyErrorCannotUseRestrictedPort);

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_FAILED, PluginErrorFailed);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_CANNOT_FIND_PLUGIN, PluginErrorCannotFindPlugin);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_CANNOT_LOAD_PLUGIN, PluginErrorCannotLoadPlugin);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_JAVA_UNAVAILABLE, PluginErrorJavaUnavailable);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_CONNECTION_CANCELLED, PluginErrorConnectionCancelled);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_PLUGIN_ERROR_WILL_HANDLE_LOAD, PluginErrorWillHandleLoad);
