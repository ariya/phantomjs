/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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
*/

#ifndef CachedImage_h
#define CachedImage_h

#include "CachedResource.h"
#include "ImageObserver.h"
#include "IntRect.h"
#include "IntSizeHash.h"
#include "LayoutSize.h"
#include "SVGImageCache.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class CachedImageClient;
class CachedResourceLoader;
class FloatSize;
class MemoryCache;
class RenderObject;
struct Length;

class CachedImage FINAL : public CachedResource, public ImageObserver {
    friend class MemoryCache;

public:
    CachedImage(const ResourceRequest&);
    CachedImage(Image*);
    virtual ~CachedImage();

    Image* image(); // Returns the nullImage() if the image is not available yet.
    Image* imageForRenderer(const RenderObject*); // Returns the nullImage() if the image is not available yet.
    bool hasImage() const { return m_image.get(); }
    bool currentFrameKnownToBeOpaque(const RenderObject*); // Side effect: ensures decoded image is in cache, therefore should only be called when about to draw the image.

    std::pair<Image*, float> brokenImage(float deviceScaleFactor) const; // Returns an image and the image's resolution scale factor.
    bool willPaintBrokenImage() const; 

    bool canRender(const RenderObject* renderer, float multiplier) { return !errorOccurred() && !imageSizeForRenderer(renderer, multiplier).isEmpty(); }

    void setContainerSizeForRenderer(const CachedImageClient*, const IntSize&, float);
    bool usesImageContainerSize() const;
    bool imageHasRelativeWidth() const;
    bool imageHasRelativeHeight() const;

    virtual void addDataBuffer(ResourceBuffer*) OVERRIDE;
    virtual void finishLoading(ResourceBuffer*) OVERRIDE;

    // This method takes a zoom multiplier that can be used to increase the natural size of the image by the zoom.
    LayoutSize imageSizeForRenderer(const RenderObject*, float multiplier); // returns the size of the complete image.
    void computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio);

    static void resumeAnimatingImagesForLoader(CachedResourceLoader*);

private:
    virtual void load(CachedResourceLoader*, const ResourceLoaderOptions&) OVERRIDE;

    void clear();

    void createImage();
    void clearImage();
    bool canBeDrawn() const;
    // If not null, changeRect is the changed part of the image.
    void notifyObservers(const IntRect* changeRect = 0);
    virtual PurgePriority purgePriority() const OVERRIDE { return PurgeFirst; }
    void checkShouldPaintBrokenImage();

    virtual void switchClientsToRevalidatedResource() OVERRIDE;
    virtual bool mayTryReplaceEncodedData() const OVERRIDE { return true; }

    virtual void didAddClient(CachedResourceClient*) OVERRIDE;
    virtual void didRemoveClient(CachedResourceClient*) OVERRIDE;

    virtual void allClientsRemoved() OVERRIDE;
    virtual void destroyDecodedData() OVERRIDE;

    virtual void addData(const char* data, unsigned length) OVERRIDE;
    virtual void error(CachedResource::Status) OVERRIDE;
    virtual void responseReceived(const ResourceResponse&) OVERRIDE;

    // For compatibility, images keep loading even if there are HTTP errors.
    virtual bool shouldIgnoreHTTPStatusCodeErrors() const OVERRIDE { return true; }

    virtual bool isImage() const OVERRIDE { return true; }
    virtual bool stillNeedsLoad() const OVERRIDE { return !errorOccurred() && status() == Unknown && !isLoading(); }

    // ImageObserver
    virtual void decodedSizeChanged(const Image*, int delta) OVERRIDE;
    virtual void didDraw(const Image*) OVERRIDE;

    virtual bool shouldPauseAnimation(const Image*) OVERRIDE;
    virtual void animationAdvanced(const Image*) OVERRIDE;
    virtual void changedInRect(const Image*, const IntRect&) OVERRIDE;

    void addIncrementalDataBuffer(ResourceBuffer*);

    typedef pair<IntSize, float> SizeAndZoom;
    typedef HashMap<const CachedImageClient*, SizeAndZoom> ContainerSizeRequests;
    ContainerSizeRequests m_pendingContainerSizeRequests;

    RefPtr<Image> m_image;
#if ENABLE(SVG)
    OwnPtr<SVGImageCache> m_svgImageCache;
#endif
    bool m_shouldPaintBrokenImage;
};

}

#endif
