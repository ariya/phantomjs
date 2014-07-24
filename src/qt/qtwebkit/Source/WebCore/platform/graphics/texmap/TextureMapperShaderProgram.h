/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Igalia S.L.

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

#ifndef TextureMapperShaderProgram_h
#define TextureMapperShaderProgram_h

#if USE(TEXTURE_MAPPER)
#include "GraphicsContext3D.h"
#include "TransformationMatrix.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {
#define TEXMAP_DECLARE_VARIABLE(Accessor, Name, Type) GC3Duint Accessor##Location() { static const AtomicString name(Name); return getLocation(name, Type); }
#define TEXMAP_DECLARE_UNIFORM(Accessor) TEXMAP_DECLARE_VARIABLE(Accessor, "u_"#Accessor, UniformVariable)
#define TEXMAP_DECLARE_ATTRIBUTE(Accessor) TEXMAP_DECLARE_VARIABLE(Accessor, "a_"#Accessor, AttribVariable)
#define TEXMAP_DECLARE_SAMPLER(Accessor) TEXMAP_DECLARE_VARIABLE(Accessor, "s_"#Accessor, UniformVariable)

class TextureMapperShaderProgram : public RefCounted<TextureMapperShaderProgram> {
public:
    enum Option {
        Texture          = 1L << 0,
        Rect             = 1L << 1,
        SolidColor       = 1L << 2,
        Opacity          = 1L << 3,
        Antialiasing     = 1L << 5,
        GrayscaleFilter  = 1L << 6,
        SepiaFilter      = 1L << 7,
        SaturateFilter   = 1L << 8,
        HueRotateFilter  = 1L << 9,
        BrightnessFilter = 1L << 10,
        ContrastFilter   = 1L << 11,
        InvertFilter     = 1L << 12,
        OpacityFilter    = 1L << 13,
        BlurFilter       = 1L << 14,
        AlphaBlur        = 1L << 15,
        ContentTexture   = 1L << 16
    };

    typedef unsigned Options;

    static PassRefPtr<TextureMapperShaderProgram> create(PassRefPtr<GraphicsContext3D>, Options);
    virtual ~TextureMapperShaderProgram();
    Platform3DObject programID() const { return m_id; }
    GraphicsContext3D* context() { return m_context.get(); }

    TEXMAP_DECLARE_ATTRIBUTE(vertex)

    TEXMAP_DECLARE_UNIFORM(modelViewMatrix)
    TEXMAP_DECLARE_UNIFORM(projectionMatrix)
    TEXMAP_DECLARE_UNIFORM(textureSpaceMatrix)
    TEXMAP_DECLARE_UNIFORM(opacity)
    TEXMAP_DECLARE_UNIFORM(color)
    TEXMAP_DECLARE_UNIFORM(expandedQuadEdgesInScreenSpace)
    TEXMAP_DECLARE_SAMPLER(sampler)
    TEXMAP_DECLARE_SAMPLER(mask)

#if ENABLE(CSS_FILTERS)
    TEXMAP_DECLARE_UNIFORM(filterAmount)
    TEXMAP_DECLARE_UNIFORM(gaussianKernel)
    TEXMAP_DECLARE_UNIFORM(blurRadius)
    TEXMAP_DECLARE_UNIFORM(shadowOffset)
    TEXMAP_DECLARE_SAMPLER(contentTexture)
#endif

    void setMatrix(GC3Duint, const TransformationMatrix&);

private:
    TextureMapperShaderProgram(PassRefPtr<GraphicsContext3D>, const String& vertexShaderSource, const String& fragmentShaderSource);
    Platform3DObject m_vertexShader;
    Platform3DObject m_fragmentShader;

    enum VariableType { UniformVariable, AttribVariable };
    GC3Duint getLocation(const AtomicString&, VariableType);

    RefPtr<GraphicsContext3D> m_context;
    Platform3DObject m_id;
    HashMap<AtomicString, GC3Duint> m_variables;
};

}
#endif

#endif // TextureMapperShaderProgram_h
