/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2009 Apple Inc. All rights reserved.
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
 *
 */

#ifndef ImageLoader_h
#define ImageLoader_h

#include "CachedImageClient.h"
#include "CachedResourceHandle.h"
#include "Timer.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Element;
class ImageLoader;
class RenderImageResource;

template<typename T> class EventSender;
typedef EventSender<ImageLoader> ImageEventSender;

class ImageLoader : public CachedImageClient {
public:
    explicit ImageLoader(Element*);
    virtual ~ImageLoader();

    // This function should be called when the element is attached to a document; starts
    // loading if a load hasn't already been started.
    void updateFromElement();

    // This function should be called whenever the 'src' attribute is set, even if its value
    // doesn't change; starts new load unconditionally (matches Firefox and Opera behavior).
    void updateFromElementIgnoringPreviousError();

    void elementDidMoveToNewDocument();

    Element* element() const { return m_element; }
    bool imageComplete() const { return m_imageComplete; }

    CachedImage* image() const { return m_image.get(); }
    void setImage(CachedImage*); // Cancels pending beforeload and load events, and doesn't dispatch new ones.

    void setLoadManually(bool loadManually) { m_loadManually = loadManually; }

    bool hasPendingBeforeLoadEvent() const { return m_hasPendingBeforeLoadEvent; }
    bool hasPendingActivity() const { return m_hasPendingLoadEvent || m_hasPendingErrorEvent; }

    void dispatchPendingEvent(ImageEventSender*);

    static void dispatchPendingBeforeLoadEvents();
    static void dispatchPendingLoadEvents();
    static void dispatchPendingErrorEvents();

protected:
    virtual void notifyFinished(CachedResource*);

private:
    virtual void dispatchLoadEvent() = 0;
    virtual String sourceURI(const AtomicString&) const = 0;

    void updatedHasPendingEvent();

    void dispatchPendingBeforeLoadEvent();
    void dispatchPendingLoadEvent();
    void dispatchPendingErrorEvent();

    RenderImageResource* renderImageResource();
    void updateRenderer();

    void setImageWithoutConsideringPendingLoadEvent(CachedImage*);
    void clearFailedLoadURL();

    void timerFired(Timer<ImageLoader>*);

    Element* m_element;
    CachedResourceHandle<CachedImage> m_image;
    Timer<ImageLoader> m_derefElementTimer;
    AtomicString m_failedLoadURL;
    bool m_hasPendingBeforeLoadEvent : 1;
    bool m_hasPendingLoadEvent : 1;
    bool m_hasPendingErrorEvent : 1;
    bool m_imageComplete : 1;
    bool m_loadManually : 1;
    bool m_elementIsProtected : 1;
};

}

#endif
