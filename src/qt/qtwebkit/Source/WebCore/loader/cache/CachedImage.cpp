/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
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

#include "config.h"
#include "CachedImage.h"

#include "BitmapImage.h"
#include "CachedImageClient.h"
#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "MemoryCache.h"
#include "Page.h"
#include "RenderObject.h"
#include "ResourceBuffer.h"
#include "Settings.h"
#include "SubresourceLoader.h"
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

#if USE(CG)
#include "PDFDocumentImage.h"
#endif

#if ENABLE(SVG)
#include "SVGImage.h"
#endif

using std::max;

namespace WebCore {

CachedImage::CachedImage(const ResourceRequest& resourceRequest)
    : CachedResource(resourceRequest, ImageResource)
    , m_image(0)
    , m_shouldPaintBrokenImage(true)
{
    setStatus(Unknown);
}

CachedImage::CachedImage(Image* image)
    : CachedResource(ResourceRequest(), ImageResource)
    , m_image(image)
    , m_shouldPaintBrokenImage(true)
{
    setStatus(Cached);
    setLoading(false);
}

CachedImage::~CachedImage()
{
    clearImage();
}

void CachedImage::load(CachedResourceLoader* cachedResourceLoader, const ResourceLoaderOptions& options)
{
    if (!cachedResourceLoader || cachedResourceLoader->autoLoadImages())
        CachedResource::load(cachedResourceLoader, options);
    else
        setLoading(false);
}

void CachedImage::didAddClient(CachedResourceClient* c)
{
    if (m_data && !m_image && !errorOccurred()) {
        createImage();
        m_image->setData(m_data->sharedBuffer(), true);
    }
    
    ASSERT(c->resourceClientType() == CachedImageClient::expectedType());
    if (m_image && !m_image->isNull())
        static_cast<CachedImageClient*>(c)->imageChanged(this);

    CachedResource::didAddClient(c);
}

void CachedImage::didRemoveClient(CachedResourceClient* c)
{
    ASSERT(c);
    ASSERT(c->resourceClientType() == CachedImageClient::expectedType());

    m_pendingContainerSizeRequests.remove(static_cast<CachedImageClient*>(c));
#if ENABLE(SVG)
    if (m_svgImageCache)
        m_svgImageCache->removeClientFromCache(static_cast<CachedImageClient*>(c));
#endif

    CachedResource::didRemoveClient(c);
}

void CachedImage::switchClientsToRevalidatedResource()
{
    ASSERT(resourceToRevalidate());
    ASSERT(resourceToRevalidate()->isImage());
    // Pending container size requests need to be transferred to the revalidated resource.
    if (!m_pendingContainerSizeRequests.isEmpty()) {
        // A copy of pending size requests is needed as they are deleted during CachedResource::switchClientsToRevalidateResouce().
        ContainerSizeRequests switchContainerSizeRequests;
        for (ContainerSizeRequests::iterator it = m_pendingContainerSizeRequests.begin(); it != m_pendingContainerSizeRequests.end(); ++it)
            switchContainerSizeRequests.set(it->key, it->value);
        CachedResource::switchClientsToRevalidatedResource();
        CachedImage* revalidatedCachedImage = static_cast<CachedImage*>(resourceToRevalidate());
        for (ContainerSizeRequests::iterator it = switchContainerSizeRequests.begin(); it != switchContainerSizeRequests.end(); ++it)
            revalidatedCachedImage->setContainerSizeForRenderer(it->key, it->value.first, it->value.second);
        return;
    }

    CachedResource::switchClientsToRevalidatedResource();
}

void CachedImage::allClientsRemoved()
{
    m_pendingContainerSizeRequests.clear();
    if (m_image && !errorOccurred())
        m_image->resetAnimation();
}

pair<Image*, float> CachedImage::brokenImage(float deviceScaleFactor) const
{
    if (deviceScaleFactor >= 2) {
        DEFINE_STATIC_LOCAL(Image*, brokenImageHiRes, (Image::loadPlatformResource("missingImage@2x").leakRef()));
        return std::make_pair(brokenImageHiRes, 2);
    }

    DEFINE_STATIC_LOCAL(Image*, brokenImageLoRes, (Image::loadPlatformResource("missingImage").leakRef()));
    return std::make_pair(brokenImageLoRes, 1);
}

bool CachedImage::willPaintBrokenImage() const
{
    return errorOccurred() && m_shouldPaintBrokenImage;
}

Image* CachedImage::image()
{
    ASSERT(!isPurgeable());

    if (errorOccurred() && m_shouldPaintBrokenImage) {
        // Returning the 1x broken image is non-ideal, but we cannot reliably access the appropriate
        // deviceScaleFactor from here. It is critical that callers use CachedImage::brokenImage() 
        // when they need the real, deviceScaleFactor-appropriate broken image icon. 
        return brokenImage(1).first;
    }

    if (m_image)
        return m_image.get();

    return Image::nullImage();
}

Image* CachedImage::imageForRenderer(const RenderObject* renderer)
{
    ASSERT(!isPurgeable());

    if (errorOccurred() && m_shouldPaintBrokenImage) {
        // Returning the 1x broken image is non-ideal, but we cannot reliably access the appropriate
        // deviceScaleFactor from here. It is critical that callers use CachedImage::brokenImage() 
        // when they need the real, deviceScaleFactor-appropriate broken image icon. 
        return brokenImage(1).first;
    }

    if (!m_image)
        return Image::nullImage();

#if ENABLE(SVG)
    if (m_image->isSVGImage()) {
        Image* image = m_svgImageCache->imageForRenderer(renderer);
        if (image != Image::nullImage())
            return image;
    }
#else
    UNUSED_PARAM(renderer);
#endif
    return m_image.get();
}

void CachedImage::setContainerSizeForRenderer(const CachedImageClient* renderer, const IntSize& containerSize, float containerZoom)
{
    if (containerSize.isEmpty())
        return;
    ASSERT(renderer);
    ASSERT(containerZoom);
    if (!m_image) {
        m_pendingContainerSizeRequests.set(renderer, SizeAndZoom(containerSize, containerZoom));
        return;
    }
#if ENABLE(SVG)
    if (!m_image->isSVGImage()) {
        m_image->setContainerSize(containerSize);
        return;
    }

    m_svgImageCache->setContainerSizeForRenderer(renderer, containerSize, containerZoom);
#else
    UNUSED_PARAM(containerZoom);
    m_image->setContainerSize(containerSize);
#endif
}

bool CachedImage::usesImageContainerSize() const
{
    if (m_image)
        return m_image->usesContainerSize();

    return false;
}

bool CachedImage::imageHasRelativeWidth() const
{
    if (m_image)
        return m_image->hasRelativeWidth();

    return false;
}

bool CachedImage::imageHasRelativeHeight() const
{
    if (m_image)
        return m_image->hasRelativeHeight();

    return false;
}

LayoutSize CachedImage::imageSizeForRenderer(const RenderObject* renderer, float multiplier)
{
    ASSERT(!isPurgeable());

    if (!m_image)
        return IntSize();

    LayoutSize imageSize;

    if (m_image->isBitmapImage() && (renderer && renderer->shouldRespectImageOrientation() == RespectImageOrientation))
        imageSize = static_cast<BitmapImage*>(m_image.get())->sizeRespectingOrientation();
#if ENABLE(SVG)
    else if (m_image->isSVGImage()) {
        imageSize = m_svgImageCache->imageSizeForRenderer(renderer);
    }
#endif
    else
        imageSize = m_image->size();

    if (multiplier == 1.0f)
        return imageSize;
        
    // Don't let images that have a width/height >= 1 shrink below 1 when zoomed.
    float widthScale = m_image->hasRelativeWidth() ? 1.0f : multiplier;
    float heightScale = m_image->hasRelativeHeight() ? 1.0f : multiplier;
    LayoutSize minimumSize(imageSize.width() > 0 ? 1 : 0, imageSize.height() > 0 ? 1 : 0);
    imageSize.scale(widthScale, heightScale);
    imageSize.clampToMinimumSize(minimumSize);
    ASSERT(multiplier != 1.0f || (imageSize.width().fraction() == 0.0f && imageSize.height().fraction() == 0.0f));
    return imageSize;
}

void CachedImage::computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    if (m_image)
        m_image->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

void CachedImage::notifyObservers(const IntRect* changeRect)
{
    CachedResourceClientWalker<CachedImageClient> w(m_clients);
    while (CachedImageClient* c = w.next())
        c->imageChanged(this, changeRect);
}

void CachedImage::checkShouldPaintBrokenImage()
{
    if (!m_loader || m_loader->reachedTerminalState())
        return;

    m_shouldPaintBrokenImage = m_loader->frameLoader()->client()->shouldPaintBrokenImage(m_resourceRequest.url());
}

void CachedImage::clear()
{
    destroyDecodedData();
    clearImage();
    m_pendingContainerSizeRequests.clear();
    setEncodedSize(0);
}

inline void CachedImage::createImage()
{
    // Create the image if it doesn't yet exist.
    if (m_image)
        return;
#if USE(CG) && !USE(WEBKIT_IMAGE_DECODERS)
    else if (m_response.mimeType() == "application/pdf")
        m_image = PDFDocumentImage::create();
#endif
#if ENABLE(SVG)
    else if (m_response.mimeType() == "image/svg+xml") {
        RefPtr<SVGImage> svgImage = SVGImage::create(this);
        m_svgImageCache = SVGImageCache::create(svgImage.get());
        m_image = svgImage.release();
    }
#endif
    else
        m_image = BitmapImage::create(this);

    if (m_image) {
        // Send queued container size requests.
        if (m_image->usesContainerSize()) {
            for (ContainerSizeRequests::iterator it = m_pendingContainerSizeRequests.begin(); it != m_pendingContainerSizeRequests.end(); ++it)
                setContainerSizeForRenderer(it->key, it->value.first, it->value.second);
        }
        m_pendingContainerSizeRequests.clear();
    }
}

inline void CachedImage::clearImage()
{
    // If our Image has an observer, it's always us so we need to clear the back pointer
    // before dropping our reference.
    if (m_image)
        m_image->setImageObserver(0);
    m_image.clear();
}

bool CachedImage::canBeDrawn() const
{
    if (!m_image || m_image->isNull())
        return false;

    if (!m_loader || m_loader->reachedTerminalState())
        return true;

    Settings* settings = m_loader->frameLoader()->frame()->settings();
    if (!settings)
        return true;

    size_t estimatedDecodedImageSize = m_image->width() * m_image->height() * 4; // no overflow check
    return estimatedDecodedImageSize <= settings->maximumDecodedImageSize();
}

void CachedImage::addIncrementalDataBuffer(ResourceBuffer* data)
{
    m_data = data;
    if (!data)
        return;

    createImage();

    // Have the image update its data from its internal buffer.
    // It will not do anything now, but will delay decoding until
    // queried for info (like size or specific image frames).
    bool sizeAvailable = m_image->setData(m_data->sharedBuffer(), false);
    if (!sizeAvailable)
        return;

    if (!canBeDrawn()) {
        // There's no image to draw or its decoded size is bigger than the maximum allowed.
        error(errorOccurred() ? status() : DecodeError);
        if (inCache())
            memoryCache()->remove(this);
        return;
    }

    // Go ahead and tell our observers to try to draw.
    // Each chunk from the network causes observers to repaint, which will
    // force that chunk to decode.
    // It would be nice to only redraw the decoded band of the image, but with the current design
    // (decoding delayed until painting) that seems hard.
    notifyObservers();

    setEncodedSize(m_image->data() ? m_image->data()->size() : 0);
}

void CachedImage::addDataBuffer(ResourceBuffer* data)
{
    ASSERT(m_options.dataBufferingPolicy == BufferData);
    addIncrementalDataBuffer(data);
}

void CachedImage::addData(const char* data, unsigned length)
{
    ASSERT(m_options.dataBufferingPolicy == DoNotBufferData);
    addIncrementalDataBuffer(ResourceBuffer::create(data, length).get());
}

void CachedImage::finishLoading(ResourceBuffer* data)
{
    m_data = data;
    if (!m_image && data)
        createImage();

    if (m_image)
        m_image->setData(m_data->sharedBuffer(), true);

    if (!canBeDrawn()) {
        // There's no image to draw or its decoded size is bigger than the maximum allowed.
        error(errorOccurred() ? status() : DecodeError);
        if (inCache())
            memoryCache()->remove(this);
        return;
    }

    notifyObservers();
    if (m_image)
        setEncodedSize(m_image->data() ? m_image->data()->size() : 0);
    CachedResource::finishLoading(data);
}

void CachedImage::error(CachedResource::Status status)
{
    checkShouldPaintBrokenImage();
    clear();
    CachedResource::error(status);
    notifyObservers();
}

void CachedImage::responseReceived(const ResourceResponse& response)
{
    if (!m_response.isNull())
        clear();
    CachedResource::responseReceived(response);
}

void CachedImage::destroyDecodedData()
{
    bool canDeleteImage = !m_image || (m_image->hasOneRef() && m_image->isBitmapImage());
    if (isSafeToMakePurgeable() && canDeleteImage && !isLoading()) {
        // Image refs the data buffer so we should not make it purgeable while the image is alive. 
        // Invoking addClient() will reconstruct the image object.
        m_image = 0;
        setDecodedSize(0);
        if (!MemoryCache::shouldMakeResourcePurgeableOnEviction())
            makePurgeable(true);
    } else if (m_image && !errorOccurred())
        m_image->destroyDecodedData();
}

void CachedImage::decodedSizeChanged(const Image* image, int delta)
{
    if (!image || image != m_image)
        return;
    
    setDecodedSize(decodedSize() + delta);
}

void CachedImage::didDraw(const Image* image)
{
    if (!image || image != m_image)
        return;
    
    double timeStamp = FrameView::currentPaintTimeStamp();
    if (!timeStamp) // If didDraw is called outside of a Frame paint.
        timeStamp = currentTime();
    
    CachedResource::didAccessDecodedData(timeStamp);
}

bool CachedImage::shouldPauseAnimation(const Image* image)
{
    if (!image || image != m_image)
        return false;
    
    CachedResourceClientWalker<CachedImageClient> w(m_clients);
    while (CachedImageClient* c = w.next()) {
        if (c->willRenderImage(this))
            return false;
    }

    return true;
}

void CachedImage::animationAdvanced(const Image* image)
{
    if (!image || image != m_image)
        return;
    notifyObservers();
}

void CachedImage::changedInRect(const Image* image, const IntRect& rect)
{
    if (!image || image != m_image)
        return;
    notifyObservers(&rect);
}

void CachedImage::resumeAnimatingImagesForLoader(CachedResourceLoader* loader)
{
    const CachedResourceLoader::DocumentResourceMap& resources = loader->allCachedResources();

    for (CachedResourceLoader::DocumentResourceMap::const_iterator it = resources.begin(), end = resources.end(); it != end; ++it) {
        const CachedResourceHandle<CachedResource>& resource = it->value;
        if (!resource || !resource->isImage())
            continue;
        CachedImage* cachedImage = static_cast<CachedImage*>(resource.get());
        if (!cachedImage->hasImage())
            continue;
        Image* image = cachedImage->image();
        if (!image->isBitmapImage())
            continue;
        BitmapImage* bitmapImage = static_cast<BitmapImage*>(image);
        if (!bitmapImage->canAnimate())
            continue;
        cachedImage->animationAdvanced(bitmapImage);
    }
}

bool CachedImage::currentFrameKnownToBeOpaque(const RenderObject* renderer)
{
    Image* image = imageForRenderer(renderer);
    if (image->isBitmapImage())
        image->nativeImageForCurrentFrame(); // force decode
    return image->currentFrameKnownToBeOpaque();
}

} // namespace WebCore
