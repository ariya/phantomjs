/*
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ChromeClientWinCE.h"

#include "FileChooser.h"
#include "FileIconLoader.h"
#include "Icon.h"
#include "NotImplemented.h"
#include "NavigationAction.h"
#include "PopupMenuWin.h"
#include "SearchPopupMenuWin.h"
#include "WebView.h"
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

ChromeClientWinCE::ChromeClientWinCE(WebView* webView)
    : m_webView(webView)
{
    ASSERT(m_webView);
}

void ChromeClientWinCE::chromeDestroyed()
{
    delete this;
}

FloatRect ChromeClientWinCE::windowRect()
{
    if (!m_webView)
        return FloatRect();

    RECT rect;
    m_webView->frameRect(&rect);
    return rect;
}

void ChromeClientWinCE::setWindowRect(const FloatRect&)
{
    notImplemented();
}

FloatRect ChromeClientWinCE::pageRect()
{
    return windowRect();
}

void ChromeClientWinCE::focus()
{
    notImplemented();
}

void ChromeClientWinCE::unfocus()
{
    notImplemented();
}

Page* ChromeClientWinCE::createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&)
{
    notImplemented();
    return 0;
}

void ChromeClientWinCE::show()
{
    notImplemented();
}

bool ChromeClientWinCE::canRunModal()
{
    notImplemented();
    return false;
}

void ChromeClientWinCE::runModal()
{
    notImplemented();
}

void ChromeClientWinCE::setToolbarsVisible(bool)
{
    notImplemented();
}

bool ChromeClientWinCE::toolbarsVisible()
{
    return false;
}

void ChromeClientWinCE::setStatusbarVisible(bool)
{
    notImplemented();
}

bool ChromeClientWinCE::statusbarVisible()
{
    notImplemented();
    return false;
}

void ChromeClientWinCE::setScrollbarsVisible(bool)
{
    notImplemented();
}

bool ChromeClientWinCE::scrollbarsVisible()
{
    notImplemented();
    return false;
}

void ChromeClientWinCE::setMenubarVisible(bool)
{
    notImplemented();
}

bool ChromeClientWinCE::menubarVisible()
{
    notImplemented();
    return false;
}

void ChromeClientWinCE::setResizable(bool)
{
    notImplemented();
}

void ChromeClientWinCE::closeWindowSoon()
{
    PostMessageW(m_webView->windowHandle(), WM_CLOSE, 0, 0);
}

bool ChromeClientWinCE::canTakeFocus(FocusDirection)
{
    return true;
}

void ChromeClientWinCE::takeFocus(FocusDirection)
{
    unfocus();
}

void ChromeClientWinCE::focusedNodeChanged(Node*)
{
    notImplemented();
}

void ChromeClientWinCE::focusedFrameChanged(Frame*)
{
}

bool ChromeClientWinCE::canRunBeforeUnloadConfirmPanel()
{
    return true;
}

bool ChromeClientWinCE::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    return runJavaScriptConfirm(frame, message);
}

void ChromeClientWinCE::addMessageToConsole(MessageSource, MessageLevel, const String&, unsigned, unsigned, const String&)
{
    notImplemented();
}

void ChromeClientWinCE::runJavaScriptAlert(Frame*, const String& message)
{
    m_webView->runJavaScriptAlert(message);
}

bool ChromeClientWinCE::runJavaScriptConfirm(Frame*, const String& message)
{
    return m_webView->runJavaScriptConfirm(message);
}

bool ChromeClientWinCE::runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result)
{
    return m_webView->runJavaScriptPrompt(message, defaultValue, result);
}

void ChromeClientWinCE::setStatusbarText(const String&)
{
    notImplemented();
}

bool ChromeClientWinCE::shouldInterruptJavaScript()
{
    notImplemented();
    return false;
}

KeyboardUIMode ChromeClientWinCE::keyboardUIMode()
{
    return KeyboardAccessTabsToLinks;
}

IntRect ChromeClientWinCE::windowResizerRect() const
{
    notImplemented();
    return IntRect();
}

void ChromeClientWinCE::invalidateRootView(const IntRect&, bool)
{
    notImplemented();
}

void ChromeClientWinCE::invalidateContentsAndRootView(const IntRect& updateRect, bool immediate)
{
    RECT rect = updateRect;
    InvalidateRect(m_webView->windowHandle(), &rect, FALSE);

    if (immediate)
        UpdateWindow(m_webView->windowHandle());
}

void ChromeClientWinCE::invalidateContentsForSlowScroll(const IntRect& updateRect, bool immediate)
{
    invalidateContentsAndRootView(updateRect, immediate);
}

void ChromeClientWinCE::scroll(const IntSize&, const IntRect& rectToScroll, const IntRect&)
{
    invalidateContentsAndRootView(rectToScroll, false);
}

IntRect ChromeClientWinCE::rootViewToScreen(const IntRect& rect) const
{
    notImplemented();
    return rect;
}

IntPoint ChromeClientWinCE::screenToRootView(const IntPoint& point) const
{
    notImplemented();
    return point;
}

PlatformPageClient ChromeClientWinCE::platformPageClient() const
{
    notImplemented();
    return 0;
}

void ChromeClientWinCE::contentsSizeChanged(Frame*, const IntSize&) const
{
    notImplemented();
}

void ChromeClientWinCE::scrollRectIntoView(const IntRect&) const
{
    notImplemented();
}

void ChromeClientWinCE::scrollbarsModeDidChange() const
{
    notImplemented();
}

void ChromeClientWinCE::mouseDidMoveOverElement(const HitTestResult&, unsigned)
{
    notImplemented();
}

void ChromeClientWinCE::setToolTip(const String&, TextDirection)
{
    notImplemented();
}

void ChromeClientWinCE::print(Frame*)
{
    notImplemented();
}

#if ENABLE(SQL_DATABASE)
void ChromeClientWinCE::exceededDatabaseQuota(Frame*, const String&, DatabaseDetails)
{
    notImplemented();
}
#endif

void ChromeClientWinCE::reachedMaxAppCacheSize(int64_t)
{
    notImplemented();
}

void ChromeClientWinCE::reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t)
{
    notImplemented();
}

#if ENABLE(TOUCH_EVENTS)
void ChromeClientWinCE::needTouchEvents(bool)
{
    notImplemented();
}
#endif

#if USE(ACCELERATED_COMPOSITING)
void ChromeClientWinCE::attachRootGraphicsLayer(Frame*, GraphicsLayer*)
{
    notImplemented();
}

void ChromeClientWinCE::setNeedsOneShotDrawingSynchronization()
{
    notImplemented();
}

void ChromeClientWinCE::scheduleCompositingLayerFlush()
{
    notImplemented();
}
#endif

void ChromeClientWinCE::runOpenPanel(Frame*, PassRefPtr<FileChooser> prpFileChooser)
{
    notImplemented();
}

void ChromeClientWinCE::loadIconForFiles(const Vector<String>& filenames, FileIconLoader* loader)
{
    loader->notifyFinished(Icon::createIconForFiles(filenames));
}

void ChromeClientWinCE::setCursor(const Cursor&)
{
    notImplemented();
}

void ChromeClientWinCE::setCursorHiddenUntilMouseMoves(bool)
{
    notImplemented();
}

void ChromeClientWinCE::setLastSetCursorToCurrentCursor()
{
    notImplemented();
}

void ChromeClientWinCE::AXStartFrameLoad()
{
    notImplemented();
}

void ChromeClientWinCE::AXFinishFrameLoad()
{
    notImplemented();
}

void ChromeClientWinCE::formStateDidChange(const Node*)
{
    notImplemented();
}

bool ChromeClientWinCE::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool ChromeClientWinCE::selectItemAlignmentFollowsMenuWritingDirection()
{
    return false;
}

bool ChromeClientWinCE::hasOpenedPopup() const
{
    notImplemented();
    return false;
}

PassRefPtr<PopupMenu> ChromeClientWinCE::createPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new PopupMenuWin(client));
}

PassRefPtr<SearchPopupMenu> ChromeClientWinCE::createSearchPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new SearchPopupMenuWin(client));
}

} // namespace WebKit
