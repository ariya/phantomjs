#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer9.cpp: Defines the D3D9 VertexBuffer implementation.

#include "libGLESv2/renderer/VertexBuffer9.h"
#include "libGLESv2/renderer/vertexconversion.h"
#include "libGLESv2/renderer/BufferStorage.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/Renderer9.h"

#include "libGLESv2/Buffer.h"

namespace rx
{

bool VertexBuffer9::mTranslationsInitialized = false;
VertexBuffer9::FormatConverter VertexBuffer9::mFormatConverters[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];

VertexBuffer9::VertexBuffer9(rx::Renderer9 *const renderer) : mRenderer(renderer)
{
    mVertexBuffer = NULL;
    mBufferSize = 0;
    mDynamicUsage = false;

    if (!mTranslationsInitialized)
    {
        initializeTranslations(renderer->getCapsDeclTypes());
        mTranslationsInitialized = true;
    }
}

VertexBuffer9::~VertexBuffer9()
{
    if (mVertexBuffer)
    {
        mVertexBuffer->Release();
        mVertexBuffer = NULL;
    }
}

bool VertexBuffer9::initialize(unsigned int size, bool dynamicUsage)
{
    if (mVertexBuffer)
    {
        mVertexBuffer->Release();
        mVertexBuffer = NULL;
    }

    updateSerial();

    if (size > 0)
    {
        DWORD flags = D3DUSAGE_WRITEONLY;
        if (dynamicUsage)
        {
            flags |= D3DUSAGE_DYNAMIC;
        }

        HRESULT result = mRenderer->createVertexBuffer(size, flags, &mVertexBuffer);

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", size);
            return false;
        }
    }

    mBufferSize = size;
    mDynamicUsage = dynamicUsage;
    return true;
}

VertexBuffer9 *VertexBuffer9::makeVertexBuffer9(VertexBuffer *vertexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(VertexBuffer9*, vertexBuffer));
    return static_cast<VertexBuffer9*>(vertexBuffer);
}

bool VertexBuffer9::storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count,
                                             GLsizei instances, unsigned int offset)
{
    if (mVertexBuffer)
    {
        gl::Buffer *buffer = attrib.mBoundBuffer.get();

        int inputStride = attrib.stride();
        int elementSize = attrib.typeSize();
        const FormatConverter &converter = formatConverter(attrib);

        DWORD lockFlags = mDynamicUsage ? D3DLOCK_NOOVERWRITE : 0;

        void *mapPtr = NULL;
        HRESULT result = mVertexBuffer->Lock(offset, spaceRequired(attrib, count, instances), &mapPtr, lockFlags);

        if (FAILED(result))
        {
            ERR("Lock failed with error 0x%08x", result);
            return false;
        }

        const char *input = NULL;
        if (buffer)
        {
            BufferStorage *storage = buffer->getStorage();
            input = static_cast<const char*>(storage->getData()) + static_cast<int>(attrib.mOffset);
        }
        else
        {
            input = static_cast<const char*>(attrib.mPointer);
        }

        if (instances == 0 || attrib.mDivisor == 0)
        {
            input += inputStride * start;
        }

        if (converter.identity && inputStride == elementSize)
        {
            memcpy(mapPtr, input, count * inputStride);
        }
        else
        {
            converter.convertArray(input, inputStride, count, mapPtr);
        }

        mVertexBuffer->Unlock();

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

bool VertexBuffer9::storeRawData(const void* data, unsigned int size, unsigned int offset)
{
    if (mVertexBuffer)
    {
        DWORD lockFlags = mDynamicUsage ? D3DLOCK_NOOVERWRITE : 0;

        void *mapPtr = NULL;
        HRESULT result = mVertexBuffer->Lock(offset, size, &mapPtr, lockFlags);

        if (FAILED(result))
        {
            ERR("Lock failed with error 0x%08x", result);
            return false;
        }

        memcpy(mapPtr, data, size);

        mVertexBuffer->Unlock();

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

unsigned int VertexBuffer9::getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances) const
{
    return spaceRequired(attrib, count, instances);
}

bool VertexBuffer9::requiresConversion(const gl::VertexAttribute &attrib) const
{
    return formatConverter(attrib).identity;
}

unsigned int VertexBuffer9::getVertexSize(const gl::VertexAttribute &attrib) const
{
    return spaceRequired(attrib, 1, 0);
}

D3DDECLTYPE VertexBuffer9::getDeclType(const gl::VertexAttribute &attrib) const
{
    return formatConverter(attrib).d3dDeclType;
}

unsigned int VertexBuffer9::getBufferSize() const
{
    return mBufferSize;
}

bool VertexBuffer9::setBufferSize(unsigned int size)
{
    if (size > mBufferSize)
    {
        return initialize(size, mDynamicUsage);
    }
    else
    {
        return true;
    }
}

bool VertexBuffer9::discard()
{
    if (mVertexBuffer)
    {
        void *dummy;
        HRESULT result;

        result = mVertexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
        if (FAILED(result))
        {
            ERR("Discard lock failed with error 0x%08x", result);
            return false;
        }

        result = mVertexBuffer->Unlock();
        if (FAILED(result))
        {
            ERR("Discard unlock failed with error 0x%08x", result);
            return false;
        }

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

IDirect3DVertexBuffer9 * VertexBuffer9::getBuffer() const
{
    return mVertexBuffer;
}

// Mapping from OpenGL-ES vertex attrib type to D3D decl type:
//
// BYTE                 SHORT (Cast)
// BYTE-norm            FLOAT (Normalize) (can't be exactly represented as SHORT-norm)
// UNSIGNED_BYTE        UBYTE4 (Identity) or SHORT (Cast)
// UNSIGNED_BYTE-norm   UBYTE4N (Identity) or FLOAT (Normalize)
// SHORT                SHORT (Identity)
// SHORT-norm           SHORT-norm (Identity) or FLOAT (Normalize)
// UNSIGNED_SHORT       FLOAT (Cast)
// UNSIGNED_SHORT-norm  USHORT-norm (Identity) or FLOAT (Normalize)
// FIXED (not in WebGL) FLOAT (FixedToFloat)
// FLOAT                FLOAT (Identity)

// GLToCType maps from GL type (as GLenum) to the C typedef.
template <GLenum GLType> struct GLToCType { };

template <> struct GLToCType<GL_BYTE>           { typedef GLbyte type;      };
template <> struct GLToCType<GL_UNSIGNED_BYTE>  { typedef GLubyte type;     };
template <> struct GLToCType<GL_SHORT>          { typedef GLshort type;     };
template <> struct GLToCType<GL_UNSIGNED_SHORT> { typedef GLushort type;    };
template <> struct GLToCType<GL_FIXED>          { typedef GLuint type;      };
template <> struct GLToCType<GL_FLOAT>          { typedef GLfloat type;     };

// This differs from D3DDECLTYPE in that it is unsized. (Size expansion is applied last.)
enum D3DVertexType
{
    D3DVT_FLOAT,
    D3DVT_SHORT,
    D3DVT_SHORT_NORM,
    D3DVT_UBYTE,
    D3DVT_UBYTE_NORM,
    D3DVT_USHORT_NORM
};

// D3DToCType maps from D3D vertex type (as enum D3DVertexType) to the corresponding C type.
template <unsigned int D3DType> struct D3DToCType { };

template <> struct D3DToCType<D3DVT_FLOAT> { typedef float type; };
template <> struct D3DToCType<D3DVT_SHORT> { typedef short type; };
template <> struct D3DToCType<D3DVT_SHORT_NORM> { typedef short type; };
template <> struct D3DToCType<D3DVT_UBYTE> { typedef unsigned char type; };
template <> struct D3DToCType<D3DVT_UBYTE_NORM> { typedef unsigned char type; };
template <> struct D3DToCType<D3DVT_USHORT_NORM> { typedef unsigned short type; };

// Encode the type/size combinations that D3D permits. For each type/size it expands to a widener that will provide the appropriate final size.
template <unsigned int type, int size> struct WidenRule { };

template <int size> struct WidenRule<D3DVT_FLOAT, size>          : NoWiden<size> { };
template <int size> struct WidenRule<D3DVT_SHORT, size>          : WidenToEven<size> { };
template <int size> struct WidenRule<D3DVT_SHORT_NORM, size>     : WidenToEven<size> { };
template <int size> struct WidenRule<D3DVT_UBYTE, size>          : WidenToFour<size> { };
template <int size> struct WidenRule<D3DVT_UBYTE_NORM, size>     : WidenToFour<size> { };
template <int size> struct WidenRule<D3DVT_USHORT_NORM, size>    : WidenToEven<size> { };

// VertexTypeFlags encodes the D3DCAPS9::DeclType flag and vertex declaration flag for each D3D vertex type & size combination.
template <unsigned int d3dtype, int size> struct VertexTypeFlags { };

template <unsigned int _capflag, unsigned int _declflag>
struct VertexTypeFlagsHelper
{
    enum { capflag = _capflag };
    enum { declflag = _declflag };
};

template <> struct VertexTypeFlags<D3DVT_FLOAT, 1> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT1> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 2> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT2> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 3> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT3> { };
template <> struct VertexTypeFlags<D3DVT_FLOAT, 4> : VertexTypeFlagsHelper<0, D3DDECLTYPE_FLOAT4> { };
template <> struct VertexTypeFlags<D3DVT_SHORT, 2> : VertexTypeFlagsHelper<0, D3DDECLTYPE_SHORT2> { };
template <> struct VertexTypeFlags<D3DVT_SHORT, 4> : VertexTypeFlagsHelper<0, D3DDECLTYPE_SHORT4> { };
template <> struct VertexTypeFlags<D3DVT_SHORT_NORM, 2> : VertexTypeFlagsHelper<D3DDTCAPS_SHORT2N, D3DDECLTYPE_SHORT2N> { };
template <> struct VertexTypeFlags<D3DVT_SHORT_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_SHORT4N, D3DDECLTYPE_SHORT4N> { };
template <> struct VertexTypeFlags<D3DVT_UBYTE, 4> : VertexTypeFlagsHelper<D3DDTCAPS_UBYTE4, D3DDECLTYPE_UBYTE4> { };
template <> struct VertexTypeFlags<D3DVT_UBYTE_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_UBYTE4N, D3DDECLTYPE_UBYTE4N> { };
template <> struct VertexTypeFlags<D3DVT_USHORT_NORM, 2> : VertexTypeFlagsHelper<D3DDTCAPS_USHORT2N, D3DDECLTYPE_USHORT2N> { };
template <> struct VertexTypeFlags<D3DVT_USHORT_NORM, 4> : VertexTypeFlagsHelper<D3DDTCAPS_USHORT4N, D3DDECLTYPE_USHORT4N> { };


// VertexTypeMapping maps GL type & normalized flag to preferred and fallback D3D vertex types (as D3DVertexType enums).
template <GLenum GLtype, bool normalized> struct VertexTypeMapping { };

template <D3DVertexType Preferred, D3DVertexType Fallback = Preferred>
struct VertexTypeMappingBase
{
    enum { preferred = Preferred };
    enum { fallback = Fallback };
};

template <> struct VertexTypeMapping<GL_BYTE, false>                        : VertexTypeMappingBase<D3DVT_SHORT> { };                       // Cast
template <> struct VertexTypeMapping<GL_BYTE, true>                         : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // Normalize
template <> struct VertexTypeMapping<GL_UNSIGNED_BYTE, false>               : VertexTypeMappingBase<D3DVT_UBYTE, D3DVT_FLOAT> { };          // Identity, Cast
template <> struct VertexTypeMapping<GL_UNSIGNED_BYTE, true>                : VertexTypeMappingBase<D3DVT_UBYTE_NORM, D3DVT_FLOAT> { };     // Identity, Normalize
template <> struct VertexTypeMapping<GL_SHORT, false>                       : VertexTypeMappingBase<D3DVT_SHORT> { };                       // Identity
template <> struct VertexTypeMapping<GL_SHORT, true>                        : VertexTypeMappingBase<D3DVT_SHORT_NORM, D3DVT_FLOAT> { };     // Cast, Normalize
template <> struct VertexTypeMapping<GL_UNSIGNED_SHORT, false>              : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // Cast
template <> struct VertexTypeMapping<GL_UNSIGNED_SHORT, true>               : VertexTypeMappingBase<D3DVT_USHORT_NORM, D3DVT_FLOAT> { };    // Cast, Normalize
template <bool normalized> struct VertexTypeMapping<GL_FIXED, normalized>   : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // FixedToFloat
template <bool normalized> struct VertexTypeMapping<GL_FLOAT, normalized>   : VertexTypeMappingBase<D3DVT_FLOAT> { };                       // Identity


// Given a GL type & norm flag and a D3D type, ConversionRule provides the type conversion rule (Cast, Normalize, Identity, FixedToFloat).
// The conversion rules themselves are defined in vertexconversion.h.

// Almost all cases are covered by Cast (including those that are actually Identity since Cast<T,T> knows it's an identity mapping).
template <GLenum fromType, bool normalized, unsigned int toType>
struct ConversionRule : Cast<typename GLToCType<fromType>::type, typename D3DToCType<toType>::type> { };

// All conversions from normalized types to float use the Normalize operator.
template <GLenum fromType> struct ConversionRule<fromType, true, D3DVT_FLOAT> : Normalize<typename GLToCType<fromType>::type> { };

// Use a full specialization for this so that it preferentially matches ahead of the generic normalize-to-float rules.
template <> struct ConversionRule<GL_FIXED, true, D3DVT_FLOAT>  : FixedToFloat<GLint, 16> { };
template <> struct ConversionRule<GL_FIXED, false, D3DVT_FLOAT> : FixedToFloat<GLint, 16> { };

// A 2-stage construction is used for DefaultVertexValues because float must use SimpleDefaultValues (i.e. 0/1)
// whether it is normalized or not.
template <class T, bool normalized> struct DefaultVertexValuesStage2 { };

template <class T> struct DefaultVertexValuesStage2<T, true>  : NormalizedDefaultValues<T> { };
template <class T> struct DefaultVertexValuesStage2<T, false> : SimpleDefaultValues<T> { };

// Work out the default value rule for a D3D type (expressed as the C type) and
template <class T, bool normalized> struct DefaultVertexValues : DefaultVertexValuesStage2<T, normalized> { };
template <bool normalized> struct DefaultVertexValues<float, normalized> : SimpleDefaultValues<float> { };

// Policy rules for use with Converter, to choose whether to use the preferred or fallback conversion.
// The fallback conversion produces an output that all D3D9 devices must support.
template <class T> struct UsePreferred { enum { type = T::preferred }; };
template <class T> struct UseFallback { enum { type = T::fallback }; };

// Converter ties it all together. Given an OpenGL type/norm/size and choice of preferred/fallback conversion,
// it provides all the members of the appropriate VertexDataConverter, the D3DCAPS9::DeclTypes flag in cap flag
// and the D3DDECLTYPE member needed for the vertex declaration in declflag.
template <GLenum fromType, bool normalized, int size, template <class T> class PreferenceRule>
struct Converter
    : VertexDataConverter<typename GLToCType<fromType>::type,
                          WidenRule<PreferenceRule< VertexTypeMapping<fromType, normalized> >::type, size>,
                          ConversionRule<fromType,
                                         normalized,
                                         PreferenceRule< VertexTypeMapping<fromType, normalized> >::type>,
                          DefaultVertexValues<typename D3DToCType<PreferenceRule< VertexTypeMapping<fromType, normalized> >::type>::type, normalized > >
{
private:
    enum { d3dtype = PreferenceRule< VertexTypeMapping<fromType, normalized> >::type };
    enum { d3dsize = WidenRule<d3dtype, size>::finalWidth };

public:
    enum { capflag = VertexTypeFlags<d3dtype, d3dsize>::capflag };
    enum { declflag = VertexTypeFlags<d3dtype, d3dsize>::declflag };
};

// Initialize a TranslationInfo
#define TRANSLATION(type, norm, size, preferred)                                    \
    {                                                                               \
        Converter<type, norm, size, preferred>::identity,                           \
        Converter<type, norm, size, preferred>::finalSize,                          \
        Converter<type, norm, size, preferred>::convertArray,                       \
        static_cast<D3DDECLTYPE>(Converter<type, norm, size, preferred>::declflag)  \
    }

#define TRANSLATION_FOR_TYPE_NORM_SIZE(type, norm, size)    \
    {                                                       \
        Converter<type, norm, size, UsePreferred>::capflag, \
        TRANSLATION(type, norm, size, UsePreferred),        \
        TRANSLATION(type, norm, size, UseFallback)          \
    }

#define TRANSLATIONS_FOR_TYPE(type)                                                                                                                                                                         \
    {                                                                                                                                                                                                       \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, true, 4) },     \
    }

#define TRANSLATIONS_FOR_TYPE_NO_NORM(type)                                                                                                                                                                 \
    {                                                                                                                                                                                                       \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
        { TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 1), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 2), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 3), TRANSLATION_FOR_TYPE_NORM_SIZE(type, false, 4) }, \
    }

const VertexBuffer9::TranslationDescription VertexBuffer9::mPossibleTranslations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4] = // [GL types as enumerated by typeIndex()][normalized][size-1]
{
    TRANSLATIONS_FOR_TYPE(GL_BYTE),
    TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_BYTE),
    TRANSLATIONS_FOR_TYPE(GL_SHORT),
    TRANSLATIONS_FOR_TYPE(GL_UNSIGNED_SHORT),
    TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FIXED),
    TRANSLATIONS_FOR_TYPE_NO_NORM(GL_FLOAT)
};

void VertexBuffer9::initializeTranslations(DWORD declTypes)
{
    for (unsigned int i = 0; i < NUM_GL_VERTEX_ATTRIB_TYPES; i++)
    {
        for (unsigned int j = 0; j < 2; j++)
        {
            for (unsigned int k = 0; k < 4; k++)
            {
                if (mPossibleTranslations[i][j][k].capsFlag == 0 || (declTypes & mPossibleTranslations[i][j][k].capsFlag) != 0)
                {
                    mFormatConverters[i][j][k] = mPossibleTranslations[i][j][k].preferredConversion;
                }
                else
                {
                    mFormatConverters[i][j][k] = mPossibleTranslations[i][j][k].fallbackConversion;
                }
            }
        }
    }
}

unsigned int VertexBuffer9::typeIndex(GLenum type)
{
    switch (type)
    {
      case GL_BYTE: return 0;
      case GL_UNSIGNED_BYTE: return 1;
      case GL_SHORT: return 2;
      case GL_UNSIGNED_SHORT: return 3;
      case GL_FIXED: return 4;
      case GL_FLOAT: return 5;

      default: UNREACHABLE(); return 5;
    }
}

const VertexBuffer9::FormatConverter &VertexBuffer9::formatConverter(const gl::VertexAttribute &attribute)
{
    return mFormatConverters[typeIndex(attribute.mType)][attribute.mNormalized][attribute.mSize - 1];
}

unsigned int VertexBuffer9::spaceRequired(const gl::VertexAttribute &attrib, std::size_t count, GLsizei instances)
{
    unsigned int elementSize = formatConverter(attrib).outputElementSize;

    if (instances == 0 || attrib.mDivisor == 0)
    {
        return elementSize * count;
    }
    else
    {
        return elementSize * ((instances + attrib.mDivisor - 1) / attrib.mDivisor);
    }
}

}
