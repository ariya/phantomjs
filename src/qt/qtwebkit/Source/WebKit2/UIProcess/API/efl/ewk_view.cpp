/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#include "config.h"
#include "ewk_view.h"
#include "ewk_view_private.h"

#include "EwkView.h"
#include "FindClientEfl.h"
#include "FormClientEfl.h"
#include "InputMethodContextEfl.h"
#include "PageLoadClientEfl.h"
#include "PagePolicyClientEfl.h"
#include "PageUIClientEfl.h"
#include "PageViewportController.h"
#include "PageViewportControllerClientEfl.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_context.h"
#include "ewk_context_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_page_group.h"
#include "ewk_page_group_private.h"
#include "ewk_private.h"
#include "ewk_settings_private.h"
#include <Ecore_Evas.h>
#include <WebKit2/WKAPICast.h>
#include <WebKit2/WKData.h>
#include <WebKit2/WKEinaSharedString.h>
#include <WebKit2/WKFindOptions.h>
#include <WebKit2/WKInspector.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKView.h>
#include <wtf/text/CString.h>

#if ENABLE(INSPECTOR)
#include "WebInspectorProxy.h"
#endif

using namespace WebKit;

static inline EwkView* toEwkViewChecked(const Evas_Object* evasObject)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(evasObject, 0);
    if (!isEwkViewEvasObject(evasObject))
        return 0;

    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject));
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData, 0);

    return smartData->priv;
}

#define EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, ...)                        \
    EwkView* impl = toEwkViewChecked(ewkView);                                 \
    do {                                                                       \
        if (!impl) {                                                           \
            EINA_LOG_CRIT("no private data for object %p", ewkView);           \
            return __VA_ARGS__;                                                \
        }                                                                      \
    } while (0)


Eina_Bool ewk_view_smart_class_set(Ewk_View_Smart_Class* api)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(api, false);

    return EwkView::initSmartClassInterface(*api);
}

Evas_Object* EWKViewCreate(WKContextRef context, WKPageGroupRef pageGroup, Evas* canvas, Evas_Smart* smart)
{
    WKRetainPtr<WKViewRef> wkView = adoptWK(WKViewCreate(context, pageGroup));
    WKPageSetUseFixedLayout(WKViewGetPage(wkView.get()), true);
    if (EwkView* ewkView = EwkView::create(wkView.get(), canvas, smart))
        return ewkView->evasObject();

    return 0;
}

WKViewRef EWKViewGetWKView(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->wkView();
}

Evas_Object* ewk_view_smart_add(Evas* canvas, Evas_Smart* smart, Ewk_Context* context, Ewk_Page_Group* pageGroup)
{
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);
    EwkPageGroup* ewkPageGroup = ewk_object_cast<EwkPageGroup*>(pageGroup);

    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext->wkContext(), 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkPageGroup, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkPageGroup->wkPageGroup(), 0);

    return EWKViewCreate(ewkContext->wkContext(), ewkPageGroup->wkPageGroup(), canvas, smart);
}

Evas_Object* ewk_view_add(Evas* canvas)
{
    return EWKViewCreate(adoptWK(WKContextCreate()).get(), 0, canvas, 0);
}

Evas_Object* ewk_view_add_with_context(Evas* canvas, Ewk_Context* context)
{
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext->wkContext(), 0);

    return EWKViewCreate(ewkContext->wkContext(), 0, canvas, 0);
}

Ewk_Context* ewk_view_context_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->ewkContext();
}

Ewk_Page_Group* ewk_view_page_group_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->ewkPageGroup();
}

Eina_Bool ewk_view_url_set(Evas_Object* ewkView, const char* url)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

    WKRetainPtr<WKURLRef> wkUrl = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(impl->wkPage(), wkUrl.get());
    impl->informURLChange();

    return true;
}

const char* ewk_view_url_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->url();
}

Evas_Object* ewk_view_favicon_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->createFavicon();
}

Eina_Bool ewk_view_reload(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageReload(impl->wkPage());
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_reload_bypass_cache(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageReloadFromOrigin(impl->wkPage());
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_stop(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageStopLoading(impl->wkPage());

    return true;
}

Ewk_Settings* ewk_view_settings_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->settings();
}

const char* ewk_view_title_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->title();
}

double ewk_view_load_progress_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);

    return WKPageGetEstimatedProgress(impl->wkPage());
}

Eina_Bool ewk_view_scale_set(Evas_Object* ewkView, double scaleFactor, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetScaleFactor(impl->wkPage(), scaleFactor, WKPointMake(x, y));
    return true;
}

double ewk_view_scale_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1);

    return WKPageGetScaleFactor(impl->wkPage());
}

Eina_Bool ewk_view_device_pixel_ratio_set(Evas_Object* ewkView, float ratio)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setDeviceScaleFactor(ratio);

    return true;
}

float ewk_view_device_pixel_ratio_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);

    return WKPageGetBackingScaleFactor(impl->wkPage());
}

void ewk_view_theme_set(Evas_Object* ewkView, const char* path)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->setThemePath(path);
}

const char* ewk_view_theme_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->themePath();
}

Eina_Bool ewk_view_back(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageRef page = impl->wkPage();
    if (WKPageCanGoBack(page)) {
        WKPageGoBack(page);
        return true;
    }

    return false;
}

Eina_Bool ewk_view_forward(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageRef page = impl->wkPage();
    if (WKPageCanGoForward(page)) {
        WKPageGoForward(page);
        return true;
    }

    return false;
}

Eina_Bool ewk_view_back_possible(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    return WKPageCanGoBack(impl->wkPage());
}

Eina_Bool ewk_view_forward_possible(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    return WKPageCanGoForward(impl->wkPage());
}

Ewk_Back_Forward_List* ewk_view_back_forward_list_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->backForwardList();
}

Eina_Bool ewk_view_navigate_to(Evas_Object* ewkView, const Ewk_Back_Forward_List_Item* item)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkBackForwardListItem, item, itemImpl, false);

    WKPageGoToBackForwardListItem(impl->wkPage(), itemImpl->wkItem());

    return true;
}

Eina_Bool ewk_view_html_string_load(Evas_Object* ewkView, const char* html, const char* baseUrl, const char* unreachableUrl)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(html, false);

    WKRetainPtr<WKStringRef> wkHTMLString = adoptWK(WKStringCreateWithUTF8CString(html));
    WKRetainPtr<WKURLRef> wkBaseURL = adoptWK(WKURLCreateWithUTF8CString(baseUrl));

    if (unreachableUrl && *unreachableUrl) {
        WKRetainPtr<WKURLRef> wkUnreachableURL = adoptWK(WKURLCreateWithUTF8CString(unreachableUrl));
        WKPageLoadAlternateHTMLString(impl->wkPage(), wkHTMLString.get(), wkBaseURL.get(), wkUnreachableURL.get());
    } else
        WKPageLoadHTMLString(impl->wkPage(), wkHTMLString.get(), wkBaseURL.get());

    impl->informURLChange();

    return true;
}

const char* ewk_view_custom_encoding_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->customTextEncodingName();
}

Eina_Bool ewk_view_custom_encoding_set(Evas_Object* ewkView, const char* encoding)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setCustomTextEncodingName(encoding);

    return true;
}

const char* ewk_view_user_agent_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, 0);

    return impl->userAgent();
}

Eina_Bool ewk_view_user_agent_set(Evas_Object* ewkView, const char* userAgent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setUserAgent(userAgent);

    return true;
}

// EwkFindOptions should be matched up orders with WkFindOptions.
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_CASE_INSENSITIVE, kWKFindOptionsCaseInsensitive);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_AT_WORD_STARTS, kWKFindOptionsAtWordStarts);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START, kWKFindOptionsTreatMedialCapitalAsWordStart);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_BACKWARDS, kWKFindOptionsBackwards);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_WRAP_AROUND, kWKFindOptionsWrapAround);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_OVERLAY, kWKFindOptionsShowOverlay);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR, kWKFindOptionsShowFindIndicator);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_HIGHLIGHT, kWKFindOptionsShowHighlight);

Eina_Bool ewk_view_text_find(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    WKRetainPtr<WKStringRef> wkText = adoptWK(WKStringCreateWithUTF8CString(text));
    WKPageFindString(impl->wkPage(), wkText.get(), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageHideFindUI(impl->wkPage());

    return true;
}

Eina_Bool ewk_view_text_matches_count(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    WKRetainPtr<WKStringRef> wkText = adoptWK(WKStringCreateWithUTF8CString(text));
    WKPageCountStringMatches(impl->wkPage(), wkText.get(), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setMouseEventsEnabled(!!enabled);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->mouseEventsEnabled();
}

Eina_Bool ewk_view_feed_touch_event(Evas_Object* ewkView, Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers)
{
#if ENABLE(TOUCH_EVENTS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(points, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->feedTouchEvent(type, points, modifiers);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(type);
    UNUSED_PARAM(points);
    UNUSED_PARAM(modifiers);
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setTouchEventsEnabled(!!enabled);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(enabled);
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_get(const Evas_Object* ewkView)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->touchEventsEnabled();
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_inspector_show(Evas_Object* ewkView)
{
#if ENABLE(INSPECTOR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKInspectorRef wkInspector = WKPageGetInspector(impl->wkPage());
    if (wkInspector)
        WKInspectorShow(wkInspector);

    return true;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_inspector_close(Evas_Object* ewkView)
{
#if ENABLE(INSPECTOR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKInspectorRef wkInspector = WKPageGetInspector(impl->wkPage());
    if (wkInspector)
        WKInspectorClose(wkInspector);

    return true;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

// Ewk_Pagination_Mode should be matched up orders with WKPaginationMode.
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_UNPAGINATED, kWKPaginationModeUnpaginated);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_LEFT_TO_RIGHT, kWKPaginationModeLeftToRight);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_RIGHT_TO_LEFT, kWKPaginationModeRightToLeft);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_TOP_TO_BOTTOM, kWKPaginationModeTopToBottom);
COMPILE_ASSERT_MATCHING_ENUM(EWK_PAGINATION_MODE_BOTTOM_TO_TOP, kWKPaginationModeBottomToTop);

Eina_Bool ewk_view_pagination_mode_set(Evas_Object* ewkView, Ewk_Pagination_Mode mode)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetPaginationMode(impl->wkPage(), static_cast<WKPaginationMode>(mode));

    return true;
}

Ewk_Pagination_Mode ewk_view_pagination_mode_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EWK_PAGINATION_MODE_INVALID);
    
    return static_cast<Ewk_Pagination_Mode>(WKPageGetPaginationMode(impl->wkPage()));
}

Eina_Bool ewk_view_fullscreen_exit(Evas_Object* ewkView)
{
#if ENABLE(FULLSCREEN_API)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKViewExitFullScreen(impl->wkView());
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

void ewk_view_draws_page_background_set(Evas_Object *ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKViewSetDrawsBackground(impl->wkView(), enabled);
}

/// Creates a type name for Ewk_Page_Contents_Context.
typedef struct Ewk_Page_Contents_Context Ewk_Page_Contents_Context;

/*
 * @brief Structure containing page contents context used for ewk_view_page_contents_get() API.
 */
struct Ewk_Page_Contents_Context {
    Ewk_Page_Contents_Type type;
    Ewk_Page_Contents_Cb callback;
    void* userData;
};

/**
 * @internal
 * Callback function used for ewk_view_page_contents_get().
 */
static void ewkViewPageContentsAsMHTMLCallback(WKDataRef wkData, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_Page_Contents_Context* contentsContext = static_cast<Ewk_Page_Contents_Context*>(context);
    contentsContext->callback(contentsContext->type, reinterpret_cast<const char*>(WKDataGetBytes(wkData)), contentsContext->userData);

    delete contentsContext;
}

/**
 * @internal
 * Callback function used for ewk_view_page_contents_get().
 */
static void ewkViewPageContentsAsStringCallback(WKStringRef wkString, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_Page_Contents_Context* contentsContext = static_cast<Ewk_Page_Contents_Context*>(context);
    contentsContext->callback(contentsContext->type, WKEinaSharedString(wkString), contentsContext->userData);

    delete contentsContext;
}

Eina_Bool ewk_view_page_contents_get(const Evas_Object* ewkView, Ewk_Page_Contents_Type type, Ewk_Page_Contents_Cb callback, void* user_data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    Ewk_Page_Contents_Context* context = new Ewk_Page_Contents_Context;
    context->type = type;
    context->callback = callback;
    context->userData = user_data;

    switch (context->type) {
    case EWK_PAGE_CONTENTS_TYPE_MHTML:
        WKPageGetContentsAsMHTMLData(impl->wkPage(), false, context, ewkViewPageContentsAsMHTMLCallback);
        break;
    case EWK_PAGE_CONTENTS_TYPE_STRING:
        WKPageGetContentsAsString(impl->wkPage(), context, ewkViewPageContentsAsStringCallback);
        break;
    default:
        delete context;
        ASSERT_NOT_REACHED();
        return false;
    }

    return true;
}

Eina_Bool ewk_view_source_mode_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKViewSetShowsAsSource(impl->wkView(), enabled);

    return true;
}

Eina_Bool ewk_view_source_mode_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKViewGetShowsAsSource(impl->wkView());
}
