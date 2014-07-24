/*
    Copyright (C) 2010 Robert Hogan <robert@roberthogan.net>
    Copyright (C) 2008,2009,2010 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2007, 2012 Apple Inc.

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
#include "DumpRenderTreeSupportQt.h"

#include "APICast.h"
#include "ApplicationCacheStorage.h"
#include "ChromeClientQt.h"
#include "ContainerNode.h"
#include "ContextMenu.h"
#include "ContextMenuClientQt.h"
#include "ContextMenuController.h"
#include "DeviceOrientationClientMock.h"
#include "DeviceOrientationController.h"
#include "DeviceOrientationData.h"
#include "DocumentLoader.h"
#include "Editor.h"
#include "EditorClientQt.h"
#include "Element.h"
#include "FocusController.h"
#include "Font.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoaderClientQt.h"
#include "FrameView.h"
#include "GCController.h"
#include "GeolocationClient.h"
#include "GeolocationClientMock.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HistoryItem.h"
#include "InspectorController.h"
#include "JSNode.h"
#include "NodeList.h"
#include "NotificationPresenterClientQt.h"
#include "Page.h"
#include "PageGroup.h"
#include "PluginDatabase.h"
#include "PluginView.h"
#include "PositionError.h"
#include "PrintContext.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "RenderListItem.h"
#include "RenderTreeAsText.h"
#include "RuntimeEnabledFeatures.h"
#include "SchemeRegistry.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"
#include "TextIterator.h"
#include "ThirdPartyCookiesQt.h"
#include "WebCoreTestSupport.h"
#include "qt_runtime.h"
#include "qwebelement.h"
#include "qwebhistory.h"
#include "qwebhistory_p.h"
#include "qwebscriptworld.h"

#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
#include "HTMLVideoElement.h"
#include "MediaPlayerPrivateQt.h"
#endif

#include <QPainter>
#include <wtf/CurrentTime.h>

using namespace WebCore;

QMap<int, QWebScriptWorld*> m_worldMap;

#if ENABLE(GEOLOCATION)
GeolocationClientMock* toGeolocationClientMock(GeolocationClient* client)
{
    ASSERT(QWebPageAdapter::drtRun);
    return static_cast<GeolocationClientMock*>(client);
}
#endif

#if ENABLE(DEVICE_ORIENTATION)
DeviceOrientationClientMock* toDeviceOrientationClientMock(DeviceOrientationClient* client)
{
    ASSERT(QWebPageAdapter::drtRun);
    return static_cast<DeviceOrientationClientMock*>(client);
}
#endif

QDRTNode::QDRTNode()
    : m_node(0)
{
}

QDRTNode::QDRTNode(WebCore::Node* node)
    : m_node(0)
{
    if (node) {
        m_node = node;
        m_node->ref();
    }
}

QDRTNode::~QDRTNode()
{
    if (m_node)
        m_node->deref();
}

QDRTNode::QDRTNode(const QDRTNode& other)
    :m_node(other.m_node)
{
    if (m_node)
        m_node->ref();
}

QDRTNode& QDRTNode::operator=(const QDRTNode& other)
{
    if (this != &other) {
        Node* otherNode = other.m_node;
        if (otherNode)
            otherNode->ref();
        if (m_node)
            m_node->deref();
        m_node = otherNode;
    }
    return *this;
}

QDRTNode QtDRTNodeRuntime::create(WebCore::Node* node)
{
    return QDRTNode(node);
}

WebCore::Node* QtDRTNodeRuntime::get(const QDRTNode& node)
{
    return node.m_node;
}

static QVariant convertJSValueToNodeVariant(JSC::JSObject* object, int *distance, HashSet<JSObjectRef>*)
{
    if (!object || !object->inherits(&JSNode::s_info))
        return QVariant();
    return QVariant::fromValue<QDRTNode>(QtDRTNodeRuntime::create((static_cast<JSNode*>(object))->impl()));
}

static JSC::JSValue convertNodeVariantToJSValue(JSC::ExecState* exec, WebCore::JSDOMGlobalObject* globalObject, const QVariant& variant)
{
    return toJS(exec, globalObject, QtDRTNodeRuntime::get(variant.value<QDRTNode>()));
}

void QtDRTNodeRuntime::initialize()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    int id = qRegisterMetaType<QDRTNode>();
    JSC::Bindings::registerCustomType(id, convertJSValueToNodeVariant, convertNodeVariantToJSValue);
}

DumpRenderTreeSupportQt::DumpRenderTreeSupportQt()
{
}

DumpRenderTreeSupportQt::~DumpRenderTreeSupportQt()
{
}

void DumpRenderTreeSupportQt::initialize()
{
    QtDRTNodeRuntime::initialize();
}

void DumpRenderTreeSupportQt::overwritePluginDirectories()
{
    PluginDatabase* db = PluginDatabase::installedPlugins(/* populate */ false);

    Vector<String> paths;
    String qtPath(qgetenv("QTWEBKIT_PLUGIN_PATH").data());
    qtPath.split(UChar(':'), /* allowEmptyEntries */ false, paths);

    db->setPluginDirectories(paths);
    db->refresh();
}

void DumpRenderTreeSupportQt::setDumpRenderTreeModeEnabled(bool b)
{
    QWebPageAdapter::drtRun = b;
#if ENABLE(NETSCAPE_PLUGIN_API) && defined(XP_UNIX)
    // PluginViewQt (X11) needs a few workarounds when running under DRT
    PluginView::setIsRunningUnderDRT(b);
#endif
}

void DumpRenderTreeSupportQt::setFrameFlatteningEnabled(QWebPageAdapter* adapter, bool enabled)
{
    adapter->page->settings()->setFrameFlatteningEnabled(enabled);
}

void DumpRenderTreeSupportQt::webPageSetGroupName(QWebPageAdapter *adapter, const QString& groupName)
{
    adapter->page->setGroupName(groupName);
}

QString DumpRenderTreeSupportQt::webPageGroupName(QWebPageAdapter* adapter)
{
    return adapter->page->groupName();
}

void DumpRenderTreeSupportQt::webInspectorExecuteScript(QWebPageAdapter* adapter, long callId, const QString& script)
{
#if ENABLE(INSPECTOR)
    if (!adapter->page->inspectorController())
        return;
    adapter->page->inspectorController()->evaluateForTestInFrontend(callId, script);
#endif
}

void DumpRenderTreeSupportQt::webInspectorShow(QWebPageAdapter* adapter)
{
#if ENABLE(INSPECTOR)
    if (!adapter->page->inspectorController())
        return;
    adapter->page->inspectorController()->show();
#endif
}

void DumpRenderTreeSupportQt::webInspectorClose(QWebPageAdapter* adapter)
{
#if ENABLE(INSPECTOR)
    if (!adapter->page->inspectorController())
        return;
    adapter->page->inspectorController()->close();
#endif
}

bool DumpRenderTreeSupportQt::hasDocumentElement(QWebFrameAdapter *adapter)
{
    return adapter->frame->document()->documentElement();
}

void DumpRenderTreeSupportQt::setValueForUser(const QWebElement& element, const QString& value)
{
    WebCore::Element* webElement = element.m_element;
    if (!webElement)
        return;
    HTMLInputElement* inputElement = webElement->toInputElement();
    if (!inputElement)
        return;

    inputElement->setValueForUser(value);
}

void DumpRenderTreeSupportQt::clearFrameName(QWebFrameAdapter *adapter)
{
    Frame* coreFrame = adapter->frame;
    coreFrame->tree()->clearName();
}

int DumpRenderTreeSupportQt::javaScriptObjectsCount()
{
    return JSDOMWindowBase::commonVM()->heap.globalObjectCount();
}

void DumpRenderTreeSupportQt::garbageCollectorCollect()
{
    gcController().garbageCollectNow();
}

void DumpRenderTreeSupportQt::garbageCollectorCollectOnAlternateThread(bool waitUntilDone)
{
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

void DumpRenderTreeSupportQt::whiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    SecurityPolicy::addOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void DumpRenderTreeSupportQt::removeWhiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void DumpRenderTreeSupportQt::resetOriginAccessWhiteLists()
{
    SecurityPolicy::resetOriginAccessWhitelists();
}

void DumpRenderTreeSupportQt::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const QString& scheme)
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme);
}

void DumpRenderTreeSupportQt::setCaretBrowsingEnabled(QWebPageAdapter* adapter, bool value)
{
    adapter->page->settings()->setCaretBrowsingEnabled(value);
}

void DumpRenderTreeSupportQt::setAuthorAndUserStylesEnabled(QWebPageAdapter* adapter, bool value)
{
    adapter->page->settings()->setAuthorAndUserStylesEnabled(value);
}

void DumpRenderTreeSupportQt::executeCoreCommandByName(QWebPageAdapter* adapter, const QString& name, const QString& value)
{
    adapter->page->focusController()->focusedOrMainFrame()->editor().command(name).execute(value);
}

bool DumpRenderTreeSupportQt::isCommandEnabled(QWebPageAdapter *adapter, const QString& name)
{
    return adapter->page->focusController()->focusedOrMainFrame()->editor().command(name).isEnabled();
}

QVariantList DumpRenderTreeSupportQt::selectedRange(QWebPageAdapter *adapter)
{
    WebCore::Frame* frame = adapter->page->focusController()->focusedOrMainFrame();
    QVariantList selectedRange;
    RefPtr<Range> range = frame->selection()->toNormalizedRange().get();

    Element* selectionRoot = frame->selection()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : frame->document()->documentElement();

    RefPtr<Range> testRange = Range::create(scope->document(), scope, 0, range->startContainer(), range->startOffset());
    ASSERT(testRange->startContainer() == scope);
    int startPosition = TextIterator::rangeLength(testRange.get());

    ExceptionCode ec;
    testRange->setEnd(range->endContainer(), range->endOffset(), ec);
    ASSERT(testRange->startContainer() == scope);
    int endPosition = TextIterator::rangeLength(testRange.get());

    selectedRange << startPosition << (endPosition - startPosition);

    return selectedRange;

}

QVariantList DumpRenderTreeSupportQt::firstRectForCharacterRange(QWebPageAdapter *adapter, int location, int length)
{
    WebCore::Frame* frame = adapter->page->focusController()->focusedOrMainFrame();
    QVariantList rect;

    if ((location + length < location) && (location + length))
        length = 0;

    RefPtr<Range> range = TextIterator::rangeFromLocationAndLength(frame->selection()->rootEditableElementOrDocumentElement(), location, length);

    if (!range)
        return QVariantList();

    QRect resultRect = frame->editor().firstRectForRange(range.get());
    rect << resultRect.x() << resultRect.y() << resultRect.width() << resultRect.height();
    return rect;
}

void DumpRenderTreeSupportQt::setWindowsBehaviorAsEditingBehavior(QWebPageAdapter* adapter)
{
    Page* corePage = adapter->page;
    if (!corePage)
        return;
    corePage->settings()->setEditingBehaviorType(EditingWindowsBehavior);
}

void DumpRenderTreeSupportQt::clearAllApplicationCaches()
{
    WebCore::cacheStorage().empty();
    WebCore::cacheStorage().vacuumDatabaseFile();
}

void DumpRenderTreeSupportQt::dumpFrameLoader(bool b)
{
    FrameLoaderClientQt::dumpFrameLoaderCallbacks = b;
}

void DumpRenderTreeSupportQt::dumpProgressFinishedCallback(bool b)
{
    FrameLoaderClientQt::dumpProgressFinishedCallback = b;
}

void DumpRenderTreeSupportQt::dumpUserGestureInFrameLoader(bool b)
{
    FrameLoaderClientQt::dumpUserGestureInFrameLoaderCallbacks = b;
}

void DumpRenderTreeSupportQt::dumpResourceLoadCallbacks(bool b)
{
    FrameLoaderClientQt::dumpResourceLoadCallbacks = b;
}

void DumpRenderTreeSupportQt::dumpResourceLoadCallbacksPath(const QString& path)
{
    FrameLoaderClientQt::dumpResourceLoadCallbacksPath = path;
}

void DumpRenderTreeSupportQt::dumpResourceResponseMIMETypes(bool b)
{
    FrameLoaderClientQt::dumpResourceResponseMIMETypes = b;
}

void DumpRenderTreeSupportQt::dumpWillCacheResponseCallbacks(bool b)
{
    FrameLoaderClientQt::dumpWillCacheResponseCallbacks = b;
}

void DumpRenderTreeSupportQt::setWillSendRequestReturnsNullOnRedirect(bool b)
{
    FrameLoaderClientQt::sendRequestReturnsNullOnRedirect = b;
}

void DumpRenderTreeSupportQt::setWillSendRequestReturnsNull(bool b)
{
    FrameLoaderClientQt::sendRequestReturnsNull = b;
}

void DumpRenderTreeSupportQt::setWillSendRequestClearHeaders(const QStringList& headers)
{
    FrameLoaderClientQt::sendRequestClearHeaders = headers;
}

void DumpRenderTreeSupportQt::setDeferMainResourceDataLoad(bool b)
{
    FrameLoaderClientQt::deferMainResourceDataLoad = b;
}

void DumpRenderTreeSupportQt::setCustomPolicyDelegate(bool enabled, bool permissive)
{
    FrameLoaderClientQt::policyDelegateEnabled = enabled;
    FrameLoaderClientQt::policyDelegatePermissive = permissive;
}

void DumpRenderTreeSupportQt::dumpHistoryCallbacks(bool b)
{
    FrameLoaderClientQt::dumpHistoryCallbacks = b;
}

void DumpRenderTreeSupportQt::dumpVisitedLinksCallbacks(bool b)
{
    ChromeClientQt::dumpVisitedLinksCallbacks = b;
}

void DumpRenderTreeSupportQt::dumpEditingCallbacks(bool b)
{
    EditorClientQt::dumpEditingCallbacks = b;
}

void DumpRenderTreeSupportQt::dumpSetAcceptsEditing(bool b)
{
    EditorClientQt::acceptsEditing = b;
}

void DumpRenderTreeSupportQt::dumpNotification(bool b)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    NotificationPresenterClientQt::dumpNotification = b;
#endif
}

QString DumpRenderTreeSupportQt::viewportAsText(QWebPageAdapter* adapter, int deviceDPI, const QSize& deviceSize, const QSize& availableSize)
{
    WebCore::ViewportArguments args = adapter->viewportArguments();

    float devicePixelRatio = deviceDPI / WebCore::ViewportArguments::deprecatedTargetDPI;
    WebCore::ViewportAttributes conf = WebCore::computeViewportAttributes(args,
        /* desktop-width    */980,
        /* device-width     */deviceSize.width(),
        /* device-height    */deviceSize.height(),
        devicePixelRatio,
        availableSize);
    WebCore::restrictMinimumScaleFactorToViewportSize(conf, availableSize, devicePixelRatio);
    WebCore::restrictScaleFactorToInitialScaleIfNotUserScalable(conf);

    QString res;
    res = res.sprintf("viewport size %dx%d scale %f with limits [%f, %f] and userScalable %f\n",
        static_cast<int>(conf.layoutSize.width()),
        static_cast<int>(conf.layoutSize.height()),
        conf.initialScale,
        conf.minimumScale,
        conf.maximumScale,
        conf.userScalable);

    return res;
}

void DumpRenderTreeSupportQt::scalePageBy(QWebFrameAdapter* adapter, float scalefactor, const QPoint& origin)
{
    WebCore::Frame* coreFrame = adapter->frame;
    if (Page* page = coreFrame->page())
        page->setPageScaleFactor(scalefactor, origin);
}

void DumpRenderTreeSupportQt::setMockDeviceOrientation(QWebPageAdapter* adapter, bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
#if ENABLE(DEVICE_ORIENTATION)
    Page* corePage = adapter->page;
    DeviceOrientationClientMock* mockClient = toDeviceOrientationClientMock(DeviceOrientationController::from(corePage)->deviceOrientationClient());
    mockClient->setOrientation(DeviceOrientationData::create(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma));
#endif
}

void DumpRenderTreeSupportQt::resetGeolocationMock(QWebPageAdapter* adapter)
{
#if ENABLE(GEOLOCATION)
    Page* corePage = adapter->page;
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage)->client());
    mockClient->reset();
#endif
}

void DumpRenderTreeSupportQt::setMockGeolocationPermission(QWebPageAdapter* adapter, bool allowed)
{
#if ENABLE(GEOLOCATION)
    Page* corePage = adapter->page;
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage)->client());
    mockClient->setPermission(allowed);
#endif
}

void DumpRenderTreeSupportQt::setMockGeolocationPosition(QWebPageAdapter* adapter, double latitude, double longitude, double accuracy)
{
#if ENABLE(GEOLOCATION)
    Page* corePage = adapter->page;
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage)->client());
    mockClient->setPosition(GeolocationPosition::create(currentTime(), latitude, longitude, accuracy));
#endif
}

void DumpRenderTreeSupportQt::setMockGeolocationPositionUnavailableError(QWebPageAdapter* adapter, const QString& message)
{
#if ENABLE(GEOLOCATION)
    Page* corePage = adapter->page;
    GeolocationClientMock* mockClient = static_cast<GeolocationClientMock*>(GeolocationController::from(corePage)->client());
    mockClient->setPositionUnavailableError(message);
#endif
}

int DumpRenderTreeSupportQt::numberOfPendingGeolocationPermissionRequests(QWebPageAdapter* adapter)
{
#if ENABLE(GEOLOCATION)
    Page* corePage = adapter->page;
    GeolocationClientMock* mockClient = toGeolocationClientMock(GeolocationController::from(corePage)->client());
    return mockClient->numberOfPendingPermissionRequests();
#else
    return -1;
#endif
}

bool DumpRenderTreeSupportQt::isTargetItem(const QWebHistoryItem& historyItem)
{
    QWebHistoryItem it = historyItem;
    if (QWebHistoryItemPrivate::core(&it)->isTargetItem())
        return true;
    return false;
}

QString DumpRenderTreeSupportQt::historyItemTarget(const QWebHistoryItem& historyItem)
{
    QWebHistoryItem it = historyItem;
    return (QWebHistoryItemPrivate::core(&it)->target());
}

QMap<QString, QWebHistoryItem> DumpRenderTreeSupportQt::getChildHistoryItems(const QWebHistoryItem& historyItem)
{
    QWebHistoryItem it = historyItem;
    HistoryItem* item = QWebHistoryItemPrivate::core(&it);
    const WebCore::HistoryItemVector& children = item->children();

    unsigned size = children.size();
    QMap<QString, QWebHistoryItem> kids;
    for (unsigned i = 0; i < size; ++i) {
        QWebHistoryItem kid(new QWebHistoryItemPrivate(children[i].get()));
        kids.insert(DumpRenderTreeSupportQt::historyItemTarget(kid), kid);
    }
    return kids;
}

bool DumpRenderTreeSupportQt::shouldClose(QWebFrameAdapter *adapter)
{
    WebCore::Frame* coreFrame = adapter->frame;
    return coreFrame->loader()->shouldClose();
}

void DumpRenderTreeSupportQt::clearScriptWorlds()
{
    m_worldMap.clear();
}

void DumpRenderTreeSupportQt::evaluateScriptInIsolatedWorld(QWebFrameAdapter *adapter, int worldID, const QString& script)
{
    QWebScriptWorld* scriptWorld;
    if (!worldID) {
        scriptWorld = new QWebScriptWorld();
    } else if (!m_worldMap.contains(worldID)) {
        scriptWorld = new QWebScriptWorld();
        m_worldMap.insert(worldID, scriptWorld);
    } else
        scriptWorld = m_worldMap.value(worldID);

    WebCore::Frame* coreFrame = adapter->frame;

    ScriptController* proxy = coreFrame->script();

    if (!proxy)
        return;
    proxy->executeScriptInWorld(scriptWorld->world(), script, true);
}

void DumpRenderTreeSupportQt::addUserStyleSheet(QWebPageAdapter* adapter, const QString& sourceCode)
{
    adapter->page->group().addUserStyleSheetToWorld(mainThreadNormalWorld(), sourceCode, QUrl(), Vector<String>(), Vector<String>(), WebCore::InjectInAllFrames);
}

void DumpRenderTreeSupportQt::removeUserStyleSheets(QWebPageAdapter* adapter)
{
    adapter->page->group().removeUserStyleSheetsFromWorld(mainThreadNormalWorld());
}

void DumpRenderTreeSupportQt::simulateDesktopNotificationClick(const QString& title)
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    NotificationPresenterClientQt::notificationPresenter()->notificationClicked(title);
#endif
}

void DumpRenderTreeSupportQt::setDefersLoading(QWebPageAdapter* adapter, bool flag)
{
    Page* corePage = adapter->page;
    if (corePage)
        corePage->setDefersLoading(flag);
}

void DumpRenderTreeSupportQt::goBack(QWebPageAdapter* adapter)
{
    Page* corePage = adapter->page;
    if (corePage)
        corePage->goBack();
}

// API Candidate?
QString DumpRenderTreeSupportQt::responseMimeType(QWebFrameAdapter* adapter)
{
    WebCore::Frame* coreFrame = adapter->frame;
    WebCore::DocumentLoader* docLoader = coreFrame->loader()->documentLoader();
    return docLoader->responseMIMEType();
}

void DumpRenderTreeSupportQt::clearOpener(QWebFrameAdapter* adapter)
{
    WebCore::Frame* coreFrame = adapter->frame;
    coreFrame->loader()->setOpener(0);
}

void DumpRenderTreeSupportQt::addURLToRedirect(const QString& origin, const QString& destination)
{
    FrameLoaderClientQt::URLsToRedirect[origin] = destination;
}

void DumpRenderTreeSupportQt::setInteractiveFormValidationEnabled(QWebPageAdapter* adapter, bool enable)
{
    Page* corePage = adapter->page;
    if (corePage)
        corePage->settings()->setInteractiveFormValidationEnabled(enable);
}

QStringList DumpRenderTreeSupportQt::contextMenu(QWebPageAdapter* page)
{
    return page->menuActionsAsText();
}

bool DumpRenderTreeSupportQt::thirdPartyCookiePolicyAllows(QWebPageAdapter *adapter, const QUrl& url, const QUrl& firstPartyUrl)
{
    Page* corePage = adapter->page;
    return thirdPartyCookiePolicyPermits(corePage->mainFrame()->loader()->networkingContext(), url, firstPartyUrl);
}

void DumpRenderTreeSupportQt::enableMockScrollbars()
{
    Settings::setMockScrollbarsEnabled(true);
}

QUrl DumpRenderTreeSupportQt::mediaContentUrlByElementId(QWebFrameAdapter* adapter, const QString& elementId)
{
    QUrl res;

#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
    Frame* coreFrame = adapter->frame;
    if (!coreFrame)
        return res;

    Document* doc = coreFrame->document();
    if (!doc)
        return res;

    Node* coreNode = doc->getElementById(elementId);
    if (!coreNode)
        return res;

    HTMLVideoElement* videoElement = toHTMLVideoElement(coreNode);
    PlatformMedia platformMedia = videoElement->platformMedia();
    if (platformMedia.type != PlatformMedia::QtMediaPlayerType)
        return res;

    MediaPlayerPrivateQt* mediaPlayerQt = static_cast<MediaPlayerPrivateQt*>(platformMedia.media.qtMediaPlayer);
    if (mediaPlayerQt && mediaPlayerQt->mediaPlayer())
        res = mediaPlayerQt->mediaPlayer()->media().canonicalUrl();
#endif

    return res;
}

// API Candidate?
void DumpRenderTreeSupportQt::setAlternateHtml(QWebFrameAdapter* adapter, const QString& html, const QUrl& baseUrl, const QUrl& failingUrl)
{
    KURL kurl(baseUrl);
    WebCore::Frame* coreFrame = adapter->frame;
    WebCore::ResourceRequest request(kurl);
    const QByteArray utf8 = html.toUtf8();
    WTF::RefPtr<WebCore::SharedBuffer> data = WebCore::SharedBuffer::create(utf8.constData(), utf8.length());
    WebCore::SubstituteData substituteData(data, WTF::String("text/html"), WTF::String("utf-8"), failingUrl);
    coreFrame->loader()->load(WebCore::FrameLoadRequest(coreFrame, request, substituteData));
}

void DumpRenderTreeSupportQt::confirmComposition(QWebPageAdapter *adapter, const char* text)
{
    Frame* frame = adapter->page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Editor& editor = frame->editor();
    if (!editor.hasComposition() && !text)
        return;

    if (editor.hasComposition()) {
        if (text)
            editor.confirmComposition(String::fromUTF8(text));
        else
            editor.confirmComposition();
    } else
        editor.insertText(String::fromUTF8(text), 0);
}

void DumpRenderTreeSupportQt::injectInternalsObject(QWebFrameAdapter* adapter)
{
    WebCore::Frame* coreFrame = adapter->frame;
    JSDOMWindow* window = toJSDOMWindow(coreFrame, mainThreadNormalWorld());
    Q_ASSERT(window);

    JSC::ExecState* exec = window->globalExec();
    Q_ASSERT(exec);
    JSC::JSLockHolder lock(exec);

    JSContextRef context = toRef(exec);
    WebCoreTestSupport::injectInternalsObject(context);
}

void DumpRenderTreeSupportQt::injectInternalsObject(JSContextRef context)
{
    WebCoreTestSupport::injectInternalsObject(context);
}

void DumpRenderTreeSupportQt::resetInternalsObject(QWebFrameAdapter* adapter)
{
    WebCore::Frame* coreFrame = adapter->frame;
    JSDOMWindow* window = toJSDOMWindow(coreFrame, mainThreadNormalWorld());
    Q_ASSERT(window);

    JSC::ExecState* exec = window->globalExec();
    Q_ASSERT(exec);
    JSC::JSLockHolder lock(exec);

    JSContextRef context = toRef(exec);
    WebCoreTestSupport::resetInternalsObject(context);
}

void DumpRenderTreeSupportQt::resetInternalsObject(JSContextRef context)
{
    WebCoreTestSupport::resetInternalsObject(context);
}

QImage DumpRenderTreeSupportQt::paintPagesWithBoundaries(QWebFrameAdapter* adapter)
{
    Frame* frame = adapter->frame;
    PrintContext printContext(frame);

    QRect rect = frame->view()->frameRect();

    IntRect pageRect(0, 0, rect.width(), rect.height());

    printContext.begin(pageRect.width(), pageRect.height());
    float pageHeight = 0;
    printContext.computePageRects(pageRect, /* headerHeight */ 0, /* footerHeight */ 0, /* userScaleFactor */ 1.0, pageHeight);

    QPainter painter;
    int pageCount = printContext.pageCount();
    // pages * pageHeight and 1px line between each page
    int totalHeight = pageCount * (pageRect.height() + 1) - 1;
    QImage image(pageRect.width(), totalHeight, QImage::Format_ARGB32);
    image.fill(Qt::white);
    painter.begin(&image);

    GraphicsContext ctx(&painter);
    for (int i = 0; i < printContext.pageCount(); ++i) {
        printContext.spoolPage(ctx, i, pageRect.width());
        // translate to next page coordinates
        ctx.translate(0, pageRect.height() + 1);

        // if there is a next page, draw a blue line between these two
        if (i + 1 < printContext.pageCount()) {
            ctx.save();
            ctx.setStrokeColor(Color(0, 0, 255), ColorSpaceDeviceRGB);
            ctx.setFillColor(Color(0, 0, 255), ColorSpaceDeviceRGB);
            ctx.drawLine(IntPoint(0, -1), IntPoint(pageRect.width(), -1));
            ctx.restore();
        }
    }

    painter.end();
    printContext.end();

    return image;
}

void DumpRenderTreeSupportQt::setTrackRepaintRects(QWebFrameAdapter* adapter, bool enable)
{
    adapter->frame->view()->setTracksRepaints(enable);
}

bool DumpRenderTreeSupportQt::trackRepaintRects(QWebFrameAdapter* adapter)
{
    return adapter->frame->view()->isTrackingRepaints();
}

void DumpRenderTreeSupportQt::getTrackedRepaintRects(QWebFrameAdapter* adapter, QVector<QRect>& result)
{
    Frame* coreFrame = adapter->frame;
    const Vector<IntRect>& rects = coreFrame->view()->trackedRepaintRects();
    result.resize(rects.size());
    for (size_t i = 0; i < rects.size(); ++i)
        result.append(rects[i]);
}

void DumpRenderTreeSupportQt::setSeamlessIFramesEnabled(bool enabled)
{
#if ENABLE(IFRAME_SEAMLESS)
    WebCore::RuntimeEnabledFeatures::setSeamlessIFramesEnabled(enabled);
#else
    UNUSED_PARAM(enabled);
#endif
}

void DumpRenderTreeSupportQt::setShouldUseFontSmoothing(bool enabled)
{
    WebCore::Font::setShouldUseSmoothing(enabled);
}

QString DumpRenderTreeSupportQt::frameRenderTreeDump(QWebFrameAdapter* adapter)
{
    if (adapter->frame->view() && adapter->frame->view()->layoutPending())
        adapter->frame->view()->layout();

    return externalRepresentation(adapter->frame);
}

void DumpRenderTreeSupportQt::clearNotificationPermissions()
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    WebCore::NotificationPresenterClientQt::notificationPresenter()->clearCachedPermissions();
#endif
}

void DumpRenderTreeSupportQt::disableDefaultTypesettingFeatures()
{
    WebCore::Font::setDefaultTypesettingFeatures(0);
}

void DumpRenderTreeSupportQt::resetPageVisibility(QWebPageAdapter* adapter)
{
#if ENABLE(PAGE_VISIBILITY_API)
    // Set visibility without emitting an event.
    adapter->page->setVisibilityState(PageVisibilityStateVisible, true);
#else
    UNUSED_PARAM(adapter);
#endif
}

void DumpRenderTreeSupportQt::getJSWindowObject(QWebFrameAdapter* adapter, JSContextRef* context, JSObjectRef* object)
{
    JSDOMWindow* window = toJSDOMWindow(adapter->frame, mainThreadNormalWorld());
    *object = toRef(window);
    *context = toRef(window->globalExec());
}
