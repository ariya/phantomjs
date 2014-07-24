/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ImageBuffer_h
#define ImageBuffer_h

#include "AffineTransform.h"
#include "ColorSpace.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#if USE(ACCELERATED_COMPOSITING)
#include "PlatformLayer.h"
#endif
#include "GraphicsTypes.h"
#include "GraphicsTypes3D.h"
#include "IntSize.h"
#include "ImageBufferData.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Uint8ClampedArray.h>
#include <wtf/Vector.h>

namespace WebCore {

    class Image;
    class ImageData;
    class IntPoint;
    class IntRect;
    class GraphicsContext3D;

    enum Multiply {
        Premultiplied,
        Unmultiplied
    };

    enum RenderingMode {
        Unaccelerated,
        UnacceleratedNonPlatformBuffer, // Use plain memory allocation rather than platform API to allocate backing store.
        Accelerated
    };
    
    enum BackingStoreCopy {
        CopyBackingStore, // Guarantee subsequent draws don't affect the copy.
        DontCopyBackingStore // Subsequent draws may affect the copy.
    };

    enum ScaleBehavior {
        Scaled,
        Unscaled
    };

    class ImageBuffer {
        WTF_MAKE_NONCOPYABLE(ImageBuffer); WTF_MAKE_FAST_ALLOCATED;
    public:
        // Will return a null pointer on allocation failure.
        static PassOwnPtr<ImageBuffer> create(const IntSize& size, float resolutionScale = 1, ColorSpace colorSpace = ColorSpaceDeviceRGB, RenderingMode renderingMode = Unaccelerated)
        {
            bool success = false;
            OwnPtr<ImageBuffer> buf = adoptPtr(new ImageBuffer(size, resolutionScale, colorSpace, renderingMode, success));
            if (!success)
                return nullptr;
            return buf.release();
        }

        static PassOwnPtr<ImageBuffer> createCompatibleBuffer(const IntSize&, float resolutionScale, ColorSpace, const GraphicsContext*, bool hasAlpha);

        ~ImageBuffer();

        // The actual resolution of the backing store
        const IntSize& internalSize() const { return m_size; }
        const IntSize& logicalSize() const { return m_logicalSize; }

        GraphicsContext* context() const;

        PassRefPtr<Image> copyImage(BackingStoreCopy = CopyBackingStore, ScaleBehavior = Scaled) const;
        // Give hints on the faster copyImage Mode, return DontCopyBackingStore if it supports the DontCopyBackingStore behavior
        // or return CopyBackingStore if it doesn't.  
        static BackingStoreCopy fastCopyImageMode();

        enum CoordinateSystem { LogicalCoordinateSystem, BackingStoreCoordinateSystem };

        PassRefPtr<Uint8ClampedArray> getUnmultipliedImageData(const IntRect&, CoordinateSystem = LogicalCoordinateSystem) const;
        PassRefPtr<Uint8ClampedArray> getPremultipliedImageData(const IntRect&, CoordinateSystem = LogicalCoordinateSystem) const;

        void putByteArray(Multiply multiplied, Uint8ClampedArray*, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem = LogicalCoordinateSystem);
        
        void convertToLuminanceMask();
        
        String toDataURL(const String& mimeType, const double* quality = 0, CoordinateSystem = LogicalCoordinateSystem) const;
#if !USE(CG)
        AffineTransform baseTransform() const { return AffineTransform(); }
        void transformColorSpace(ColorSpace srcColorSpace, ColorSpace dstColorSpace);
        void platformTransformColorSpace(const Vector<int>&);
#else
        AffineTransform baseTransform() const { return AffineTransform(1, 0, 0, -1, 0, internalSize().height()); }
#endif
#if USE(ACCELERATED_COMPOSITING)
        PlatformLayer* platformLayer() const;
#endif

        // FIXME: current implementations of this method have the restriction that they only work
        // with textures that are RGB or RGBA format, and UNSIGNED_BYTE type.
        bool copyToPlatformTexture(GraphicsContext3D&, Platform3DObject, GC3Denum, bool, bool);

    private:
#if USE(CG)
        PassNativeImagePtr copyNativeImage(BackingStoreCopy = CopyBackingStore) const;
        void flushContext() const;
        void flushContextIfNecessary() const;
#endif
        void clip(GraphicsContext*, const FloatRect&) const;

        void draw(GraphicsContext*, ColorSpace, const FloatRect& destRect, const FloatRect& srcRect = FloatRect(0, 0, -1, -1), CompositeOperator = CompositeSourceOver, BlendMode = BlendModeNormal, bool useLowQualityScale = false);
        void drawPattern(GraphicsContext*, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator, const FloatRect& destRect);

        inline void genericConvertToLuminanceMask();

        friend class GraphicsContext;
        friend class GeneratedImage;
        friend class CrossfadeGeneratedImage;
        friend class GeneratorGeneratedImage;

    private:
        ImageBufferData m_data;
        IntSize m_size;
        IntSize m_logicalSize;
        float m_resolutionScale;
        OwnPtr<GraphicsContext> m_context;

        // This constructor will place its success into the given out-variable
        // so that create() knows when it should return failure.
        ImageBuffer(const IntSize&, float resolutionScale, ColorSpace, RenderingMode, bool& success);
    };

#if USE(CG)
    String ImageDataToDataURL(const ImageData&, const String& mimeType, const double* quality);
#endif

} // namespace WebCore

#endif // ImageBuffer_h
