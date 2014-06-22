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
#include "EwkView.h"

#include "ContextMenuClientEfl.h"
#include "EflScreenUtilities.h"
#include "FindClientEfl.h"
#include "FormClientEfl.h"
#include "InputMethodContextEfl.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "NotImplemented.h"
#include "PageLoadClientEfl.h"
#include "PagePolicyClientEfl.h"
#include "PageUIClientEfl.h"
#include "PageViewportController.h"
#include "PageViewportControllerClientEfl.h"
#include "SnapshotImageGL.h"
#include "ViewClientEfl.h"
#include "WKArray.h"
#include "WKDictionary.h"
#include "WKEventEfl.h"
#include "WKGeometry.h"
#include "WKNumber.h"
#include "WKPageGroup.h"
#include "WKPopupItem.h"
#include "WKString.h"
#include "WKView.h"
#include "WKViewEfl.h"
#include "WebContext.h"
#include "WebImage.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_color_picker_private.h"
#include "ewk_context_menu_item_private.h"
#include "ewk_context_menu_private.h"
#include "ewk_context_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_page_group_private.h"
#include "ewk_popup_menu_item_private.h"
#include "ewk_popup_menu_private.h"
#include "ewk_private.h"
#include "ewk_security_origin_private.h"
#include "ewk_settings_private.h"
#include "ewk_view.h"
#include "ewk_window_features_private.h"
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Edje.h>
#if USE(ACCELERATED_COMPOSITING)
#include <Evas_GL.h>
#endif
#include <WebCore/CairoUtilitiesEfl.h>
#include <WebCore/Cursor.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/PlatformContextCairo.h>
#include <WebKit2/WKImageCairo.h>
#include <wtf/MathExtras.h>

#if ENABLE(VIBRATION)
#include "VibrationClientEfl.h"
#endif

#if ENABLE(FULLSCREEN_API)
#include "WebFullScreenManagerProxy.h"
#endif

using namespace EwkViewCallbacks;
using namespace WebCore;
using namespace WebKit;

static const char smartClassName[] = "EWK2_View";
static const int defaultCursorSize = 16;

// Auxiliary functions.

static inline void smartDataChanged(Ewk_View_Smart_Data* smartData)
{
    ASSERT(smartData);

    if (smartData->changed.any)
        return;

    smartData->changed.any = true;
    evas_object_smart_changed(smartData->self);
}

static Evas_Smart* defaultSmartClassInstance()
{
    static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(smartClassName);
    static Evas_Smart* smart = 0;

    if (!smart) {
        EwkView::initSmartClassInterface(api);
        smart = evas_smart_class_new(&api.sc);
    }

    return smart;
}

static inline Ewk_View_Smart_Data* toSmartData(Evas_Object* evasObject)
{
    ASSERT(evasObject);
    ASSERT(isEwkViewEvasObject(evasObject));

    return static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject));
}

static inline EwkView* toEwkView(const Ewk_View_Smart_Data* smartData)
{
    ASSERT(smartData);
    ASSERT(smartData->priv);

    return smartData->priv;
}

static inline void showEvasObjectsIfNeeded(const Ewk_View_Smart_Data* smartData)
{
    ASSERT(smartData);

    if (evas_object_clipees_get(smartData->base.clipper))
        evas_object_show(smartData->base.clipper);
    evas_object_show(smartData->image);
}

// EwkViewEventHandler implementation.

template <Evas_Callback_Type EventType>
class EwkViewEventHandler {
public:
    static void subscribe(Evas_Object* evasObject)
    {
        evas_object_event_callback_add(evasObject, EventType, handleEvent, toSmartData(evasObject));
    }

    static void unsubscribe(Evas_Object* evasObject)
    {
        evas_object_event_callback_del(evasObject, EventType, handleEvent);
    }

    static void handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo);
};

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_down)
        smartData->api->mouse_down(smartData, static_cast<Evas_Event_Mouse_Down*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_UP>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_up)
        smartData->api->mouse_up(smartData, static_cast<Evas_Event_Mouse_Up*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_move)
        smartData->api->mouse_move(smartData, static_cast<Evas_Event_Mouse_Move*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_FOCUS_IN>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->focus_in)
        smartData->api->focus_in(smartData);
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_FOCUS_OUT>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->focus_out)
        smartData->api->focus_out(smartData);
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->mouse_wheel)
        smartData->api->mouse_wheel(smartData, static_cast<Evas_Event_Mouse_Wheel*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_KEY_DOWN>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->key_down)
        smartData->api->key_down(smartData, static_cast<Evas_Event_Key_Down*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_KEY_UP>::handleEvent(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    if (smartData->api->key_up)
        smartData->api->key_up(smartData, static_cast<Evas_Event_Key_Up*>(eventInfo));
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_SHOW>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    WKViewSetIsVisible(toEwkView(smartData)->wkView(), true);
}

template <>
void EwkViewEventHandler<EVAS_CALLBACK_HIDE>::handleEvent(void* data, Evas*, Evas_Object*, void*)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    WKViewSetIsVisible(toEwkView(smartData)->wkView(), false);
}

typedef HashMap<WKPageRef, Evas_Object*> WKPageToEvasObjectMap;

static inline WKPageToEvasObjectMap& wkPageToEvasObjectMap()
{
    DEFINE_STATIC_LOCAL(WKPageToEvasObjectMap, map, ());
    return map;
}

// EwkView implementation.

EwkView::EwkView(WKViewRef view, Evas_Object* evasObject)
    : m_webView(view)
    , m_evasObject(evasObject)
    , m_context(EwkContext::findOrCreateWrapper(WKPageGetContext(wkPage())))
    , m_pageGroup(EwkPageGroup::findOrCreateWrapper(WKPageGetPageGroup(wkPage())))
#if USE(ACCELERATED_COMPOSITING)
    , m_pendingSurfaceResize(false)
#endif
    , m_pageLoadClient(PageLoadClientEfl::create(this))
    , m_pagePolicyClient(PagePolicyClientEfl::create(this))
    , m_pageUIClient(PageUIClientEfl::create(this))
    , m_contextMenuClient(ContextMenuClientEfl::create(this))
    , m_findClient(FindClientEfl::create(this))
    , m_formClient(FormClientEfl::create(this))
    , m_viewClient(ViewClientEfl::create(this))
#if ENABLE(VIBRATION)
    , m_vibrationClient(VibrationClientEfl::create(this))
#endif
    , m_backForwardList(EwkBackForwardList::create(WKPageGetBackForwardList(wkPage())))
    , m_settings(EwkSettings::create(this))
    , m_cursorIdentifier(0)
    , m_userAgent(WKEinaSharedString(AdoptWK, WKPageCopyUserAgent(wkPage())))
    , m_mouseEventsEnabled(false)
#if ENABLE(TOUCH_EVENTS)
    , m_touchEventsEnabled(false)
#endif
    , m_displayTimer(this, &EwkView::displayTimerFired)
    , m_inputMethodContext(InputMethodContextEfl::create(this, smartData()->base.evas))
#if USE(ACCELERATED_COMPOSITING)
    , m_pageViewportControllerClient(PageViewportControllerClientEfl::create(this))
    , m_pageViewportController(adoptPtr(new PageViewportController(page(), m_pageViewportControllerClient.get())))
#endif
    , m_isAccelerated(true)
{
    ASSERT(m_evasObject);
    ASSERT(m_context);

    // FIXME: Remove when possible.
    static_cast<WebViewEfl*>(webView())->setEwkView(this);
#if USE(ACCELERATED_COMPOSITING)
    m_evasGL = adoptPtr(evas_gl_new(evas_object_evas_get(m_evasObject)));
    if (m_evasGL)
        m_evasGLContext = EvasGLContext::create(m_evasGL.get());

    if (!m_evasGLContext) {
        WARN("Failed to create Evas_GL, falling back to software mode.");
        m_isAccelerated = false;
    }
#endif
    WKViewInitialize(wkView());

    WKPageGroupRef wkPageGroup = WKPageGetPageGroup(wkPage());
    WKPreferencesRef wkPreferences = WKPageGroupGetPreferences(wkPageGroup);
#if USE(ACCELERATED_COMPOSITING)
    WKPreferencesSetWebGLEnabled(wkPreferences, true);
#endif
    WKPreferencesSetFullScreenEnabled(wkPreferences, true);
    WKPreferencesSetWebAudioEnabled(wkPreferences, true);
    WKPreferencesSetOfflineWebApplicationCacheEnabled(wkPreferences, true);
#if ENABLE(SPELLCHECK)
    WKPreferencesSetAsynchronousSpellCheckingEnabled(wkPreferences, true);
#endif
    WKPreferencesSetInteractiveFormValidationEnabled(wkPreferences, true);

    // Enable mouse events by default
    setMouseEventsEnabled(true);

    // Listen for favicon changes.
    EwkFaviconDatabase* iconDatabase = m_context->faviconDatabase();
    ASSERT(iconDatabase);

    iconDatabase->watchChanges(IconChangeCallbackData(EwkView::handleFaviconChanged, this));

    WKPageToEvasObjectMap::AddResult result = wkPageToEvasObjectMap().add(wkPage(), m_evasObject);
    ASSERT_UNUSED(result, result.isNewEntry);
}

EwkView::~EwkView()
{
    // Unregister icon change callback.
    EwkFaviconDatabase* iconDatabase = m_context->faviconDatabase();
    ASSERT(iconDatabase);

    iconDatabase->unwatchChanges(EwkView::handleFaviconChanged);

    ASSERT(wkPageToEvasObjectMap().get(wkPage()) == m_evasObject);
    wkPageToEvasObjectMap().remove(wkPage());
}

EwkView* EwkView::create(WKViewRef webView, Evas* canvas, Evas_Smart* smart)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    Evas_Object* evasObject = evas_object_smart_add(canvas, smart ? smart : defaultSmartClassInstance());
    EINA_SAFETY_ON_NULL_RETURN_VAL(evasObject, 0);

    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    if (!smartData) {
        evas_object_del(evasObject);
        return 0;
    }

    ASSERT(!smartData->priv);

    smartData->priv = new EwkView(webView, evasObject);

    return smartData->priv;
}

bool EwkView::initSmartClassInterface(Ewk_View_Smart_Class& api)
{
    if (api.version != EWK_VIEW_SMART_CLASS_VERSION) {
        EINA_LOG_CRIT("Ewk_View_Smart_Class %p is version %lu while %lu was expected.",
            &api, api.version, EWK_VIEW_SMART_CLASS_VERSION);
        return false;
    }

    if (!parentSmartClass.add)
        evas_object_smart_clipped_smart_set(&parentSmartClass);

    evas_object_smart_clipped_smart_set(&api.sc);

    // Set Evas_Smart_Class callbacks.
    api.sc.add = handleEvasObjectAdd;
    api.sc.del = handleEvasObjectDelete;
    api.sc.move = handleEvasObjectMove;
    api.sc.resize = handleEvasObjectResize;
    api.sc.show = handleEvasObjectShow;
    api.sc.hide = handleEvasObjectHide;
    api.sc.color_set = handleEvasObjectColorSet;
    api.sc.calculate = handleEvasObjectCalculate;
    api.sc.data = smartClassName; // It is used for type checking.

    // Set Ewk_View_Smart_Class callbacks.
    api.focus_in = handleEwkViewFocusIn;
    api.focus_out = handleEwkViewFocusOut;
    api.mouse_wheel = handleEwkViewMouseWheel;
    api.mouse_down = handleEwkViewMouseDown;
    api.mouse_up = handleEwkViewMouseUp;
    api.mouse_move = handleEwkViewMouseMove;
    api.key_down = handleEwkViewKeyDown;
    api.key_up = handleEwkViewKeyUp;

    return true;
}

Evas_Object* EwkView::toEvasObject(WKPageRef page)
{
    ASSERT(page);
    return wkPageToEvasObjectMap().get(page);
}

WKPageRef EwkView::wkPage() const
{
    return WKViewGetPage(wkView());
}

void EwkView::setCursor(const Cursor& cursor)
{
    if (cursor.image()) {
        // Custom cursor.
        if (cursor.image() == m_cursorIdentifier)
            return;

        m_cursorIdentifier = cursor.image();

        Ewk_View_Smart_Data* sd = smartData();
        RefPtr<Evas_Object> cursorObject = adoptRef(cursor.image()->getEvasObject(sd->base.evas));
        if (!cursorObject)
            return;

        // Resize cursor.
        evas_object_resize(cursorObject.get(), cursor.image()->size().width(), cursor.image()->size().height());

        // Get cursor hot spot.
        IntPoint hotSpot;
        cursor.image()->getHotSpot(hotSpot);

        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        // ecore_evas takes care of freeing the cursor object.
        ecore_evas_object_cursor_set(ecoreEvas, cursorObject.release().leakRef(), EVAS_LAYER_MAX, hotSpot.x(), hotSpot.y());

        return;
    }

    // Standard cursor.
    const char* group = cursor.platformCursor();
    if (!group || group == m_cursorIdentifier)
        return;

    m_cursorIdentifier = group;
    Ewk_View_Smart_Data* sd = smartData();
    RefPtr<Evas_Object> cursorObject = adoptRef(edje_object_add(sd->base.evas));

    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
    if (!m_theme || !edje_object_file_set(cursorObject.get(), m_theme, group)) {
        ecore_evas_object_cursor_set(ecoreEvas, 0, 0, 0, 0);
#ifdef HAVE_ECORE_X
        if (WebCore::isUsingEcoreX(sd->base.evas))
            WebCore::applyFallbackCursor(ecoreEvas, group);
#endif
        return;
    }

    // Set cursor size.
    Evas_Coord width, height;
    edje_object_size_min_get(cursorObject.get(), &width, &height);
    if (width <= 0 || height <= 0)
        edje_object_size_min_calc(cursorObject.get(), &width, &height);
    if (width <= 0 || height <= 0) {
        width = defaultCursorSize;
        height = defaultCursorSize;
    }
    evas_object_resize(cursorObject.get(), width, height);

    // Get cursor hot spot.
    const char* data;
    int hotspotX = 0;
    data = edje_object_data_get(cursorObject.get(), "hot.x");
    if (data)
        hotspotX = atoi(data);

    int hotspotY = 0;
    data = edje_object_data_get(cursorObject.get(), "hot.y");
    if (data)
        hotspotY = atoi(data);

    // ecore_evas takes care of freeing the cursor object.
    ecore_evas_object_cursor_set(ecoreEvas, cursorObject.release().leakRef(), EVAS_LAYER_MAX, hotspotX, hotspotY);
}

void EwkView::setDeviceScaleFactor(float scale)
{
    const WKSize& deviceSize = WKViewGetSize(wkView());
    WKPageSetCustomBackingScaleFactor(wkPage(), scale);

    // Update internal viewport size after device-scale change.
    WKViewSetSize(wkView(), deviceSize);
}

float EwkView::deviceScaleFactor() const
{
    return WKPageGetBackingScaleFactor(wkPage());
}

AffineTransform EwkView::transformToScreen() const
{
    AffineTransform transform;

    int windowGlobalX = 0;
    int windowGlobalY = 0;

    Ewk_View_Smart_Data* sd = smartData();

#ifdef HAVE_ECORE_X
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);

    Ecore_X_Window window;
    window = ecore_evas_gl_x11_window_get(ecoreEvas);
    // Fallback to software mode if necessary.
    if (!window)
        window = ecore_evas_software_x11_window_get(ecoreEvas); // Returns 0 if none.

    int x, y; // x, y are relative to parent (in a reparenting window manager).
    while (window) {
        ecore_x_window_geometry_get(window, &x, &y, 0, 0);
        windowGlobalX += x;
        windowGlobalY += y;
        window = ecore_x_window_parent_get(window);
    }
#endif

    transform.translate(-sd->view.x, -sd->view.y);
    transform.translate(windowGlobalX, windowGlobalY);

    return transform;
}

inline Ewk_View_Smart_Data* EwkView::smartData() const
{
    return toSmartData(m_evasObject);
}

inline IntSize EwkView::size() const
{
    // WebPage expects a size in UI units, and not raw device units.
    FloatSize uiSize = deviceSize();
    uiSize.scale(1 / deviceScaleFactor());
    return roundedIntSize(uiSize);
}

inline IntSize EwkView::deviceSize() const
{
    return toIntSize(WKViewGetSize(wkView()));
}

void EwkView::displayTimerFired(Timer<EwkView>*)
{
    Ewk_View_Smart_Data* sd = smartData();

#if USE(ACCELERATED_COMPOSITING)
    if (m_pendingSurfaceResize) {
        // Create a GL surface here so that Evas has no chance of painting to an empty GL surface.
        if (!createGLSurface())
            return;
        // Make Evas objects visible here in order not to paint empty Evas objects with black color.
        showEvasObjectsIfNeeded(sd);

        m_pendingSurfaceResize = false;
    }
#endif

    if (!m_isAccelerated) {
        RefPtr<cairo_surface_t> surface = createSurfaceForImage(sd->image);
        if (!surface)
            return;

        WKViewPaintToCairoSurface(wkView(), surface.get());
        evas_object_image_data_update_add(sd->image, 0, 0, sd->view.w, sd->view.h);
        return;
    }
#if USE(ACCELERATED_COMPOSITING)
    evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context());

    WKViewPaintToCurrentGLContext(wkView());
#endif
    // sd->image is tied to a native surface, which is in the parent's coordinates.
    evas_object_image_data_update_add(sd->image, sd->view.x, sd->view.y, sd->view.w, sd->view.h);
}

void EwkView::scheduleUpdateDisplay()
{
    if (deviceSize().isEmpty())
        return;

    if (!m_displayTimer.isActive())
        m_displayTimer.startOneShot(0);
}

#if ENABLE(FULLSCREEN_API)
/**
 * @internal
 * Calls fullscreen_enter callback or falls back to default behavior and enables fullscreen mode.
 */
void EwkView::enterFullScreen()
{
    Ewk_View_Smart_Data* sd = smartData();

    RefPtr<EwkSecurityOrigin> origin = EwkSecurityOrigin::create(m_url);

    if (!sd->api->fullscreen_enter || !sd->api->fullscreen_enter(sd, origin.get())) {
        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_fullscreen_set(ecoreEvas, true);
    }
}

/**
 * @internal
 * Calls fullscreen_exit callback or falls back to default behavior and disables fullscreen mode.
 */
void EwkView::exitFullScreen()
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->fullscreen_exit || !sd->api->fullscreen_exit(sd)) {
        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_fullscreen_set(ecoreEvas, false);
    }
}
#endif

WKRect EwkView::windowGeometry() const
{
    Evas_Coord x, y, width, height;
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->window_geometry_get || !sd->api->window_geometry_get(sd, &x, &y, &width, &height)) {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_request_geometry_get(ee, &x, &y, &width, &height);
    }

    return WKRectMake(x, y, width, height);
}

void EwkView::setWindowGeometry(const WKRect& rect)
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->window_geometry_set || !sd->api->window_geometry_set(sd, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)) {
        Ecore_Evas* ee = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_move_resize(ee, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
    }
}

const char* EwkView::title() const
{
    m_title = WKEinaSharedString(AdoptWK, WKPageCopyTitle(wkPage()));

    return m_title;
}

/**
 * @internal
 * This function may return @c NULL.
 */
InputMethodContextEfl* EwkView::inputMethodContext()
{
    return m_inputMethodContext.get();
}

const char* EwkView::themePath() const
{
    return m_theme;
}

void EwkView::setThemePath(const char* theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        WKRetainPtr<WKStringRef> wkTheme = adoptWK(WKStringCreateWithUTF8CString(theme));
        WKViewSetThemePath(wkView(), wkTheme.get());
    }
}

void EwkView::setCustomTextEncodingName(const char* customEncoding)
{
    if (m_customEncoding == customEncoding)
        return;

    m_customEncoding = customEncoding;
    WKRetainPtr<WKStringRef> wkCustomEncoding = adoptWK(WKStringCreateWithUTF8CString(customEncoding));
    WKPageSetCustomTextEncodingName(wkPage(), wkCustomEncoding.get());
}

void EwkView::setUserAgent(const char* userAgent)
{
    if (m_userAgent == userAgent)
        return;

    WKRetainPtr<WKStringRef> wkUserAgent = adoptWK(WKStringCreateWithUTF8CString(userAgent));
    WKPageSetCustomUserAgent(wkPage(), wkUserAgent.get());

    // When 'userAgent' is 0, user agent is set as a standard user agent by WKPageSetCustomUserAgent()
    // so m_userAgent needs to be updated using WKPageCopyUserAgent().
    m_userAgent = WKEinaSharedString(AdoptWK, WKPageCopyUserAgent(wkPage()));
}

void EwkView::setMouseEventsEnabled(bool enabled)
{
    if (m_mouseEventsEnabled == enabled)
        return;

    m_mouseEventsEnabled = enabled;
    if (enabled) {
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::subscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_UP>::subscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::subscribe(m_evasObject);
    } else {
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_DOWN>::unsubscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_UP>::unsubscribe(m_evasObject);
        EwkViewEventHandler<EVAS_CALLBACK_MOUSE_MOVE>::unsubscribe(m_evasObject);
    }
}

#if ENABLE(TOUCH_EVENTS)
static WKTouchPointState toWKTouchPointState(Evas_Touch_Point_State state)
{
    switch (state) {
    case EVAS_TOUCH_POINT_UP:
        return kWKTouchPointStateTouchReleased;
    case EVAS_TOUCH_POINT_MOVE:
        return kWKTouchPointStateTouchMoved;
    case EVAS_TOUCH_POINT_DOWN:
        return kWKTouchPointStateTouchPressed;
    case EVAS_TOUCH_POINT_STILL:
        return kWKTouchPointStateTouchStationary;
    case EVAS_TOUCH_POINT_CANCEL:
    default:
        return kWKTouchPointStateTouchCancelled;
    }
}

static WKEventModifiers toWKEventModifiers(const Evas_Modifier* modifiers)
{
    WKEventModifiers wkModifiers = 0;
    if (evas_key_modifier_is_set(modifiers, "Shift"))
        wkModifiers |= kWKEventModifiersShiftKey;
    if (evas_key_modifier_is_set(modifiers, "Control"))
        wkModifiers |= kWKEventModifiersControlKey;
    if (evas_key_modifier_is_set(modifiers, "Alt"))
        wkModifiers |= kWKEventModifiersAltKey;
    if (evas_key_modifier_is_set(modifiers, "Meta"))
        wkModifiers |= kWKEventModifiersMetaKey;

    return wkModifiers;
}

void EwkView::feedTouchEvent(Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers)
{
    unsigned length = eina_list_count(points);
    OwnArrayPtr<WKTypeRef> touchPoints = adoptArrayPtr(new WKTypeRef[length]);
    for (unsigned i = 0; i < length; ++i) {
        Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(eina_list_nth(points, i));
        ASSERT(point);
        IntPoint position(point->x, point->y);
        touchPoints[i] = WKTouchPointCreate(point->id, toAPI(IntPoint(position)), toAPI(transformToScreen().mapPoint(position)), toWKTouchPointState(point->state), WKSizeMake(0, 0), 0, 1);
    }
    WKRetainPtr<WKArrayRef> wkTouchPoints(AdoptWK, WKArrayCreateAdoptingValues(touchPoints.get(), length));

    WKViewSendTouchEvent(wkView(), adoptWK(WKTouchEventCreate(static_cast<WKEventType>(type), wkTouchPoints.get(), toWKEventModifiers(modifiers), ecore_time_get())).get());
}

void EwkView::setTouchEventsEnabled(bool enabled)
{
    if (m_touchEventsEnabled == enabled)
        return;

    m_touchEventsEnabled = enabled;

    if (enabled) {
        // FIXME: We have to connect touch callbacks with mouse and multi events
        // because the Evas creates mouse events for first touch and multi events
        // for second and third touches. Below codes should be fixed when the Evas
        // supports the touch events.
        // See https://bugs.webkit.org/show_bug.cgi?id=97785 for details.
        Ewk_View_Smart_Data* sd = smartData();
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MOUSE_DOWN, handleTouchDown, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MOUSE_UP, handleTouchUp, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MOUSE_MOVE, handleTouchMove, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MULTI_DOWN, handleTouchDown, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MULTI_UP, handleTouchUp, sd);
        evas_object_event_callback_add(m_evasObject, EVAS_CALLBACK_MULTI_MOVE, handleTouchMove, sd);
    } else {
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MOUSE_DOWN, handleTouchDown);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MOUSE_UP, handleTouchUp);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MOUSE_MOVE, handleTouchMove);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MULTI_DOWN, handleTouchDown);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MULTI_UP, handleTouchUp);
        evas_object_event_callback_del(m_evasObject, EVAS_CALLBACK_MULTI_MOVE, handleTouchMove);
    }
}

void EwkView::doneWithTouchEvent(WKTouchEventRef, bool /* wasEventHandled */)
{
    notImplemented();
}
#endif

#if USE(ACCELERATED_COMPOSITING)
bool EwkView::createGLSurface()
{
    if (!m_isAccelerated)
        return true;

    static Evas_GL_Config evasGLConfig = {
        EVAS_GL_RGBA_8888,
        EVAS_GL_DEPTH_BIT_8,
        EVAS_GL_STENCIL_NONE,
        EVAS_GL_OPTIONS_NONE,
        EVAS_GL_MULTISAMPLE_NONE
    };

    // Recreate to current size: Replaces if non-null, and frees existing surface after (OwnPtr).
    m_evasGLSurface = EvasGLSurface::create(m_evasGL.get(), &evasGLConfig, deviceSize());
    if (!m_evasGLSurface)
        return false;

    Evas_Native_Surface nativeSurface;
    evas_gl_native_surface_get(m_evasGL.get(), m_evasGLSurface->surface(), &nativeSurface);
    evas_object_image_native_surface_set(smartData()->image, &nativeSurface);

    evas_gl_make_current(m_evasGL.get(), m_evasGLSurface->surface(), m_evasGLContext->context());

    Evas_GL_API* gl = evas_gl_api_get(m_evasGL.get());

    WKPoint boundsEnd = WKViewUserViewportToScene(wkView(), WKPointMake(deviceSize().width(), deviceSize().height()));
    gl->glViewport(0, 0, boundsEnd.x, boundsEnd.y);
    gl->glClearColor(1.0, 1.0, 1.0, 0);
    gl->glClear(GL_COLOR_BUFFER_BIT);

    return true;
}
#endif

#if ENABLE(INPUT_TYPE_COLOR)
/**
 * @internal
 * Requests to show external color picker.
 */
void EwkView::requestColorPicker(WKColorPickerResultListenerRef listener, const WebCore::Color& color)
{
    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api->input_picker_color_request);

    if (!sd->api->input_picker_color_request)
        return;

    if (m_colorPicker)
        dismissColorPicker();

    m_colorPicker = EwkColorPicker::create(listener, color);

    sd->api->input_picker_color_request(sd, m_colorPicker.get());
}

/**
 * @internal
 * Requests to hide external color picker.
 */
void EwkView::dismissColorPicker()
{
    if (!m_colorPicker)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api->input_picker_color_dismiss);

    if (sd->api->input_picker_color_dismiss)
        sd->api->input_picker_color_dismiss(sd);

    m_colorPicker.clear();
}
#endif

COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_RIGHT_TO_LEFT, RTL);
COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_LEFT_TO_RIGHT, LTR);

void EwkView::customContextMenuItemSelected(WKContextMenuItemRef contextMenuItem)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->custom_item_selected)
        return;

    OwnPtr<EwkContextMenuItem> item = EwkContextMenuItem::create(contextMenuItem, 0);

    sd->api->custom_item_selected(sd, item.get());
}

void EwkView::showContextMenu(WKPoint position, WKArrayRef items)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->context_menu_show)
        return;

    if (m_contextMenu)
        hideContextMenu();

    m_contextMenu = EwkContextMenu::create(this, items);

    position = WKViewContentsToUserViewport(wkView(), position);

    sd->api->context_menu_show(sd, position.x, position.y, m_contextMenu.get());
}

void EwkView::hideContextMenu()
{
    if (!m_contextMenu)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (sd->api->context_menu_hide)
        sd->api->context_menu_hide(sd);

    m_contextMenu.clear();
}

COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_RIGHT_TO_LEFT, kWKPopupItemTextDirectionRTL);
COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_LEFT_TO_RIGHT, kWKPopupItemTextDirectionLTR);

void EwkView::requestPopupMenu(WKPopupMenuListenerRef popupMenuListener, const WKRect& rect, WKPopupItemTextDirection textDirection, double pageScaleFactor, WKArrayRef items, int32_t selectedIndex)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    ASSERT(popupMenuListener);

    if (!sd->api->popup_menu_show)
        return;

    if (m_popupMenu)
        closePopupMenu();

    m_popupMenu = EwkPopupMenu::create(this, popupMenuListener, items, selectedIndex);

    WKPoint popupMenuPosition = WKViewContentsToUserViewport(wkView(), rect.origin);

    Eina_Rectangle einaRect;
    EINA_RECTANGLE_SET(&einaRect, popupMenuPosition.x, popupMenuPosition.y, rect.size.width, rect.size.height);

    sd->api->popup_menu_show(sd, einaRect, static_cast<Ewk_Text_Direction>(textDirection), pageScaleFactor, m_popupMenu.get());
}

void EwkView::closePopupMenu()
{
    if (!m_popupMenu)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (sd->api->popup_menu_hide)
        sd->api->popup_menu_hide(sd);

    m_popupMenu.clear();
}

/**
 * @internal
 * Calls a smart member function for javascript alert().
 */
void EwkView::requestJSAlertPopup(const WKEinaSharedString& message)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_alert)
        return;

    sd->api->run_javascript_alert(sd, message);
}

/**
 * @internal
 * Calls a smart member function for javascript confirm() and returns a value from the function. Returns false by default.
 */
bool EwkView::requestJSConfirmPopup(const WKEinaSharedString& message)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_confirm)
        return false;

    return sd->api->run_javascript_confirm(sd, message);
}

/**
 * @internal
 * Calls a smart member function for javascript prompt() and returns a value from the function. Returns null string by default.
 */
WKEinaSharedString EwkView::requestJSPromptPopup(const WKEinaSharedString& message, const WKEinaSharedString& defaultValue)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_prompt)
        return WKEinaSharedString();

    return WKEinaSharedString::adopt(sd->api->run_javascript_prompt(sd, message, defaultValue));
}

#if ENABLE(SQL_DATABASE)
/**
 * @internal
 * Calls exceeded_database_quota callback or falls back to default behavior returns default database quota.
 */
unsigned long long EwkView::informDatabaseQuotaReached(const String& databaseName, const String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    static const unsigned long long defaultQuota = 5 * 1024 * 1204; // 5 MB
    if (sd->api->exceeded_database_quota)
        return sd->api->exceeded_database_quota(sd, databaseName.utf8().data(), displayName.utf8().data(), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);

    return defaultQuota;
}
#endif

WebView* EwkView::webView()
{
    return toImpl(m_webView.get());
}

/**
 * @internal
 * The url of view was changed by the frame loader.
 *
 * Emits signal: "url,changed" with pointer to new url string.
 */
void EwkView::informURLChange()
{
    WKRetainPtr<WKURLRef> wkActiveURL = adoptWK(WKPageCopyActiveURL(wkPage()));
    WKRetainPtr<WKStringRef> wkURLString = wkActiveURL ? adoptWK(WKURLCopyString(wkActiveURL.get())) : adoptWK(WKStringCreateWithUTF8CString(""));

    if (WKStringIsEqualToUTF8CString(wkURLString.get(), m_url))
        return;

    m_url = WKEinaSharedString(wkURLString.get());
    smartCallback<URLChanged>().call(m_url);

    // Update the view's favicon.
    smartCallback<FaviconChanged>().call();
}

Evas_Object* EwkView::createFavicon() const
{
    EwkFaviconDatabase* iconDatabase = m_context->faviconDatabase();
    ASSERT(iconDatabase);

    return ewk_favicon_database_icon_get(iconDatabase, m_url, smartData()->base.evas);
}

EwkWindowFeatures* EwkView::windowFeatures()
{
    if (!m_windowFeatures)
        m_windowFeatures = EwkWindowFeatures::create(0, this);

    return m_windowFeatures.get();
}

WKPageRef EwkView::createNewPage(PassRefPtr<EwkUrlRequest> request, WKDictionaryRef windowFeatures)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->window_create)
        return 0;

    RefPtr<EwkWindowFeatures> ewkWindowFeatures = EwkWindowFeatures::create(windowFeatures, this);

    Evas_Object* newEwkView = sd->api->window_create(sd, request->url(), ewkWindowFeatures.get());
    if (!newEwkView)
        return 0;

    EwkView* newViewImpl = toEwkView(newEwkView);
    ASSERT(newViewImpl);

    newViewImpl->m_windowFeatures = ewkWindowFeatures;

    return static_cast<WKPageRef>(WKRetain(newViewImpl->page()));
}

void EwkView::close()
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->window_close)
        return;

    sd->api->window_close(sd);
}

void EwkView::handleEvasObjectAdd(Evas_Object* evasObject)
{
    const Evas_Smart* smart = evas_object_smart_smart_get(evasObject);
    const Evas_Smart_Class* smartClass = evas_smart_class_get(smart);
    const Ewk_View_Smart_Class* api = reinterpret_cast<const Ewk_View_Smart_Class*>(smartClass);
    ASSERT(api);

    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);

    if (!smartData) {
        // Allocating with 'calloc' as the API contract is that it should be deleted with 'free()'.
        smartData = static_cast<Ewk_View_Smart_Data*>(calloc(1, sizeof(Ewk_View_Smart_Data)));
        evas_object_smart_data_set(evasObject, smartData);
    }

    smartData->self = evasObject;
    smartData->api = api;

    parentSmartClass.add(evasObject);

    smartData->priv = 0; // Will be initialized further.

    // Create evas_object_image to draw web contents.
    smartData->image = evas_object_image_add(smartData->base.evas);
    evas_object_image_alpha_set(smartData->image, false);
    evas_object_image_filled_set(smartData->image, true);
    evas_object_smart_member_add(smartData->image, evasObject);
    evas_object_show(smartData->image);

    EwkViewEventHandler<EVAS_CALLBACK_FOCUS_IN>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_FOCUS_OUT>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_MOUSE_WHEEL>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_KEY_DOWN>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_KEY_UP>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_SHOW>::subscribe(evasObject);
    EwkViewEventHandler<EVAS_CALLBACK_HIDE>::subscribe(evasObject);
}

void EwkView::handleEvasObjectDelete(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    if (smartData) {
        ASSERT(smartData->priv); // smartData->priv is EwkView instance.
        delete smartData->priv;
    }

    parentSmartClass.del(evasObject);
}

void EwkView::handleEvasObjectResize(Evas_Object* evasObject, Evas_Coord width, Evas_Coord height)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    evas_object_resize(smartData->image, width, height);
    evas_object_image_size_set(smartData->image, width, height);
    evas_object_image_fill_set(smartData->image, 0, 0, width, height);

    smartData->changed.size = true;
    smartDataChanged(smartData);
}

void EwkView::handleEvasObjectMove(Evas_Object* evasObject, Evas_Coord /*x*/, Evas_Coord /*y*/)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    smartData->changed.position = true;
    smartDataChanged(smartData);
}

void EwkView::handleEvasObjectCalculate(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    EwkView* self = toEwkView(smartData);

    smartData->changed.any = false;

    Evas_Coord x, y, width, height;
    evas_object_geometry_get(evasObject, &x, &y, &width, &height);

    if (smartData->changed.position) {
        smartData->changed.position = false;
        smartData->view.x = x;
        smartData->view.y = y;
        evas_object_move(smartData->image, x, y);
        WKViewSetUserViewportTranslation(self->wkView(), x, y);
    }

    if (smartData->changed.size) {
        smartData->changed.size = false;
        smartData->view.w = width;
        smartData->view.h = height;

        WKViewSetSize(self->wkView(), WKSizeMake(width, height));
#if USE(ACCELERATED_COMPOSITING)
        if (WKPageUseFixedLayout(self->wkPage()))
            self->pageViewportController()->didChangeViewportSize(self->size());

        self->setNeedsSurfaceResize();
#endif
    }
}

void EwkView::handleEvasObjectShow(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    if (!toEwkView(smartData)->m_isAccelerated)
        showEvasObjectsIfNeeded(smartData);
}

void EwkView::handleEvasObjectHide(Evas_Object* evasObject)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    evas_object_hide(smartData->base.clipper);
    evas_object_hide(smartData->image);
}

void EwkView::handleEvasObjectColorSet(Evas_Object* evasObject, int red, int green, int blue, int alpha)
{
    Ewk_View_Smart_Data* smartData = toSmartData(evasObject);
    ASSERT(smartData);

    EwkView* view = toEwkView(smartData);
    ASSERT(view);

    alpha = clampTo(alpha, 0, 255);
    red = clampTo(red, 0, alpha);
    green = clampTo(green, 0, alpha);
    blue = clampTo(blue, 0, alpha);

    evas_object_image_alpha_set(smartData->image, alpha < 255);
    WKViewSetDrawsBackground(view->wkView(), red || green || blue);
    WKViewSetDrawsTransparentBackground(view->wkView(), alpha < 255);

    parentSmartClass.color_set(evasObject, red, green, blue, alpha);
}

Eina_Bool EwkView::handleEwkViewFocusIn(Ewk_View_Smart_Data* smartData)
{
    WKViewSetIsFocused(toEwkView(smartData)->wkView(), true);
    return true;
}

Eina_Bool EwkView::handleEwkViewFocusOut(Ewk_View_Smart_Data* smartData)
{
    WKViewSetIsFocused(toEwkView(smartData)->wkView(), false);
    return true;
}

Eina_Bool EwkView::handleEwkViewMouseWheel(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Wheel* wheelEvent)
{
    EwkView* self = toEwkView(smartData);
    self->page()->handleWheelEvent(NativeWebWheelEvent(wheelEvent, self->webView()->transformFromScene(), self->transformToScreen()));
    return true;
}

Eina_Bool EwkView::handleEwkViewMouseDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Down* downEvent)
{
    EwkView* self = toEwkView(smartData);
    self->page()->handleMouseEvent(NativeWebMouseEvent(downEvent, self->webView()->transformFromScene(), self->transformToScreen()));
    return true;
}

Eina_Bool EwkView::handleEwkViewMouseUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Up* upEvent)
{
    EwkView* self = toEwkView(smartData);
    self->page()->handleMouseEvent(NativeWebMouseEvent(upEvent, self->webView()->transformFromScene(), self->transformToScreen()));

    if (InputMethodContextEfl* inputMethodContext = self->inputMethodContext())
        inputMethodContext->handleMouseUpEvent(upEvent);

    return true;
}

Eina_Bool EwkView::handleEwkViewMouseMove(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Move* moveEvent)
{
    EwkView* self = toEwkView(smartData);
    self->page()->handleMouseEvent(NativeWebMouseEvent(moveEvent, self->webView()->transformFromScene(), self->transformToScreen()));
    return true;
}

Eina_Bool EwkView::handleEwkViewKeyDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Down* downEvent)
{
    bool isFiltered = false;
    EwkView* self = toEwkView(smartData);
    if (InputMethodContextEfl* inputMethodContext = self->inputMethodContext())
        inputMethodContext->handleKeyDownEvent(downEvent, &isFiltered);

    self->page()->handleKeyboardEvent(NativeWebKeyboardEvent(downEvent, isFiltered));
    return true;
}

Eina_Bool EwkView::handleEwkViewKeyUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Up* upEvent)
{
    toEwkView(smartData)->page()->handleKeyboardEvent(NativeWebKeyboardEvent(upEvent));
    return true;
}

#if ENABLE(TOUCH_EVENTS)
void EwkView::feedTouchEvents(Ewk_Touch_Event_Type type)
{
    Ewk_View_Smart_Data* sd = smartData();

    unsigned length = evas_touch_point_list_count(sd->base.evas);
    if (!length)
        return;

    OwnArrayPtr<WKTypeRef> touchPoints = adoptArrayPtr(new WKTypeRef[length]);
    for (unsigned i = 0; i < length; ++i) {
        int x, y;
        evas_touch_point_list_nth_xy_get(sd->base.evas, i, &x, &y);
        IntPoint position(x, y);
        Evas_Touch_Point_State state = evas_touch_point_list_nth_state_get(sd->base.evas, i);
        int id = evas_touch_point_list_nth_id_get(sd->base.evas, i);
        touchPoints[i] = WKTouchPointCreate(id, toAPI(IntPoint(position)), toAPI(transformToScreen().mapPoint(position)), toWKTouchPointState(state), WKSizeMake(0, 0), 0, 1);
    }
    WKRetainPtr<WKArrayRef> wkTouchPoints(AdoptWK, WKArrayCreateAdoptingValues(touchPoints.get(), length));

    WKViewSendTouchEvent(wkView(), adoptWK(WKTouchEventCreate(static_cast<WKEventType>(type), wkTouchPoints.get(), toWKEventModifiers(evas_key_modifier_get(sd->base.evas)), ecore_time_get())).get());
}

void EwkView::handleTouchDown(void* /* data */, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_START);
}

void EwkView::handleTouchUp(void* /* data */, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_END);
}

void EwkView::handleTouchMove(void* /* data */, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */)
{
    toEwkView(ewkView)->feedTouchEvents(EWK_TOUCH_MOVE);
}
#endif

void EwkView::handleFaviconChanged(const char* pageURL, void* eventInfo)
{
    EwkView* view = static_cast<EwkView*>(eventInfo);

    if (!view->url() || strcasecmp(view->url(), pageURL))
        return;

    view->smartCallback<FaviconChanged>().call();
}

PassRefPtr<cairo_surface_t> EwkView::takeSnapshot()
{
    // Suspend all animations before taking the snapshot.
    WKViewSuspendActiveDOMObjectsAndAnimations(wkView());

    // Wait for the pending repaint events to be processed.
    while (m_displayTimer.isActive())
        ecore_main_loop_iterate();

    Ewk_View_Smart_Data* sd = smartData();
#if USE(ACCELERATED_COMPOSITING)
    if (m_isAccelerated) {
        RefPtr<cairo_surface_t> snapshot = getImageSurfaceFromFrameBuffer(0, 0, sd->view.w, sd->view.h);
        // Resume all animations.
        WKViewResumeActiveDOMObjectsAndAnimations(wkView());

        return snapshot.release();
    }
#endif
    RefPtr<cairo_surface_t> snapshot = createSurfaceForImage(sd->image);
    // Resume all animations.
    WKViewResumeActiveDOMObjectsAndAnimations(wkView());

    return snapshot.release();
}

void EwkView::didFindZoomableArea(const WKPoint& point, const WKRect& area)
{
    notImplemented();
}

Evas_Smart_Class EwkView::parentSmartClass = EVAS_SMART_CLASS_INIT_NULL;

// Free Ewk View functions.

EwkView* toEwkView(const Evas_Object* evasObject)
{
    ASSERT(evasObject);
    ASSERT(isEwkViewEvasObject(evasObject));

    return toEwkView(static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject)));
}

bool isEwkViewEvasObject(const Evas_Object* evasObject)
{
    ASSERT(evasObject);

    const char* evasObjectType = evas_object_type_get(evasObject);
    const Evas_Smart* evasSmart = evas_object_smart_smart_get(evasObject);
    if (!evasSmart) {
        EINA_LOG_CRIT("%p (%s) is not a smart object!", evasObject, evasObjectType ? evasObjectType : "(null)");
        return false;
    }

    const Evas_Smart_Class* smartClass = evas_smart_class_get(evasSmart);
    if (!smartClass) {
        EINA_LOG_CRIT("%p (%s) is not a smart class object!", evasObject, evasObjectType ? evasObjectType : "(null)");
        return false;
    }

    if (smartClass->data != smartClassName) {
        EINA_LOG_CRIT("%p (%s) is not of an ewk_view (need %p, got %p)!", evasObject, evasObjectType ? evasObjectType : "(null)",
            smartClassName, smartClass->data);
        return false;
    }

    return true;
}
