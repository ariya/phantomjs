/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PluginView_h
#define PluginView_h

#include "FrameLoadRequest.h"
#include "IntRect.h"
#include "MediaCanStartListener.h"
#include "PluginViewBase.h"
#include "ResourceRequest.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

#if ENABLE(NETSCAPE_PLUGIN_API)
#include "PluginStream.h"
#include "npruntime_internal.h"
#endif

#if OS(WINDOWS) && (PLATFORM(GTK) || PLATFORM(QT))
typedef struct HWND__* HWND;
typedef HWND PlatformPluginWidget;
#else
typedef PlatformWidget PlatformPluginWidget;
#endif
#if PLATFORM(QT)
#if USE(TEXTURE_MAPPER)
#include "TextureMapperPlatformLayer.h"
#endif

#include <QImage>
QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE
#endif
#if PLATFORM(QT) && USE(ACCELERATED_COMPOSITING) && ENABLE(NETSCAPE_PLUGIN_API) && defined(XP_UNIX)
#ifndef WTF_USE_ACCELERATED_COMPOSITING_PLUGIN_LAYER
#define WTF_USE_ACCELERATED_COMPOSITING_PLUGIN_LAYER 1
#endif
#endif
#if PLATFORM(GTK)
typedef struct _GtkSocket GtkSocket;
#endif

namespace JSC {
    namespace Bindings {
        class Instance;
    }
}

namespace WebCore {
    class Frame;
    class Image;
    class HTMLPlugInElement;
    class KeyboardEvent;
    class MouseEvent;
    class KURL;
#if OS(WINDOWS) && ENABLE(NETSCAPE_PLUGIN_API)
    class PluginMessageThrottlerWin;
#endif
    class PluginPackage;
    class PluginRequest;
    class PluginStream;
    class ResourceError;
    class ResourceResponse;
    class WheelEvent;

    enum PluginStatus {
        PluginStatusCanNotFindPlugin,
        PluginStatusCanNotLoadPlugin,
        PluginStatusLoadedSuccessfully
    };

    class PluginRequest {
        WTF_MAKE_NONCOPYABLE(PluginRequest); WTF_MAKE_FAST_ALLOCATED;
    public:
        PluginRequest(const FrameLoadRequest& frameLoadRequest, bool sendNotification, void* notifyData, bool shouldAllowPopups)
            : m_frameLoadRequest(frameLoadRequest)
            , m_notifyData(notifyData)
            , m_sendNotification(sendNotification)
            , m_shouldAllowPopups(shouldAllowPopups) { }
    public:
        const FrameLoadRequest& frameLoadRequest() const { return m_frameLoadRequest; }
        void* notifyData() const { return m_notifyData; }
        bool sendNotification() const { return m_sendNotification; }
        bool shouldAllowPopups() const { return m_shouldAllowPopups; }
    private:
        FrameLoadRequest m_frameLoadRequest;
        void* m_notifyData;
        bool m_sendNotification;
        bool m_shouldAllowPopups;
    };

    class PluginManualLoader {
    public:
        virtual ~PluginManualLoader() {}
        virtual void didReceiveResponse(const ResourceResponse&) = 0;
        virtual void didReceiveData(const char*, int) = 0;
        virtual void didFinishLoading() = 0;
        virtual void didFail(const ResourceError&) = 0;
    };

    class PluginView : public PluginViewBase
#if ENABLE(NETSCAPE_PLUGIN_API)
                     , private PluginStreamClient
#endif
                     , public PluginManualLoader
                     , private MediaCanStartListener {
    public:
        static PassRefPtr<PluginView> create(Frame* parentFrame, const IntSize&, HTMLPlugInElement*, const KURL&, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually);
        virtual ~PluginView();

        PluginPackage* plugin() const { return m_plugin.get(); }
#if ENABLE(NETSCAPE_PLUGIN_API)
        NPP instance() const { return m_instance; }
#endif

        void setNPWindowRect(const IntRect&);
        static PluginView* currentPluginView();

#if ENABLE(NETSCAPE_PLUGIN_API)
        NPObject* npObject();
#endif
        PassRefPtr<JSC::Bindings::Instance> bindingInstance();

        PluginStatus status() const { return m_status; }

#if ENABLE(NETSCAPE_PLUGIN_API)
        // NPN functions
        NPError getURLNotify(const char* url, const char* target, void* notifyData);
        NPError getURL(const char* url, const char* target);
        NPError postURLNotify(const char* url, const char* target, uint32_t len, const char* but, NPBool file, void* notifyData);
        NPError postURL(const char* url, const char* target, uint32_t len, const char* but, NPBool file);
        NPError newStream(NPMIMEType type, const char* target, NPStream** stream);
        int32_t write(NPStream* stream, int32_t len, void* buffer);
        NPError destroyStream(NPStream* stream, NPReason reason);
#endif
        const char* userAgent();
#if ENABLE(NETSCAPE_PLUGIN_API)
        static const char* userAgentStatic();
#endif
        void status(const char* message);
        
#if ENABLE(NETSCAPE_PLUGIN_API)
        NPError getValue(NPNVariable variable, void* value);
        static NPError getValueStatic(NPNVariable variable, void* value);
        NPError setValue(NPPVariable variable, void* value);
        NPError getValueForURL(NPNURLVariable variable, const char* url, char** value, uint32_t* len);
        NPError setValueForURL(NPNURLVariable variable, const char* url, const char* value, uint32_t len);
        NPError getAuthenticationInfo(const char* protocol, const char* host, int32_t port, const char* scheme, const char* realm, char** username, uint32_t* ulen, char** password, uint32_t* plen);
        void invalidateRect(NPRect*);
        void invalidateRegion(NPRegion);
#endif
        void forceRedraw();
        void pushPopupsEnabledState(bool state);
        void popPopupsEnabledState();

        virtual void invalidateRect(const IntRect&);

        bool arePopupsAllowed() const;

        void setJavaScriptPaused(bool);

        void privateBrowsingStateChanged(bool);

        void disconnectStream(PluginStream*);
        void streamDidFinishLoading(PluginStream* stream) { disconnectStream(stream); }

        // Widget functions
        virtual void setFrameRect(const IntRect&);
        virtual void frameRectsChanged();
        virtual void setFocus(bool);
        virtual void show();
        virtual void hide();
        virtual void paint(GraphicsContext*, const IntRect&);
        virtual void clipRectChanged() OVERRIDE;

        // This method is used by plugins on all platforms to obtain a clip rect that includes clips set by WebCore,
        // e.g., in overflow:auto sections.  The clip rects coordinates are in the containing window's coordinate space.
        // This clip includes any clips that the widget itself sets up for its children.
        IntRect windowClipRect() const;

        virtual void handleEvent(Event*);
        virtual void setParent(ScrollView*);
        virtual void setParentVisible(bool);

        virtual bool isPluginView() const OVERRIDE { return true; }

        Frame* parentFrame() const { return m_parentFrame.get(); }

        void focusPluginElement();

        const String& pluginsPage() const { return m_pluginsPage; }
        const String& mimeType() const { return m_mimeType; }
        const KURL& url() const { return m_url; }

#if defined(XP_MACOSX) && ENABLE(NETSCAPE_PLUGIN_API)
        bool popUpContextMenu(NPMenu*);
#endif

#if OS(WINDOWS) && ENABLE(NETSCAPE_PLUGIN_API)
        static LRESULT CALLBACK PluginViewWndProc(HWND, UINT, WPARAM, LPARAM);
        LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
        WNDPROC pluginWndProc() const { return m_pluginWndProc; }
#endif

        // Used for manual loading
        void didReceiveResponse(const ResourceResponse&);
        void didReceiveData(const char*, int);
        void didFinishLoading();
        void didFail(const ResourceError&);

        static bool isCallingPlugin();

        bool start();

#if ENABLE(NETSCAPE_PLUGIN_API)
        static void keepAlive(NPP);
#endif
        void keepAlive();

#if USE(ACCELERATED_COMPOSITING)
#if USE(ACCELERATED_COMPOSITING_PLUGIN_LAYER)
        virtual PlatformLayer* platformLayer() const;
        bool shouldUseAcceleratedCompositing() const;
#else
        virtual PlatformLayer* platformLayer() const { return 0; }
#endif
#endif

#if PLATFORM(QT) && ENABLE(NETSCAPE_PLUGIN_API) && defined(XP_UNIX)
        // PluginViewQt (X11) needs a few workarounds when running under DRT
        static void setIsRunningUnderDRT(bool flag) { s_isRunningUnderDRT = flag; }
#endif

    private:
        PluginView(Frame* parentFrame, const IntSize&, PluginPackage*, HTMLPlugInElement*, const KURL&, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually);

        void setParameters(const Vector<String>& paramNames, const Vector<String>& paramValues);
        bool startOrAddToUnstartedList();
        void init();
        bool platformStart();
        void stop();
        void platformDestroy();
        static void setCurrentPluginView(PluginView*);
#if ENABLE(NETSCAPE_PLUGIN_API)
        NPError load(const FrameLoadRequest&, bool sendNotification, void* notifyData);
        NPError handlePost(const char* url, const char* target, uint32_t len, const char* buf, bool file, void* notifyData, bool sendNotification, bool allowHeaders);
        NPError handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf);
#endif
        static void freeStringArray(char** stringArray, int length);
        void setCallingPlugin(bool) const;

        void invalidateWindowlessPluginRect(const IntRect&);

        virtual void mediaCanStart();

#if OS(WINDOWS) && ENABLE(NETSCAPE_PLUGIN_API)
        void paintWindowedPluginIntoContext(GraphicsContext*, const IntRect&);
        static HDC WINAPI hookedBeginPaint(HWND, PAINTSTRUCT*);
        static BOOL WINAPI hookedEndPaint(HWND, const PAINTSTRUCT*);
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
        static bool platformGetValueStatic(NPNVariable variable, void* value, NPError* result);
        bool platformGetValue(NPNVariable variable, void* value, NPError* result);
#endif

        RefPtr<Frame> m_parentFrame;
        RefPtr<PluginPackage> m_plugin;
        HTMLPlugInElement* m_element;
        bool m_isStarted;
        KURL m_url;
        PluginStatus m_status;
        Vector<IntRect> m_invalidRects;

        void performRequest(PluginRequest*);
        void scheduleRequest(PassOwnPtr<PluginRequest>);
        void requestTimerFired(Timer<PluginView>*);
        void invalidateTimerFired(Timer<PluginView>*);
        Timer<PluginView> m_requestTimer;
        Timer<PluginView> m_invalidateTimer;

        void popPopupsStateTimerFired(Timer<PluginView>*);
        Timer<PluginView> m_popPopupsStateTimer;

        void lifeSupportTimerFired(Timer<PluginView>*);
        Timer<PluginView> m_lifeSupportTimer;

#if ENABLE(NETSCAPE_PLUGIN_API)
        bool dispatchNPEvent(NPEvent&);
#endif
#if defined(XP_MACOSX) && ENABLE(NETSCAPE_PLUGIN_API)
        int16_t dispatchNPCocoaEvent(NPCocoaEvent&);
        bool m_updatedCocoaTextInputRequested;
        bool m_keyDownSent;
        uint16_t m_disregardKeyUpCounter;
#endif

#if defined(XP_MACOSX)
        void handleWheelEvent(WheelEvent*);
#endif
        void updatePluginWidget();
        void paintMissingPluginIcon(GraphicsContext*, const IntRect&);

        void handleKeyboardEvent(KeyboardEvent*);
        void handleMouseEvent(MouseEvent*);
#if defined(XP_UNIX) && ENABLE(NETSCAPE_PLUGIN_API)
        void handleFocusInEvent();
        void handleFocusOutEvent();
#endif

#if OS(WINDOWS)
        void paintIntoTransformedContext(HDC);
        PassRefPtr<Image> snapshot();
#endif

        int m_mode;
        int m_paramCount;
        char** m_paramNames;
        char** m_paramValues;
        String m_pluginsPage;

        String m_mimeType;
        WTF::CString m_userAgent;

#if ENABLE(NETSCAPE_PLUGIN_API)
        NPP m_instance;
        NPP_t m_instanceStruct;
        NPWindow m_npWindow;
#endif

        Vector<bool, 4> m_popupStateStack;

        HashSet<RefPtr<PluginStream> > m_streams;
        Vector<OwnPtr<PluginRequest> > m_requests;

        bool m_isWindowed;
        bool m_isTransparent;
        bool m_haveInitialized;
        bool m_isWaitingToStart;

#if defined(XP_UNIX)
        bool m_needsXEmbed;
#endif

#if OS(WINDOWS) && ENABLE(NETSCAPE_PLUGIN_API)
        OwnPtr<PluginMessageThrottlerWin> m_messageThrottler;
        WNDPROC m_pluginWndProc;
        unsigned m_lastMessage;
        bool m_isCallingPluginWndProc;
        HDC m_wmPrintHDC;
        bool m_haveUpdatedPluginWidget;
#endif

#if ((PLATFORM(GTK) || PLATFORM(QT)) && OS(WINDOWS)) || PLATFORM(EFL)
        // On Mac OSX and Qt/Windows the plugin does not have its own native widget,
        // but is using the containing window as its reference for positioning/painting.
        PlatformPluginWidget m_window;
public:
        PlatformPluginWidget platformPluginWidget() const { return m_window; }
        void setPlatformPluginWidget(PlatformPluginWidget widget) { m_window = widget; }
#else
public:
        void setPlatformPluginWidget(PlatformPluginWidget widget) { setPlatformWidget(widget); }
        PlatformPluginWidget platformPluginWidget() const { return platformWidget(); }
#endif

private:

#if defined(XP_UNIX) || PLATFORM(GTK)
        void setNPWindowIfNeeded();
#elif defined(XP_MACOSX)
        NP_CGContext m_npCgContext;
        CGContextRef m_contextRef;

        void setNPWindowIfNeeded();
#endif

#if defined(XP_UNIX) && ENABLE(NETSCAPE_PLUGIN_API)
        bool m_hasPendingGeometryChange;
        Pixmap m_drawable;
        Visual* m_visual;
        Colormap m_colormap;
        Display* m_pluginDisplay;

        void initXEvent(XEvent* event);
#endif

#if PLATFORM(QT)
#if defined(XP_UNIX) && ENABLE(NETSCAPE_PLUGIN_API)
        static bool s_isRunningUnderDRT;
        static void setXKeyEventSpecificFields(XEvent*, KeyboardEvent*);
        void paintUsingXPixmap(QPainter* painter, const QRect &exposedRect);
        QWebPageClient* platformPageClient() const;
#endif
#if USE(ACCELERATED_COMPOSITING_PLUGIN_LAYER)
        OwnPtr<PlatformLayer> m_platformLayer;
        friend class PluginGraphicsLayerQt;
#endif // USE(ACCELERATED_COMPOSITING_PLUGIN_LAYER)
#endif // PLATFORM(QT)

#if PLATFORM(GTK)
        static gboolean plugRemovedCallback(GtkSocket*, PluginView*);
        static void plugAddedCallback(GtkSocket*, PluginView*);
        void updateWidgetAllocationAndClip();
        bool m_plugAdded;
        IntRect m_delayedAllocation;
#endif

        IntRect m_clipRect; // The clip rect to apply to a windowed plug-in
        IntRect m_windowRect; // Our window rect.

        bool m_loadManually;
        RefPtr<PluginStream> m_manualStream;

        bool m_isJavaScriptPaused;

        bool m_haveCalledSetWindow;

        static PluginView* s_currentPluginView;
    };

inline PluginView* toPluginView(Widget* widget)
{
    ASSERT(!widget || widget->isPluginView());
    return static_cast<PluginView*>(widget);
}

inline const PluginView* toPluginView(const Widget* widget)
{
    ASSERT(!widget || widget->isPluginView());
    return static_cast<const PluginView*>(widget);
}

// This will catch anyone doing an unnecessary cast.
void toPluginView(const PluginView*);

} // namespace WebCore

#endif
