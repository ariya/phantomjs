/*
    Copyright (C) 2010 Robert Hogan <robert@roberthogan.net>
    Copyright (C) 2008,2009,2010 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2007 Apple Inc.

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

#if USE(JSC)
#include "APICast.h"
#endif
#include "ApplicationCacheStorage.h"
#include "CSSComputedStyleDeclaration.h"
#include "ChromeClientQt.h"
#include "ContainerNode.h"
#include "ContextMenu.h"
#include "ContextMenuClientQt.h"
#include "ContextMenuController.h"
#include "DeviceOrientation.h"
#include "DeviceOrientationClientMockQt.h"
#include "DocumentLoader.h"
#include "Editor.h"
#include "EditorClientQt.h"
#include "Element.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoaderClientQt.h"
#include "FrameView.h"
#if USE(JSC)
#include "GCController.h"
#elif USE(V8)
#include "V8GCController.h"
#include "V8Proxy.h"
#endif
#include "GeolocationClient.h"
#include "GeolocationClientMock.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "HistoryItem.h"
#include "HTMLInputElement.h"
#include "InputElement.h"
#include "InspectorController.h"
#include "NodeList.h"
#include "NotificationPresenterClientQt.h"
#include "Page.h"
#include "PageGroup.h"
#include "PluginDatabase.h"
#include "PositionError.h"
#include "PrintContext.h"
#include "RenderListItem.h"
#include "RenderTreeAsText.h"
#include "ShadowRoot.h"
#include "ScriptController.h"
#include "ScriptValue.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#if ENABLE(SVG)
#include "SVGDocumentExtensions.h"
#include "SVGSMILElement.h"
#endif
#include "TextIterator.h"
#include "WebCoreTestSupport.h"
#include "WorkerThread.h"
#include <wtf/CurrentTime.h>

#include "qwebelement.h"
#include "qwebframe.h"
#include "qwebframe_p.h"
#include "qwebhistory.h"
#include "qwebhistory_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include "qwebscriptworld.h"

#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
#include "HTMLVideoElement.h"
#include "MediaPlayerPrivateQt.h"
#endif

using namespace WebCore;

QMap<int, QWebScriptWorld*> m_worldMap;

#if ENABLE(CLIENT_BASED_GEOLOCATION)
GeolocationClientMock* toGeolocationClientMock(GeolocationClient* client)
{
     ASSERT(QWebPagePrivate::drtRun);
     return static_cast<GeolocationClientMock*>(client);
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


DumpRenderTreeSupportQt::DumpRenderTreeSupportQt()
{
}

DumpRenderTreeSupportQt::~DumpRenderTreeSupportQt()
{
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

int DumpRenderTreeSupportQt::workerThreadCount()
{
#if ENABLE(WORKERS)
    return WebCore::WorkerThread::workerThreadCount();
#else
    return 0;
#endif
}

void DumpRenderTreeSupportQt::setDumpRenderTreeModeEnabled(bool b)
{
    QWebPagePrivate::drtRun = b;
}

void DumpRenderTreeSupportQt::setFrameFlatteningEnabled(QWebPage* page, bool enabled)
{
    QWebPagePrivate::core(page)->settings()->setFrameFlatteningEnabled(enabled);
}

void DumpRenderTreeSupportQt::webPageSetGroupName(QWebPage* page, const QString& groupName)
{
    page->handle()->page->setGroupName(groupName);
}

QString DumpRenderTreeSupportQt::webPageGroupName(QWebPage* page)
{
    return page->handle()->page->groupName();
}

void DumpRenderTreeSupportQt::webInspectorExecuteScript(QWebPage* page, long callId, const QString& script)
{
#if ENABLE(INSPECTOR)
    if (!page->handle()->page->inspectorController())
        return;
    page->handle()->page->inspectorController()->evaluateForTestInFrontend(callId, script);
#endif
}

void DumpRenderTreeSupportQt::webInspectorClose(QWebPage* page)
{
#if ENABLE(INSPECTOR)
    if (!page->handle()->page->inspectorController())
        return;
    page->handle()->page->inspectorController()->close();
#endif
}

void DumpRenderTreeSupportQt::webInspectorShow(QWebPage* page)
{
#if ENABLE(INSPECTOR)
    if (!page->handle()->page->inspectorController())
        return;
    page->handle()->page->inspectorController()->show();
#endif
}

void DumpRenderTreeSupportQt::setTimelineProfilingEnabled(QWebPage* page, bool enabled)
{
#if ENABLE(INSPECTOR)
    InspectorController* controller = page->handle()->page->inspectorController();
    if (!controller)
        return;
    if (enabled)
        controller->startTimelineProfiler();
    else
        controller->stopTimelineProfiler();
#endif
}

bool DumpRenderTreeSupportQt::hasDocumentElement(QWebFrame* frame)
{
    return QWebFramePrivate::core(frame)->document()->documentElement();
}

void DumpRenderTreeSupportQt::setAutofilled(const QWebElement& element, bool isAutofilled)
{
    WebCore::Element* webElement = element.m_element;
    if (!webElement)
        return;
    InputElement* inputElement = webElement->toInputElement();
    if (!inputElement)
        return;

    static_cast<HTMLInputElement*>(inputElement)->setAutofilled(isAutofilled);
}

void DumpRenderTreeSupportQt::setJavaScriptProfilingEnabled(QWebFrame* frame, bool enabled)
{
#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
    Frame* coreFrame = QWebFramePrivate::core(frame);
    InspectorController* controller = coreFrame->page()->inspectorController();
    if (!controller)
        return;
    if (enabled)
        controller->enableProfiler();
    else
        controller->disableProfiler();
#endif
}

// Pause a given CSS animation or transition on the target node at a specific time.
// If the animation or transition is already paused, it will update its pause time.
// This method is only intended to be used for testing the CSS animation and transition system.
bool DumpRenderTreeSupportQt::pauseAnimation(QWebFrame *frame, const QString &animationName, double time, const QString &elementId)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return false;

    AnimationController* controller = coreFrame->animation();
    if (!controller)
        return false;

    Document* doc = coreFrame->document();
    Q_ASSERT(doc);

    Node* coreNode = doc->getElementById(elementId);
    if (!coreNode || !coreNode->renderer())
        return false;

    return controller->pauseAnimationAtTime(coreNode->renderer(), animationName, time);
}

bool DumpRenderTreeSupportQt::pauseTransitionOfProperty(QWebFrame *frame, const QString &propertyName, double time, const QString &elementId)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return false;

    AnimationController* controller = coreFrame->animation();
    if (!controller)
        return false;

    Document* doc = coreFrame->document();
    Q_ASSERT(doc);

    Node* coreNode = doc->getElementById(elementId);
    if (!coreNode || !coreNode->renderer())
        return false;

    return controller->pauseTransitionAtTime(coreNode->renderer(), propertyName, time);
}

// Pause a given SVG animation on the target node at a specific time.
// This method is only intended to be used for testing the SVG animation system.
bool DumpRenderTreeSupportQt::pauseSVGAnimation(QWebFrame *frame, const QString &animationId, double time, const QString &elementId)
{
#if !ENABLE(SVG)
    return false;
#else
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return false;

    Document* doc = coreFrame->document();
    Q_ASSERT(doc);

    if (!doc->svgExtensions())
        return false;

    Node* coreNode = doc->getElementById(animationId);
    if (!coreNode || !SVGSMILElement::isSMILElement(coreNode))
        return false;

    return doc->accessSVGExtensions()->sampleAnimationAtTime(elementId, static_cast<SVGSMILElement*>(coreNode), time);
#endif
}

// Returns the total number of currently running animations (includes both CSS transitions and CSS animations).
int DumpRenderTreeSupportQt::numberOfActiveAnimations(QWebFrame *frame)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return false;

    AnimationController* controller = coreFrame->animation();
    if (!controller)
        return false;

    return controller->numberOfActiveAnimations();
}

void DumpRenderTreeSupportQt::suspendAnimations(QWebFrame *frame)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return;

    AnimationController* controller = coreFrame->animation();
    if (!controller)
        return;

    controller->suspendAnimations();
}

void DumpRenderTreeSupportQt::resumeAnimations(QWebFrame *frame)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return;

    AnimationController* controller = coreFrame->animation();
    if (!controller)
        return;

    controller->resumeAnimations();
}

void DumpRenderTreeSupportQt::clearFrameName(QWebFrame* frame)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    coreFrame->tree()->clearName();
}

int DumpRenderTreeSupportQt::javaScriptObjectsCount()
{
#if USE(JSC)
    return JSDOMWindowBase::commonJSGlobalData()->heap.globalObjectCount();
#elif USE(V8)
    // FIXME: Find a way to do this using V8.
    return 1;
#endif
}

void DumpRenderTreeSupportQt::garbageCollectorCollect()
{
#if USE(JSC)
    gcController().garbageCollectNow();
#elif USE(V8)
    v8::V8::LowMemoryNotification();
#endif
}

void DumpRenderTreeSupportQt::garbageCollectorCollectOnAlternateThread(bool waitUntilDone)
{
#if USE(JSC)
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
#elif USE(V8)
    // FIXME: Find a way to do this using V8.
    garbageCollectorCollect();
#endif
}

// Returns the value of counter in the element specified by \a id.
QString DumpRenderTreeSupportQt::counterValueForElementById(QWebFrame* frame, const QString& id)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (Document* document = coreFrame->document()) {
        if (Element* element = document->getElementById(id))
            return WebCore::counterValueForElement(element);
    }
    return QString();
}

int DumpRenderTreeSupportQt::pageNumberForElementById(QWebFrame* frame, const QString& id, float width, float height)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return -1;

    Element* element = coreFrame->document()->getElementById(AtomicString(id));
    if (!element)
        return -1;

    return PrintContext::pageNumberForElement(element, FloatSize(width, height));
}

int DumpRenderTreeSupportQt::numberOfPages(QWebFrame* frame, float width, float height)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return -1;

    return PrintContext::numberOfPages(coreFrame, FloatSize(width, height));
}

// Suspend active DOM objects in this frame.
void DumpRenderTreeSupportQt::suspendActiveDOMObjects(QWebFrame* frame)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (coreFrame->document())
        // FIXME: This function should be changed take a ReasonForSuspension parameter 
        // https://bugs.webkit.org/show_bug.cgi?id=45732
        coreFrame->document()->suspendActiveDOMObjects(ActiveDOMObject::JavaScriptDebuggerPaused);
}

// Resume active DOM objects in this frame.
void DumpRenderTreeSupportQt::resumeActiveDOMObjects(QWebFrame* frame)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (coreFrame->document())
        coreFrame->document()->resumeActiveDOMObjects();
}

void DumpRenderTreeSupportQt::whiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    SecurityOrigin::addOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void DumpRenderTreeSupportQt::removeWhiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    SecurityOrigin::removeOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void DumpRenderTreeSupportQt::resetOriginAccessWhiteLists()
{
    SecurityOrigin::resetOriginAccessWhitelists();
}

void DumpRenderTreeSupportQt::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const QString& scheme)
{
    SecurityOrigin::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme);
}

void DumpRenderTreeSupportQt::setCaretBrowsingEnabled(QWebPage* page, bool value)
{
    page->handle()->page->settings()->setCaretBrowsingEnabled(value);
}

void DumpRenderTreeSupportQt::setMediaType(QWebFrame* frame, const QString& type)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    WebCore::FrameView* view = coreFrame->view();
    view->setMediaType(type);
    coreFrame->document()->styleSelectorChanged(RecalcStyleImmediately);
    view->layout();
}

void DumpRenderTreeSupportQt::setSmartInsertDeleteEnabled(QWebPage* page, bool enabled)
{
    page->d->smartInsertDeleteEnabled = enabled;
}


void DumpRenderTreeSupportQt::setSelectTrailingWhitespaceEnabled(QWebPage* page, bool enabled)
{
    page->d->selectTrailingWhitespaceEnabled = enabled;
}


void DumpRenderTreeSupportQt::executeCoreCommandByName(QWebPage* page, const QString& name, const QString& value)
{
    page->handle()->page->focusController()->focusedOrMainFrame()->editor()->command(name).execute(value);
}

bool DumpRenderTreeSupportQt::isCommandEnabled(QWebPage* page, const QString& name)
{
    return page->handle()->page->focusController()->focusedOrMainFrame()->editor()->command(name).isEnabled();
}

bool DumpRenderTreeSupportQt::findString(QWebPage* page, const QString& string, const QStringList& optionArray)
{
    // 1. Parse the options from the array
    WebCore::FindOptions options = 0;
    const int optionCount = optionArray.size();
    for (int i = 0; i < optionCount; ++i) {
        const QString& option = optionArray.at(i);
        if (option == QLatin1String("CaseInsensitive"))
            options |= WebCore::CaseInsensitive;
        else if (option == QLatin1String("AtWordStarts"))
            options |= WebCore::AtWordStarts;
        else if (option == QLatin1String("TreatMedialCapitalAsWordStart"))
            options |= WebCore::TreatMedialCapitalAsWordStart;
        else if (option == QLatin1String("Backwards"))
            options |= WebCore::Backwards;
        else if (option == QLatin1String("WrapAround"))
            options |= WebCore::WrapAround;
        else if (option == QLatin1String("StartInSelection"))
            options |= WebCore::StartInSelection;
    }

    // 2. find the string
    WebCore::Frame* frame = page->handle()->page->focusController()->focusedOrMainFrame();
    return frame && frame->editor()->findString(string, options);
}

QString DumpRenderTreeSupportQt::markerTextForListItem(const QWebElement& listItem)
{
    return WebCore::markerTextForListItem(listItem.m_element);
}

static QString convertToPropertyName(const QString& name)
{
    QStringList parts = name.split(QLatin1Char('-'));
    QString camelCaseName;
    for (int j = 0; j < parts.count(); ++j) {
        QString part = parts.at(j);
        if (j)
            camelCaseName.append(part.replace(0, 1, part.left(1).toUpper()));
        else
            camelCaseName.append(part);
    }
    return camelCaseName;
}

QVariantMap DumpRenderTreeSupportQt::computedStyleIncludingVisitedInfo(const QWebElement& element)
{
    QVariantMap res;

    WebCore::Element* webElement = element.m_element;
    if (!webElement)
        return res;

    RefPtr<WebCore::CSSComputedStyleDeclaration> style = computedStyle(webElement, true);
    for (unsigned i = 0; i < style->length(); i++) {
        QString name = style->item(i);
        QString value = (static_cast<WebCore::CSSStyleDeclaration*>(style.get()))->getPropertyValue(name);
        res[convertToPropertyName(name)] = QVariant(value);
    }
    return res;
}

QVariantList DumpRenderTreeSupportQt::selectedRange(QWebPage* page)
{
    WebCore::Frame* frame = page->handle()->page->focusController()->focusedOrMainFrame();
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

QVariantList DumpRenderTreeSupportQt::firstRectForCharacterRange(QWebPage* page, int location, int length)
{
    WebCore::Frame* frame = page->handle()->page->focusController()->focusedOrMainFrame();
    QVariantList rect;

    if ((location + length < location) && (location + length))
        length = 0;

    Element* selectionRoot = frame->selection()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : frame->document()->documentElement();
    RefPtr<Range> range = TextIterator::rangeFromLocationAndLength(scope, location, length);

    if (!range)
        return QVariantList();

    QRect resultRect = frame->editor()->firstRectForRange(range.get());
    rect << resultRect.x() << resultRect.y() << resultRect.width() << resultRect.height();
    return rect;
}

bool DumpRenderTreeSupportQt::elementDoesAutoCompleteForElementWithId(QWebFrame* frame, const QString& elementId)
{
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return false;

    Document* doc = coreFrame->document();
    Q_ASSERT(doc);

    Node* coreNode = doc->getElementById(elementId);
    if (!coreNode || !coreNode->renderer())
        return false;

    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(coreNode);

    return inputElement->isTextField() && !inputElement->isPasswordField() && inputElement->autoComplete();
}

void DumpRenderTreeSupportQt::setEditingBehavior(QWebPage* page, const QString& editingBehavior)
{
    WebCore::EditingBehaviorType coreEditingBehavior;

    if (editingBehavior == QLatin1String("win"))
        coreEditingBehavior = EditingWindowsBehavior;
    else if (editingBehavior == QLatin1String("mac"))
        coreEditingBehavior = EditingMacBehavior;
    else if (editingBehavior == QLatin1String("unix"))
        coreEditingBehavior = EditingUnixBehavior;
    else {
        ASSERT_NOT_REACHED();
        return;
    }

    Page* corePage = QWebPagePrivate::core(page);
    if (!corePage)
        return;

    corePage->settings()->setEditingBehaviorType(coreEditingBehavior);
}

void DumpRenderTreeSupportQt::clearAllApplicationCaches()
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    WebCore::cacheStorage().empty();
    WebCore::cacheStorage().vacuumDatabaseFile();
#endif
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
#if ENABLE(NOTIFICATIONS)
    NotificationPresenterClientQt::dumpNotification = b;
#endif
}

QString DumpRenderTreeSupportQt::viewportAsText(QWebPage* page, int deviceDPI, const QSize& deviceSize, const QSize& availableSize)
{
    WebCore::ViewportArguments args = page->d->viewportArguments();

    WebCore::ViewportAttributes conf = WebCore::computeViewportAttributes(args,
        /* desktop-width */ 980,
        /* device-width  */ deviceSize.width(),
        /* device-height */ deviceSize.height(),
        /* device-dpi    */ deviceDPI,
        availableSize);

    QString res;
    res = res.sprintf("viewport size %dx%d scale %f with limits [%f, %f] and userScalable %f\n",
            conf.layoutSize.width(),
            conf.layoutSize.height(),
            conf.initialScale,
            conf.minimumScale,
            conf.maximumScale,
            conf.userScalable);

    return res;
}

void DumpRenderTreeSupportQt::activeMockDeviceOrientationClient(bool b)
{
#if ENABLE(DEVICE_ORIENTATION)
    DeviceOrientationClientMockQt::mockIsActive = b;
#endif
}

void DumpRenderTreeSupportQt::removeMockDeviceOrientation()
{
#if ENABLE(DEVICE_ORIENTATION)
    DeviceOrientationClientMockQt* client = DeviceOrientationClientMockQt::client();
    delete client;
#endif
}

void DumpRenderTreeSupportQt::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
#if ENABLE(DEVICE_ORIENTATION)
    DeviceOrientationClientMockQt::client()->setOrientation(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma);
#endif
}

void DumpRenderTreeSupportQt::resetGeolocationMock(QWebPage* page)
{
#if ENABLE(CLIENT_BASED_GEOLOCATION)
    Page* corePage = QWebPagePrivate::core(page);
    GeolocationClientMock* mockClient = toGeolocationClientMock(corePage->geolocationController()->client());
    mockClient->reset();
#endif
}

void DumpRenderTreeSupportQt::setMockGeolocationPermission(QWebPage* page, bool allowed)
{
#if ENABLE(CLIENT_BASED_GEOLOCATION)
    Page* corePage = QWebPagePrivate::core(page);
    GeolocationClientMock* mockClient = toGeolocationClientMock(corePage->geolocationController()->client());
    mockClient->setPermission(allowed);
#endif
}

void DumpRenderTreeSupportQt::setMockGeolocationPosition(QWebPage* page, double latitude, double longitude, double accuracy)
{
#if ENABLE(CLIENT_BASED_GEOLOCATION)
    Page* corePage = QWebPagePrivate::core(page);
    GeolocationClientMock* mockClient = toGeolocationClientMock(corePage->geolocationController()->client());
    mockClient->setPosition(GeolocationPosition::create(currentTime(), latitude, longitude, accuracy));
#endif
}

void DumpRenderTreeSupportQt::setMockGeolocationError(QWebPage* page, int errorCode, const QString& message)
{
#if ENABLE(CLIENT_BASED_GEOLOCATION)
    Page* corePage = QWebPagePrivate::core(page);

    GeolocationError::ErrorCode code = GeolocationError::PositionUnavailable;
    switch (errorCode) {
    case PositionError::PERMISSION_DENIED:
        code = GeolocationError::PermissionDenied;
        break;
    case PositionError::POSITION_UNAVAILABLE:
        code = GeolocationError::PositionUnavailable;
        break;
    }

    GeolocationClientMock* mockClient = static_cast<GeolocationClientMock*>(corePage->geolocationController()->client());
    mockClient->setError(GeolocationError::create(code, message));
#endif
}

int DumpRenderTreeSupportQt::numberOfPendingGeolocationPermissionRequests(QWebPage* page)
{
#if ENABLE(CLIENT_BASED_GEOLOCATION)
    Page* corePage = QWebPagePrivate::core(page);
    GeolocationClientMock* mockClient = toGeolocationClientMock(corePage->geolocationController()->client());
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

bool DumpRenderTreeSupportQt::shouldClose(QWebFrame* frame)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    return coreFrame->loader()->shouldClose();
}

void DumpRenderTreeSupportQt::clearScriptWorlds()
{
    m_worldMap.clear();
}

void DumpRenderTreeSupportQt::evaluateScriptInIsolatedWorld(QWebFrame* frame, int worldID, const QString& script)
{
    QWebScriptWorld* scriptWorld;
    if (!worldID) {
        scriptWorld = new QWebScriptWorld();
    } else if (!m_worldMap.contains(worldID)) {
        scriptWorld = new QWebScriptWorld();
        m_worldMap.insert(worldID, scriptWorld);
    } else
        scriptWorld = m_worldMap.value(worldID);

    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);

    ScriptController* proxy = coreFrame->script();

    if (!proxy)
        return;
#if USE(JSC)
    proxy->executeScriptInWorld(scriptWorld->world(), script, true);
#elif USE(V8)
    ScriptSourceCode source(script);
    Vector<ScriptSourceCode> sources;
    sources.append(source);
    proxy->evaluateInIsolatedWorld(0, sources, true);
#endif
}

bool DumpRenderTreeSupportQt::isPageBoxVisible(QWebFrame* frame, int pageIndex)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    return coreFrame->document()->isPageBoxVisible(pageIndex);
}

QString DumpRenderTreeSupportQt::pageSizeAndMarginsInPixels(QWebFrame* frame, int pageIndex, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    return PrintContext::pageSizeAndMarginsInPixels(coreFrame, pageIndex, width, height,
                                                    marginTop, marginRight, marginBottom, marginLeft);
}

QString DumpRenderTreeSupportQt::pageProperty(QWebFrame* frame, const QString& propertyName, int pageNumber)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    return PrintContext::pageProperty(coreFrame, propertyName.toUtf8().constData(), pageNumber);
}

void DumpRenderTreeSupportQt::addUserStyleSheet(QWebPage* page, const QString& sourceCode)
{
    page->handle()->page->group().addUserStyleSheetToWorld(mainThreadNormalWorld(), sourceCode, QUrl(), nullptr, nullptr, WebCore::InjectInAllFrames);
}

void DumpRenderTreeSupportQt::simulateDesktopNotificationClick(const QString& title)
{
#if ENABLE(NOTIFICATIONS)
    NotificationPresenterClientQt::notificationPresenter()->notificationClicked(title);
#endif
}

QString DumpRenderTreeSupportQt::plainText(const QVariant& range)
{
    QMap<QString, QVariant> map = range.toMap();
    QVariant startContainer  = map.value(QLatin1String("startContainer"));
    map = startContainer.toMap();

    return map.value(QLatin1String("innerText")).toString();
}

QVariantList DumpRenderTreeSupportQt::nodesFromRect(const QWebElement& document, int x, int y, unsigned top, unsigned right, unsigned bottom, unsigned left, bool ignoreClipping)
{
    QVariantList res;
    WebCore::Element* webElement = document.m_element;
    if (!webElement)
        return res;

    Document* doc = webElement->document();
    if (!doc)
        return res;
    RefPtr<NodeList> nodes = doc->nodesFromRect(x, y, top, right, bottom, left, ignoreClipping);
    for (unsigned i = 0; i < nodes->length(); i++) {
        // QWebElement will be null if the Node is not an HTML Element
        if (nodes->item(i)->isHTMLElement())
            res << QVariant::fromValue(QWebElement(nodes->item(i)));
        else
            res << QVariant::fromValue(QDRTNode(nodes->item(i)));
    }
    return res;
}

// API Candidate?
QString DumpRenderTreeSupportQt::responseMimeType(QWebFrame* frame)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    WebCore::DocumentLoader* docLoader = coreFrame->loader()->documentLoader();
    return docLoader->responseMIMEType();
}

void DumpRenderTreeSupportQt::clearOpener(QWebFrame* frame)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    coreFrame->loader()->setOpener(0);
}

void DumpRenderTreeSupportQt::addURLToRedirect(const QString& origin, const QString& destination)
{
    FrameLoaderClientQt::URLsToRedirect[origin] = destination;
}

static QStringList iterateContextMenu(QMenu* menu)
{
    if (!menu)
        return QStringList();

    QStringList items;
    QList<QAction *> actions = menu->actions();
    for (int i = 0; i < actions.count(); ++i) {
        if (actions.at(i)->isSeparator())
            items << QLatin1String("<separator>");
        else
            items << actions.at(i)->text();
        if (actions.at(i)->menu())
            items << iterateContextMenu(actions.at(i)->menu());
    }
    return items;
}

QStringList DumpRenderTreeSupportQt::contextMenu(QWebPage* page)
{
#ifndef QT_NO_CONTEXTMENU
    return iterateContextMenu(page->d->currentContextMenu);
#else
    return QStringList();
#endif
}

double DumpRenderTreeSupportQt::defaultMinimumTimerInterval()
{
    return Settings::defaultMinDOMTimerInterval();
}

void DumpRenderTreeSupportQt::setMinimumTimerInterval(QWebPage* page, double interval)
{
    Page* corePage = QWebPagePrivate::core(page);
    if (!corePage)
        return;

    corePage->settings()->setMinDOMTimerInterval(interval);
}

QUrl DumpRenderTreeSupportQt::mediaContentUrlByElementId(QWebFrame* frame, const QString& elementId)
{
    QUrl res;

#if ENABLE(VIDEO) && USE(QT_MULTIMEDIA)
    Frame* coreFrame = QWebFramePrivate::core(frame);
    if (!coreFrame)
        return res;

    Document* doc = coreFrame->document();
    if (!doc)
        return res;

    Node* coreNode = doc->getElementById(elementId);
    if (!coreNode)
        return res;

    HTMLVideoElement* videoElement = static_cast<HTMLVideoElement*>(coreNode);
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
void DumpRenderTreeSupportQt::setAlternateHtml(QWebFrame* frame, const QString& html, const QUrl& baseUrl, const QUrl& failingUrl)
{
    KURL kurl(baseUrl);
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
    WebCore::ResourceRequest request(kurl);
    const QByteArray utf8 = html.toUtf8();
    WTF::RefPtr<WebCore::SharedBuffer> data = WebCore::SharedBuffer::create(utf8.constData(), utf8.length());
    WebCore::SubstituteData substituteData(data, WTF::String("text/html"), WTF::String("utf-8"), failingUrl);
    coreFrame->loader()->load(request, substituteData, false);
}

QVariant DumpRenderTreeSupportQt::shadowRoot(const QWebElement& element)
{
    WebCore::Element* webElement = element.m_element;
    if (!webElement)
        return QVariant();

    ShadowRoot* webShadowRoot = webElement->shadowRoot();
    if (!webShadowRoot)
        return QVariant();

    return QVariant::fromValue(QDRTNode(webShadowRoot));
}

QVariant DumpRenderTreeSupportQt::ensureShadowRoot(const QWebElement& element)
{
    WebCore::Element* webElement = element.m_element;
    if (!webElement)
        return QVariant();

    return QVariant::fromValue(QDRTNode(webElement->ensureShadowRoot()));
}

void DumpRenderTreeSupportQt::removeShadowRoot(const QWebElement& element)
{
    WebCore::Element* webElement = element.m_element;
    if (!webElement)
        return;
    webElement->removeShadowRoot();
}

void DumpRenderTreeSupportQt::injectInternalsObject(QWebFrame* frame)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
#if USE(JSC)
    JSC::JSLock lock(JSC::SilenceAssertionsOnly);

    JSDOMWindow* window = toJSDOMWindow(coreFrame, mainThreadNormalWorld());
    Q_ASSERT(window);

    JSC::ExecState* exec = window->globalExec();
    Q_ASSERT(exec);

    JSContextRef context = toRef(exec);
    WebCoreTestSupport::injectInternalsObject(context);
#elif USE(V8)
    WebCoreTestSupport::injectInternalsObject(V8Proxy::mainWorldContext(coreFrame));
#endif
}

void DumpRenderTreeSupportQt::resetInternalsObject(QWebFrame* frame)
{
    WebCore::Frame* coreFrame = QWebFramePrivate::core(frame);
#if USE(JSC)
    JSC::JSLock lock(JSC::SilenceAssertionsOnly);

    JSDOMWindow* window = toJSDOMWindow(coreFrame, mainThreadNormalWorld());
    Q_ASSERT(window);

    JSC::ExecState* exec = window->globalExec();
    Q_ASSERT(exec);

    JSContextRef context = toRef(exec);
    WebCoreTestSupport::resetInternalsObject(context);
#elif USE(V8)
    WebCoreTestSupport::resetInternalsObject(V8Proxy::mainWorldContext(coreFrame));
#endif
}

// Provide a backward compatibility with previously exported private symbols as of QtWebKit 4.6 release

void QWEBKIT_EXPORT qt_resumeActiveDOMObjects(QWebFrame* frame)
{
    DumpRenderTreeSupportQt::resumeActiveDOMObjects(frame);
}

void QWEBKIT_EXPORT qt_suspendActiveDOMObjects(QWebFrame* frame)
{
    DumpRenderTreeSupportQt::suspendActiveDOMObjects(frame);
}

void QWEBKIT_EXPORT qt_drt_clearFrameName(QWebFrame* frame)
{
    DumpRenderTreeSupportQt::clearFrameName(frame);
}

void QWEBKIT_EXPORT qt_drt_garbageCollector_collect()
{
    DumpRenderTreeSupportQt::garbageCollectorCollect();
}

void QWEBKIT_EXPORT qt_drt_garbageCollector_collectOnAlternateThread(bool waitUntilDone)
{
    DumpRenderTreeSupportQt::garbageCollectorCollectOnAlternateThread(waitUntilDone);
}

int QWEBKIT_EXPORT qt_drt_javaScriptObjectsCount()
{
    return DumpRenderTreeSupportQt::javaScriptObjectsCount();
}

int QWEBKIT_EXPORT qt_drt_numberOfActiveAnimations(QWebFrame* frame)
{
    return DumpRenderTreeSupportQt::numberOfActiveAnimations(frame);
}

void QWEBKIT_EXPORT qt_drt_overwritePluginDirectories()
{
    DumpRenderTreeSupportQt::overwritePluginDirectories();
}

bool QWEBKIT_EXPORT qt_drt_pauseAnimation(QWebFrame* frame, const QString& animationName, double time, const QString& elementId)
{
    return DumpRenderTreeSupportQt::pauseAnimation(frame, animationName, time, elementId);
}

bool QWEBKIT_EXPORT qt_drt_pauseTransitionOfProperty(QWebFrame* frame, const QString& propertyName, double time, const QString &elementId)
{
    return DumpRenderTreeSupportQt::pauseTransitionOfProperty(frame, propertyName, time, elementId);
}

void QWEBKIT_EXPORT qt_drt_resetOriginAccessWhiteLists()
{
    DumpRenderTreeSupportQt::resetOriginAccessWhiteLists();
}

void QWEBKIT_EXPORT qt_drt_run(bool b)
{
    DumpRenderTreeSupportQt::setDumpRenderTreeModeEnabled(b);
}

void QWEBKIT_EXPORT qt_drt_setJavaScriptProfilingEnabled(QWebFrame* frame, bool enabled)
{
    DumpRenderTreeSupportQt::setJavaScriptProfilingEnabled(frame, enabled);
}

void QWEBKIT_EXPORT qt_drt_whiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains)
{
    DumpRenderTreeSupportQt::whiteListAccessFromOrigin(sourceOrigin, destinationProtocol, destinationHost, allowDestinationSubdomains);
}

QString QWEBKIT_EXPORT qt_webpage_groupName(QWebPage* page)
{
    return DumpRenderTreeSupportQt::webPageGroupName(page);
}

void QWEBKIT_EXPORT qt_webpage_setGroupName(QWebPage* page, const QString& groupName)
{
    DumpRenderTreeSupportQt::webPageSetGroupName(page, groupName);
}

void QWEBKIT_EXPORT qt_dump_frame_loader(bool b)
{
    DumpRenderTreeSupportQt::dumpFrameLoader(b);
}

void QWEBKIT_EXPORT qt_dump_resource_load_callbacks(bool b)
{
    DumpRenderTreeSupportQt::dumpResourceLoadCallbacks(b);
}

void QWEBKIT_EXPORT qt_dump_editing_callbacks(bool b)
{
    DumpRenderTreeSupportQt::dumpEditingCallbacks(b);
}

void QWEBKIT_EXPORT qt_dump_set_accepts_editing(bool b)
{
    DumpRenderTreeSupportQt::dumpSetAcceptsEditing(b);
}

