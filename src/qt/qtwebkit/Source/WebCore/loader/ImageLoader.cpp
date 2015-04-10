/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ImageLoader.h"

#include "CachedImage.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CrossOriginAccessControl.h"
#include "Document.h"
#include "Element.h"
#include "Event.h"
#include "EventSender.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLParserIdioms.h"
#include "RenderImage.h"
#include "ScriptCallStack.h"
#include "SecurityOrigin.h"

#if ENABLE(SVG)
#include "RenderSVGImage.h"
#endif
#if ENABLE(VIDEO)
#include "RenderVideo.h"
#endif

#if !ASSERT_DISABLED
// ImageLoader objects are allocated as members of other objects, so generic pointer check would always fail.
namespace WTF {

template<> struct ValueCheck<WebCore::ImageLoader*> {
    typedef WebCore::ImageLoader* TraitType;
    static void checkConsistency(const WebCore::ImageLoader* p)
    {
        if (!p)
            return;
        ASSERT(p->element());
        ValueCheck<WebCore::Element*>::checkConsistency(p->element());
    }
};

}
#endif

namespace WebCore {

static ImageEventSender& beforeLoadEventSender()
{
    DEFINE_STATIC_LOCAL(ImageEventSender, sender, (eventNames().beforeloadEvent));
    return sender;
}

static ImageEventSender& loadEventSender()
{
    DEFINE_STATIC_LOCAL(ImageEventSender, sender, (eventNames().loadEvent));
    return sender;
}

static ImageEventSender& errorEventSender()
{
    DEFINE_STATIC_LOCAL(ImageEventSender, sender, (eventNames().errorEvent));
    return sender;
}

static inline bool pageIsBeingDismissed(Document* document)
{
    Frame* frame = document->frame();
    return frame && frame->loader()->pageDismissalEventBeingDispatched() != FrameLoader::NoDismissal;
}

ImageLoader::ImageLoader(Element* element)
    : m_element(element)
    , m_image(0)
    , m_derefElementTimer(this, &ImageLoader::timerFired)
    , m_hasPendingBeforeLoadEvent(false)
    , m_hasPendingLoadEvent(false)
    , m_hasPendingErrorEvent(false)
    , m_imageComplete(true)
    , m_loadManually(false)
    , m_elementIsProtected(false)
{
}

ImageLoader::~ImageLoader()
{
    if (m_image)
        m_image->removeClient(this);

    ASSERT(m_hasPendingBeforeLoadEvent || !beforeLoadEventSender().hasPendingEvents(this));
    if (m_hasPendingBeforeLoadEvent)
        beforeLoadEventSender().cancelEvent(this);

    ASSERT(m_hasPendingLoadEvent || !loadEventSender().hasPendingEvents(this));
    if (m_hasPendingLoadEvent)
        loadEventSender().cancelEvent(this);

    ASSERT(m_hasPendingErrorEvent || !errorEventSender().hasPendingEvents(this));
    if (m_hasPendingErrorEvent)
        errorEventSender().cancelEvent(this);

    // If the ImageLoader is being destroyed but it is still protecting its image-loading Element,
    // remove that protection here.
    if (m_elementIsProtected)
        m_element->deref();
}

void ImageLoader::setImage(CachedImage* newImage)
{
    setImageWithoutConsideringPendingLoadEvent(newImage);

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingEvent();
}

void ImageLoader::setImageWithoutConsideringPendingLoadEvent(CachedImage* newImage)
{
    ASSERT(m_failedLoadURL.isEmpty());
    CachedImage* oldImage = m_image.get();
    if (newImage != oldImage) {
        m_image = newImage;
        if (m_hasPendingBeforeLoadEvent) {
            beforeLoadEventSender().cancelEvent(this);
            m_hasPendingBeforeLoadEvent = false;
        }
        if (m_hasPendingLoadEvent) {
            loadEventSender().cancelEvent(this);
            m_hasPendingLoadEvent = false;
        }
        if (m_hasPendingErrorEvent) {
            errorEventSender().cancelEvent(this);
            m_hasPendingErrorEvent = false;
        }
        m_imageComplete = true;
        if (newImage)
            newImage->addClient(this);
        if (oldImage)
            oldImage->removeClient(this);
    }

    if (RenderImageResource* imageResource = renderImageResource())
        imageResource->resetAnimation();
}

void ImageLoader::updateFromElement()
{
    // If we're not making renderers for the page, then don't load images.  We don't want to slow
    // down the raw HTML parsing case by loading images we don't intend to display.
    Document* document = m_element->document();
    if (!document->renderer())
        return;

    AtomicString attr = m_element->imageSourceURL();

    if (!m_failedLoadURL.isEmpty() && attr == m_failedLoadURL)
        return;

    // Do not load any image if the 'src' attribute is missing or if it is
    // an empty string.
    CachedResourceHandle<CachedImage> newImage = 0;
    if (!attr.isNull() && !stripLeadingAndTrailingHTMLSpaces(attr).isEmpty()) {
        CachedResourceRequest request(ResourceRequest(document->completeURL(sourceURI(attr))));
        request.setInitiator(element());

        String crossOriginMode = m_element->fastGetAttribute(HTMLNames::crossoriginAttr);
        if (!crossOriginMode.isNull()) {
            StoredCredentials allowCredentials = equalIgnoringCase(crossOriginMode, "use-credentials") ? AllowStoredCredentials : DoNotAllowStoredCredentials;
            updateRequestForAccessControl(request.mutableResourceRequest(), document->securityOrigin(), allowCredentials);
        }

        if (m_loadManually) {
            bool autoLoadOtherImages = document->cachedResourceLoader()->autoLoadImages();
            document->cachedResourceLoader()->setAutoLoadImages(false);
            newImage = new CachedImage(request.resourceRequest());
            newImage->setLoading(true);
            newImage->setOwningCachedResourceLoader(document->cachedResourceLoader());
            document->cachedResourceLoader()->m_documentResources.set(newImage->url(), newImage.get());
            document->cachedResourceLoader()->setAutoLoadImages(autoLoadOtherImages);
        } else
            newImage = document->cachedResourceLoader()->requestImage(request);

        // If we do not have an image here, it means that a cross-site
        // violation occurred, or that the image was blocked via Content
        // Security Policy, or the page is being dismissed. Trigger an
        // error event if the page is not being dismissed.
        if (!newImage && !pageIsBeingDismissed(document)) {
            m_failedLoadURL = attr;
            m_hasPendingErrorEvent = true;
            errorEventSender().dispatchEventSoon(this);
        } else
            clearFailedLoadURL();
    } else if (!attr.isNull()) {
        // Fire an error event if the url is empty.
        // FIXME: Should we fire this event asynchronoulsy via errorEventSender()?
        m_element->dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    }
    
    CachedImage* oldImage = m_image.get();
    if (newImage != oldImage) {
        if (m_hasPendingBeforeLoadEvent) {
            beforeLoadEventSender().cancelEvent(this);
            m_hasPendingBeforeLoadEvent = false;
        }
        if (m_hasPendingLoadEvent) {
            loadEventSender().cancelEvent(this);
            m_hasPendingLoadEvent = false;
        }

        // Cancel error events that belong to the previous load, which is now cancelled by changing the src attribute.
        // If newImage is null and m_hasPendingErrorEvent is true, we know the error event has been just posted by
        // this load and we should not cancel the event.
        // FIXME: If both previous load and this one got blocked with an error, we can receive one error event instead of two.
        if (m_hasPendingErrorEvent && newImage) {
            errorEventSender().cancelEvent(this);
            m_hasPendingErrorEvent = false;
        }

        m_image = newImage;
        m_hasPendingBeforeLoadEvent = !m_element->document()->isImageDocument() && newImage;
        m_hasPendingLoadEvent = newImage;
        m_imageComplete = !newImage;

        if (newImage) {
            if (!m_element->document()->isImageDocument()) {
                if (!m_element->document()->hasListenerType(Document::BEFORELOAD_LISTENER))
                    dispatchPendingBeforeLoadEvent();
                else
                    beforeLoadEventSender().dispatchEventSoon(this);
            } else
                updateRenderer();

            // If newImage is cached, addClient() will result in the load event
            // being queued to fire. Ensure this happens after beforeload is
            // dispatched.
            newImage->addClient(this);
        } else {
            updateRenderer();
        }

        if (oldImage)
            oldImage->removeClient(this);
    }

    if (RenderImageResource* imageResource = renderImageResource())
        imageResource->resetAnimation();

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingEvent();
}

void ImageLoader::updateFromElementIgnoringPreviousError()
{
    clearFailedLoadURL();
    updateFromElement();
}

void ImageLoader::notifyFinished(CachedResource* resource)
{
    ASSERT(m_failedLoadURL.isEmpty());
    ASSERT(resource == m_image.get());

    m_imageComplete = true;
    if (!hasPendingBeforeLoadEvent())
        updateRenderer();

    if (!m_hasPendingLoadEvent)
        return;

    if (m_element->fastHasAttribute(HTMLNames::crossoriginAttr)
        && !m_element->document()->securityOrigin()->canRequest(image()->response().url())
        && !resource->passesAccessControlCheck(m_element->document()->securityOrigin())) {

        setImageWithoutConsideringPendingLoadEvent(0);

        m_hasPendingErrorEvent = true;
        errorEventSender().dispatchEventSoon(this);

        DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Cross-origin image load denied by Cross-Origin Resource Sharing policy.")));
        m_element->document()->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, consoleMessage);

        ASSERT(!m_hasPendingLoadEvent);

        // Only consider updating the protection ref-count of the Element immediately before returning
        // from this function as doing so might result in the destruction of this ImageLoader.
        updatedHasPendingEvent();
        return;
    }

    if (resource->wasCanceled()) {
        m_hasPendingLoadEvent = false;
        // Only consider updating the protection ref-count of the Element immediately before returning
        // from this function as doing so might result in the destruction of this ImageLoader.
        updatedHasPendingEvent();
        return;
    }

    loadEventSender().dispatchEventSoon(this);
}

RenderImageResource* ImageLoader::renderImageResource()
{
    RenderObject* renderer = m_element->renderer();

    if (!renderer)
        return 0;

    // We don't return style generated image because it doesn't belong to the ImageLoader.
    // See <https://bugs.webkit.org/show_bug.cgi?id=42840>
    if (renderer->isImage() && !static_cast<RenderImage*>(renderer)->isGeneratedContent())
        return toRenderImage(renderer)->imageResource();

#if ENABLE(SVG)
    if (renderer->isSVGImage())
        return toRenderSVGImage(renderer)->imageResource();
#endif

#if ENABLE(VIDEO)
    if (renderer->isVideo())
        return toRenderVideo(renderer)->imageResource();
#endif

    return 0;
}

void ImageLoader::updateRenderer()
{
    RenderImageResource* imageResource = renderImageResource();

    if (!imageResource)
        return;

    // Only update the renderer if it doesn't have an image or if what we have
    // is a complete image.  This prevents flickering in the case where a dynamic
    // change is happening between two images.
    CachedImage* cachedImage = imageResource->cachedImage();
    if (m_image != cachedImage && (m_imageComplete || !cachedImage))
        imageResource->setCachedImage(m_image.get());
}

void ImageLoader::updatedHasPendingEvent()
{
    // If an Element that does image loading is removed from the DOM the load/error event for the image is still observable.
    // As long as the ImageLoader is actively loading, the Element itself needs to be ref'ed to keep it from being
    // destroyed by DOM manipulation or garbage collection.
    // If such an Element wishes for the load to stop when removed from the DOM it needs to stop the ImageLoader explicitly.
    bool wasProtected = m_elementIsProtected;
    m_elementIsProtected = m_hasPendingLoadEvent || m_hasPendingErrorEvent;
    if (wasProtected == m_elementIsProtected)
        return;

    if (m_elementIsProtected) {
        if (m_derefElementTimer.isActive())
            m_derefElementTimer.stop();
        else
            m_element->ref();
    } else {
        ASSERT(!m_derefElementTimer.isActive());
        m_derefElementTimer.startOneShot(0);
    }   
}

void ImageLoader::timerFired(Timer<ImageLoader>*)
{
    m_element->deref();
}

void ImageLoader::dispatchPendingEvent(ImageEventSender* eventSender)
{
    ASSERT(eventSender == &beforeLoadEventSender() || eventSender == &loadEventSender() || eventSender == &errorEventSender());
    const AtomicString& eventType = eventSender->eventType();
    if (eventType == eventNames().beforeloadEvent)
        dispatchPendingBeforeLoadEvent();
    if (eventType == eventNames().loadEvent)
        dispatchPendingLoadEvent();
    if (eventType == eventNames().errorEvent)
        dispatchPendingErrorEvent();
}

void ImageLoader::dispatchPendingBeforeLoadEvent()
{
    if (!m_hasPendingBeforeLoadEvent)
        return;
    if (!m_image)
        return;
    if (!m_element->document()->attached())
        return;
    m_hasPendingBeforeLoadEvent = false;
    if (m_element->dispatchBeforeLoadEvent(m_image->url())) {
        updateRenderer();
        return;
    }
    if (m_image) {
        m_image->removeClient(this);
        m_image = 0;
    }

    loadEventSender().cancelEvent(this);
    m_hasPendingLoadEvent = false;
    
    if (m_element->hasTagName(HTMLNames::objectTag))
        static_cast<HTMLObjectElement*>(m_element)->renderFallbackContent();

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingEvent();
}

void ImageLoader::dispatchPendingLoadEvent()
{
    if (!m_hasPendingLoadEvent)
        return;
    if (!m_image)
        return;
    m_hasPendingLoadEvent = false;
    if (element()->document()->attached())
        dispatchLoadEvent();

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingEvent();
}

void ImageLoader::dispatchPendingErrorEvent()
{
    if (!m_hasPendingErrorEvent)
        return;
    m_hasPendingErrorEvent = false;
    if (element()->document()->attached())
        element()->dispatchEvent(Event::create(eventNames().errorEvent, false, false));

    // Only consider updating the protection ref-count of the Element immediately before returning
    // from this function as doing so might result in the destruction of this ImageLoader.
    updatedHasPendingEvent();
}

void ImageLoader::dispatchPendingBeforeLoadEvents()
{
    beforeLoadEventSender().dispatchPendingEvents();
}

void ImageLoader::dispatchPendingLoadEvents()
{
    loadEventSender().dispatchPendingEvents();
}

void ImageLoader::dispatchPendingErrorEvents()
{
    errorEventSender().dispatchPendingEvents();
}

void ImageLoader::elementDidMoveToNewDocument()
{
    clearFailedLoadURL();
    setImage(0);
}

inline void ImageLoader::clearFailedLoadURL()
{
    m_failedLoadURL = AtomicString();
}

}
