/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebPage.h"

#include "EditorState.h"
#include "EventHandler.h"
#include "NotImplemented.h"
#include "WebEvent.h"
#include "WindowsKeyboardCodes.h"
#include <WebCore/EflKeyboardUtilities.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/RenderThemeEfl.h>
#include <WebCore/Settings.h>

#if HAVE(ACCESSIBILITY)
#include "WebPageAccessibilityObject.h"
#endif

using namespace WebCore;

namespace WebKit {

void WebPage::platformInitialize()
{
#if HAVE(ACCESSIBILITY)
    m_accessibilityObject = adoptGRef(webPageAccessibilityObjectNew(this));
#else
    notImplemented();
#endif
}

#if HAVE(ACCESSIBILITY)
void WebPage::updateAccessibilityTree()
{
    if (!m_accessibilityObject)
        return;

    webPageAccessibilityObjectRefresh(m_accessibilityObject.get());
}
#endif

void WebPage::platformPreferencesDidChange(const WebPreferencesStore&)
{
    notImplemented();
}

static inline void scroll(Page* page, ScrollDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->scrollRecursively(direction, granularity);
}

bool WebPage::performDefaultBehaviorForKeyEvent(const WebKeyboardEvent& keyboardEvent)
{
    if (keyboardEvent.type() != WebEvent::KeyDown && keyboardEvent.type() != WebEvent::RawKeyDown)
        return false;

    switch (keyboardEvent.windowsVirtualKeyCode()) {
    case VK_BACK:
        if (keyboardEvent.shiftKey())
            m_page->goForward();
        else
            m_page->goBack();
        break;
    case VK_SPACE:
        scroll(m_page.get(), keyboardEvent.shiftKey() ? ScrollUp : ScrollDown, ScrollByPage);
        break;
    case VK_LEFT:
        scroll(m_page.get(), ScrollLeft, ScrollByLine);
        break;
    case VK_RIGHT:
        scroll(m_page.get(), ScrollRight, ScrollByLine);
        break;
    case VK_UP:
        scroll(m_page.get(), ScrollUp, ScrollByLine);
        break;
    case VK_DOWN:
        scroll(m_page.get(), ScrollDown, ScrollByLine);
        break;
    case VK_HOME:
        scroll(m_page.get(), ScrollUp, ScrollByDocument);
        break;
    case VK_END:
        scroll(m_page.get(), ScrollDown, ScrollByDocument);
        break;
    case VK_PRIOR:
        scroll(m_page.get(), ScrollUp, ScrollByPage);
        break;
    case VK_NEXT:
        scroll(m_page.get(), ScrollDown, ScrollByPage);
        break;
    default:
        return false;
    }

    return true;
}

bool WebPage::platformHasLocalDataForURL(const KURL&)
{
    notImplemented();
    return false;
}

String WebPage::cachedResponseMIMETypeForURL(const KURL&)
{
    notImplemented();
    return String();
}

bool WebPage::platformCanHandleRequest(const ResourceRequest&)
{
    notImplemented();
    return true;
}

String WebPage::cachedSuggestedFilenameForURL(const KURL&)
{
    notImplemented();
    return String();
}

PassRefPtr<SharedBuffer> WebPage::cachedResponseDataForURL(const KURL&)
{
    notImplemented();
    return 0;
}

const char* WebPage::interpretKeyEvent(const KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    if (event->type() == eventNames().keydownEvent)
        return getKeyDownCommandName(event);

    return getKeyPressCommandName(event);
}

void WebPage::setThemePath(const String& themePath)
{
    WebCore::RenderThemeEfl* theme = static_cast<WebCore::RenderThemeEfl*>(m_page->theme());
    theme->setThemePath(themePath);
}

static Frame* targetFrameForEditing(WebPage* page)
{
    Frame* frame = page->corePage()->focusController()->focusedOrMainFrame();
    if (!frame)
        return 0;

    Editor& editor = frame->editor();
    if (!editor.canEdit())
        return 0;

    if (editor.hasComposition()) {
        // We should verify the parent node of this IME composition node are
        // editable because JavaScript may delete a parent node of the composition
        // node. In this case, WebKit crashes while deleting texts from the parent
        // node, which doesn't exist any longer.
        if (PassRefPtr<Range> range = editor.compositionRange()) {
            Node* node = range->startContainer();
            if (!node || !node->isContentEditable())
                return 0;
        }
    }

    return frame;
}

void WebPage::confirmComposition(const String& compositionString)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;

    targetFrame->editor().confirmComposition(compositionString);
}

void WebPage::setComposition(const String& compositionString, const Vector<WebCore::CompositionUnderline>& underlines, uint64_t cursorPosition)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;

    targetFrame->editor().setComposition(compositionString, underlines, cursorPosition, 0);
}

void WebPage::cancelComposition()
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    frame->editor().cancelComposition();
}

} // namespace WebKit
