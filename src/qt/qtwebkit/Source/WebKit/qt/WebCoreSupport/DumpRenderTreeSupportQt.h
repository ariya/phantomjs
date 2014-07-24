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

#ifndef DumpRenderTreeSupportQt_h
#define DumpRenderTreeSupportQt_h

#include "qwebkitglobal.h"
#include <QNetworkCookieJar>
#include <QVariant>
#include <QVector>

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSValue* JSObjectRef;

namespace WebCore {
class Text;
class Node;
}

namespace JSC {
namespace Bindings {
class QtDRTNodeRuntime;
}
}

class QWebElement;
class QWebFrame;
class QWebFrameAdapter;
class QWebPageAdapter;
class QWebHistoryItem;
class QWebScriptWorld;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

extern QMap<int, QWebScriptWorld*> m_worldMap;

// Used to pass WebCore::Node's to layout tests using TestRunner
class QWEBKIT_EXPORT QDRTNode {
public:
    QDRTNode();
    QDRTNode(const QDRTNode&);
    QDRTNode &operator=(const QDRTNode&);
    ~QDRTNode();

private:
    explicit QDRTNode(WebCore::Node*);

    friend class DumpRenderTreeSupportQt;

    friend class QtDRTNodeRuntime;

    WebCore::Node* m_node;
};

Q_DECLARE_METATYPE(QDRTNode)

class QtDRTNodeRuntime {
public:
    static QDRTNode create(WebCore::Node*);
    static WebCore::Node* get(const QDRTNode&);
    static void initialize();
};

class QWEBKIT_EXPORT DumpRenderTreeSupportQt {

public:

    DumpRenderTreeSupportQt();
    ~DumpRenderTreeSupportQt();

    static void initialize();

    static void executeCoreCommandByName(QWebPageAdapter*, const QString& name, const QString& value);
    static bool isCommandEnabled(QWebPageAdapter*, const QString& name);
    static QVariantList selectedRange(QWebPageAdapter*);
    static QVariantList firstRectForCharacterRange(QWebPageAdapter*, int location, int length);
    static void confirmComposition(QWebPageAdapter*, const char* text);

    static void setDomainRelaxationForbiddenForURLScheme(bool forbidden, const QString& scheme);
    static void setFrameFlatteningEnabled(QWebPageAdapter*, bool);
    static void setCaretBrowsingEnabled(QWebPageAdapter*, bool value);
    static void setAuthorAndUserStylesEnabled(QWebPageAdapter*, bool);
    static void setDumpRenderTreeModeEnabled(bool);

    static void garbageCollectorCollect();
    static void garbageCollectorCollectOnAlternateThread(bool waitUntilDone);
    static void setValueForUser(const QWebElement&, const QString& value);
    static int javaScriptObjectsCount();
    static void clearScriptWorlds();
    static void evaluateScriptInIsolatedWorld(QWebFrameAdapter*, int worldID, const QString& script);

    static void webInspectorExecuteScript(QWebPageAdapter*, long callId, const QString& script);
    static void webInspectorShow(QWebPageAdapter*);
    static void webInspectorClose(QWebPageAdapter*);

    static QString webPageGroupName(QWebPageAdapter*);
    static void webPageSetGroupName(QWebPageAdapter*, const QString& groupName);
    static void clearFrameName(QWebFrameAdapter*);
    static void overwritePluginDirectories();
    static bool hasDocumentElement(QWebFrameAdapter*);
    static void setWindowsBehaviorAsEditingBehavior(QWebPageAdapter*);

    static void clearAllApplicationCaches();

    static void whiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains);
    static void removeWhiteListAccessFromOrigin(const QString& sourceOrigin, const QString& destinationProtocol, const QString& destinationHost, bool allowDestinationSubdomains);
    static void resetOriginAccessWhiteLists();

    static void setMockDeviceOrientation(QWebPageAdapter*, bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma);

    static void resetGeolocationMock(QWebPageAdapter*);
    static void setMockGeolocationPermission(QWebPageAdapter*, bool allowed);
    static void setMockGeolocationPosition(QWebPageAdapter*, double latitude, double longitude, double accuracy);
    static void setMockGeolocationPositionUnavailableError(QWebPageAdapter*, const QString& message);
    static int numberOfPendingGeolocationPermissionRequests(QWebPageAdapter*);

    static void dumpFrameLoader(bool);
    static void dumpProgressFinishedCallback(bool);
    static void dumpUserGestureInFrameLoader(bool);
    static void dumpResourceLoadCallbacks(bool);
    static void dumpResourceResponseMIMETypes(bool);
    static void dumpResourceLoadCallbacksPath(const QString& path);
    static void dumpWillCacheResponseCallbacks(bool);
    static void setWillSendRequestReturnsNullOnRedirect(bool);
    static void setWillSendRequestReturnsNull(bool);
    static void setWillSendRequestClearHeaders(const QStringList&);
    static void dumpHistoryCallbacks(bool);
    static void dumpVisitedLinksCallbacks(bool);

    static void setDeferMainResourceDataLoad(bool);

    static void dumpEditingCallbacks(bool);
    static void dumpSetAcceptsEditing(bool);

    static void dumpNotification(bool);
    static QString viewportAsText(QWebPageAdapter*, int deviceDPI, const QSize& deviceSize, const QSize& availableSize);

    static QMap<QString, QWebHistoryItem> getChildHistoryItems(const QWebHistoryItem&);
    static bool isTargetItem(const QWebHistoryItem&);
    static QString historyItemTarget(const QWebHistoryItem&);

    static bool shouldClose(QWebFrameAdapter*);

    static void setCustomPolicyDelegate(bool enabled, bool permissive);

    static void addUserStyleSheet(QWebPageAdapter*, const QString& sourceCode);
    static void removeUserStyleSheets(QWebPageAdapter*);
    static void simulateDesktopNotificationClick(const QString& title);

    static void scalePageBy(QWebFrameAdapter*, float scale, const QPoint& origin);

    static QString responseMimeType(QWebFrameAdapter*);
    static void clearOpener(QWebFrameAdapter*);
    static void addURLToRedirect(const QString& origin, const QString& destination);
    static QStringList contextMenu(QWebPageAdapter*);

    static QUrl mediaContentUrlByElementId(QWebFrameAdapter*, const QString& elementId);
    static void setAlternateHtml(QWebFrameAdapter*, const QString& html, const QUrl& baseUrl, const QUrl& failingUrl);

    static void injectInternalsObject(QWebFrameAdapter*);
    static void injectInternalsObject(JSContextRef);
    static void resetInternalsObject(QWebFrameAdapter*);
    static void resetInternalsObject(JSContextRef);

    static void setInteractiveFormValidationEnabled(QWebPageAdapter*, bool);

    static void setDefersLoading(QWebPageAdapter*, bool flag);
    static void goBack(QWebPageAdapter*);

    static bool thirdPartyCookiePolicyAllows(QWebPageAdapter*, const QUrl&, const QUrl& firstPartyUrl);

    static void enableMockScrollbars();

    static QImage paintPagesWithBoundaries(QWebFrameAdapter*);

    static void setTrackRepaintRects(QWebFrameAdapter*, bool enable);
    static bool trackRepaintRects(QWebFrameAdapter*);
    static void getTrackedRepaintRects(QWebFrameAdapter*, QVector<QRect>& result);

    static void setSeamlessIFramesEnabled(bool);
    static void setShouldUseFontSmoothing(bool);

    static QString frameRenderTreeDump(QWebFrameAdapter*);
    static void clearNotificationPermissions();

    static void disableDefaultTypesettingFeatures();
    static void resetPageVisibility(QWebPageAdapter*);

    static void getJSWindowObject(QWebFrameAdapter*, JSContextRef*, JSObjectRef*);
};

#endif
