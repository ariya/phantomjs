/*
 * Copyright (C) 2006, 2008, 2013 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PluginDocument.h"

#include "DocumentLoader.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLEmbedElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RawDataDocumentParser.h"
#include "RenderEmbeddedObject.h"
#include "Settings.h"

namespace WebCore {
    
using namespace HTMLNames;

// FIXME: Share more code with MediaDocumentParser.
class PluginDocumentParser : public RawDataDocumentParser {
public:
    static PassRefPtr<PluginDocumentParser> create(PluginDocument* document)
    {
        return adoptRef(new PluginDocumentParser(document));
    }

private:
    PluginDocumentParser(Document* document)
        : RawDataDocumentParser(document)
        , m_embedElement(0)
    {
    }

    virtual void appendBytes(DocumentWriter*, const char*, size_t);

    void createDocumentStructure();

    HTMLEmbedElement* m_embedElement;
};

void PluginDocumentParser::createDocumentStructure()
{
    RefPtr<Element> rootElement = document()->createElement(htmlTag, false);
    document()->appendChild(rootElement, IGNORE_EXCEPTION);
    static_cast<HTMLHtmlElement*>(rootElement.get())->insertedByParser();

    if (document()->frame() && document()->frame()->loader())
        document()->frame()->loader()->dispatchDocumentElementAvailable();

    RefPtr<Element> body = document()->createElement(bodyTag, false);
    body->setAttribute(marginwidthAttr, "0");
    body->setAttribute(marginheightAttr, "0");
    body->setAttribute(styleAttr, "background-color: rgb(38,38,38)");

    rootElement->appendChild(body, IGNORE_EXCEPTION);
        
    RefPtr<Element> embedElement = document()->createElement(embedTag, false);
        
    m_embedElement = static_cast<HTMLEmbedElement*>(embedElement.get());
    m_embedElement->setAttribute(widthAttr, "100%");
    m_embedElement->setAttribute(heightAttr, "100%");
    
    m_embedElement->setAttribute(nameAttr, "plugin");
    m_embedElement->setAttribute(srcAttr, document()->url().string());
    
    DocumentLoader* loader = document()->loader();
    ASSERT(loader);
    if (loader)
        m_embedElement->setAttribute(typeAttr, loader->writer()->mimeType());

    toPluginDocument(document())->setPluginElement(m_embedElement);

    body->appendChild(embedElement, IGNORE_EXCEPTION);
}

void PluginDocumentParser::appendBytes(DocumentWriter*, const char*, size_t)
{
    if (m_embedElement)
        return;

    createDocumentStructure();

    Frame* frame = document()->frame();
    if (!frame)
        return;
    Settings* settings = frame->settings();
    if (!settings)
        return;

    document()->updateLayout();

    // Below we assume that renderer->widget() to have been created by
    // document()->updateLayout(). However, in some cases, updateLayout() will 
    // recurse too many times and delay its post-layout tasks (such as creating
    // the widget). Here we kick off the pending post-layout tasks so that we
    // can synchronously redirect data to the plugin.
    frame->view()->flushAnyPendingPostLayoutTasks();

    if (RenderPart* renderer = m_embedElement->renderPart()) {
        if (Widget* widget = renderer->widget()) {
            frame->loader()->client()->redirectDataToPlugin(widget);
            // In a plugin document, the main resource is the plugin. If we have a null widget, that means
            // the loading of the plugin was cancelled, which gives us a null mainResourceLoader(), so we
            // need to have this call in a null check of the widget or of mainResourceLoader().
            frame->loader()->activeDocumentLoader()->setMainResourceDataBufferingPolicy(DoNotBufferData);
        }
    }
}

PluginDocument::PluginDocument(Frame* frame, const KURL& url)
    : HTMLDocument(frame, url, PluginDocumentClass)
    , m_shouldLoadPluginManually(true)
{
    setCompatibilityMode(QuirksMode);
    lockCompatibilityMode();
}

PassRefPtr<DocumentParser> PluginDocument::createParser()
{
    return PluginDocumentParser::create(this);
}

Widget* PluginDocument::pluginWidget()
{
    if (m_pluginElement && m_pluginElement->renderer()) {
        ASSERT(m_pluginElement->renderer()->isEmbeddedObject());
        return toRenderEmbeddedObject(m_pluginElement->renderer())->widget();
    }
    return 0;
}

void PluginDocument::setPluginElement(PassRefPtr<HTMLPlugInElement> element)
{
    m_pluginElement = element;
}

void PluginDocument::detach(const AttachContext& context)
{
    // Release the plugin Element so that we don't have a circular reference.
    m_pluginElement = 0;
    if (FrameLoader* loader = frame()->loader())
        loader->client()->redirectDataToPlugin(0);
    HTMLDocument::detach(context);
}

void PluginDocument::cancelManualPluginLoad()
{
    // PluginDocument::cancelManualPluginLoad should only be called once, but there are issues
    // with how many times we call beforeload on object elements. <rdar://problem/8441094>.
    if (!shouldLoadPluginManually())
        return;

    DocumentLoader* documentLoader = frame()->loader()->activeDocumentLoader();
    documentLoader->cancelMainResourceLoad(frame()->loader()->cancelledError(documentLoader->request()));
    setShouldLoadPluginManually(false);
}

}
