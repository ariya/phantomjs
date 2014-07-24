/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#ifndef SVGImageCache_h
#define SVGImageCache_h

#if ENABLE(SVG)
#include "FloatSize.h"
#include "Image.h"
#include "IntSize.h"
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CachedImage;
class CachedImageClient;
class ImageBuffer;
class SVGImage;
class SVGImageForContainer;
class RenderObject;

class SVGImageCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ~SVGImageCache();

    static PassOwnPtr<SVGImageCache> create(SVGImage* image)
    {
        return adoptPtr(new SVGImageCache(image));
    }

    void removeClientFromCache(const CachedImageClient*);

    void setContainerSizeForRenderer(const CachedImageClient*, const IntSize&, float);
    IntSize imageSizeForRenderer(const RenderObject*) const;

    Image* imageForRenderer(const RenderObject*);

private:
    SVGImageCache(SVGImage*);

    typedef HashMap<const CachedImageClient*, RefPtr<SVGImageForContainer> > ImageForContainerMap;

    SVGImage* m_svgImage;
    ImageForContainerMap m_imageForContainerMap;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGImageCache_h
