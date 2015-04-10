/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
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
#include "ImageDocument.h"

#include "CachedImage.h"
#include "DocumentLoader.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLHtmlElement.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "RawDataDocumentParser.h"
#include "ResourceBuffer.h"
#include "Settings.h"

using std::min;

namespace WebCore {

using namespace HTMLNames;

class ImageEventListener : public EventListener {
public:
    static PassRefPtr<ImageEventListener> create(ImageDocument* document) { return adoptRef(new ImageEventListener(document)); }
    static const ImageEventListener* cast(const EventListener* listener)
    {
        return listener->type() == ImageEventListenerType
            ? static_cast<const ImageEventListener*>(listener)
            : 0;
    }

    virtual bool operator==(const EventListener& other);

private:
    ImageEventListener(ImageDocument* document)
        : EventListener(ImageEventListenerType)
        , m_doc(document)
    {
    }

    virtual void handleEvent(ScriptExecutionContext*, Event*);

    ImageDocument* m_doc;
};
    
class ImageDocumentParser : public RawDataDocumentParser {
public:
    static PassRefPtr<ImageDocumentParser> create(ImageDocument* document)
    {
        return adoptRef(new ImageDocumentParser(document));
    }

    ImageDocument* document() const
    {
        return toImageDocument(RawDataDocumentParser::document());
    }
    
private:
    ImageDocumentParser(ImageDocument* document)
        : RawDataDocumentParser(document)
    {
    }

    virtual void appendBytes(DocumentWriter*, const char*, size_t);
    virtual void finish();
};

class ImageDocumentElement FINAL : public HTMLImageElement {
public:
    static PassRefPtr<ImageDocumentElement> create(ImageDocument*);

private:
    ImageDocumentElement(ImageDocument* document)
        : HTMLImageElement(imgTag, document)
        , m_imageDocument(document)
    {
    }

    virtual ~ImageDocumentElement();
    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

    ImageDocument* m_imageDocument;
};

inline PassRefPtr<ImageDocumentElement> ImageDocumentElement::create(ImageDocument* document)
{
    return adoptRef(new ImageDocumentElement(document));
}

// --------

static float pageZoomFactor(const Document* document)
{
    Frame* frame = document->frame();
    return frame ? frame->pageZoomFactor() : 1;
}

void ImageDocumentParser::appendBytes(DocumentWriter*, const char*, size_t)
{
    Frame* frame = document()->frame();
    Settings* settings = frame->settings();
    if (!frame->loader()->client()->allowImage(!settings || settings->areImagesEnabled(), document()->url()))
        return;

    CachedImage* cachedImage = document()->cachedImage();
    RefPtr<ResourceBuffer> resourceData = frame->loader()->documentLoader()->mainResourceData();
    cachedImage->addDataBuffer(resourceData.get());

    document()->imageUpdated();
}

void ImageDocumentParser::finish()
{
    if (!isStopped() && document()->imageElement()) {
        CachedImage* cachedImage = document()->cachedImage();
        RefPtr<ResourceBuffer> data = document()->frame()->loader()->documentLoader()->mainResourceData();

        // If this is a multipart image, make a copy of the current part, since the resource data
        // will be overwritten by the next part.
        if (document()->frame()->loader()->documentLoader()->isLoadingMultipartContent())
            data = data->copy();

        cachedImage->finishLoading(data.get());
        cachedImage->finish();

        cachedImage->setResponse(document()->frame()->loader()->documentLoader()->response());

        // Report the natural image size in the page title, regardless of zoom level.
        // At a zoom level of 1 the image is guaranteed to have an integer size.
        IntSize size = flooredIntSize(cachedImage->imageSizeForRenderer(document()->imageElement()->renderer(), 1.0f));
        if (size.width()) {
            // Compute the title, we use the decoded filename of the resource, falling
            // back on the (decoded) hostname if there is no path.
            String fileName = decodeURLEscapeSequences(document()->url().lastPathComponent());
            if (fileName.isEmpty())
                fileName = document()->url().host();
            document()->setTitle(imageTitle(fileName, size));
        }

        document()->imageUpdated();
    }

    document()->finishedParsing();
}
    
// --------

ImageDocument::ImageDocument(Frame* frame, const KURL& url)
    : HTMLDocument(frame, url, ImageDocumentClass)
    , m_imageElement(0)
    , m_imageSizeIsKnown(false)
    , m_didShrinkImage(false)
    , m_shouldShrinkImage(shouldShrinkToFit())
{
    setCompatibilityMode(QuirksMode);
    lockCompatibilityMode();
}
    
PassRefPtr<DocumentParser> ImageDocument::createParser()
{
    return ImageDocumentParser::create(this);
}

void ImageDocument::createDocumentStructure()
{
    RefPtr<Element> rootElement = Document::createElement(htmlTag, false);
    appendChild(rootElement, IGNORE_EXCEPTION);
    static_cast<HTMLHtmlElement*>(rootElement.get())->insertedByParser();

    if (frame() && frame()->loader())
        frame()->loader()->dispatchDocumentElementAvailable();
    
    RefPtr<Element> body = Document::createElement(bodyTag, false);
    body->setAttribute(styleAttr, "margin: 0px;");
    
    rootElement->appendChild(body, IGNORE_EXCEPTION);
    
    RefPtr<ImageDocumentElement> imageElement = ImageDocumentElement::create(this);
    
    imageElement->setAttribute(styleAttr, "-webkit-user-select: none");        
    imageElement->setLoadManually(true);
    imageElement->setSrc(url().string());
    
    body->appendChild(imageElement, IGNORE_EXCEPTION);
    
    if (shouldShrinkToFit()) {
        // Add event listeners
        RefPtr<EventListener> listener = ImageEventListener::create(this);
        if (DOMWindow* domWindow = this->domWindow())
            domWindow->addEventListener("resize", listener, false);
        imageElement->addEventListener("click", listener.release(), false);
    }

    m_imageElement = imageElement.get();
}

float ImageDocument::scale() const
{
    if (!m_imageElement)
        return 1.0f;

    FrameView* view = frame()->view();
    if (!view)
        return 1;

    LayoutSize imageSize = m_imageElement->cachedImage()->imageSizeForRenderer(m_imageElement->renderer(), pageZoomFactor(this));
    LayoutSize windowSize = LayoutSize(view->width(), view->height());
    
    float widthScale = (float)windowSize.width() / imageSize.width();
    float heightScale = (float)windowSize.height() / imageSize.height();

    return min(widthScale, heightScale);
}

void ImageDocument::resizeImageToFit()
{
    if (!m_imageElement)
        return;

    LayoutSize imageSize = m_imageElement->cachedImage()->imageSizeForRenderer(m_imageElement->renderer(), pageZoomFactor(this));

    float scale = this->scale();
    m_imageElement->setWidth(static_cast<int>(imageSize.width() * scale));
    m_imageElement->setHeight(static_cast<int>(imageSize.height() * scale));
    
    m_imageElement->setInlineStyleProperty(CSSPropertyCursor, "-webkit-zoom-in", false);
}

void ImageDocument::imageClicked(int x, int y)
{
    if (!m_imageSizeIsKnown || imageFitsInWindow())
        return;

    m_shouldShrinkImage = !m_shouldShrinkImage;
    
    if (m_shouldShrinkImage)
        windowSizeChanged();
    else {
        restoreImageSize();
        
        updateLayout();
        
        float scale = this->scale();
        
        int scrollX = static_cast<int>(x / scale - (float)frame()->view()->width() / 2);
        int scrollY = static_cast<int>(y / scale - (float)frame()->view()->height() / 2);
        
        frame()->view()->setScrollPosition(IntPoint(scrollX, scrollY));
    }
}

void ImageDocument::imageUpdated()
{
    ASSERT(m_imageElement);
    
    if (m_imageSizeIsKnown)
        return;

    if (m_imageElement->cachedImage()->imageSizeForRenderer(m_imageElement->renderer(), pageZoomFactor(this)).isEmpty())
        return;
    
    m_imageSizeIsKnown = true;
    
    if (shouldShrinkToFit()) {
        // Force resizing of the image
        windowSizeChanged();
    }
}

void ImageDocument::restoreImageSize()
{
    if (!m_imageElement || !m_imageSizeIsKnown)
        return;
    
    LayoutSize imageSize = m_imageElement->cachedImage()->imageSizeForRenderer(m_imageElement->renderer(), pageZoomFactor(this));
    m_imageElement->setWidth(imageSize.width());
    m_imageElement->setHeight(imageSize.height());
    
    if (imageFitsInWindow())
        m_imageElement->removeInlineStyleProperty(CSSPropertyCursor);
    else
        m_imageElement->setInlineStyleProperty(CSSPropertyCursor, "-webkit-zoom-out", false);
        
    m_didShrinkImage = false;
}

bool ImageDocument::imageFitsInWindow() const
{
    if (!m_imageElement)
        return true;

    FrameView* view = frame()->view();
    if (!view)
        return true;

    LayoutSize imageSize = m_imageElement->cachedImage()->imageSizeForRenderer(m_imageElement->renderer(), pageZoomFactor(this));
    LayoutSize windowSize = LayoutSize(view->width(), view->height());
    
    return imageSize.width() <= windowSize.width() && imageSize.height() <= windowSize.height();    
}

void ImageDocument::windowSizeChanged()
{
    if (!m_imageElement || !m_imageSizeIsKnown)
        return;

    bool fitsInWindow = imageFitsInWindow();
    
    // If the image has been explicitly zoomed in, restore the cursor if the image fits
    // and set it to a zoom out cursor if the image doesn't fit
    if (!m_shouldShrinkImage) {
        if (fitsInWindow)
            m_imageElement->removeInlineStyleProperty(CSSPropertyCursor);
        else
            m_imageElement->setInlineStyleProperty(CSSPropertyCursor, "-webkit-zoom-out", false);
        return;
    }
    
    if (m_didShrinkImage) {
        // If the window has been resized so that the image fits, restore the image size
        // otherwise update the restored image size.
        if (fitsInWindow)
            restoreImageSize();
        else
            resizeImageToFit();
    } else {
        // If the image isn't resized but needs to be, then resize it.
        if (!fitsInWindow) {
            resizeImageToFit();
            m_didShrinkImage = true;
        }
    }
}

CachedImage* ImageDocument::cachedImage()
{ 
    if (!m_imageElement)
        createDocumentStructure();
    
    return m_imageElement->cachedImage();
}

bool ImageDocument::shouldShrinkToFit() const
{
    return frame()->page()->settings()->shrinksStandaloneImagesToFit() &&
        frame()->page()->mainFrame() == frame();
}

// --------

void ImageEventListener::handleEvent(ScriptExecutionContext*, Event* event)
{
    if (event->type() == eventNames().resizeEvent)
        m_doc->windowSizeChanged();
    else if (event->type() == eventNames().clickEvent && event->isMouseEvent()) {
        MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
        m_doc->imageClicked(mouseEvent->x(), mouseEvent->y());
    }
}

bool ImageEventListener::operator==(const EventListener& listener)
{
    if (const ImageEventListener* imageEventListener = ImageEventListener::cast(&listener))
        return m_doc == imageEventListener->m_doc;
    return false;
}

// --------

ImageDocumentElement::~ImageDocumentElement()
{
    if (m_imageDocument)
        m_imageDocument->disconnectImageElement();
}

void ImageDocumentElement::didMoveToNewDocument(Document* oldDocument)
{
    if (m_imageDocument) {
        m_imageDocument->disconnectImageElement();
        m_imageDocument = 0;
    }
    HTMLImageElement::didMoveToNewDocument(oldDocument);
}

}
