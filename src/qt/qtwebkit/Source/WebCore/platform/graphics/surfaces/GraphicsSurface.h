/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef GraphicsSurface_h
#define GraphicsSurface_h

#if USE(GRAPHICS_SURFACE)

#include "GraphicsContext.h"
#include "GraphicsContext3D.h"
#include "GraphicsSurfaceToken.h"
#include "IntRect.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if OS(DARWIN)
typedef struct __IOSurface* IOSurfaceRef;
typedef IOSurfaceRef PlatformGraphicsSurface;
#endif

#if OS(LINUX)
typedef uint32_t PlatformGraphicsSurface;
#endif

#if OS(WINDOWS)
typedef HANDLE PlatformGraphicsSurface;
#endif

namespace WebCore {

class BitmapTexture;
class TextureMapper;
struct GraphicsSurfacePrivate;

class GraphicsSurface : public RefCounted<GraphicsSurface> {
public:
    enum Flag {
        SupportsAlpha = 0x01,
        SupportsSoftwareWrite = 0x02,
        SupportsSoftwareRead = 0x04,
        SupportsTextureTarget = 0x08,
        SupportsTextureSource = 0x10,
        SupportsCopyToTexture = 0x20,
        SupportsCopyFromTexture = 0x40,
        SupportsSharing = 0x80,
        SupportsSingleBuffered = 0x100
    };

    enum LockOption {
        RetainPixels = 0x01,
        ReadOnly = 0x02
    };

    typedef int Flags;
    typedef int LockOptions;

    Flags flags() const { return m_flags; }
    IntSize size() const;

    static PassRefPtr<GraphicsSurface> create(const IntSize&, Flags, const PlatformGraphicsContext3D shareContext = 0);
    static PassRefPtr<GraphicsSurface> create(const IntSize&, Flags, const GraphicsSurfaceToken&);
    void copyToGLTexture(uint32_t target, uint32_t texture, const IntRect& targetRect, const IntPoint& sourceOffset);
    void copyFromTexture(uint32_t texture, const IntRect& sourceRect);
    void paintToTextureMapper(TextureMapper*, const FloatRect& targetRect, const TransformationMatrix&, float opacity);
    uint32_t frontBuffer();
    uint32_t swapBuffers();
    GraphicsSurfaceToken exportToken();
    uint32_t getTextureID();
    PassOwnPtr<GraphicsContext> beginPaint(const IntRect&, LockOptions);
    PassRefPtr<Image> createReadOnlyImage(const IntRect&);
    ~GraphicsSurface();

protected:
    static PassRefPtr<GraphicsSurface> platformCreate(const IntSize&, Flags, const PlatformGraphicsContext3D);
    static PassRefPtr<GraphicsSurface> platformImport(const IntSize&, Flags, const GraphicsSurfaceToken&);
    GraphicsSurfaceToken platformExport();
    void platformDestroy();

    uint32_t platformGetTextureID();
    char* platformLock(const IntRect&, int* stride, LockOptions);
    void platformUnlock();
    void platformCopyToGLTexture(uint32_t target, uint32_t texture, const IntRect&, const IntPoint&);
    void platformCopyFromTexture(uint32_t texture, const IntRect& sourceRect);
    void platformPaintToTextureMapper(TextureMapper*, const FloatRect& targetRect, const TransformationMatrix&, float opacity);
    uint32_t platformFrontBuffer() const;
    uint32_t platformSwapBuffers();
    IntSize platformSize() const;

    PassOwnPtr<GraphicsContext> platformBeginPaint(const IntSize&, char* bits, int stride);

protected:
    LockOptions m_lockOptions;
    Flags m_flags;

private:
    GraphicsSurface(const IntSize&, Flags);

#if PLATFORM(QT)
    static void didReleaseImage(void*);
#endif

private:
    uint32_t m_fbo;
    GraphicsSurfacePrivate* m_private;
};

}
#endif // USE(GRAPHICS_SURFACE)

#endif // GraphicsSurface_h
