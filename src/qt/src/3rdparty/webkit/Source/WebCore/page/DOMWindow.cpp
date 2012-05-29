/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "DOMWindow.h"

#include "AbstractDatabase.h"
#include "BackForwardController.h"
#include "BarInfo.h"
#include "Base64.h"
#include "BeforeUnloadEvent.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSRuleList.h"
#include "CSSStyleSelector.h"
#include "Chrome.h"
#include "Console.h"
#include "Crypto.h"
#include "DOMApplicationCache.h"
#include "DOMSelection.h"
#include "DOMSettableTokenList.h"
#include "DOMStringList.h"
#include "DOMTimer.h"
#include "DOMTokenList.h"
#include "DOMURL.h"
#include "Database.h"
#include "DatabaseCallback.h"
#include "DeviceMotionController.h"
#include "DeviceOrientationController.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Element.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "History.h"
#include "IDBFactory.h"
#include "IDBFactoryBackendInterface.h"
#include "InspectorInstrumentation.h"
#include "KURL.h"
#include "Location.h"
#include "MediaQueryList.h"
#include "MediaQueryMatcher.h"
#include "MessageEvent.h"
#include "Navigator.h"
#include "NotificationCenter.h"
#include "Page.h"
#include "PageGroup.h"
#include "PageTransitionEvent.h"
#include "Performance.h"
#include "PlatformScreen.h"
#include "PlatformString.h"
#include "Screen.h"
#include "SecurityOrigin.h"
#include "SerializedScriptValue.h"
#include "Settings.h"
#include "Storage.h"
#include "StorageArea.h"
#include "StorageInfo.h"
#include "StorageNamespace.h"
#include "StyleMedia.h"
#include "SuddenTermination.h"
#include "WebKitPoint.h"
#include "WindowFeatures.h"
#include <algorithm>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/text/StringConcatenate.h>

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystem.h"
#include "DOMFileSystem.h"
#include "DOMFileSystemBase.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileError.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "LocalFileSystem.h"
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
#include "RequestAnimationFrameCallback.h"
#endif

using std::min;
using std::max;

namespace WebCore {

class PostMessageTimer : public TimerBase {
public:
    PostMessageTimer(DOMWindow* window, PassRefPtr<SerializedScriptValue> message, const String& sourceOrigin, PassRefPtr<DOMWindow> source, PassOwnPtr<MessagePortChannelArray> channels, SecurityOrigin* targetOrigin)
        : m_window(window)
        , m_message(message)
        , m_origin(sourceOrigin)
        , m_source(source)
        , m_channels(channels)
        , m_targetOrigin(targetOrigin)
    {
    }

    PassRefPtr<MessageEvent> event(ScriptExecutionContext* context)
    {
        OwnPtr<MessagePortArray> messagePorts = MessagePort::entanglePorts(*context, m_channels.release());
        return MessageEvent::create(messagePorts.release(), m_message, m_origin, "", m_source);
    }
    SecurityOrigin* targetOrigin() const { return m_targetOrigin.get(); }

private:
    virtual void fired()
    {
        m_window->postMessageTimerFired(adoptPtr(this));
        // This object is deleted now.
    }

    RefPtr<DOMWindow> m_window;
    RefPtr<SerializedScriptValue> m_message;
    String m_origin;
    RefPtr<DOMWindow> m_source;
    OwnPtr<MessagePortChannelArray> m_channels;
    RefPtr<SecurityOrigin> m_targetOrigin;
};

typedef HashCountedSet<DOMWindow*> DOMWindowSet;

static DOMWindowSet& windowsWithUnloadEventListeners()
{
    DEFINE_STATIC_LOCAL(DOMWindowSet, windowsWithUnloadEventListeners, ());
    return windowsWithUnloadEventListeners;
}

static DOMWindowSet& windowsWithBeforeUnloadEventListeners()
{
    DEFINE_STATIC_LOCAL(DOMWindowSet, windowsWithBeforeUnloadEventListeners, ());
    return windowsWithBeforeUnloadEventListeners;
}

static void addUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    if (set.isEmpty())
        disableSuddenTermination();
    set.add(domWindow);
}

static void removeUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.remove(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static void removeAllUnloadEventListeners(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.removeAll(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static void addBeforeUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    if (set.isEmpty())
        disableSuddenTermination();
    set.add(domWindow);
}

static void removeBeforeUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.remove(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static void removeAllBeforeUnloadEventListeners(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.removeAll(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static bool allowsBeforeUnloadListeners(DOMWindow* window)
{
    ASSERT_ARG(window, window);
    Frame* frame = window->frame();
    if (!frame)
        return false;
    Page* page = frame->page();
    if (!page)
        return false;
    return frame == page->mainFrame();
}

bool DOMWindow::dispatchAllPendingBeforeUnloadEvents()
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    if (set.isEmpty())
        return true;

    static bool alreadyDispatched = false;
    ASSERT(!alreadyDispatched);
    if (alreadyDispatched)
        return true;

    Vector<RefPtr<DOMWindow> > windows;
    DOMWindowSet::iterator end = set.end();
    for (DOMWindowSet::iterator it = set.begin(); it != end; ++it)
        windows.append(it->first);

    size_t size = windows.size();
    for (size_t i = 0; i < size; ++i) {
        DOMWindow* window = windows[i].get();
        if (!set.contains(window))
            continue;

        Frame* frame = window->frame();
        if (!frame)
            continue;

        if (!frame->loader()->shouldClose())
            return false;
    }

    enableSuddenTermination();

    alreadyDispatched = true;

    return true;
}

unsigned DOMWindow::pendingUnloadEventListeners() const
{
    return windowsWithUnloadEventListeners().count(const_cast<DOMWindow*>(this));
}

void DOMWindow::dispatchAllPendingUnloadEvents()
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    if (set.isEmpty())
        return;

    static bool alreadyDispatched = false;
    ASSERT(!alreadyDispatched);
    if (alreadyDispatched)
        return;

    Vector<RefPtr<DOMWindow> > windows;
    DOMWindowSet::iterator end = set.end();
    for (DOMWindowSet::iterator it = set.begin(); it != end; ++it)
        windows.append(it->first);

    size_t size = windows.size();
    for (size_t i = 0; i < size; ++i) {
        DOMWindow* window = windows[i].get();
        if (!set.contains(window))
            continue;

        window->dispatchEvent(PageTransitionEvent::create(eventNames().pagehideEvent, false), window->document());
        window->dispatchEvent(Event::create(eventNames().unloadEvent, false, false), window->document());
    }

    enableSuddenTermination();

    alreadyDispatched = true;
}

// This function:
// 1) Validates the pending changes are not changing to NaN
// 2) Constrains the window rect to no smaller than 100 in each dimension and no
//    bigger than the the float rect's dimensions.
// 3) Constrain window rect to within the top and left boundaries of the screen rect
// 4) Constraint the window rect to within the bottom and right boundaries of the
//    screen rect.
// 5) Translate the window rect coordinates to be within the coordinate space of
//    the screen rect.
void DOMWindow::adjustWindowRect(const FloatRect& screen, FloatRect& window, const FloatRect& pendingChanges)
{
    // Make sure we're in a valid state before adjusting dimensions.
    ASSERT(isfinite(screen.x()));
    ASSERT(isfinite(screen.y()));
    ASSERT(isfinite(screen.width()));
    ASSERT(isfinite(screen.height()));
    ASSERT(isfinite(window.x()));
    ASSERT(isfinite(window.y()));
    ASSERT(isfinite(window.width()));
    ASSERT(isfinite(window.height()));
    
    // Update window values if new requested values are not NaN.
    if (!isnan(pendingChanges.x()))
        window.setX(pendingChanges.x());
    if (!isnan(pendingChanges.y()))
        window.setY(pendingChanges.y());
    if (!isnan(pendingChanges.width()))
        window.setWidth(pendingChanges.width());
    if (!isnan(pendingChanges.height()))
        window.setHeight(pendingChanges.height());
    
    // Resize the window to between 100 and the screen width and height.
    window.setWidth(min(max(100.0f, window.width()), screen.width()));
    window.setHeight(min(max(100.0f, window.height()), screen.height()));
    
    // Constrain the window position to the screen.
    window.setX(max(screen.x(), min(window.x(), screen.maxX() - window.width())));
    window.setY(max(screen.y(), min(window.y(), screen.maxY() - window.height())));
}

// FIXME: We can remove this function once V8 showModalDialog is changed to use DOMWindow.
void DOMWindow::parseModalDialogFeatures(const String& string, HashMap<String, String>& map)
{
    WindowFeatures::parseDialogFeatures(string, map);
}

bool DOMWindow::allowPopUp(Frame* firstFrame)
{
    ASSERT(firstFrame);

    if (ScriptController::processingUserGesture())
        return true;

    Settings* settings = firstFrame->settings();
    return settings && settings->javaScriptCanOpenWindowsAutomatically();
}

bool DOMWindow::allowPopUp()
{
    return m_frame && allowPopUp(m_frame);
}

bool DOMWindow::canShowModalDialog(const Frame* frame)
{
    if (!frame)
        return false;
    Page* page = frame->page();
    if (!page)
        return false;
    return page->chrome()->canRunModal();
}

bool DOMWindow::canShowModalDialogNow(const Frame* frame)
{
    if (!frame)
        return false;
    Page* page = frame->page();
    if (!page)
        return false;
    return page->chrome()->canRunModalNow();
}

DOMWindow::DOMWindow(Frame* frame)
    : m_shouldPrintWhenFinishedLoading(false)
    , m_frame(frame)
{
}

DOMWindow::~DOMWindow()
{
    if (m_frame)
        m_frame->clearFormerDOMWindow(this);

    removeAllUnloadEventListeners(this);
    removeAllBeforeUnloadEventListeners(this);
}

ScriptExecutionContext* DOMWindow::scriptExecutionContext() const
{
    return document();
}

PassRefPtr<MediaQueryList> DOMWindow::matchMedia(const String& media)
{
    return document() ? document()->mediaQueryMatcher()->matchMedia(media) : 0;
}

void DOMWindow::setSecurityOrigin(SecurityOrigin* securityOrigin)
{
    m_securityOrigin = securityOrigin;
}

void DOMWindow::disconnectFrame()
{
    m_frame = 0;
    clear();
}

void DOMWindow::clear()
{
    if (m_screen)
        m_screen->disconnectFrame();
    m_screen = 0;

    if (m_selection)
        m_selection->disconnectFrame();
    m_selection = 0;

    if (m_history)
        m_history->disconnectFrame();
    m_history = 0;

    m_crypto = 0;

    if (m_locationbar)
        m_locationbar->disconnectFrame();
    m_locationbar = 0;

    if (m_menubar)
        m_menubar->disconnectFrame();
    m_menubar = 0;

    if (m_personalbar)
        m_personalbar->disconnectFrame();
    m_personalbar = 0;

    if (m_scrollbars)
        m_scrollbars->disconnectFrame();
    m_scrollbars = 0;

    if (m_statusbar)
        m_statusbar->disconnectFrame();
    m_statusbar = 0;

    if (m_toolbar)
        m_toolbar->disconnectFrame();
    m_toolbar = 0;

    if (m_console)
        m_console->disconnectFrame();
    m_console = 0;

    if (m_navigator)
        m_navigator->disconnectFrame();
    m_navigator = 0;

#if ENABLE(WEB_TIMING)
    if (m_performance)
        m_performance->disconnectFrame();
    m_performance = 0;
#endif

    if (m_location)
        m_location->disconnectFrame();
    m_location = 0;

    if (m_media)
        m_media->disconnectFrame();
    m_media = 0;
    
#if ENABLE(DOM_STORAGE)
    if (m_sessionStorage)
        m_sessionStorage->disconnectFrame();
    m_sessionStorage = 0;

    if (m_localStorage)
        m_localStorage->disconnectFrame();
    m_localStorage = 0;
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    if (m_applicationCache)
        m_applicationCache->disconnectFrame();
    m_applicationCache = 0;
#endif

#if ENABLE(NOTIFICATIONS)
    if (m_notifications)
        m_notifications->disconnectFrame();
    m_notifications = 0;
#endif

#if ENABLE(INDEXED_DATABASE)
    m_idbFactory = 0;
#endif
}

#if ENABLE(ORIENTATION_EVENTS)
int DOMWindow::orientation() const
{
    if (!m_frame)
        return 0;
    
    return m_frame->orientation();
}
#endif

Screen* DOMWindow::screen() const
{
    if (!m_screen)
        m_screen = Screen::create(m_frame);
    return m_screen.get();
}

History* DOMWindow::history() const
{
    if (!m_history)
        m_history = History::create(m_frame);
    return m_history.get();
}

Crypto* DOMWindow::crypto() const
{
    if (!m_crypto)
        m_crypto = Crypto::create();
    return m_crypto.get();
}

BarInfo* DOMWindow::locationbar() const
{
    if (!m_locationbar)
        m_locationbar = BarInfo::create(m_frame, BarInfo::Locationbar);
    return m_locationbar.get();
}

BarInfo* DOMWindow::menubar() const
{
    if (!m_menubar)
        m_menubar = BarInfo::create(m_frame, BarInfo::Menubar);
    return m_menubar.get();
}

BarInfo* DOMWindow::personalbar() const
{
    if (!m_personalbar)
        m_personalbar = BarInfo::create(m_frame, BarInfo::Personalbar);
    return m_personalbar.get();
}

BarInfo* DOMWindow::scrollbars() const
{
    if (!m_scrollbars)
        m_scrollbars = BarInfo::create(m_frame, BarInfo::Scrollbars);
    return m_scrollbars.get();
}

BarInfo* DOMWindow::statusbar() const
{
    if (!m_statusbar)
        m_statusbar = BarInfo::create(m_frame, BarInfo::Statusbar);
    return m_statusbar.get();
}

BarInfo* DOMWindow::toolbar() const
{
    if (!m_toolbar)
        m_toolbar = BarInfo::create(m_frame, BarInfo::Toolbar);
    return m_toolbar.get();
}

Console* DOMWindow::console() const
{
    if (!m_console)
        m_console = Console::create(m_frame);
    return m_console.get();
}

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
DOMApplicationCache* DOMWindow::applicationCache() const
{
    if (!m_applicationCache)
        m_applicationCache = DOMApplicationCache::create(m_frame);
    return m_applicationCache.get();
}
#endif

Navigator* DOMWindow::navigator() const
{
    if (!m_navigator)
        m_navigator = Navigator::create(m_frame);
    return m_navigator.get();
}

#if ENABLE(WEB_TIMING)
Performance* DOMWindow::performance() const
{
    if (!m_performance)
        m_performance = Performance::create(m_frame);
    return m_performance.get();
}
#endif

Location* DOMWindow::location() const
{
    if (!m_location)
        m_location = Location::create(m_frame);
    return m_location.get();
}

#if ENABLE(DOM_STORAGE)
Storage* DOMWindow::sessionStorage(ExceptionCode& ec) const
{
    if (m_sessionStorage)
        return m_sessionStorage.get();

    Document* document = this->document();
    if (!document)
        return 0;

    if (!document->securityOrigin()->canAccessLocalStorage()) {
        ec = SECURITY_ERR;
        return 0;
    }

    Page* page = document->page();
    if (!page)
        return 0;

    RefPtr<StorageArea> storageArea = page->sessionStorage()->storageArea(document->securityOrigin());
    InspectorInstrumentation::didUseDOMStorage(page, storageArea.get(), false, m_frame);

    m_sessionStorage = Storage::create(m_frame, storageArea.release());
    return m_sessionStorage.get();
}

Storage* DOMWindow::localStorage(ExceptionCode& ec) const
{
    if (m_localStorage)
        return m_localStorage.get();

    Document* document = this->document();
    if (!document)
        return 0;

    if (!document->securityOrigin()->canAccessLocalStorage()) {
        ec = SECURITY_ERR;
        return 0;
    }

    Page* page = document->page();
    if (!page)
        return 0;

    if (!page->settings()->localStorageEnabled())
        return 0;

    RefPtr<StorageArea> storageArea = page->group().localStorage()->storageArea(document->securityOrigin());
    InspectorInstrumentation::didUseDOMStorage(page, storageArea.get(), true, m_frame);

    m_localStorage = Storage::create(m_frame, storageArea.release());
    return m_localStorage.get();
}
#endif

#if ENABLE(NOTIFICATIONS)
NotificationCenter* DOMWindow::webkitNotifications() const
{
    if (m_notifications)
        return m_notifications.get();

    Document* document = this->document();
    if (!document)
        return 0;
    
    Page* page = document->page();
    if (!page)
        return 0;

    NotificationPresenter* provider = page->chrome()->notificationPresenter();
    if (provider) 
        m_notifications = NotificationCenter::create(document, provider);    
      
    return m_notifications.get();
}
#endif

void DOMWindow::pageDestroyed()
{
#if ENABLE(NOTIFICATIONS)
    // Clearing Notifications requests involves accessing the client so it must be done
    // before the frame is detached.
    if (m_notifications)
        m_notifications->disconnectFrame();
    m_notifications = 0;
#endif
}

void DOMWindow::resetGeolocation()
{
    // Geolocation should cancel activities and permission requests when the page is detached.
    if (m_navigator)
        m_navigator->resetGeolocation();
}

#if ENABLE(INDEXED_DATABASE)
IDBFactory* DOMWindow::webkitIndexedDB() const
{
    if (m_idbFactory)
        return m_idbFactory.get();

    Document* document = this->document();
    if (!document)
        return 0;

    // FIXME: See if access is allowed.

    Page* page = document->page();
    if (!page)
        return 0;

    // FIXME: See if indexedDatabase access is allowed.

    m_idbFactory = IDBFactory::create(page->group().idbFactory());
    return m_idbFactory.get();
}
#endif

#if ENABLE(FILE_SYSTEM)
void DOMWindow::webkitRequestFileSystem(int type, long long size, PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    Document* document = this->document();
    if (!document)
        return;

    if (!AsyncFileSystem::isAvailable() || !document->securityOrigin()->canAccessFileSystem()) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    AsyncFileSystem::Type fileSystemType = static_cast<AsyncFileSystem::Type>(type);
    if (fileSystemType != AsyncFileSystem::Temporary && fileSystemType != AsyncFileSystem::Persistent && fileSystemType != AsyncFileSystem::External) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::INVALID_MODIFICATION_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().requestFileSystem(document, fileSystemType, size, FileSystemCallbacks::create(successCallback, errorCallback, document), false);
}

void DOMWindow::webkitResolveLocalFileSystemURL(const String& url, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    Document* document = this->document();
    if (!document)
        return;

    SecurityOrigin* securityOrigin = document->securityOrigin();
    KURL completedURL = document->completeURL(url);
    if (!AsyncFileSystem::isAvailable() || !securityOrigin->canAccessFileSystem() || !securityOrigin->canRequest(completedURL)) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    AsyncFileSystem::Type type;
    String filePath;
    if (!completedURL.isValid() || !DOMFileSystemBase::crackFileSystemURL(completedURL, type, filePath)) {
        DOMFileSystem::scheduleCallback(document, errorCallback, FileError::create(FileError::ENCODING_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().readFileSystem(document, type, ResolveURICallbacks::create(successCallback, errorCallback, document, filePath));
}

COMPILE_ASSERT(static_cast<int>(DOMWindow::EXTERNAL) == static_cast<int>(AsyncFileSystem::External), enum_mismatch);

COMPILE_ASSERT(static_cast<int>(DOMWindow::TEMPORARY) == static_cast<int>(AsyncFileSystem::Temporary), enum_mismatch);
COMPILE_ASSERT(static_cast<int>(DOMWindow::PERSISTENT) == static_cast<int>(AsyncFileSystem::Persistent), enum_mismatch);

#endif

void DOMWindow::postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort* port, const String& targetOrigin, DOMWindow* source, ExceptionCode& ec)
{
    MessagePortArray ports;
    if (port)
        ports.append(port);
    postMessage(message, &ports, targetOrigin, source, ec);
}

void DOMWindow::postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray* ports, const String& targetOrigin, DOMWindow* source, ExceptionCode& ec)
{
    if (!m_frame)
        return;

    // Compute the target origin.  We need to do this synchronously in order
    // to generate the SYNTAX_ERR exception correctly.
    RefPtr<SecurityOrigin> target;
    if (targetOrigin != "*") {
        target = SecurityOrigin::createFromString(targetOrigin);
        if (target->isEmpty()) {
            ec = SYNTAX_ERR;
            return;
        }
    }

    OwnPtr<MessagePortChannelArray> channels = MessagePort::disentanglePorts(ports, ec);
    if (ec)
        return;

    // Capture the source of the message.  We need to do this synchronously
    // in order to capture the source of the message correctly.
    Document* sourceDocument = source->document();
    if (!sourceDocument)
        return;
    String sourceOrigin = sourceDocument->securityOrigin()->toString();

    // Schedule the message.
    PostMessageTimer* timer = new PostMessageTimer(this, message, sourceOrigin, source, channels.release(), target.get());
    timer->startOneShot(0);
}

void DOMWindow::postMessageTimerFired(PassOwnPtr<PostMessageTimer> t)
{
    OwnPtr<PostMessageTimer> timer(t);

    if (!document())
        return;

    if (timer->targetOrigin()) {
        // Check target origin now since the target document may have changed since the simer was scheduled.
        if (!timer->targetOrigin()->isSameSchemeHostPort(document()->securityOrigin())) {
            String message = makeString("Unable to post message to ", timer->targetOrigin()->toString(),
                                        ". Recipient has origin ", document()->securityOrigin()->toString(), ".\n");
            console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, 0, String());
            return;
        }
    }

    dispatchEvent(timer->event(document()));
}

DOMSelection* DOMWindow::getSelection()
{
    if (!m_selection)
        m_selection = DOMSelection::create(m_frame);
    return m_selection.get();
}

Element* DOMWindow::frameElement() const
{
    if (!m_frame)
        return 0;

    return m_frame->ownerElement();
}

void DOMWindow::focus()
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    // If we're a top level window, bring the window to the front.
    if (m_frame == page->mainFrame())
        page->chrome()->focus();

    if (!m_frame)
        return;

    m_frame->eventHandler()->focusDocumentView();
}

void DOMWindow::blur()
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    page->chrome()->unfocus();
}

void DOMWindow::close(ScriptExecutionContext* context)
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    if (context) {
        ASSERT(WTF::isMainThread());
        Frame* activeFrame = static_cast<Document*>(context)->frame();
        if (!activeFrame)
            return;

        if (!activeFrame->loader()->shouldAllowNavigation(m_frame))
            return;
    }

    Settings* settings = m_frame->settings();
    bool allowScriptsToCloseWindows = settings && settings->allowScriptsToCloseWindows();

    if (!(page->openedByDOM() || page->backForward()->count() <= 1 || allowScriptsToCloseWindows))
        return;

    if (!m_frame->loader()->shouldClose())
        return;

    page->chrome()->closeWindowSoon();
}

void DOMWindow::print()
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame->loader()->activeDocumentLoader()->isLoading()) {
        m_shouldPrintWhenFinishedLoading = true;
        return;
    }
    m_shouldPrintWhenFinishedLoading = false;
    page->chrome()->print(m_frame);
}

void DOMWindow::stop()
{
    if (!m_frame)
        return;

    // We must check whether the load is complete asynchronously, because we might still be parsing
    // the document until the callstack unwinds.
    m_frame->loader()->stopForUserCancel(true);
}

void DOMWindow::alert(const String& message)
{
    if (!m_frame)
        return;

    m_frame->document()->updateStyleIfNeeded();

    Page* page = m_frame->page();
    if (!page)
        return;

    page->chrome()->runJavaScriptAlert(m_frame, message);
}

bool DOMWindow::confirm(const String& message)
{
    if (!m_frame)
        return false;

    m_frame->document()->updateStyleIfNeeded();

    Page* page = m_frame->page();
    if (!page)
        return false;

    return page->chrome()->runJavaScriptConfirm(m_frame, message);
}

String DOMWindow::prompt(const String& message, const String& defaultValue)
{
    if (!m_frame)
        return String();

    m_frame->document()->updateStyleIfNeeded();

    Page* page = m_frame->page();
    if (!page)
        return String();

    String returnValue;
    if (page->chrome()->runJavaScriptPrompt(m_frame, message, defaultValue, returnValue))
        return returnValue;

    return String();
}

String DOMWindow::btoa(const String& stringToEncode, ExceptionCode& ec)
{
    if (stringToEncode.isNull())
        return String();

    if (!stringToEncode.containsOnlyLatin1()) {
        ec = INVALID_CHARACTER_ERR;
        return String();
    }

    return base64Encode(stringToEncode.latin1());
}

String DOMWindow::atob(const String& encodedString, ExceptionCode& ec)
{
    if (encodedString.isNull())
        return String();

    if (!encodedString.containsOnlyLatin1()) {
        ec = INVALID_CHARACTER_ERR;
        return String();
    }

    Vector<char> out;
    if (!base64Decode(encodedString, out, FailOnInvalidCharacter)) {
        ec = INVALID_CHARACTER_ERR;
        return String();
    }

    return String(out.data(), out.size());
}

bool DOMWindow::find(const String& string, bool caseSensitive, bool backwards, bool wrap, bool /*wholeWord*/, bool /*searchInFrames*/, bool /*showDialog*/) const
{
    if (!m_frame)
        return false;

    // FIXME (13016): Support wholeWord, searchInFrames and showDialog
    return m_frame->editor()->findString(string, !backwards, caseSensitive, wrap, false);
}

bool DOMWindow::offscreenBuffering() const
{
    return true;
}

int DOMWindow::outerHeight() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().height());
}

int DOMWindow::outerWidth() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().width());
}

int DOMWindow::innerHeight() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;
    
    return static_cast<int>(view->height() / m_frame->pageZoomFactor());
}

int DOMWindow::innerWidth() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    return static_cast<int>(view->width() / m_frame->pageZoomFactor());
}

int DOMWindow::screenX() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().x());
}

int DOMWindow::screenY() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().y());
}

int DOMWindow::scrollX() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    return static_cast<int>(view->scrollX() / m_frame->pageZoomFactor());
}

int DOMWindow::scrollY() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    return static_cast<int>(view->scrollY() / m_frame->pageZoomFactor());
}

bool DOMWindow::closed() const
{
    return !m_frame;
}

unsigned DOMWindow::length() const
{
    if (!m_frame)
        return 0;

    return m_frame->tree()->childCount();
}

String DOMWindow::name() const
{
    if (!m_frame)
        return String();

    return m_frame->tree()->name();
}

void DOMWindow::setName(const String& string)
{
    if (!m_frame)
        return;

    m_frame->tree()->setName(string);
}

void DOMWindow::setStatus(const String& string) 
{
    m_status = string;

    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    ASSERT(m_frame->document()); // Client calls shouldn't be made when the frame is in inconsistent state.
    page->chrome()->setStatusbarText(m_frame, m_status);
} 
    
void DOMWindow::setDefaultStatus(const String& string) 
{
    m_defaultStatus = string;

    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    ASSERT(m_frame->document()); // Client calls shouldn't be made when the frame is in inconsistent state.
    page->chrome()->setStatusbarText(m_frame, m_defaultStatus);
}

DOMWindow* DOMWindow::self() const
{
    if (!m_frame)
        return 0;

    return m_frame->domWindow();
}

DOMWindow* DOMWindow::opener() const
{
    if (!m_frame)
        return 0;

    Frame* opener = m_frame->loader()->opener();
    if (!opener)
        return 0;

    return opener->domWindow();
}

DOMWindow* DOMWindow::parent() const
{
    if (!m_frame)
        return 0;

    Frame* parent = m_frame->tree()->parent(true);
    if (parent)
        return parent->domWindow();

    return m_frame->domWindow();
}

DOMWindow* DOMWindow::top() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return m_frame->tree()->top(true)->domWindow();
}

Document* DOMWindow::document() const
{
    // FIXME: This function shouldn't need a frame to work.
    if (!m_frame)
        return 0;

    // The m_frame pointer is not zeroed out when the window is put into b/f cache, so it can hold an unrelated document/window pair.
    // FIXME: We should always zero out the frame pointer on navigation to avoid accidentally accessing the new frame content.
    if (m_frame->domWindow() != this)
        return 0;

    ASSERT(m_frame->document());
    return m_frame->document();
}

PassRefPtr<StyleMedia> DOMWindow::styleMedia() const
{
    if (!m_media)
        m_media = StyleMedia::create(m_frame);
    return m_media.get();
}

PassRefPtr<CSSStyleDeclaration> DOMWindow::getComputedStyle(Element* elt, const String& pseudoElt) const
{
    if (!elt)
        return 0;

    return computedStyle(elt, false, pseudoElt);
}

PassRefPtr<CSSRuleList> DOMWindow::getMatchedCSSRules(Element* elt, const String&, bool authorOnly) const
{
    if (!m_frame)
        return 0;

    Settings* settings = m_frame->settings();
    return m_frame->document()->styleSelector()->styleRulesForElement(elt, authorOnly, false, settings && settings->crossOriginCheckInGetMatchedCSSRulesDisabled() ? AllCSSRules : SameOriginCSSRulesOnly);
}

PassRefPtr<WebKitPoint> DOMWindow::webkitConvertPointFromNodeToPage(Node* node, const WebKitPoint* p) const
{
    if (!node || !p)
        return 0;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    FloatPoint pagePoint(p->x(), p->y());
    pagePoint = node->convertToPage(pagePoint);
    return WebKitPoint::create(pagePoint.x(), pagePoint.y());
}

PassRefPtr<WebKitPoint> DOMWindow::webkitConvertPointFromPageToNode(Node* node, const WebKitPoint* p) const
{
    if (!node || !p)
        return 0;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    FloatPoint nodePoint(p->x(), p->y());
    nodePoint = node->convertFromPage(nodePoint);
    return WebKitPoint::create(nodePoint.x(), nodePoint.y());
}

double DOMWindow::devicePixelRatio() const
{
    if (!m_frame)
        return 0.0;

    Page* page = m_frame->page();
    if (!page)
        return 0.0;

    return page->chrome()->scaleFactor();
}

#if ENABLE(DATABASE)
PassRefPtr<Database> DOMWindow::openDatabase(const String& name, const String& version, const String& displayName, unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback, ExceptionCode& ec)
{
    RefPtr<Database> database = 0;
    if (m_frame && AbstractDatabase::isAvailable() && m_frame->document()->securityOrigin()->canAccessDatabase())
        database = Database::openDatabase(m_frame->document(), name, version, displayName, estimatedSize, creationCallback, ec);

    if (!database && !ec)
        ec = SECURITY_ERR;

    return database;
}
#endif

void DOMWindow::scrollBy(int x, int y) const
{
    if (!m_frame)
        return;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    FrameView* view = m_frame->view();
    if (!view)
        return;

    view->scrollBy(IntSize(x, y));
}

void DOMWindow::scrollTo(int x, int y) const
{
    if (!m_frame)
        return;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    RefPtr<FrameView> view = m_frame->view();
    if (!view)
        return;

    int zoomedX = static_cast<int>(x * m_frame->pageZoomFactor());
    int zoomedY = static_cast<int>(y * m_frame->pageZoomFactor());
    view->setScrollPosition(IntPoint(zoomedX, zoomedY));
}

void DOMWindow::moveBy(float x, float y) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatRect update = fr;
    update.move(x, y);
    // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
    adjustWindowRect(screenAvailableRect(page->mainFrame()->view()), fr, update);
    page->chrome()->setWindowRect(fr);
}

void DOMWindow::moveTo(float x, float y) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatRect sr = screenAvailableRect(page->mainFrame()->view());
    fr.setLocation(sr.location());
    FloatRect update = fr;
    update.move(x, y);     
    // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
    adjustWindowRect(sr, fr, update);
    page->chrome()->setWindowRect(fr);
}

void DOMWindow::resizeBy(float x, float y) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatSize dest = fr.size() + FloatSize(x, y);
    FloatRect update(fr.location(), dest);
    adjustWindowRect(screenAvailableRect(page->mainFrame()->view()), fr, update);
    page->chrome()->setWindowRect(fr);
}

void DOMWindow::resizeTo(float width, float height) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatSize dest = FloatSize(width, height);
    FloatRect update(fr.location(), dest);
    adjustWindowRect(screenAvailableRect(page->mainFrame()->view()), fr, update);
    page->chrome()->setWindowRect(fr);
}

int DOMWindow::setTimeout(PassOwnPtr<ScheduledAction> action, int timeout, ExceptionCode& ec)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context) {
        ec = INVALID_ACCESS_ERR;
        return -1;
    }
    return DOMTimer::install(context, action, timeout, true);
}

void DOMWindow::clearTimeout(int timeoutId)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context)
        return;
    DOMTimer::removeById(context, timeoutId);
}

int DOMWindow::setInterval(PassOwnPtr<ScheduledAction> action, int timeout, ExceptionCode& ec)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context) {
        ec = INVALID_ACCESS_ERR;
        return -1;
    }
    return DOMTimer::install(context, action, timeout, false);
}

void DOMWindow::clearInterval(int timeoutId)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context)
        return;
    DOMTimer::removeById(context, timeoutId);
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
int DOMWindow::webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback> callback, Element* e)
{
    if (Document* d = document())
        return d->webkitRequestAnimationFrame(callback, e);
    return 0;
}

void DOMWindow::webkitCancelRequestAnimationFrame(int id)
{
    if (Document* d = document())
        d->webkitCancelRequestAnimationFrame(id);
}
#endif

bool DOMWindow::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    if (!EventTarget::addEventListener(eventType, listener, useCapture))
        return false;

    if (Document* document = this->document())
        document->addListenerTypeIfNeeded(eventType);

    if (eventType == eventNames().unloadEvent)
        addUnloadEventListener(this);
    else if (eventType == eventNames().beforeunloadEvent && allowsBeforeUnloadListeners(this))
        addBeforeUnloadEventListener(this);
#if ENABLE(DEVICE_ORIENTATION)
    else if (eventType == eventNames().devicemotionEvent && frame() && frame()->page() && frame()->page()->deviceMotionController())
        frame()->page()->deviceMotionController()->addListener(this);
    else if (eventType == eventNames().deviceorientationEvent && frame() && frame()->page() && frame()->page()->deviceOrientationController())
        frame()->page()->deviceOrientationController()->addListener(this);
#endif

    return true;
}

bool DOMWindow::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    if (!EventTarget::removeEventListener(eventType, listener, useCapture))
        return false;

    if (eventType == eventNames().unloadEvent)
        removeUnloadEventListener(this);
    else if (eventType == eventNames().beforeunloadEvent && allowsBeforeUnloadListeners(this))
        removeBeforeUnloadEventListener(this);
#if ENABLE(DEVICE_ORIENTATION)
    else if (eventType == eventNames().devicemotionEvent && frame() && frame()->page() && frame()->page()->deviceMotionController())
        frame()->page()->deviceMotionController()->removeListener(this);
    else if (eventType == eventNames().deviceorientationEvent && frame() && frame()->page() && frame()->page()->deviceOrientationController())
        frame()->page()->deviceOrientationController()->removeListener(this);
#endif

    return true;
}

void DOMWindow::dispatchLoadEvent()
{
    RefPtr<Event> loadEvent(Event::create(eventNames().loadEvent, false, false));
    if (m_frame && m_frame->loader()->documentLoader() && !m_frame->loader()->documentLoader()->timing()->loadEventStart) {
        // The DocumentLoader (and thus its DocumentLoadTiming) might get destroyed while dispatching
        // the event, so protect it to prevent writing the end time into freed memory.
        RefPtr<DocumentLoader> documentLoader = m_frame->loader()->documentLoader();
        DocumentLoadTiming* timing = documentLoader->timing();
        dispatchTimedEvent(loadEvent, document(), &timing->loadEventStart, &timing->loadEventEnd);
    } else
        dispatchEvent(loadEvent, document());

    // For load events, send a separate load event to the enclosing frame only.
    // This is a DOM extension and is independent of bubbling/capturing rules of
    // the DOM.
    Element* ownerElement = m_frame ? m_frame->ownerElement() : 0;
    if (ownerElement)
        ownerElement->dispatchEvent(Event::create(eventNames().loadEvent, false, false));

    InspectorInstrumentation::loadEventFired(frame(), url());
}

bool DOMWindow::dispatchEvent(PassRefPtr<Event> prpEvent, PassRefPtr<EventTarget> prpTarget)
{
    RefPtr<EventTarget> protect = this;
    RefPtr<Event> event = prpEvent;

    event->setTarget(prpTarget ? prpTarget : this);
    event->setCurrentTarget(this);
    event->setEventPhase(Event::AT_TARGET);

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willDispatchEventOnWindow(frame(), *event, this);

    bool result = fireEventListeners(event.get());

    InspectorInstrumentation::didDispatchEventOnWindow(cookie);

    return result;
}

void DOMWindow::dispatchTimedEvent(PassRefPtr<Event> event, Document* target, double* startTime, double* endTime)
{
    ASSERT(startTime);
    ASSERT(endTime);
    *startTime = currentTime();
    dispatchEvent(event, target);
    *endTime = currentTime();
}

void DOMWindow::removeAllEventListeners()
{
    EventTarget::removeAllEventListeners();

#if ENABLE(DEVICE_ORIENTATION)
    if (frame() && frame()->page() && frame()->page()->deviceMotionController())
        frame()->page()->deviceMotionController()->removeAllListeners(this);
    if (frame() && frame()->page() && frame()->page()->deviceOrientationController())
        frame()->page()->deviceOrientationController()->removeAllListeners(this);
#endif

    removeAllUnloadEventListeners(this);
    removeAllBeforeUnloadEventListeners(this);
}

void DOMWindow::captureEvents()
{
    // Not implemented.
}

void DOMWindow::releaseEvents()
{
    // Not implemented.
}

void DOMWindow::finishedLoading()
{
    if (m_shouldPrintWhenFinishedLoading) {
        m_shouldPrintWhenFinishedLoading = false;
        print();
    }
}

EventTargetData* DOMWindow::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* DOMWindow::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void DOMWindow::setLocation(const String& urlString, DOMWindow* activeWindow, DOMWindow* firstWindow, SetLocationLocking locking)
{
    if (!m_frame)
        return;

    Frame* activeFrame = activeWindow->frame();
    if (!activeFrame)
        return;

    if (!activeFrame->loader()->shouldAllowNavigation(m_frame))
        return;

    Frame* firstFrame = firstWindow->frame();
    if (!firstFrame)
        return;

    KURL completedURL = firstFrame->document()->completeURL(urlString);
    if (completedURL.isNull())
        return;

    if (isInsecureScriptAccess(activeWindow, completedURL))
        return;

    // We want a new history item if we are processing a user gesture.
    m_frame->navigationScheduler()->scheduleLocationChange(activeFrame->document()->securityOrigin(),
        completedURL, activeFrame->loader()->outgoingReferrer(),
        locking != LockHistoryBasedOnGestureState || !activeFrame->script()->anyPageIsProcessingUserGesture(),
        locking != LockHistoryBasedOnGestureState);
}

void DOMWindow::printErrorMessage(const String& message)
{
    if (message.isEmpty())
        return;

    Settings* settings = m_frame->settings();
    if (!settings)
        return;
    if (settings->privateBrowsingEnabled())
        return;

    // FIXME: Add arguments so that we can provide a correct source URL and line number.
    console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, 1, String());
}

String DOMWindow::crossDomainAccessErrorMessage(DOMWindow* activeWindow)
{
    const KURL& activeWindowURL = activeWindow->url();
    if (activeWindowURL.isNull())
        return String();

    // FIXME: This error message should contain more specifics of why the same origin check has failed.
    // Perhaps we should involve the security origin object in composing it.
    // FIXME: This message, and other console messages, have extra newlines. Should remove them.
    return makeString("Unsafe JavaScript attempt to access frame with URL ", m_url.string(),
        " from frame with URL ", activeWindowURL.string(), ". Domains, protocols and ports must match.\n");
}

bool DOMWindow::isInsecureScriptAccess(DOMWindow* activeWindow, const String& urlString)
{
    if (!protocolIsJavaScript(urlString))
        return false;

    // FIXME: Is there some way to eliminate the need for a separate "activeWindow == this" check?
    if (activeWindow == this)
        return false;

    // FIXME: The name canAccess seems to be a roundabout way to ask "can execute script".
    // Can we name the SecurityOrigin function better to make this more clear?
    if (activeWindow->securityOrigin()->canAccess(securityOrigin()))
        return false;

    printErrorMessage(crossDomainAccessErrorMessage(activeWindow));
    return true;
}

Frame* DOMWindow::createWindow(const String& urlString, const AtomicString& frameName, const WindowFeatures& windowFeatures,
    DOMWindow* activeWindow, Frame* firstFrame, Frame* openerFrame, PrepareDialogFunction function, void* functionContext)
{
    Frame* activeFrame = activeWindow->frame();

    // For whatever reason, Firefox uses the first frame to determine the outgoingReferrer. We replicate that behavior here.
    String referrer = firstFrame->loader()->outgoingReferrer();

    KURL completedURL = urlString.isEmpty() ? KURL(ParsedURLString, "") : firstFrame->document()->completeURL(urlString);
    ResourceRequest request(completedURL, referrer);
    FrameLoader::addHTTPOriginIfNeeded(request, firstFrame->loader()->outgoingOrigin());
    FrameLoadRequest frameRequest(activeWindow->securityOrigin(), request, frameName);

    // We pass the opener frame for the lookupFrame in case the active frame is different from
    // the opener frame, and the name references a frame relative to the opener frame.
    bool created;
    Frame* newFrame = WebCore::createWindow(activeFrame, openerFrame, frameRequest, windowFeatures, created);
    if (!newFrame)
        return 0;

    newFrame->loader()->setOpener(openerFrame);
    newFrame->page()->setOpenedByDOM();

    if (newFrame->domWindow()->isInsecureScriptAccess(activeWindow, completedURL))
        return newFrame;

    if (function)
        function(newFrame->domWindow(), functionContext);

    if (created)
        newFrame->loader()->changeLocation(activeWindow->securityOrigin(), completedURL, referrer, false, false);
    else if (!urlString.isEmpty()) {
        newFrame->navigationScheduler()->scheduleLocationChange(activeWindow->securityOrigin(), completedURL.string(), referrer,
            !activeFrame->script()->anyPageIsProcessingUserGesture(), false);
    }

    return newFrame;
}

PassRefPtr<DOMWindow> DOMWindow::open(const String& urlString, const AtomicString& frameName, const String& windowFeaturesString,
    DOMWindow* activeWindow, DOMWindow* firstWindow)
{
    if (!m_frame)
        return 0;
    Frame* activeFrame = activeWindow->frame();
    if (!activeFrame)
        return 0;
    Frame* firstFrame = firstWindow->frame();
    if (!firstFrame)
        return 0;

    if (!firstWindow->allowPopUp()) {
        // Because FrameTree::find() returns true for empty strings, we must check for empty frame names.
        // Otherwise, illegitimate window.open() calls with no name will pass right through the popup blocker.
        if (frameName.isEmpty() || !m_frame->tree()->find(frameName))
            return 0;
    }

    // Get the target frame for the special cases of _top and _parent.
    // In those cases, we schedule a location change right now and return early.
    Frame* targetFrame = 0;
    if (frameName == "_top")
        targetFrame = m_frame->tree()->top();
    else if (frameName == "_parent") {
        if (Frame* parent = m_frame->tree()->parent())
            targetFrame = parent;
        else
            targetFrame = m_frame;
    }
    if (targetFrame) {
        if (!activeFrame->loader()->shouldAllowNavigation(targetFrame))
            return 0;

        KURL completedURL = firstFrame->document()->completeURL(urlString);

        if (targetFrame->domWindow()->isInsecureScriptAccess(activeWindow, completedURL))
            return targetFrame->domWindow();

        if (urlString.isEmpty())
            return targetFrame->domWindow();

        // For whatever reason, Firefox uses the first window rather than the active window to
        // determine the outgoing referrer. We replicate that behavior here.
        targetFrame->navigationScheduler()->scheduleLocationChange(activeFrame->document()->securityOrigin(),
            completedURL,
            firstFrame->loader()->outgoingReferrer(),
            !activeFrame->script()->anyPageIsProcessingUserGesture(), false);

        return targetFrame->domWindow();
    }

    WindowFeatures windowFeatures(windowFeaturesString);
    FloatRect windowRect(windowFeatures.xSet ? windowFeatures.x : 0, windowFeatures.ySet ? windowFeatures.y : 0,
        windowFeatures.widthSet ? windowFeatures.width : 0, windowFeatures.heightSet ? windowFeatures.height : 0);
    Page* page = m_frame->page();
    DOMWindow::adjustWindowRect(screenAvailableRect(page ? page->mainFrame()->view() : 0), windowRect, windowRect);
    windowFeatures.x = windowRect.x();
    windowFeatures.y = windowRect.y();
    windowFeatures.height = windowRect.height();
    windowFeatures.width = windowRect.width();

    Frame* result = createWindow(urlString, frameName, windowFeatures, activeWindow, firstFrame, m_frame);
    return result ? result->domWindow() : 0;
}

void DOMWindow::showModalDialog(const String& urlString, const String& dialogFeaturesString,
    DOMWindow* activeWindow, DOMWindow* firstWindow, PrepareDialogFunction function, void* functionContext)
{
    if (!m_frame)
        return;
    Frame* activeFrame = activeWindow->frame();
    if (!activeFrame)
        return;
    Frame* firstFrame = firstWindow->frame();
    if (!firstFrame)
        return;

    if (m_frame->page())
        m_frame->page()->chrome()->willRunModalHTMLDialog(m_frame);

    if (!canShowModalDialogNow(m_frame) || !firstWindow->allowPopUp())
        return;

    Frame* dialogFrame = createWindow(urlString, emptyAtom, WindowFeatures(dialogFeaturesString, screenAvailableRect(m_frame->view())),
        activeWindow, firstFrame, m_frame, function, functionContext);
    if (!dialogFrame)
        return;

    dialogFrame->page()->chrome()->runModal();
}

#if ENABLE(BLOB)
DOMURL* DOMWindow::webkitURL() const
{
    if (!m_domURL)
        m_domURL = DOMURL::create(this->scriptExecutionContext());
    return m_domURL.get();
}
#endif

#if ENABLE(QUOTA)
StorageInfo* DOMWindow::webkitStorageInfo() const
{
    if (!m_storageInfo)
        m_storageInfo = StorageInfo::create();
    return m_storageInfo.get();
}
#endif

} // namespace WebCore
