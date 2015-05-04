//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer9_utils.cpp: Conversion functions and other utility routines
// specific to the D3D9 renderer.

#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/renderer/Workarounds.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/renderer/d3d/d3d9/RenderTarget9.h"

#include "common/mathutil.h"
#include "common/debug.h"

#include "third_party/systeminfo/SystemInfo.h"

namespace rx
{

namespace gl_d3d9
{

D3DCMPFUNC ConvertComparison(GLenum comparison)
{
    D3DCMPFUNC d3dComp = D3DCMP_ALWAYS;
    switch (comparison)
    {
      case GL_NEVER:    d3dComp = D3DCMP_NEVER;        break;
      case GL_ALWAYS:   d3dComp = D3DCMP_ALWAYS;       break;
      case GL_LESS:     d3dComp = D3DCMP_LESS;         break;
      case GL_LEQUAL:   d3dComp = D3DCMP_LESSEQUAL;    break;
      case GL_EQUAL:    d3dComp = D3DCMP_EQUAL;        break;
      case GL_GREATER:  d3dComp = D3DCMP_GREATER;      break;
      case GL_GEQUAL:   d3dComp = D3DCMP_GREATEREQUAL; break;
      case GL_NOTEQUAL: d3dComp = D3DCMP_NOTEQUAL;     break;
      default: UNREACHABLE();
    }

    return d3dComp;
}

D3DCOLOR ConvertColor(gl::ColorF color)
{
    return D3DCOLOR_RGBA(gl::unorm<8>(color.red),
                         gl::unorm<8>(color.green),
                         gl::unorm<8>(color.blue),
                         gl::unorm<8>(color.alpha));
}

D3DBLEND ConvertBlendFunc(GLenum blend)
{
    D3DBLEND d3dBlend = D3DBLEND_ZERO;

    switch (blend)
    {
      case GL_ZERO:                     d3dBlend = D3DBLEND_ZERO;           break;
      case GL_ONE:                      d3dBlend = D3DBLEND_ONE;            break;
      case GL_SRC_COLOR:                d3dBlend = D3DBLEND_SRCCOLOR;       break;
      case GL_ONE_MINUS_SRC_COLOR:      d3dBlend = D3DBLEND_INVSRCCOLOR;    break;
      case GL_DST_COLOR:                d3dBlend = D3DBLEND_DESTCOLOR;      break;
      case GL_ONE_MINUS_DST_COLOR:      d3dBlend = D3DBLEND_INVDESTCOLOR;   break;
      case GL_SRC_ALPHA:                d3dBlend = D3DBLEND_SRCALPHA;       break;
      case GL_ONE_MINUS_SRC_ALPHA:      d3dBlend = D3DBLEND_INVSRCALPHA;    break;
      case GL_DST_ALPHA:                d3dBlend = D3DBLEND_DESTALPHA;      break;
      case GL_ONE_MINUS_DST_ALPHA:      d3dBlend = D3DBLEND_INVDESTALPHA;   break;
      case GL_CONSTANT_COLOR:           d3dBlend = D3DBLEND_BLENDFACTOR;    break;
      case GL_ONE_MINUS_CONSTANT_COLOR: d3dBlend = D3DBLEND_INVBLENDFACTOR; break;
      case GL_CONSTANT_ALPHA:           d3dBlend = D3DBLEND_BLENDFACTOR;    break;
      case GL_ONE_MINUS_CONSTANT_ALPHA: d3dBlend = D3DBLEND_INVBLENDFACTOR; break;
      case GL_SRC_ALPHA_SATURATE:       d3dBlend = D3DBLEND_SRCALPHASAT;    break;
      default: UNREACHABLE();
    }

    return d3dBlend;
}

D3DBLENDOP ConvertBlendOp(GLenum blendOp)
{
    D3DBLENDOP d3dBlendOp = D3DBLENDOP_ADD;

    switch (blendOp)
    {
      case GL_FUNC_ADD:              d3dBlendOp = D3DBLENDOP_ADD;         break;
      case GL_FUNC_SUBTRACT:         d3dBlendOp = D3DBLENDOP_SUBTRACT;    break;
      case GL_FUNC_REVERSE_SUBTRACT: d3dBlendOp = D3DBLENDOP_REVSUBTRACT; break;
      case GL_MIN_EXT:               d3dBlendOp = D3DBLENDOP_MIN;         break;
      case GL_MAX_EXT:               d3dBlendOp = D3DBLENDOP_MAX;         break;
      default: UNREACHABLE();
    }

    return d3dBlendOp;
}

D3DSTENCILOP ConvertStencilOp(GLenum stencilOp)
{
    D3DSTENCILOP d3dStencilOp = D3DSTENCILOP_KEEP;

    switch (stencilOp)
    {
      case GL_ZERO:      d3dStencilOp = D3DSTENCILOP_ZERO;    break;
      case GL_KEEP:      d3dStencilOp = D3DSTENCILOP_KEEP;    break;
      case GL_REPLACE:   d3dStencilOp = D3DSTENCILOP_REPLACE; break;
      case GL_INCR:      d3dStencilOp = D3DSTENCILOP_INCRSAT; break;
      case GL_DECR:      d3dStencilOp = D3DSTENCILOP_DECRSAT; break;
      case GL_INVERT:    d3dStencilOp = D3DSTENCILOP_INVERT;  break;
      case GL_INCR_WRAP: d3dStencilOp = D3DSTENCILOP_INCR;    break;
      case GL_DECR_WRAP: d3dStencilOp = D3DSTENCILOP_DECR;    break;
      default: UNREACHABLE();
    }

    return d3dStencilOp;
}

D3DTEXTUREADDRESS ConvertTextureWrap(GLenum wrap)
{
    D3DTEXTUREADDRESS d3dWrap = D3DTADDRESS_WRAP;

    switch (wrap)
    {
      case GL_REPEAT:            d3dWrap = D3DTADDRESS_WRAP;   break;
      case GL_CLAMP_TO_EDGE:     d3dWrap = D3DTADDRESS_CLAMP;  break;
      case GL_MIRRORED_REPEAT:   d3dWrap = D3DTADDRESS_MIRROR; break;
      default: UNREACHABLE();
    }

    return d3dWrap;
}

D3DCULL ConvertCullMode(GLenum cullFace, GLenum frontFace)
{
    D3DCULL cull = D3DCULL_CCW;
    switch (cullFace)
    {
      case GL_FRONT:
        cull = (frontFace == GL_CCW ? D3DCULL_CW : D3DCULL_CCW);
        break;
      case GL_BACK:
        cull = (frontFace == GL_CCW ? D3DCULL_CCW : D3DCULL_CW);
        break;
      case GL_FRONT_AND_BACK:
        cull = D3DCULL_NONE; // culling will be handled during draw
        break;
      default: UNREACHABLE();
    }

    return cull;
}

D3DCUBEMAP_FACES ConvertCubeFace(GLenum cubeFace)
{
    D3DCUBEMAP_FACES face = D3DCUBEMAP_FACE_POSITIVE_X;

    switch (cubeFace)
    {
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        face = D3DCUBEMAP_FACE_POSITIVE_X;
        break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        face = D3DCUBEMAP_FACE_NEGATIVE_X;
        break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        face = D3DCUBEMAP_FACE_POSITIVE_Y;
        break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        face = D3DCUBEMAP_FACE_NEGATIVE_Y;
        break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        face = D3DCUBEMAP_FACE_POSITIVE_Z;
        break;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        face = D3DCUBEMAP_FACE_NEGATIVE_Z;
        break;
      default: UNREACHABLE();
    }

    return face;
}

DWORD ConvertColorMask(bool red, bool green, bool blue, bool alpha)
{
    return (red   ? D3DCOLORWRITEENABLE_RED   : 0) |
           (green ? D3DCOLORWRITEENABLE_GREEN : 0) |
           (blue  ? D3DCOLORWRITEENABLE_BLUE  : 0) |
           (alpha ? D3DCOLORWRITEENABLE_ALPHA : 0);
}

D3DTEXTUREFILTERTYPE ConvertMagFilter(GLenum magFilter, float maxAnisotropy)
{
    if (maxAnisotropy > 1.0f)
    {
        return D3DTEXF_ANISOTROPIC;
    }

    D3DTEXTUREFILTERTYPE d3dMagFilter = D3DTEXF_POINT;
    switch (magFilter)
    {
      case GL_NEAREST: d3dMagFilter = D3DTEXF_POINT;  break;
      case GL_LINEAR:  d3dMagFilter = D3DTEXF_LINEAR; break;
      default: UNREACHABLE();
    }

    return d3dMagFilter;
}

void ConvertMinFilter(GLenum minFilter, D3DTEXTUREFILTERTYPE *d3dMinFilter, D3DTEXTUREFILTERTYPE *d3dMipFilter, float maxAnisotropy)
{
    switch (minFilter)
    {
      case GL_NEAREST:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_NONE;
        break;
      case GL_LINEAR:
        *d3dMinFilter = D3DTEXF_LINEAR;
        *d3dMipFilter = D3DTEXF_NONE;
        break;
      case GL_NEAREST_MIPMAP_NEAREST:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_POINT;
        break;
      case GL_LINEAR_MIPMAP_NEAREST:
        *d3dMinFilter = D3DTEXF_LINEAR;
        *d3dMipFilter = D3DTEXF_POINT;
        break;
      case GL_NEAREST_MIPMAP_LINEAR:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_LINEAR;
        break;
      case GL_LINEAR_MIPMAP_LINEAR:
        *d3dMinFilter = D3DTEXF_LINEAR;
        *d3dMipFilter = D3DTEXF_LINEAR;
        break;
      default:
        *d3dMinFilter = D3DTEXF_POINT;
        *d3dMipFilter = D3DTEXF_NONE;
        UNREACHABLE();
    }

    if (maxAnisotropy > 1.0f)
    {
        *d3dMinFilter = D3DTEXF_ANISOTROPIC;
    }
}

D3DMULTISAMPLE_TYPE GetMultisampleType(GLuint samples)
{
    return (samples > 1) ? static_cast<D3DMULTISAMPLE_TYPE>(samples) : D3DMULTISAMPLE_NONE;
}

}

namespace d3d9_gl
{

GLsizei GetSamplesCount(D3DMULTISAMPLE_TYPE type)
{
    return (type != D3DMULTISAMPLE_NONMASKABLE) ? type : 0;
}

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format)
{
    GLenum internalFormat = d3d9::GetD3DFormatInfo(d3dformat).internalFormat;
    GLenum convertedFormat = gl::GetInternalFormatInfo(internalFormat).format;
    return convertedFormat == format;
}

static gl::TextureCaps GenerateTextureFormatCaps(GLenum internalFormat, IDirect3D9 *d3d9, D3DDEVTYPE deviceType,
                                                 UINT adapter, D3DFORMAT adapterFormat)
{
    gl::TextureCaps textureCaps;

    const d3d9::TextureFormat &d3dFormatInfo = d3d9::GetTextureFormatInfo(internalFormat);
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
    if (formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
    {
        textureCaps.texturable = SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, 0, D3DRTYPE_TEXTURE, d3dFormatInfo.texFormat));
    }
    else
    {
        textureCaps.texturable = SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, 0, D3DRTYPE_TEXTURE, d3dFormatInfo.texFormat)) &&
                                 SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, 0, D3DRTYPE_CUBETEXTURE, d3dFormatInfo.texFormat));
    }

    textureCaps.filterable = SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, d3dFormatInfo.texFormat));
    textureCaps.renderable = SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, d3dFormatInfo.renderFormat)) ||
                             SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, adapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, d3dFormatInfo.renderFormat));

    textureCaps.sampleCounts.insert(1);
    for (size_t i = D3DMULTISAMPLE_2_SAMPLES; i <= D3DMULTISAMPLE_16_SAMPLES; i++)
    {
        D3DMULTISAMPLE_TYPE multisampleType = D3DMULTISAMPLE_TYPE(i);

        HRESULT result = d3d9->CheckDeviceMultiSampleType(adapter, deviceType, d3dFormatInfo.renderFormat, TRUE, multisampleType, NULL);
        if (SUCCEEDED(result))
        {
            textureCaps.sampleCounts.insert(i);
        }
    }

    return textureCaps;
}

void GenerateCaps(IDirect3D9 *d3d9, IDirect3DDevice9 *device, D3DDEVTYPE deviceType, UINT adapter, gl::Caps *caps,
                  gl::TextureCapsMap *textureCapsMap, gl::Extensions *extensions)
{
    D3DCAPS9 deviceCaps;
    if (FAILED(d3d9->GetDeviceCaps(adapter, deviceType, &deviceCaps)))
    {
        // Can't continue with out device caps
        return;
    }

    D3DDISPLAYMODE currentDisplayMode;
    d3d9->GetAdapterDisplayMode(adapter, &currentDisplayMode);

    GLuint maxSamples = 0;
    const gl::FormatSet &allFormats = gl::GetAllSizedInternalFormats();
    for (gl::FormatSet::const_iterator internalFormat = allFormats.begin(); internalFormat != allFormats.end(); ++internalFormat)
    {
        gl::TextureCaps textureCaps = GenerateTextureFormatCaps(*internalFormat, d3d9, deviceType, adapter,
                                                                currentDisplayMode.Format);
        textureCapsMap->insert(*internalFormat, textureCaps);

        maxSamples = std::max(maxSamples, textureCaps.getMaxSamples());

        if (gl::GetInternalFormatInfo(*internalFormat).compressed)
        {
            caps->compressedTextureFormats.push_back(*internalFormat);
        }
    }

    // GL core feature limits
    caps->maxElementIndex = static_cast<GLint64>(std::numeric_limits<unsigned int>::max());

    // 3D textures are unimplemented in D3D9
    caps->max3DTextureSize = 1;

    // Only one limit in GL, use the minimum dimension
    caps->max2DTextureSize = std::min(deviceCaps.MaxTextureWidth, deviceCaps.MaxTextureHeight);

    // D3D treats cube maps as a special case of 2D textures
    caps->maxCubeMapTextureSize = caps->max2DTextureSize;

    // Array textures are not available in D3D9
    caps->maxArrayTextureLayers = 1;

    // ES3-only feature
    caps->maxLODBias = 0.0f;

    // No specific limits on render target size, maximum 2D texture size is equivalent
    caps->maxRenderbufferSize = caps->max2DTextureSize;

    // Draw buffers are not supported in D3D9
    caps->maxDrawBuffers = 1;
    caps->maxColorAttachments = 1;

    // No specific limits on viewport size, maximum 2D texture size is equivalent
    caps->maxViewportWidth = caps->max2DTextureSize;
    caps->maxViewportHeight = caps->maxViewportWidth;

    // Point size is clamped to 1.0f when the shader model is less than 3
    caps->minAliasedPointSize = 1.0f;
    caps->maxAliasedPointSize = ((D3DSHADER_VERSION_MAJOR(deviceCaps.PixelShaderVersion) >= 3) ? deviceCaps.MaxPointSize : 1.0f);

    // Wide lines not supported
    caps->minAliasedLineWidth = 1.0f;
    caps->maxAliasedLineWidth = 1.0f;

    // Primitive count limits (unused in ES2)
    caps->maxElementsIndices = 0;
    caps->maxElementsVertices = 0;

    // Program and shader binary formats (no supported shader binary formats)
    caps->programBinaryFormats.push_back(GL_PROGRAM_BINARY_ANGLE);

    // WaitSync is ES3-only, set to zero
    caps->maxServerWaitTimeout = 0;

    // Vertex shader limits
    caps->maxVertexAttributes = 16;

    const size_t reservedVertexUniformVectors = 2; // dx_ViewAdjust and dx_DepthRange.
    const size_t MAX_VERTEX_CONSTANT_VECTORS_D3D9 = 256;
    caps->maxVertexUniformVectors = MAX_VERTEX_CONSTANT_VECTORS_D3D9 - reservedVertexUniformVectors;
    caps->maxVertexUniformComponents = caps->maxVertexUniformVectors * 4;

    caps->maxVertexUniformBlocks = 0;

    // SM3 only supports 11 output variables, with a special 12th register for PSIZE.
    const size_t MAX_VERTEX_OUTPUT_VECTORS_SM3 = 9;
    const size_t MAX_VERTEX_OUTPUT_VECTORS_SM2 = 7;
    caps->maxVertexOutputComponents = ((deviceCaps.VertexShaderVersion >= D3DVS_VERSION(3, 0)) ? MAX_VERTEX_OUTPUT_VECTORS_SM3
                                                                                               : MAX_VERTEX_OUTPUT_VECTORS_SM2) * 4;

    // Only Direct3D 10 ready devices support all the necessary vertex texture formats.
    // We test this using D3D9 by checking support for the R16F format.
    if (deviceCaps.VertexShaderVersion >= D3DVS_VERSION(3, 0) &&
        SUCCEEDED(d3d9->CheckDeviceFormat(adapter, deviceType, currentDisplayMode.Format,
                                          D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_R16F)))
    {
        const size_t MAX_TEXTURE_IMAGE_UNITS_VTF_SM3 = 4;
        caps->maxVertexTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS_VTF_SM3;
    }
    else
    {
        caps->maxVertexTextureImageUnits = 0;
    }

    // Fragment shader limits
    const size_t reservedPixelUniformVectors = 3; // dx_ViewCoords, dx_DepthFront and dx_DepthRange.

    const size_t MAX_PIXEL_CONSTANT_VECTORS_SM3 = 224;
    const size_t MAX_PIXEL_CONSTANT_VECTORS_SM2 = 32;
    caps->maxFragmentUniformVectors = ((deviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0)) ? MAX_PIXEL_CONSTANT_VECTORS_SM3
                                                                                              : MAX_PIXEL_CONSTANT_VECTORS_SM2) - reservedPixelUniformVectors;
    caps->maxFragmentUniformComponents = caps->maxFragmentUniformVectors * 4;
    caps->maxFragmentUniformBlocks = 0;
    caps->maxFragmentInputComponents = caps->maxVertexOutputComponents;
    caps->maxTextureImageUnits = 16;
    caps->minProgramTexelOffset = 0;
    caps->maxProgramTexelOffset = 0;

    // Aggregate shader limits (unused in ES2)
    caps->maxUniformBufferBindings = 0;
    caps->maxUniformBlockSize = 0;
    caps->uniformBufferOffsetAlignment = 0;
    caps->maxCombinedUniformBlocks = 0;
    caps->maxCombinedVertexUniformComponents = 0;
    caps->maxCombinedFragmentUniformComponents = 0;
    caps->maxVaryingComponents = 0;

    // Aggregate shader limits
    caps->maxVaryingVectors = caps->maxVertexOutputComponents / 4;
    caps->maxCombinedTextureImageUnits = caps->maxVertexTextureImageUnits + caps->maxTextureImageUnits;

    // Transform feedback limits
    caps->maxTransformFeedbackInterleavedComponents = 0;
    caps->maxTransformFeedbackSeparateAttributes = 0;
    caps->maxTransformFeedbackSeparateComponents = 0;

    // GL extension support
    extensions->setTextureExtensionSupport(*textureCapsMap);
    extensions->elementIndexUint = deviceCaps.MaxVertexIndex >= (1 << 16);
    extensions->packedDepthStencil = true;
    extensions->getProgramBinary = true;
    extensions->rgb8rgba8 = true;
    extensions->readFormatBGRA = true;
    extensions->pixelBufferObject = false;
    extensions->mapBuffer = false;
    extensions->mapBufferRange = false;

    // ATI cards on XP have problems with non-power-of-two textures.
    D3DADAPTER_IDENTIFIER9 adapterId = { 0 };
    if (SUCCEEDED(d3d9->GetAdapterIdentifier(adapter, 0, &adapterId)))
    {
        extensions->textureNPOT = !(deviceCaps.TextureCaps & D3DPTEXTURECAPS_POW2) &&
                                      !(deviceCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2) &&
                                      !(deviceCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) &&
                                      !(!isWindowsVistaOrGreater() && adapterId.VendorId == VENDOR_ID_AMD);
    }
    else
    {
        extensions->textureNPOT = false;
    }

    extensions->drawBuffers = false;
    extensions->textureStorage = true;

    // Must support a minimum of 2:1 anisotropy for max anisotropy to be considered supported, per the spec
    extensions->textureFilterAnisotropic = (deviceCaps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) != 0 && deviceCaps.MaxAnisotropy >= 2;
    extensions->maxTextureAnisotropy = static_cast<GLfloat>(deviceCaps.MaxAnisotropy);

    // Check occlusion query support by trying to create one
    IDirect3DQuery9 *occlusionQuery = NULL;
    extensions->occlusionQueryBoolean = SUCCEEDED(device->CreateQuery(D3DQUERYTYPE_OCCLUSION, &occlusionQuery)) && occlusionQuery;
    SafeRelease(occlusionQuery);

    // Check event query support by trying to create one
    IDirect3DQuery9 *eventQuery = NULL;
    extensions->fence = SUCCEEDED(device->CreateQuery(D3DQUERYTYPE_EVENT, &eventQuery)) && eventQuery;
    SafeRelease(eventQuery);

    extensions->timerQuery = false; // Unimplemented
    extensions->robustness = true;
    extensions->blendMinMax = true;
    extensions->framebufferBlit = true;
    extensions->framebufferMultisample = true;
    extensions->maxSamples = maxSamples;
    extensions->instancedArrays = deviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0);
    extensions->packReverseRowOrder = true;
    extensions->standardDerivatives = (deviceCaps.PS20Caps.Caps & D3DPS20CAPS_GRADIENTINSTRUCTIONS) != 0;
    extensions->shaderTextureLOD = true;
    extensions->fragDepth = true;
    extensions->textureUsage = true;
    extensions->translatedShaderSource = true;
    extensions->colorBufferFloat = false;
}

}

namespace d3d9
{

GLuint ComputeBlockSize(D3DFORMAT format, GLuint width, GLuint height)
{
    const D3DFormat &d3dFormatInfo = d3d9::GetD3DFormatInfo(format);
    GLuint numBlocksWide = (width + d3dFormatInfo.blockWidth - 1) / d3dFormatInfo.blockWidth;
    GLuint numBlocksHight = (height + d3dFormatInfo.blockHeight - 1) / d3dFormatInfo.blockHeight;
    return (d3dFormatInfo.pixelBytes * numBlocksWide * numBlocksHight);
}

void MakeValidSize(bool isImage, D3DFORMAT format, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset)
{
    const D3DFormat &d3dFormatInfo = d3d9::GetD3DFormatInfo(format);

    int upsampleCount = 0;
    // Don't expand the size of full textures that are at least (blockWidth x blockHeight) already.
    if (isImage || *requestWidth < static_cast<GLsizei>(d3dFormatInfo.blockWidth) ||
        *requestHeight < static_cast<GLsizei>(d3dFormatInfo.blockHeight))
    {
        while (*requestWidth % d3dFormatInfo.blockWidth != 0 || *requestHeight % d3dFormatInfo.blockHeight != 0)
        {
            *requestWidth <<= 1;
            *requestHeight <<= 1;
            upsampleCount++;
        }
    }
    *levelOffset = upsampleCount;
}

gl::Error GetAttachmentRenderTarget(gl::FramebufferAttachment *attachment, RenderTarget9 **outRT)
{
    RenderTarget *renderTarget = NULL;
    gl::Error error = rx::GetAttachmentRenderTarget(attachment, &renderTarget);
    if (error.isError())
    {
        return error;
    }
    *outRT = RenderTarget9::makeRenderTarget9(renderTarget);
    return gl::Error(GL_NO_ERROR);
}

Workarounds GenerateWorkarounds()
{
    Workarounds workarounds;
    workarounds.mrtPerfWorkaround = true;
    workarounds.setDataFasterThanImageUpload = false;
    return workarounds;
}

}

}
