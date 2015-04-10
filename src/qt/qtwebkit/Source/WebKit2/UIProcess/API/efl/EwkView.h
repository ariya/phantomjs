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

#ifndef EwkView_h
#define EwkView_h

#include "EvasGLContext.h"
#include "EvasGLSurface.h"
#include "EwkViewCallbacks.h"
#include "ImmutableDictionary.h"
#include "RefPtrEfl.h"
#include "WKEinaSharedString.h"
#include "WKRetainPtr.h"
#include "WebViewEfl.h"
#include "ewk_url_request_private.h"
#include <Evas.h>
#include <WebCore/FloatPoint.h>
#include <WebCore/IntRect.h>
#include <WebCore/RefPtrCairo.h>
#include <WebCore/TextDirection.h>
#include <WebCore/Timer.h>
#include <WebKit2/WKBase.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TOUCH_EVENTS)
#include "ewk_touch.h"
#endif

#include "WebContext.h"
#include "WebPageGroup.h"
#include "WebPreferences.h"

typedef struct _cairo_surface cairo_surface_t;

namespace WebKit {
class ContextMenuClientEfl;
class FindClientEfl;
class FormClientEfl;
class InputMethodContextEfl;
class PageLoadClientEfl;
class PagePolicyClientEfl;
class PageUIClientEfl;
class ViewClientEfl;
#if USE(ACCELERATED_COMPOSITING)
class PageViewportController;
class PageViewportControllerClientEfl;
#endif
class WebPageGroup;
class WebPageProxy;

#if ENABLE(VIBRATION)
class VibrationClientEfl;
#endif
}

namespace WebCore {
class AffineTransform;
class Color;
class CoordinatedGraphicsScene;
class Cursor;
class IntSize;
class TransformationMatrix;
}

class EwkContext;
class EwkBackForwardList;
class EwkColorPicker;
class EwkContextMenu;
class EwkPageGroup;
class EwkPopupMenu;
class EwkSettings;
class EwkWindowFeatures;

#if USE(ACCELERATED_COMPOSITING)
typedef struct _Evas_GL_Context Evas_GL_Context;
typedef struct _Evas_GL_Surface Evas_GL_Surface;
#endif

typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
typedef struct Ewk_View_Smart_Class Ewk_View_Smart_Class;

class EwkView {
public:
    static EwkView* create(WKViewRef, Evas* canvas, Evas_Smart* smart = 0);

    static bool initSmartClassInterface(Ewk_View_Smart_Class&);

    static Evas_Object* toEvasObject(WKPageRef);

    Evas_Object* evasObject() { return m_evasObject; }

    WKViewRef wkView() const { return m_webView.get(); }
    WKPageRef wkPage() const;

    WebKit::WebPageProxy* page() { return webView()->page(); }
    EwkContext* ewkContext() { return m_context.get(); }
    EwkPageGroup* ewkPageGroup() { return m_pageGroup.get(); }
    EwkSettings* settings() { return m_settings.get(); }
    EwkBackForwardList* backForwardList() { return m_backForwardList.get(); }
    EwkWindowFeatures* windowFeatures();

#if USE(ACCELERATED_COMPOSITING)
    WebKit::PageViewportController* pageViewportController() { return m_pageViewportController.get(); }
#endif

    void setDeviceScaleFactor(float scale);
    float deviceScaleFactor() const;

    WebCore::AffineTransform transformToScreen() const;

    const char* url() const { return m_url; }
    Evas_Object* createFavicon() const;
    const char* title() const;
    WebKit::InputMethodContextEfl* inputMethodContext();

    const char* themePath() const;
    void setThemePath(const char* theme);
    const char* customTextEncodingName() const { return m_customEncoding; }
    void setCustomTextEncodingName(const char* customEncoding);
    const char* userAgent() const { return m_userAgent; }
    void setUserAgent(const char* userAgent);

    bool mouseEventsEnabled() const { return m_mouseEventsEnabled; }
    void setMouseEventsEnabled(bool enabled);
#if ENABLE(TOUCH_EVENTS)
    void feedTouchEvent(Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers);
    bool touchEventsEnabled() const { return m_touchEventsEnabled; }
    void setTouchEventsEnabled(bool enabled);
    void doneWithTouchEvent(WKTouchEventRef, bool);
#endif

    void setCursor(const WebCore::Cursor& cursor);

    void scheduleUpdateDisplay();

#if ENABLE(FULLSCREEN_API)
    void enterFullScreen();
    void exitFullScreen();
#endif

    WKRect windowGeometry() const;
    void setWindowGeometry(const WKRect&);
#if USE(ACCELERATED_COMPOSITING)
    bool createGLSurface();
    void setNeedsSurfaceResize() { m_pendingSurfaceResize = true; }
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    void requestColorPicker(WKColorPickerResultListenerRef listener, const WebCore::Color&);
    void dismissColorPicker();
#endif

    WKPageRef createNewPage(PassRefPtr<EwkUrlRequest>, WKDictionaryRef windowFeatures);
    void close();

    void requestPopupMenu(WKPopupMenuListenerRef, const WKRect&, WKPopupItemTextDirection, double pageScaleFactor, WKArrayRef items, int32_t selectedIndex);
    void closePopupMenu();

    void customContextMenuItemSelected(WKContextMenuItemRef contextMenuItem);
    void showContextMenu(WKPoint position, WKArrayRef items);
    void hideContextMenu();

    void updateTextInputState();

    void requestJSAlertPopup(const WKEinaSharedString& message);
    bool requestJSConfirmPopup(const WKEinaSharedString& message);
    WKEinaSharedString requestJSPromptPopup(const WKEinaSharedString& message, const WKEinaSharedString& defaultValue);

    template<EwkViewCallbacks::CallbackType callbackType>
    EwkViewCallbacks::CallBack<callbackType> smartCallback() const
    {
        return EwkViewCallbacks::CallBack<callbackType>(m_evasObject);
    }

    unsigned long long informDatabaseQuotaReached(const String& databaseName, const String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage);

    // FIXME: Remove when possible.
    WebKit::WebView* webView();

    // FIXME: needs refactoring (split callback invoke)
    void informURLChange();

    PassRefPtr<cairo_surface_t> takeSnapshot();

    void didFindZoomableArea(const WKPoint&, const WKRect&);

private:
    EwkView(WKViewRef, Evas_Object*);
    ~EwkView();

    void setDeviceSize(const WebCore::IntSize&);
    Ewk_View_Smart_Data* smartData() const;

    WebCore::IntSize size() const;
    WebCore::IntSize deviceSize() const;

    void displayTimerFired(WebCore::Timer<EwkView>*);

    // Evas_Smart_Class callback interface:
    static void handleEvasObjectAdd(Evas_Object*);
    static void handleEvasObjectDelete(Evas_Object*);
    static void handleEvasObjectMove(Evas_Object*, Evas_Coord x, Evas_Coord y);
    static void handleEvasObjectResize(Evas_Object*, Evas_Coord width, Evas_Coord height);
    static void handleEvasObjectShow(Evas_Object*);
    static void handleEvasObjectHide(Evas_Object*);
    static void handleEvasObjectColorSet(Evas_Object*, int red, int green, int blue, int alpha);
    static void handleEvasObjectCalculate(Evas_Object*);

    // Ewk_View_Smart_Class callback interface:
    static Eina_Bool handleEwkViewFocusIn(Ewk_View_Smart_Data* smartData);
    static Eina_Bool handleEwkViewFocusOut(Ewk_View_Smart_Data* smartData);
    static Eina_Bool handleEwkViewMouseWheel(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Wheel* wheelEvent);
    static Eina_Bool handleEwkViewMouseDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Down* downEvent);
    static Eina_Bool handleEwkViewMouseUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Up* upEvent);
    static Eina_Bool handleEwkViewMouseMove(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Move* moveEvent);
    static Eina_Bool handleEwkViewKeyDown(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Down* downEvent);
    static Eina_Bool handleEwkViewKeyUp(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Up* upEvent);

#if ENABLE(TOUCH_EVENTS)
    void feedTouchEvents(Ewk_Touch_Event_Type type);
    static void handleTouchDown(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleTouchUp(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void handleTouchMove(void* data, Evas*, Evas_Object*, void* eventInfo);
#endif
    static void handleFaviconChanged(const char* pageURL, void* eventInfo);

private:
    // Note, initialization order matters.
    WKRetainPtr<WKViewRef> m_webView;
    Evas_Object* m_evasObject;
    RefPtr<EwkContext> m_context;
    RefPtr<EwkPageGroup> m_pageGroup;
#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<Evas_GL> m_evasGL;
    OwnPtr<WebKit::EvasGLContext> m_evasGLContext;
    OwnPtr<WebKit::EvasGLSurface> m_evasGLSurface;
    bool m_pendingSurfaceResize;
#endif
    WebCore::TransformationMatrix m_userViewportTransform;
    OwnPtr<WebKit::PageLoadClientEfl> m_pageLoadClient;
    OwnPtr<WebKit::PagePolicyClientEfl> m_pagePolicyClient;
    OwnPtr<WebKit::PageUIClientEfl> m_pageUIClient;
    OwnPtr<WebKit::ContextMenuClientEfl> m_contextMenuClient;
    OwnPtr<WebKit::FindClientEfl> m_findClient;
    OwnPtr<WebKit::FormClientEfl> m_formClient;
    OwnPtr<WebKit::ViewClientEfl> m_viewClient;
#if ENABLE(VIBRATION)
    OwnPtr<WebKit::VibrationClientEfl> m_vibrationClient;
#endif
    OwnPtr<EwkBackForwardList> m_backForwardList;
    OwnPtr<EwkSettings> m_settings;
    RefPtr<EwkWindowFeatures> m_windowFeatures;
    const void* m_cursorIdentifier; // This is an address, do not free it.
    WKEinaSharedString m_url;
    mutable WKEinaSharedString m_title;
    WKEinaSharedString m_theme;
    WKEinaSharedString m_customEncoding;
    WKEinaSharedString m_userAgent;
    bool m_mouseEventsEnabled;
#if ENABLE(TOUCH_EVENTS)
    bool m_touchEventsEnabled;
#endif
    WebCore::Timer<EwkView> m_displayTimer;
    RefPtr<EwkContextMenu> m_contextMenu;
    OwnPtr<EwkPopupMenu> m_popupMenu;
    OwnPtr<WebKit::InputMethodContextEfl> m_inputMethodContext;
#if ENABLE(INPUT_TYPE_COLOR)
    OwnPtr<EwkColorPicker> m_colorPicker;
#endif
#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<WebKit::PageViewportControllerClientEfl> m_pageViewportControllerClient;
    OwnPtr<WebKit::PageViewportController> m_pageViewportController;
#endif
    bool m_isAccelerated;

    static Evas_Smart_Class parentSmartClass;
};

EwkView* toEwkView(const Evas_Object*);

bool isEwkViewEvasObject(const Evas_Object*);

#endif // EwkView_h
