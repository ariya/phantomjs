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
#include "DatabaseTracker.h"
#include "FileChooser.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClientQt.h"
#include "FrameView.h"
#include "Geolocation.h"
#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayer.h"
#endif
#include "HitTestResult.h"
#include "Icon.h"
#include "NavigationAction.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "NotificationPresenterClientQt.h"
#include "PageClientQt.h"
#include "PopupMenuQt.h"
#if defined(Q_WS_MAEMO_5)
#include "QtMaemoWebPopup.h"
#else
#include "QtFallbackWebPopup.h"
#endif
#include "QWebPageClient.h"
#include "ScrollbarTheme.h"
#include "SearchPopupMenuQt.h"
#include "SecurityOrigin.h"
#include "ViewportArguments.h"
#include "WindowFeatures.h"

#include "qgraphicswebview.h"
#include "qwebframe_p.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include "qwebsecurityorigin.h"
#include "qwebsecurityorigin_p.h"
#include "qwebview.h"
#include <qdebug.h>
#include <qeventloop.h>
#include <qtextdocument.h>
#include <qtooltip.h>
#include <wtf/OwnPtr.h>

#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA) || USE(QTKIT))
#include "FullScreenVideoQt.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#if USE(QT_MULTIMEDIA)
#include "MediaPlayerPrivateQt.h"
#endif
#endif

namespace WebCore {

bool ChromeClientQt::dumpVisitedLinksCallbacks = false;

ChromeClientQt::ChromeClientQt(QWebPage* webPage)
    : m_webPage(webPage)
    , m_eventLoop(0)
#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA) || USE(QTKIT))
    , m_fullScreenVideo(0)
#endif
{
    toolBarsVisible = statusBarVisible = menuBarVisible = true;
}

ChromeClientQt::~ChromeClientQt()
{
    if (m_eventLoop)
        m_eventLoop->exit();

#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA) || USE(QTKIT))
    delete m_fullScreenVideo;
#endif
}

void ChromeClientQt::setWindowRect(const FloatRect& rect)
{
    if (!m_webPage)
        return;
    emit m_webPage->geometryChangeRequested(QRect(qRound(rect.x()), qRound(rect.y()),
                            qRound(rect.width()), qRound(rect.height())));
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
    return platformPageClient()->allowsAcceleratedCompositing();
}

FloatRect ChromeClientQt::pageRect()
{
    if (!m_webPage)
        return FloatRect();
    return FloatRect(QRectF(QPointF(0, 0), m_webPage->viewportSize()));
}

float ChromeClientQt::scaleFactor()
{
    if (!m_webPage)
        return 1;
    return m_webPage->d->pixelRatio;
}

void ChromeClientQt::focus()
{
    if (!m_webPage)
        return;
    QWidget* view = m_webPage->view();
    if (!view)
        return;

    view->setFocus();
}


void ChromeClientQt::unfocus()
{
    if (!m_webPage)
        return;
    QWidget* view = m_webPage->view();
    if (!view)
        return;
    view->clearFocus();
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
    QWebPage* newPage = m_webPage->createWindow(features.dialog ? QWebPage::WebModalDialog : QWebPage::WebBrowserWindow);
    if (!newPage)
        return 0;

    // A call to QWebPage::mainFrame() implicitly creates the main frame.
    // Make sure it exists, as WebCore expects it when returning from this call.
    newPage->mainFrame();
    return newPage->d->page;
}

void ChromeClientQt::show()
{
    if (!m_webPage)
        return;
    QWidget* view = m_webPage->view();
    if (!view)
        return;
    view->window()->show();
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
    emit m_webPage->toolBarVisibilityChangeRequested(visible);
}


bool ChromeClientQt::toolbarsVisible()
{
    return toolBarsVisible;
}


void ChromeClientQt::setStatusbarVisible(bool visible)
{
    emit m_webPage->statusBarVisibilityChangeRequested(visible);
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
    emit m_webPage->menuBarVisibilityChangeRequested(visible);
}

bool ChromeClientQt::menubarVisible()
{
    return menuBarVisible;
}

void ChromeClientQt::setResizable(bool)
{
    notImplemented();
}

void ChromeClientQt::addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message,
                                         unsigned int lineNumber, const String& sourceID)
{
    QString x = message;
    QString y = sourceID;
    m_webPage->javaScriptConsoleMessage(x, lineNumber, y);
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
    m_webPage->d->page->setGroupName(String());
    m_webPage->mainFrame()->d->frame->loader()->stopAllLoaders();
    emit m_webPage->windowCloseRequested();
}

void ChromeClientQt::runJavaScriptAlert(Frame* f, const String& msg)
{
    QString x = msg;
    QWebFrame* webFrame = qobject_cast<QWebFrame*>(f->loader()->networkingContext()->originatingObject());
    m_webPage->javaScriptAlert(webFrame, x);
}

bool ChromeClientQt::runJavaScriptConfirm(Frame* f, const String& msg)
{
    QString x = msg;
    QWebFrame* webFrame = qobject_cast<QWebFrame*>(f->loader()->networkingContext()->originatingObject());
    return m_webPage->javaScriptConfirm(webFrame, x);
}

bool ChromeClientQt::runJavaScriptPrompt(Frame* f, const String& message, const String& defaultValue, String& result)
{
    QString x = result;
    QWebFrame* webFrame = qobject_cast<QWebFrame*>(f->loader()->networkingContext()->originatingObject());
    bool rc = m_webPage->javaScriptPrompt(webFrame, (QString)message, (QString)defaultValue, &x);

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
    emit m_webPage->statusBarMessage(x);
}

bool ChromeClientQt::shouldInterruptJavaScript()
{
    bool shouldInterrupt = false;
    QMetaObject::invokeMethod(m_webPage, "shouldInterruptJavaScript", Qt::DirectConnection, Q_RETURN_ARG(bool, shouldInterrupt));
    return shouldInterrupt;
}

KeyboardUIMode ChromeClientQt::keyboardUIMode()
{
    return m_webPage->settings()->testAttribute(QWebSettings::LinksIncludedInFocusChain)
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

    QWidget* ownerWidget = pageClient->ownerWidget();
    if (!ownerWidget)
        return IntRect();

    QWidget* topLevelWidget = ownerWidget->window();
    QRect topLevelGeometry(topLevelWidget->geometry());

    // There's no API in Qt to query for the size of the resizer, so we assume
    // it has the same width and height as the scrollbar thickness.
    int scollbarThickness = ScrollbarTheme::nativeTheme()->scrollbarThickness();

    // There's no API in Qt to query for the position of the resizer. Sometimes
    // it's drawn by the system, and sometimes it's a QSizeGrip. For RTL locales
    // it might even be on the lower left side of the window, but in WebKit we
    // always draw scrollbars on the right hand side, so we assume this to be the
    // location when computing the resize rect to reserve for WebKit.
    QPoint resizeCornerTopLeft = ownerWidget->mapFrom(topLevelWidget,
            QPoint(topLevelGeometry.width(), topLevelGeometry.height())
            - QPoint(scollbarThickness, scollbarThickness));

    QRect resizeCornerRect = QRect(resizeCornerTopLeft, QSize(scollbarThickness, scollbarThickness));
    return resizeCornerRect.intersected(pageClient->geometryRelativeToOwnerWidget());
#else
    return IntRect();
#endif
}

void ChromeClientQt::invalidateWindow(const IntRect& windowRect, bool)
{
#if ENABLE(TILED_BACKING_STORE)
    if (platformPageClient()) {
        WebCore::TiledBackingStore* backingStore = QWebFramePrivate::core(m_webPage->mainFrame())->tiledBackingStore();
        if (!backingStore)
            return;
        backingStore->invalidate(windowRect);
    }
#else
    Q_UNUSED(windowRect);
#endif
}

void ChromeClientQt::invalidateContentsAndWindow(const IntRect& windowRect, bool immediate)
{
    // No double buffer, so only update the QWidget if content changed.
    if (platformPageClient()) {
        QRect rect(windowRect);
        rect = rect.intersected(QRect(QPoint(0, 0), m_webPage->viewportSize()));
        if (!rect.isEmpty())
            platformPageClient()->update(rect);
    }
    QMetaObject::invokeMethod(m_webPage, "repaintRequested", Qt::QueuedConnection, Q_ARG(QRect, windowRect));

    // FIXME: There is no "immediate" support for window painting.  This should be done always whenever the flag
    // is set.
}

void ChromeClientQt::invalidateContentsForSlowScroll(const IntRect& windowRect, bool immediate)
{
    invalidateContentsAndWindow(windowRect, immediate);
}

void ChromeClientQt::scroll(const IntSize& delta, const IntRect& scrollViewRect, const IntRect&)
{
    if (platformPageClient())
        platformPageClient()->scroll(delta.width(), delta.height(), scrollViewRect);
    emit m_webPage->scrollRequested(delta.width(), delta.height(), scrollViewRect);
}

#if ENABLE(TILED_BACKING_STORE)
void ChromeClientQt::delegatedScrollRequested(const IntPoint& point)
{
    QPoint currentPosition(m_webPage->mainFrame()->scrollPosition());
    emit m_webPage->scrollRequested(point.x() - currentPosition.x(), point.y() - currentPosition.y(), QRect(QPoint(0, 0), m_webPage->viewportSize()));
}
#endif

IntRect ChromeClientQt::windowToScreen(const IntRect& rect) const
{
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return rect;

    QWidget* ownerWidget = pageClient->ownerWidget();
    if (!ownerWidget)
       return rect;

    QRect screenRect(rect);
    screenRect.translate(ownerWidget->mapToGlobal(QPoint(0, 0)));

    return screenRect;
}

IntPoint ChromeClientQt::screenToWindow(const IntPoint& point) const
{
    QWebPageClient* pageClient = platformPageClient();
    if (!pageClient)
        return point;

    QWidget* ownerWidget = pageClient->ownerWidget();
    if (!ownerWidget)
        return point;

    return ownerWidget->mapFromGlobal(point);
}

PlatformPageClient ChromeClientQt::platformPageClient() const
{
    return m_webPage->d->client.get();
}

void ChromeClientQt::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    if (frame->loader()->networkingContext())
        QWebFramePrivate::kit(frame)->contentsSizeChanged(size);
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
        emit m_webPage->linkHovered(lastHoverURL.prettyURL(),
                lastHoverTitle, lastHoverContent);
    }
}

void ChromeClientQt::setToolTip(const String &tip, TextDirection)
{
#ifndef QT_NO_TOOLTIP
    QWidget* view = m_webPage->view();
    if (!view)
        return;

    if (tip.isEmpty()) {
        view->setToolTip(QString());
        QToolTip::hideText();
    } else {
        QString dtip = QLatin1String("<p>") + Qt::escape(tip) + QLatin1String("</p>");
        view->setToolTip(dtip);
    }
#else
    Q_UNUSED(tip);
#endif
}

void ChromeClientQt::print(Frame* frame)
{
    emit m_webPage->printRequested(QWebFramePrivate::kit(frame));
}

#if ENABLE(DATABASE)
void ChromeClientQt::exceededDatabaseQuota(Frame* frame, const String& databaseName)
{
    quint64 quota = QWebSettings::offlineStorageDefaultQuota();

    if (!DatabaseTracker::tracker().hasEntryForOrigin(frame->document()->securityOrigin()))
        DatabaseTracker::tracker().setQuota(frame->document()->securityOrigin(), quota);

    emit m_webPage->databaseQuotaExceeded(QWebFramePrivate::kit(frame), databaseName);
}
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
void ChromeClientQt::reachedMaxAppCacheSize(int64_t)
{
    // FIXME: Free some space.
    notImplemented();
}

void ChromeClientQt::reachedApplicationCacheOriginQuota(SecurityOrigin* origin)
{
    int64_t quota;
    quint64 defaultOriginQuota = WebCore::cacheStorage().defaultOriginQuota();

    QWebSecurityOriginPrivate* priv = new QWebSecurityOriginPrivate(origin);
    QWebSecurityOrigin* securityOrigin = new QWebSecurityOrigin(priv);

    if (!WebCore::cacheStorage().quotaForOrigin(origin, quota))
       WebCore::cacheStorage().storeUpdatedQuotaForOrigin(origin, defaultOriginQuota);

    emit m_webPage->applicationCacheQuotaExceeded(securityOrigin, defaultOriginQuota);
}
#endif

#if ENABLE(NOTIFICATIONS)
NotificationPresenter* ChromeClientQt::notificationPresenter() const
{
    return NotificationPresenterClientQt::notificationPresenter();
}
#endif

void ChromeClientQt::runOpenPanel(Frame* frame, PassRefPtr<FileChooser> prpFileChooser)
{
    RefPtr<FileChooser> fileChooser = prpFileChooser;
    bool supportMulti = m_webPage->supportsExtension(QWebPage::ChooseMultipleFilesExtension);

    if (fileChooser->allowsMultipleFiles() && supportMulti) {
        QWebPage::ChooseMultipleFilesExtensionOption option;
        option.parentFrame = QWebFramePrivate::kit(frame);

        if (!fileChooser->filenames().isEmpty())
            for (unsigned i = 0; i < fileChooser->filenames().size(); ++i)
                option.suggestedFileNames += fileChooser->filenames()[i];

        QWebPage::ChooseMultipleFilesExtensionReturn output;
        m_webPage->extension(QWebPage::ChooseMultipleFilesExtension, &option, &output);

        if (!output.fileNames.isEmpty()) {
            Vector<String> names;
            for (int i = 0; i < output.fileNames.count(); ++i)
                names.append(output.fileNames.at(i));
            fileChooser->chooseFiles(names);
        }
    } else {
        QString suggestedFile;
        if (!fileChooser->filenames().isEmpty())
            suggestedFile = fileChooser->filenames()[0];
        QString file = m_webPage->chooseFile(QWebFramePrivate::kit(frame), suggestedFile);
        if (!file.isEmpty())
            fileChooser->chooseFile(file);
    }
}

void ChromeClientQt::chooseIconForFiles(const Vector<String>& filenames, FileChooser* chooser)
{
    chooser->iconLoaded(Icon::createIconForFiles(filenames));
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


#if USE(ACCELERATED_COMPOSITING)
void ChromeClientQt::attachRootGraphicsLayer(Frame* frame, GraphicsLayer* graphicsLayer)
{
    if (platformPageClient())
        platformPageClient()->setRootGraphicsLayer(graphicsLayer ? graphicsLayer->platformLayer() : 0);
}

void ChromeClientQt::setNeedsOneShotDrawingSynchronization()
{
    // we want the layers to synchronize next time we update the screen anyway
    if (platformPageClient())
        platformPageClient()->markForSync(false);
}

void ChromeClientQt::scheduleCompositingLayerSync()
{
    // we want the layers to synchronize ASAP
    if (platformPageClient())
        platformPageClient()->markForSync(true);
}

ChromeClient::CompositingTriggerFlags ChromeClientQt::allowedCompositingTriggers() const
{
    if (platformPageClient() && platformPageClient()->allowsAcceleratedCompositing())
        return AllTriggers;

    return 0;
}

#endif

#if ENABLE(TILED_BACKING_STORE)
IntRect ChromeClientQt::visibleRectForTiledBackingStore() const
{
    if (!platformPageClient() || !m_webPage)
        return IntRect();

    if (!platformPageClient()->viewResizesToContentsEnabled())
        return QRect(m_webPage->mainFrame()->scrollPosition(), m_webPage->mainFrame()->geometry().size());

    return enclosingIntRect(FloatRect(platformPageClient()->graphicsItemVisibleRect()));
}
#endif

#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA) || USE(QTKIT))
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

QWebSelectMethod* ChromeClientQt::createSelectPopup() const
{
    QWebSelectMethod* result = m_platformPlugin.createSelectInputMethod();
    if (result)
        return result;

#if defined(Q_WS_MAEMO_5)
    return new QtMaemoWebPopup;
#elif !defined(QT_NO_COMBOBOX)
    return new QtFallbackWebPopup(this);
#else
    return 0;
#endif
}

void ChromeClientQt::dispatchViewportDataDidChange(const ViewportArguments&) const
{
    emit m_webPage->viewportChangeRequested();
}

bool ChromeClientQt::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool ChromeClientQt::selectItemAlignmentFollowsMenuWritingDirection()
{
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
                qPrintable(m_webPage->mainFrame()->url().toString()));
    }
}

} // namespace WebCore
