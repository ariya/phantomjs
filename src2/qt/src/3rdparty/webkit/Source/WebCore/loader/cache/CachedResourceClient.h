/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef CachedResourceClient_h
#define CachedResourceClient_h

#include <wtf/FastAllocBase.h>
#include <wtf/Forward.h>

namespace WebCore {

    class CachedCSSStyleSheet;
    class CachedFont;
    class CachedResource;
    class CachedImage;
    class Image;
    class IntRect;
    class KURL;

    /**
     * @internal
     *
     * a client who wants to load stylesheets, images or scripts from the web has to
     * inherit from this class and overload one of the 3 functions
     *
     */
    class CachedResourceClient {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        virtual ~CachedResourceClient() { }

        // Called whenever a frame of an image changes, either because we got more data from the network or
        // because we are animating. If not null, the IntRect is the changed rect of the image.
        virtual void imageChanged(CachedImage*, const IntRect* = 0) { };
        
        // Called to find out if this client wants to actually display the image.  Used to tell when we
        // can halt animation.  Content nodes that hold image refs for example would not render the image,
        // but RenderImages would (assuming they have visibility: visible and their render tree isn't hidden
        // e.g., in the b/f cache or in a background tab).
        virtual bool willRenderImage(CachedImage*) { return false; }

        virtual void setCSSStyleSheet(const String& /* href */, const KURL& /* baseURL */, const String& /* charset */, const CachedCSSStyleSheet*) { }
        virtual void setXSLStyleSheet(const String& /* href */, const KURL& /* baseURL */, const String& /* sheet */) { }
        virtual void fontLoaded(CachedFont*) {};
        virtual void notifyFinished(CachedResource*) { }
    };

}

#endif
