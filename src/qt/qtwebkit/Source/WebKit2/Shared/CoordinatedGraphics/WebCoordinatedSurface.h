/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Company 100, Inc.

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

#ifndef WebCoordinatedSurface_h
#define WebCoordinatedSurface_h

#if USE(COORDINATED_GRAPHICS)
#include "ShareableBitmap.h"
#include <WebCore/CoordinatedSurface.h>

#if USE(GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

namespace WebCore {
class BitmapTexture;
class GraphicsContext;
}

namespace WebKit {

class WebCoordinatedSurface : public WebCore::CoordinatedSurface {
public:
    class Handle {
        WTF_MAKE_NONCOPYABLE(Handle);
    public:
        Handle();

        void encode(CoreIPC::ArgumentEncoder&) const;
        static bool decode(CoreIPC::ArgumentDecoder&, Handle&);

#if USE(GRAPHICS_SURFACE)
        WebCore::GraphicsSurfaceToken graphicsSurfaceToken() const { return m_graphicsSurfaceToken; }
#endif

    private:
        friend class WebCoordinatedSurface;
        mutable ShareableBitmap::Handle m_bitmapHandle;
#if USE(GRAPHICS_SURFACE)
        WebCore::GraphicsSurfaceToken m_graphicsSurfaceToken;
#endif
        WebCore::IntSize m_size;
        WebCore::CoordinatedSurface::Flags m_flags;
    };

    // Create a new WebCoordinatedSurface, and allocate either a GraphicsSurface or a ShareableBitmap as backing.
    static PassRefPtr<WebCoordinatedSurface> create(const WebCore::IntSize&, Flags);

    // Create a shareable surface from a handle.
    static PassRefPtr<WebCoordinatedSurface> create(const Handle&);

    // Create a handle.
    bool createHandle(Handle&);

    virtual ~WebCoordinatedSurface();

    virtual void paintToSurface(const WebCore::IntRect&, WebCore::CoordinatedSurface::Client*) OVERRIDE;

#if USE(TEXTURE_MAPPER)
    virtual void copyToTexture(PassRefPtr<WebCore::BitmapTexture>, const WebCore::IntRect& target, const WebCore::IntPoint& sourceOffset) OVERRIDE;
#endif

private:
    WebCoordinatedSurface(const WebCore::IntSize&, Flags, PassRefPtr<ShareableBitmap>);

    // Create a WebCoordinatedSurface referencing an existing ShareableBitmap.
    static PassRefPtr<WebCoordinatedSurface> create(const WebCore::IntSize&, Flags, PassRefPtr<ShareableBitmap>);

    PassOwnPtr<WebCore::GraphicsContext> createGraphicsContext(const WebCore::IntRect&);
#if USE(GRAPHICS_SURFACE)
    WebCoordinatedSurface(const WebCore::IntSize&, Flags, PassRefPtr<WebCore::GraphicsSurface>);
    // Create a shareable bitmap backed by a graphics surface.
    static PassRefPtr<WebCoordinatedSurface> createWithSurface(const WebCore::IntSize&, Flags);
    // Create a WebCoordinatedSurface referencing an existing GraphicsSurface.
    static PassRefPtr<WebCoordinatedSurface> create(const WebCore::IntSize&, Flags, PassRefPtr<WebCore::GraphicsSurface>);

    bool isBackedByGraphicsSurface() const { return !!m_graphicsSurface; }
#endif

    RefPtr<ShareableBitmap> m_bitmap;

#if USE(GRAPHICS_SURFACE)
    RefPtr<WebCore::GraphicsSurface> m_graphicsSurface;
#endif
};

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
#endif // WebCoordinatedSurface_h
