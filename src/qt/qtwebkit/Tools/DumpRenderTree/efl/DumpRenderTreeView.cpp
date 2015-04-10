/*
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Red istributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define __STDC_FORMAT_MACROS
#include "config.h"
#include "DumpRenderTreeView.h"

#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "DumpRenderTreeEfl.h"
#include "TestRunner.h"
#include <EWebKit.h>
#include <Ecore.h>
#include <Eina.h>
#include <Evas.h>
#include <cstdio>
#include <cstdlib>
#include <inttypes.h>
#include <wtf/NotFound.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace std;

static Ewk_View_Smart_Class gParentSmartClass = EWK_VIEW_SMART_CLASS_INIT_NULL;

static WTF::String urlSuitableForTestResult(const WTF::String& uriString)
{
    if (uriString.isEmpty() || !uriString.startsWith("file://"))
        return uriString;

    const size_t index = uriString.reverseFind('/');
    return (index == WTF::notFound) ? uriString : uriString.substring(index + 1);
}

static void onConsoleMessage(Ewk_View_Smart_Data* smartData, const char* message, unsigned lineNumber, const char*)
{
    Evas_Object* evasObject = smartData->self;
    if (evas_object_data_get(evasObject, "ignore-console-messages"))
        return;

    // Tests expect only the filename part of local URIs
    WTF::String newMessage = WTF::String::fromUTF8(message);
    if (!newMessage.isEmpty()) {
        const size_t fileProtocol = newMessage.find("file://");
        if (fileProtocol != WTF::notFound)
            newMessage = newMessage.left(fileProtocol) + urlSuitableForTestResult(newMessage.substring(fileProtocol));
    }

    // Ignore simple translation-related messages and unnecessary messages
    if (newMessage.contains("Localized string") || newMessage.contains("Protocol Error: the message is for non-existing domain 'Profiler'"))
        return;

    printf("CONSOLE MESSAGE: ");
    if (lineNumber)
        printf("line %u: ", lineNumber);
    printf("%s\n", newMessage.utf8().data());
}

static void onJavaScriptAlert(Ewk_View_Smart_Data*, Evas_Object*, const char* message)
{
    printf("ALERT: %s\n", message);
    fflush(stdout);
}

static Eina_Bool onJavaScriptConfirm(Ewk_View_Smart_Data*, Evas_Object*, const char* message)
{
    printf("CONFIRM: %s\n", message);
    return EINA_TRUE;
}

static Eina_Bool onBeforeUnloadConfirm(Ewk_View_Smart_Data*, Evas_Object*, const char* message)
{
    printf("CONFIRM NAVIGATION: %s\n", message);
    return !gTestRunner->shouldStayOnPageAfterHandlingBeforeUnload();
}

static Eina_Bool onJavaScriptPrompt(Ewk_View_Smart_Data*, Evas_Object*, const char* message, const char* defaultValue, const char** value)
{
    printf("PROMPT: %s, default text: %s\n", message, defaultValue);
    *value = eina_stringshare_add(defaultValue);
    return EINA_TRUE;
}

static Evas_Object* onWindowCreate(Ewk_View_Smart_Data*, Eina_Bool, const Ewk_Window_Features*)
{
    return gTestRunner->canOpenWindows() ? browser->createNewWindow() : 0;
}

static Eina_Bool onWindowCloseDelayed(void* data)
{
    Evas_Object* view = static_cast<Evas_Object*>(data);
    browser->removeWindow(view);
    return EINA_FALSE;
}

static void onWindowClose(Ewk_View_Smart_Data* smartData)
{
    Evas_Object* view = smartData->self;
    ecore_idler_add(onWindowCloseDelayed, view);
}

static uint64_t onExceededDatabaseQuota(Ewk_View_Smart_Data* smartData, Evas_Object* frame, const char* databaseName, uint64_t currentSize, uint64_t expectedSize)
{
    if (!gTestRunner->dumpDatabaseCallbacks())
        return 0;

    Ewk_Security_Origin* origin = ewk_frame_security_origin_get(frame);
    printf("UI DELEGATE DATABASE CALLBACK: exceededDatabaseQuotaForSecurityOrigin:{%s, %s, %i} database:%s\n",
            ewk_security_origin_protocol_get(origin),
            ewk_security_origin_host_get(origin),
            ewk_security_origin_port_get(origin),
            databaseName);
    ewk_security_origin_free(origin);

    return 5 * 1024 * 1024;
}

static int64_t onExceededApplicationCacheQuota(Ewk_View_Smart_Data*, Ewk_Security_Origin *origin, int64_t defaultOriginQuota, int64_t totalSpaceNeeded)
{
    if (gTestRunner->dumpApplicationCacheDelegateCallbacks()) {
        // For example, numbers from 30000 - 39999 will output as 30000.
        // Rounding up or down does not really matter for these tests. It's
        // sufficient to just get a range of 10000 to determine if we were
        // above or below a threshold.
        int64_t truncatedSpaceNeeded = (totalSpaceNeeded / 10000) * 10000;
        printf("UI DELEGATE APPLICATION CACHE CALLBACK: exceededApplicationCacheOriginQuotaForSecurityOrigin:{%s, %s, %i} totalSpaceNeeded:~%" PRId64 "\n",
               ewk_security_origin_protocol_get(origin),
               ewk_security_origin_host_get(origin),
               ewk_security_origin_port_get(origin),
               truncatedSpaceNeeded);
    }

    if (gTestRunner->disallowIncreaseForApplicationCacheQuota())
        return 0;

    return defaultOriginQuota;
}

static bool shouldUseTiledBackingStore()
{
    const char* useTiledBackingStore = getenv("DRT_USE_TILED_BACKING_STORE");
    return useTiledBackingStore && *useTiledBackingStore == '1';
}

static bool chooseAndInitializeAppropriateSmartClass(Ewk_View_Smart_Class* api)
{
    return !shouldUseTiledBackingStore() ? ewk_view_single_smart_set(api) : ewk_view_tiled_smart_set(api);
}

// Taken from the file "WebKit/Tools/DumpRenderTree/chromium/WebViewHost.cpp".
static inline const char* navigationTypeToString(const Ewk_Navigation_Type type)
{
    switch (type) {
    case EWK_NAVIGATION_TYPE_LINK_CLICKED:
        return "link clicked";
    case EWK_NAVIGATION_TYPE_FORM_SUBMITTED:
        return "form submitted";
    case EWK_NAVIGATION_TYPE_BACK_FORWARD:
        return "back/forward";
    case EWK_NAVIGATION_TYPE_RELOAD:
        return "reload";
    case EWK_NAVIGATION_TYPE_FORM_RESUBMITTED:
        return "form resubmitted";
    case EWK_NAVIGATION_TYPE_OTHER:
        return "other";
    }
    return "illegal value";
}

static Eina_Bool onNavigationPolicyDecision(Ewk_View_Smart_Data*, Ewk_Frame_Resource_Request* request, Ewk_Navigation_Type navigationType)
{
    if (!policyDelegateEnabled)
        return true;

    printf("Policy delegate: attempt to load %s with navigation type '%s'\n", urlSuitableForTestResult(request->url).utf8().data(),
           navigationTypeToString(navigationType));

    if (gTestRunner)
        gTestRunner->notifyDone();

    return policyDelegatePermissive;
}

static Eina_Bool onFocusCanCycle(Ewk_View_Smart_Data*, Ewk_Focus_Direction)
{
    // This is the behavior of Mac and Chromium ports and is expected by some test cases.
    return true;
}

Evas_Object* drtViewAdd(Evas* evas)
{
    static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION("DRT_View");

    if (!chooseAndInitializeAppropriateSmartClass(&api))
        return 0;

    if (EINA_UNLIKELY(!gParentSmartClass.sc.add))
        ewk_view_base_smart_set(&gParentSmartClass);

    api.add_console_message = onConsoleMessage;
    api.run_javascript_alert = onJavaScriptAlert;
    api.run_javascript_confirm = onJavaScriptConfirm;
    api.run_before_unload_confirm = onBeforeUnloadConfirm;
    api.run_javascript_prompt = onJavaScriptPrompt;
    api.window_create = onWindowCreate;
    api.window_close = onWindowClose;
    api.exceeded_application_cache_quota = onExceededApplicationCacheQuota;
    api.exceeded_database_quota = onExceededDatabaseQuota;
    api.navigation_policy_decision = onNavigationPolicyDecision;
    api.focus_can_cycle = onFocusCanCycle;

    return evas_object_smart_add(evas, evas_smart_class_new(&api.sc));
}
