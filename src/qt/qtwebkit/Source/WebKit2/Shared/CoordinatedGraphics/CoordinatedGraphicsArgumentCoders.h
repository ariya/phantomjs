/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Company 100, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CoordinatedGraphicsArgumentCoders_h
#define CoordinatedGraphicsArgumentCoders_h

#if USE(COORDINATED_GRAPHICS)
#include "ArgumentCoders.h"

namespace WebCore {
struct CoordinatedGraphicsLayerState;
struct CoordinatedGraphicsState;
class FloatPoint3D;
class GraphicsLayerAnimation;
class GraphicsLayerAnimations;
class SurfaceUpdateInfo;
struct TileCreationInfo;
struct TileUpdateInfo;
class TransformationMatrix;
class TransformOperations;
struct Length;

#if ENABLE(CSS_FILTERS)
class FilterOperations;
#endif

#if ENABLE(CSS_SHADERS)
class CustomFilterProgramInfo;
#endif

#if USE(GRAPHICS_SURFACE)
struct GraphicsSurfaceToken;
#endif
}

namespace CoreIPC {

template<> struct ArgumentCoder<WebCore::FloatPoint3D> {
    static void encode(ArgumentEncoder&, const WebCore::FloatPoint3D&);
    static bool decode(ArgumentDecoder&, WebCore::FloatPoint3D&);
};

template<> struct ArgumentCoder<WebCore::Length> {
    static void encode(ArgumentEncoder&, const WebCore::Length&);
    static bool decode(ArgumentDecoder&, WebCore::Length&);
};

template<> struct ArgumentCoder<WebCore::TransformationMatrix> {
    static void encode(ArgumentEncoder&, const WebCore::TransformationMatrix&);
    static bool decode(ArgumentDecoder&, WebCore::TransformationMatrix&);
};

#if ENABLE(CSS_FILTERS)
template<> struct ArgumentCoder<WebCore::FilterOperations> {
    static void encode(ArgumentEncoder&, const WebCore::FilterOperations&);
    static bool decode(ArgumentDecoder&, WebCore::FilterOperations&);
};
#endif

#if ENABLE(CSS_SHADERS)
template<> struct ArgumentCoder<WebCore::CustomFilterProgramInfo> {
    static void encode(ArgumentEncoder&, const WebCore::CustomFilterProgramInfo&);
    static bool decode(ArgumentDecoder&, WebCore::CustomFilterProgramInfo&);
};
#endif

template<> struct ArgumentCoder<WebCore::TransformOperations> {
    static void encode(ArgumentEncoder&, const WebCore::TransformOperations&);
    static bool decode(ArgumentDecoder&, WebCore::TransformOperations&);
};

template<> struct ArgumentCoder<WebCore::GraphicsLayerAnimations> {
    static void encode(ArgumentEncoder&, const WebCore::GraphicsLayerAnimations&);
    static bool decode(ArgumentDecoder&, WebCore::GraphicsLayerAnimations&);
};

template<> struct ArgumentCoder<WebCore::GraphicsLayerAnimation> {
    static void encode(ArgumentEncoder&, const WebCore::GraphicsLayerAnimation&);
    static bool decode(ArgumentDecoder&, WebCore::GraphicsLayerAnimation&);
};

#if USE(GRAPHICS_SURFACE)
template<> struct ArgumentCoder<WebCore::GraphicsSurfaceToken> {
    static void encode(ArgumentEncoder&, const WebCore::GraphicsSurfaceToken&);
    static bool decode(ArgumentDecoder&, WebCore::GraphicsSurfaceToken&);
};
#endif

template<> struct ArgumentCoder<WebCore::SurfaceUpdateInfo> {
    static void encode(ArgumentEncoder&, const WebCore::SurfaceUpdateInfo&);
    static bool decode(ArgumentDecoder&, WebCore::SurfaceUpdateInfo&);
};

template<> struct ArgumentCoder<WebCore::CoordinatedGraphicsLayerState> {
    static void encode(ArgumentEncoder&, const WebCore::CoordinatedGraphicsLayerState&);
    static bool decode(ArgumentDecoder&, WebCore::CoordinatedGraphicsLayerState&);
};

template<> struct ArgumentCoder<WebCore::TileUpdateInfo> {
    static void encode(ArgumentEncoder&, const WebCore::TileUpdateInfo&);
    static bool decode(ArgumentDecoder&, WebCore::TileUpdateInfo&);
};

template<> struct ArgumentCoder<WebCore::TileCreationInfo> {
    static void encode(ArgumentEncoder&, const WebCore::TileCreationInfo&);
    static bool decode(ArgumentDecoder&, WebCore::TileCreationInfo&);
};

template<> struct ArgumentCoder<WebCore::CoordinatedGraphicsState> {
    static void encode(ArgumentEncoder&, const WebCore::CoordinatedGraphicsState&);
    static bool decode(ArgumentDecoder&, WebCore::CoordinatedGraphicsState&);
};

} // namespace CoreIPC

#endif // USE(COORDINATED_GRAPHICS)

#endif // CoordinatedGraphicsArgumentCoders_h
