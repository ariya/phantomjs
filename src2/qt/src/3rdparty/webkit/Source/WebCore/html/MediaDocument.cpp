/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaDocument.h"

#include "DocumentLoader.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "HTMLEmbedElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#include "KeyboardEvent.h"
#include "MainResourceLoader.h"
#include "NodeList.h"
#include "RawDataDocumentParser.h"

namespace WebCore {

using namespace HTMLNames;

// FIXME: Share more code with PluginDocumentParser.
class MediaDocumentParser : public RawDataDocumentParser {
public:
    static PassRefPtr<MediaDocumentParser> create(MediaDocument* document)
    {
        return adoptRef(new MediaDocumentParser(document));
    }
    
private:
    MediaDocumentParser(Document* document)
        : RawDataDocumentParser(document)
        , m_mediaElement(0)
    {
    }

    virtual void appendBytes(DocumentWriter*, const char*, int, bool);

    void createDocumentStructure();

    HTMLMediaElement* m_mediaElement;
};
    
void MediaDocumentParser::createDocumentStructure()
{
    ExceptionCode ec;
    RefPtr<Element> rootElement = document()->createElement(htmlTag, false);
    document()->appendChild(rootElement, ec);
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    static_cast<HTMLHtmlElement*>(rootElement.get())->insertedByParser();
#endif

    if (document()->frame())
        document()->frame()->loader()->dispatchDocumentElementAvailable();
        
    RefPtr<Element> body = document()->createElement(bodyTag, false);
    body->setAttribute(styleAttr, "background-color: rgb(38,38,38);");

    rootElement->appendChild(body, ec);
        
    RefPtr<Element> mediaElement = document()->createElement(videoTag, false);
        
    m_mediaElement = static_cast<HTMLVideoElement*>(mediaElement.get());
    m_mediaElement->setAttribute(controlsAttr, "");
    m_mediaElement->setAttribute(autoplayAttr, "");
    m_mediaElement->setAttribute(styleAttr, "margin: auto; position: absolute; top: 0; right: 0; bottom: 0; left: 0;");

    m_mediaElement->setAttribute(nameAttr, "media");
    m_mediaElement->setSrc(document()->url());
    
    body->appendChild(mediaElement, ec);

    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->loader()->activeDocumentLoader()->mainResourceLoader()->setShouldBufferData(false);
}

void MediaDocumentParser::appendBytes(DocumentWriter*, const char*, int, bool)
{
    ASSERT(!m_mediaElement);
    if (m_mediaElement)
        return;

    createDocumentStructure();
    finish();
}
    
MediaDocument::MediaDocument(Frame* frame, const KURL& url)
    : HTMLDocument(frame, url)
    , m_replaceMediaElementTimer(this, &MediaDocument::replaceMediaElementTimerFired)
{
    setCompatibilityMode(QuirksMode);
    lockCompatibilityMode();
}

MediaDocument::~MediaDocument()
{
    ASSERT(!m_replaceMediaElementTimer.isActive());
}

PassRefPtr<DocumentParser> MediaDocument::createParser()
{
    return MediaDocumentParser::create(this);
}

static inline HTMLVideoElement* descendentVideoElement(Node* node)
{
    ASSERT(node);

    if (node->hasTagName(videoTag))
        return static_cast<HTMLVideoElement*>(node);

    RefPtr<NodeList> nodeList = node->getElementsByTagNameNS(videoTag.namespaceURI(), videoTag.localName());
   
    if (nodeList.get()->length() > 0)
        return static_cast<HTMLVideoElement*>(nodeList.get()->item(0));

    return 0;
}

static inline HTMLVideoElement* ancestorVideoElement(Node* node)
{
    while (node && !node->hasTagName(videoTag))
        node = node->parentOrHostNode();

    return static_cast<HTMLVideoElement*>(node);
}

void MediaDocument::defaultEventHandler(Event* event)
{
    // Match the default Quicktime plugin behavior to allow 
    // clicking and double-clicking to pause and play the media.
    Node* targetNode = event->target()->toNode();
    if (!targetNode)
        return;

    if (HTMLVideoElement* video = ancestorVideoElement(targetNode)) {
        if (event->type() == eventNames().clickEvent) {
            if (!video->canPlay()) {
                video->pause(event->fromUserGesture());
                event->setDefaultHandled();
            }
        } else if (event->type() == eventNames().dblclickEvent) {
            if (video->canPlay()) {
                video->play(event->fromUserGesture());
                event->setDefaultHandled();
            }
        }
    }
    
    if (event->type() == eventNames().keydownEvent && event->isKeyboardEvent()) {
        HTMLVideoElement* video = descendentVideoElement(targetNode);
        if (!video)
            return;

        KeyboardEvent* keyboardEvent = static_cast<KeyboardEvent*>(event);
        if (keyboardEvent->keyIdentifier() == "U+0020") { // space
            if (video->paused()) {
                if (video->canPlay())
                    video->play(event->fromUserGesture());
            } else
                video->pause(event->fromUserGesture());
            event->setDefaultHandled();
        }
    }
}

void MediaDocument::mediaElementSawUnsupportedTracks()
{
    // The HTMLMediaElement was told it has something that the underlying 
    // MediaPlayer cannot handle so we should switch from <video> to <embed> 
    // and let the plugin handle this. Don't do it immediately as this 
    // function may be called directly from a media engine callback, and 
    // replaceChild will destroy the element, media player, and media engine.
    m_replaceMediaElementTimer.startOneShot(0);
}

void MediaDocument::replaceMediaElementTimerFired(Timer<MediaDocument>*)
{
    HTMLElement* htmlBody = body();
    if (!htmlBody)
        return;

    // Set body margin width and height to 0 as that is what a PluginDocument uses.
    htmlBody->setAttribute(marginwidthAttr, "0");
    htmlBody->setAttribute(marginheightAttr, "0");

    if (HTMLVideoElement* videoElement = descendentVideoElement(htmlBody)) {
        RefPtr<Element> element = Document::createElement(embedTag, false);
        HTMLEmbedElement* embedElement = static_cast<HTMLEmbedElement*>(element.get());

        embedElement->setAttribute(widthAttr, "100%");
        embedElement->setAttribute(heightAttr, "100%");
        embedElement->setAttribute(nameAttr, "plugin");
        embedElement->setAttribute(srcAttr, url().string());
        embedElement->setAttribute(typeAttr, loader()->writer()->mimeType());

        ExceptionCode ec;
        videoElement->parentNode()->replaceChild(embedElement, videoElement, ec);
    }
}

}
#endif
