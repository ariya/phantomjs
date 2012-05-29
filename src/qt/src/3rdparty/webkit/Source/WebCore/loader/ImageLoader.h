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

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Element;
class ImageLoadEventSender;
class RenderImageResource;

class ImageLoader : public CachedResourceClient {
public:
    ImageLoader(Element*);
    virtual ~ImageLoader();

    // This function should be called when the element is attached to a document; starts
    // loading if a load hasn't already been started.
    void updateFromElement();

    // This function should be called whenever the 'src' attribute is set, even if its value
    // doesn't change; starts new load unconditionally (matches Firefox and Opera behavior).
    void updateFromElementIgnoringPreviousError();

    void elementWillMoveToNewOwnerDocument();

    Element* element() const { return m_element; }
    bool imageComplete() const { return m_imageComplete; }

    CachedImage* image() const { return m_image.get(); }
    void setImage(CachedImage*); // Cancels pending beforeload and load events, and doesn't dispatch new ones.

    void setLoadManually(bool loadManually) { m_loadManually = loadManually; }

    bool haveFiredBeforeLoadEvent() const { return m_firedBeforeLoad; }
    bool haveFiredLoadEvent() const { return m_firedLoad; }

    static void dispatchPendingBeforeLoadEvents();
    static void dispatchPendingLoadEvents();

protected:
    virtual void notifyFinished(CachedResource*);

private:
    virtual void dispatchLoadEvent() = 0;
    virtual String sourceURI(const AtomicString&) const = 0;

    friend class ImageEventSender;
    void dispatchPendingBeforeLoadEvent();
    void dispatchPendingLoadEvent();

    RenderImageResource* renderImageResource();
    void updateRenderer();

    Element* m_element;
    CachedResourceHandle<CachedImage> m_image;
    AtomicString m_failedLoadURL;
    bool m_firedBeforeLoad : 1;
    bool m_firedLoad : 1;
    bool m_imageComplete : 1;
    bool m_loadManually : 1;
};

}

#endif
