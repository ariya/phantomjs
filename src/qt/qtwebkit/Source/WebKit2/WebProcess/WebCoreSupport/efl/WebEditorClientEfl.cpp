/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
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
#include "WebEditorClient.h"

#include "Frame.h"
#include "NativeWebKeyboardEvent.h"
#include "PlatformKeyboardEvent.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/FocusController.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Page.h>
#include <WebKit2/Shared/WebCoreArgumentCoders.h>

using namespace WebCore;

namespace WebKit {

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    if (m_page->handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent* event)
{
    Frame* frame = m_page->corePage()->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;

    // FIXME: sending sync message might make input lagging.
    bool handled = false;
    m_page->sendSync(Messages::WebPageProxy::HandleInputMethodKeydown(), Messages::WebPageProxy::HandleInputMethodKeydown::Reply(handled));

    if (handled)
        event->setDefaultHandled();
}

#if USE(UNIFIED_TEXT_CHECKING)
void WebEditorClient::checkTextOfParagraph(const UChar* text, int length, WebCore::TextCheckingTypeMask checkingTypes, Vector<TextCheckingResult>& results)
{
    m_page->sendSync(Messages::WebPageProxy::CheckTextOfParagraph(String(text, length), checkingTypes), Messages::WebPageProxy::CheckTextOfParagraph::Reply(results));
}
#endif

}
