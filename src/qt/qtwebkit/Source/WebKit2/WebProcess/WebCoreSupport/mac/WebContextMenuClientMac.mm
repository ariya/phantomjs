/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebContextMenuClient.h"

#if ENABLE(CONTEXT_MENUS)

#import "DictionaryPopupInfo.h"
#import "WebCoreArgumentCoders.h"
#import "WebPage.h"
#import "WebPageProxyMessages.h"
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/Page.h>
#import <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

void WebContextMenuClient::lookUpInDictionary(Frame* frame)
{
    m_page->performDictionaryLookupForSelection(frame, frame->selection()->selection());
}

bool WebContextMenuClient::isSpeaking()
{
    return m_page->isSpeaking();
}

void WebContextMenuClient::speak(const String& string)
{
    m_page->speak(string);
}

void WebContextMenuClient::stopSpeaking()
{
    m_page->stopSpeaking();
}

void WebContextMenuClient::searchWithGoogle(const Frame* frame)
{
    String searchString = frame->editor().selectedText();
    searchString.stripWhiteSpace();
    
    m_page->send(Messages::WebPageProxy::SearchTheWeb(searchString));
}

void WebContextMenuClient::searchWithSpotlight()
{
    // FIXME: Why do we need to search all the frames like this?
    // Isn't there any function in WebCore that can do this?
    // If not, can we find a place in WebCore to put this?

    Frame* mainFrame = m_page->corePage()->mainFrame();
    
    Frame* selectionFrame = mainFrame;
    for (; selectionFrame; selectionFrame = selectionFrame->tree()->traverseNext(mainFrame)) {
        if (selectionFrame->selection()->isRange())
            break;
    }
    if (!selectionFrame)
        selectionFrame = mainFrame;

    String selectedString = selectionFrame->displayStringModifiedByEncoding(selectionFrame->editor().selectedText());
    
    if (selectedString.isEmpty())
        return;

    m_page->send(Messages::WebPageProxy::SearchWithSpotlight(selectedString));
}

} // namespace WebKit

#endif // ENABLE(CONTEXT_MENUS)
