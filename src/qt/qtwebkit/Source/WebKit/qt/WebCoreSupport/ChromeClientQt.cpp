/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * All rights reserved.
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
#include "ChromeClientQt.h"

#include "ApplicationCacheStorage.h"
#include "ColorChooser.h"
#include "ColorChooserClient.h"
#include "DatabaseManager.h"
#include "Document.h"
#include "FileChooser.h"
#include "FileIconLoader.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClientQt.h"
#include "FrameView.h"
#include "Geolocation.h"
#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayer.h"
#endif
#include "HTMLFormElement.h"
#include "HitTestResult.h"
#include "Icon.h"
#include "NavigationAction.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PopupMenuQt.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "QWebPageClient.h"
#include "ScrollbarTheme.h"
#include "SearchPopupMenuQt.h"
#include "SecurityOrigin.h"
#include "TextureMapperLayerClientQt.h"
#include "TiledBackingStore.h"
#include "ViewportArguments.h"
#include "WindowFeatures.h"
#include "qwebkitplatformplugin.h"
#include "qwebsecurityorigin.h"
#include "qwebsecurityorigin_p.h"
#include "qwebsettings.h"

#include <qabstractanimation.h>
#include <qdebug.h>
#include <qeventloop.h>
#include <qwindow.h>
#include <wtf/CurrentTime.h>
#include <wtf/OwnPtr.h>

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
#include "FullScreenVideoQt.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#if USE(QT_MULTIMEDIA)
#include "MediaPlayerPrivateQt.h"
#endif
#endif

namespace WebCore {

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
class RefreshAnimation : public QAbstractAnimation {
public:
    RefreshAnimation(ChromeClientQt* chromeClient)
        : QAbstractAnimation()
        , m_chromeClient(chromeClient)
        , m_animationScheduled(false)
    { }

    virtual int duration() const { return -1; }

    void scheduleAnimation()
    {
        m_animationScheduled = true;
        if (state() != QAbstractAnimation::Running)
            QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
    }

protected:
    virtual void updateCurrentTime(int currentTime)
    {
        UNUSED_PARAM(currentTime);
        if (m_animationScheduled) {
            m_animationScheduled = false;
            m_chromeClient->serviceScriptedAnimations();
        } else
            stop();
    }
private:
    ChromeClientQt* m_chromeClient;
    bool m_animationScheduled;
};
#endif

bool ChromeClientQt::dumpVisitedLinksCallbacks = false;

ChromeClientQt::ChromeClientQt(QWebPageAdapter* webPageAdapter)
    : m_webPage(webPageAdapter)
    , m_eventLoop(0)
#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    , m_fullScreenVideo(0)
#endif
{
    toolBarsVisible = statusBarVisible = menuBarVisible = true;
}

ChromeClientQt::~ChromeClientQt()
{
    if (m_eventLoop)
        m_eventLoop->exit();

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    delete m_fullScreenVideo;
#endif
}

void ChromeClientQt::setWindowRect(const FloatRect& rect)
{
    if (!m_webPage)
        return;
    m_webPage->setWindowRect(QRect(qRound(rect.x()), qRound(rect.y()), qRound(rect.width()), qRound(rect.height())));
}

/*!
    windowRect represents the rect of the Window, including all interface elements
    like toolbars/scrollbars etc. It is used by the viewport meta tag as well as
    by the DOM Window object: outerHeight(), outerWidth(), screenX(), screenY().
*/
FloatRect ChromeClientQt::windowRect()
{
    if (!platformPageClient())
        return FloatRect();
    return platformPageClient()->windowRect();
}

bool ChromeClientQt::allowsAcceleratedCompositing() const
{
    if (!platformPageClient())
        return false;
#if USE(ACCELERATED_COMPOSITING)
    return true;
#else
    return false;
#endif
}

FloatRect ChromeClientQt::pageRect()
{
    if (!m_webPage)
        return FloatRect();
    return FloatRect(QRectF(QPointF(0, 0), m_webPage->viewportSize()));
}

void ChromeClientQt::focus()
{
    if (!m_webPage)
        return;
    m_webPage->setFocus();
}


void ChromeClientQt::unfocus()
{
    if (!m_webPage)
        return;
    m_webPage->unfocus();
}

bool ChromeClientQt::canTakeFocus(FocusDirection)
{
    // This is called when cycling through links/focusable objects and we
    // reach the last focusable object. Then we want to claim that we can
    // take the focus to avoid wrapping.
    return true;
}

void ChromeClientQt::takeFocus(FocusDirection)
{
    // don't do anything. This is only called when cycling to links/focusable objects,
    // which in turn is called from focusNextPrevChild. We let focusNextPrevChild
    // call QWidget::focusNextPrevChild accordingly, so there is no need to do anything
    // here.
}


void ChromeClientQt::focusedNodeChanged(Node*)
{
}

void ChromeClientQt::focusedFrameChanged(Frame*)
{
}

Page* ChromeClientQt::createWindow(Frame*, const FrameLoadRequest& request, const WindowFeatures& features, const NavigationAction&)
{
    QWebPageAdapter* newPage = m_webPage->createWindow(features.dialog);
    if (!newPage)
        return 0;

    return newPage->page;
}

void ChromeClientQt::show()
{
    if (!m_webPage)
        return;
    m_webPage->show();
}


bool ChromeClientQt::canRunModal()
{
    return true;
}


void ChromeClientQt::runModal()
{
    m_eventLoop = new QEventLoop();
    QEventLoop* eventLoop = m_eventLoop;
    m_eventLoop->exec();
    delete eventLoop;
}


void ChromeClientQt::setToolbarsVisible(bool visible)
{
    toolBarsVisible = visible;
    QMetaObject::invokeMethod(m_webPage->handle(), "toolBarVisibilityChangeRequested", Q_ARG(bool, visible));
}


bool ChromeClientQt::toolbarsVisible()
{
    return toolBarsVisible;
}


void ChromeClientQt::setStatusbarVisible(bool visible)
{
    QMetaObject::invokeMethod(m_webPage->handle(), "statusBarVisibilityChangeRequested", Q_ARG(bool, visible));
    statusBarVisible = visible;
}


bool ChromeClientQt::statusbarVisible()
{
    return statusBarVisible;
}


void ChromeClientQt::setScrollbarsVisible(bool)
{
    notImplemented();
}


bool ChromeClientQt::scrollbarsVisible()
{
    notImplemented();
    return true;
}


void ChromeClientQt::setMenubarVisible(bool visible)
{
    menuBarVisible = visible;
    QMetaObject::invokeMethod(m_webPage->handle(), "menuBarVisibilityChangeRequested", Q_ARG(bool, visible));
}

bool ChromeClientQt::menubarVisible()
{
    return menuBarVisible;
}

void ChromeClientQt::setResizable(bool)
{
    notImplemented();
}

void ChromeClientQt::addMessageToConsole(MessageSource, MessageLevel level, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID, const String& stack)
{
    QString x = message;
    QString y = sourceID;
    UNUSED_PARAM(columnNumber);
    if (level == ErrorMessageLevel) {
        m_webPage->javaScriptError(x, lineNumber, y, stack);
    } else {
        m_webPage->javaScriptConsoleMessage(x, lineNumber, y);
    }
}

void ChromeClientQt::chromeDestroyed()
{
    delete this;
}

bool ChromeClientQt::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClientQt::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    return runJavaScriptConfirm(frame, message);
}

void ChromeClientQt::closeWindowSoon()
{
    m_webPage->page->setGroupName(String());
    m_webPage->page->mainFrame()->loader()->stopAllLoaders();
    QMetaObject::invokeMethod(m_webPage->handle(), "windowCloseRequested");
}

void ChromeClientQt::runJavaScriptAlert(Frame* f, const String& msg)
{
    m_webPage->javaScriptAlert(QWebFrameAdapter::kit(f), msg);
}

bool ChromeClientQt::runJavaScriptConfirm(Frame* f, const String& msg)
{
    return m_webPage->javaScriptConfirm(QWebFrameAdapter::kit(f), msg);
}

bool ChromeClientQt::runJavaScriptPrompt(Frame* f, const String& message, const String& defaultValue, String& result)
{
    QString x = result;
    QWebFrameAdapter* webFrame = QWebFrameAdapter::kit(f);
    bool rc = m_webPage->javaScriptPrompt(webFrame, message, defaultValue, &x);

    // Fix up a quirk in the QInputDialog class. If no input happened the string should be empty
    // but it is null. See https://bugs.webkit.org/show_bug.cgi?id=30914.
    if (rc && x.isNull())
        result = String("");
    else
        result = x;

    return rc;
}

void ChromeClientQt::setStatusbarText(const String& msg)
{
    QString x = msg;
    QMetaObject::invokeMethod(m_webPage->handle(), "statusBarMessage", Q_ARG(QString, x));
}

bool ChromeClientQt::shouldInterruptJavaScript()
{
    return m_webPage->shouldInterruptJavaScript();
}

KeyboardUIMode ChromeClientQt::keyboardUIMode()
{
    return m_webPage->settings->testAttribute(QWebSettings::LinksIncludedInFocusChain)
        ? KeyboardAccessTabsToLinks : KeyboardAccessDefault;
}

IntRect ChromeClientQt::windowResizerRect() const
{
#if defined(Q_WS_MAC)
    if (!m_webPage)
        return IntRect();

    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return IntRect();

    QWindow* topLevelWidget = pageClient->ownerWindow();
    if (!topLevelWidget)
        return IntRect();

    QRect topLevelGeometry(topLevelWidget->geometry());

    // There's no API in Qt to query for the size of the resizer, so we assume
    // it has the same width and height as the scrollbar thickness.
    int scollbarThickness = ScrollbarTheme::theme()->scrollbarThickness();

    // There's no API in Qt to query for the position of the resizer. Sometimes
    // it's drawn by the system, and sometimes it's a QSizeGrip. For RTL locales
    // it might even be on the lower left side of the window, but in WebKit we
    // always draw scrollbars on the right hand side, so we assume this to be the
    // location when computing the resize rect to reserve for WebKit.
    QPoint resizeCornerTopLeft = QPoint(topLevelGeometry.width(), topLevelGeometry.height())
        - QPoint(scollbarThickness, scollbarThickness)
        - m_webPage->viewRectRelativeToWindow().topLeft();

    QRect resizeCornerRect = QRect(resizeCornerTopLeft, QSize(scollbarThickness, scollbarThickness));
    return resizeCornerRect.intersected(pageClient->geometryRelativeToOwnerWidget());

#else
    return IntRect();
#endif
}

void ChromeClientQt::invalidateRootView(const IntRect& windowRect, bool)
{
#if USE(TILED_BACKING_STORE)
    if (platformPageClient()) {
        WebCore::TiledBackingStore* backingStore = m_webPage->mainFrameAdapter()->frame->tiledBackingStore();
        if (!backingStore)
            return;
        backingStore->invalidate(windowRect);
    }
#else
    Q_UNUSED(windowRect);
#endif
}

void ChromeClientQt::invalidateContentsAndRootView(const IntRect& windowRect, bool)
{
    // No double buffer, so only update the QWidget if content changed.
    if (platformPageClient()) {
        QRect rect(windowRect);
        rect = rect.intersected(QRect(QPoint(0, 0), m_webPage->viewportSize()));
        if (!rect.isEmpty())
            platformPageClient()->update(rect);
    }
    QMetaObject::invokeMethod(m_webPage->handle(), "repaintRequested", Qt::QueuedConnection, Q_ARG(QRect, windowRect));

    // FIXME: There is no "immediate" support for window painting. This should be done always whenever the flag
    // is set.
}

void ChromeClientQt::invalidateContentsForSlowScroll(const IntRect& windowRect, bool immediate)
{
    invalidateContentsAndRootView(windowRect, immediate);
}

void ChromeClientQt::scroll(const IntSize& delta, const IntRect& scrollViewRect, const IntRect&)
{
    if (platformPageClient())
        platformPageClient()->scroll(delta.width(), delta.height(), scrollViewRect);
    QMetaObject::invokeMethod(m_webPage->handle(), "scrollRequested", Q_ARG(int, delta.width()), Q_ARG(int, delta.height()), Q_ARG(QRect, scrollViewRect));
}

#if USE(TILED_BACKING_STORE)
void ChromeClientQt::delegatedScrollRequested(const IntPoint& point)
{

    const QPoint ofs = m_webPage->mainFrameAdapter()->scrollPosition();
    IntSize currentPosition(ofs.x(), ofs.y());
    int x = point.x() - currentPosition.width();
    int y = point.y() - currentPosition.height();
    const QRect rect(QPoint(0, 0), m_webPage->viewportSize());
    QMetaObject::invokeMethod(m_webPage->handle(), "scrollRequested", Q_ARG(int, x), Q_ARG(int, y), Q_ARG(QRect, rect));
}
#endif

IntRect ChromeClientQt::rootViewToScreen(const IntRect& rect) const
{
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return rect;

    QWindow* ownerWindow = pageClient->ownerWindow();
    if (!ownerWindow)
        return rect;

    QRect screenRect(rect);
    screenRect.translate(ownerWindow->mapToGlobal(m_webPage->viewRectRelativeToWindow().topLeft()));

    return screenRect;
}

IntPoint ChromeClientQt::screenToRootView(const IntPoint& point) const
{
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return point;

    QWindow* ownerWindow = pageClient->ownerWindow();
    if (!ownerWindow)
        return point;

    return ownerWindow->mapFromGlobal(point) - m_webPage->viewRectRelativeToWindow().topLeft();
}

PlatformPageClient ChromeClientQt::platformPageClient() const
{
    return m_webPage->client.data();
}

void ChromeClientQt::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    if (frame->loader()->networkingContext())
        QWebFrameAdapter::kit(frame)->contentsSizeDidChange(size);
}

void ChromeClientQt::mouseDidMoveOverElement(const HitTestResult& result, unsigned)
{
    TextDirection dir;
    if (result.absoluteLinkURL() != lastHoverURL
        || result.title(dir) != lastHoverTitle
        || result.textContent() != lastHoverContent) {
        lastHoverURL = result.absoluteLinkURL();
        lastHoverTitle = result.title(dir);
        lastHoverContent = result.textContent();
        QMetaObject::invokeMethod(m_webPage->handle(), "linkHovered", Q_ARG(QString, lastHoverURL.string()),
            Q_ARG(QString, lastHoverTitle), Q_ARG(QString, lastHoverContent));
    }
}

void ChromeClientQt::setToolTip(const String &tip, TextDirection)
{
    m_webPage->setToolTip(tip);
}

void ChromeClientQt::print(Frame* frame)
{
    emit m_webPage->printRequested(QWebFrameAdapter::kit(frame));
}

#if ENABLE(SQL_DATABASE)
void ChromeClientQt::exceededDatabaseQuota(Frame* frame, const String& databaseName, DatabaseDetails)
{
    quint64 quota = QWebSettings::offlineStorageDefaultQuota();

    if (!DatabaseManager::manager().hasEntryForOrigin(frame->document()->securityOrigin()))
        DatabaseManager::manager().setQuota(frame->document()->securityOrigin(), quota);

    m_webPage->databaseQuotaExceeded(QWebFrameAdapter::kit(frame), databaseName);
}
#endif

void ChromeClientQt::reachedMaxAppCacheSize(int64_t)
{
    // FIXME: Free some space.
    notImplemented();
}

void ChromeClientQt::reachedApplicationCacheOriginQuota(SecurityOrigin* origin, int64_t totalSpaceNeeded)
{
    int64_t quota;
    quint64 defaultOriginQuota = WebCore::cacheStorage().defaultOriginQuota();

    QWebSecurityOriginPrivate* priv = new QWebSecurityOriginPrivate(origin);
    QWebSecurityOrigin* securityOrigin = new QWebSecurityOrigin(priv);

    if (!WebCore::cacheStorage().calculateQuotaForOrigin(origin, quota))
        WebCore::cacheStorage().storeUpdatedQuotaForOrigin(origin, defaultOriginQuota);

    m_webPage->applicationCacheQuotaExceeded(securityOrigin, defaultOriginQuota, static_cast<quint64>(totalSpaceNeeded));
}

#if ENABLE(INPUT_TYPE_COLOR)
PassOwnPtr<ColorChooser> ChromeClientQt::createColorChooser(ColorChooserClient* client, const Color& color)
{
    const QColor selectedColor = m_webPage->colorSelectionRequested(QColor(color));
    client->didChooseColor(selectedColor);
    client->didEndChooser();
    return nullptr;
}
#endif

void ChromeClientQt::runOpenPanel(Frame* frame, PassRefPtr<FileChooser> prpFileChooser)
{
    RefPtr<FileChooser> fileChooser = prpFileChooser;

    QStringList suggestedFileNames;
    for (unsigned i = 0; i < fileChooser->settings().selectedFiles.size(); ++i)
        suggestedFileNames += fileChooser->settings().selectedFiles[i];

    const bool allowMultiple = fileChooser->settings().allowsMultipleFiles;

    QStringList result = m_webPage->chooseFiles(QWebFrameAdapter::kit(frame), allowMultiple, suggestedFileNames);
    if (!result.isEmpty()) {
        if (allowMultiple) {
            Vector<String> names;
            for (int i = 0; i < result.count(); ++i)
                names.append(result.at(i));
            fileChooser->chooseFiles(names);
        } else
            fileChooser->chooseFile(result.first());
    }
}

void ChromeClientQt::loadIconForFiles(const Vector<String>& filenames, FileIconLoader* loader)
{
    loader->notifyFinished(Icon::createIconForFiles(filenames));
}

void ChromeClientQt::setCursor(const Cursor& cursor)
{
#ifndef QT_NO_CURSOR
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return;
    pageClient->setCursor(*cursor.platformCursor());
#else
    UNUSED_PARAM(cursor);
#endif
}

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
void ChromeClientQt::scheduleAnimation()
{
    if (!m_refreshAnimation)
        m_refreshAnimation = adoptPtr(new RefreshAnimation(this));
    m_refreshAnimation->scheduleAnimation();
}

void ChromeClientQt::serviceScriptedAnimations()
{
    m_webPage->mainFrameAdapter()->frame->view()->serviceScriptedAnimations(currentTime());
}
#endif

#if USE(ACCELERATED_COMPOSITING)
void ChromeClientQt::attachRootGraphicsLayer(Frame* frame, GraphicsLayer* graphicsLayer)
{
    if (!m_textureMapperLayerClient)
        m_textureMapperLayerClient = adoptPtr(new TextureMapperLayerClientQt(m_webPage->mainFrameAdapter()));
    m_textureMapperLayerClient->setRootGraphicsLayer(graphicsLayer);
}

void ChromeClientQt::setNeedsOneShotDrawingSynchronization()
{
    // we want the layers to synchronize next time we update the screen anyway
    if (m_textureMapperLayerClient)
        m_textureMapperLayerClient->markForSync(false);
}

void ChromeClientQt::scheduleCompositingLayerFlush()
{
    // we want the layers to synchronize ASAP
    if (m_textureMapperLayerClient)
        m_textureMapperLayerClient->markForSync(true);
}

ChromeClient::CompositingTriggerFlags ChromeClientQt::allowedCompositingTriggers() const
{
    if (allowsAcceleratedCompositing())
        return ThreeDTransformTrigger | CanvasTrigger | AnimationTrigger | AnimatedOpacityTrigger;

    return 0;
}

#endif

#if USE(TILED_BACKING_STORE)
IntRect ChromeClientQt::visibleRectForTiledBackingStore() const
{
    if (!platformPageClient() || !m_webPage)
        return IntRect();

    if (!platformPageClient()->viewResizesToContentsEnabled()) {
        const QPoint ofs = m_webPage->mainFrameAdapter()->scrollPosition();
        IntSize offset(ofs.x(), ofs.y());
        return QRect(QPoint(offset.width(), offset.height()), m_webPage->mainFrameAdapter()->frameRect().size());
    }

    return enclosingIntRect(FloatRect(platformPageClient()->graphicsItemVisibleRect()));
}
#endif

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
FullScreenVideoQt* ChromeClientQt::fullScreenVideo()
{
    if (!m_fullScreenVideo)
        m_fullScreenVideo = new FullScreenVideoQt(this);
    return m_fullScreenVideo;
}

bool ChromeClientQt::supportsFullscreenForNode(const Node* node)
{
    ASSERT(node);
    return node->hasTagName(HTMLNames::videoTag) && fullScreenVideo()->isValid();
}

bool ChromeClientQt::requiresFullscreenForVideoPlayback()
{
    return fullScreenVideo()->requiresFullScreenForVideoPlayback();
}

void ChromeClientQt::enterFullscreenForNode(Node* node)
{
    ASSERT(node && node->hasTagName(HTMLNames::videoTag));

    fullScreenVideo()->enterFullScreenForNode(node);
}

void ChromeClientQt::exitFullscreenForNode(Node* node)
{
    ASSERT(node && node->hasTagName(HTMLNames::videoTag));

    fullScreenVideo()->exitFullScreenForNode(node);
} 
#endif

PassOwnPtr<QWebSelectMethod> ChromeClientQt::createSelectPopup() const
{
    OwnPtr<QWebSelectMethod> result = m_platformPlugin.createSelectInputMethod();
    if (result)
        return result.release();

#if !defined(QT_NO_COMBOBOX)
    return adoptPtr(m_webPage->createSelectPopup());
#else
    return nullptr;
#endif
}

void ChromeClientQt::dispatchViewportPropertiesDidChange(const ViewportArguments&) const
{
    m_webPage->emitViewportChangeRequested();
}

#if USE(QT_MULTIMEDIA)
QWebFullScreenVideoHandler* ChromeClientQt::createFullScreenVideoHandler()
{
    QWebFullScreenVideoHandler* handler = m_platformPlugin.createFullScreenVideoHandler().leakPtr();
    if (!handler)
        handler = m_webPage->createFullScreenVideoHandler();
    return handler;
}
#endif

bool ChromeClientQt::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool ChromeClientQt::selectItemAlignmentFollowsMenuWritingDirection()
{
    return false;
}

bool ChromeClientQt::hasOpenedPopup() const
{
    notImplemented();
    return false;
}

PassRefPtr<PopupMenu> ChromeClientQt::createPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new PopupMenuQt(client, this));
}

PassRefPtr<SearchPopupMenu> ChromeClientQt::createSearchPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new SearchPopupMenuQt(createPopupMenu(client)));
}

void ChromeClientQt::populateVisitedLinks()
{
    // We don't need to do anything here because history is tied to QWebPage rather than stored
    // in a separate database
    if (dumpVisitedLinksCallbacks) {
        printf("Asked to populate visited links for WebView \"%s\"\n",
            qPrintable(QUrl(m_webPage->mainFrameAdapter()->url).toString()));
    }
}

} // namespace WebCore
