//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/Caps.h"
#include "common/debug.h"
#include "common/angleutils.h"

#include "angle_gl.h"

#include <algorithm>
#include <sstream>

namespace gl
{

TextureCaps::TextureCaps()
    : texturable(false),
      filterable(false),
      renderable(false),
      sampleCounts()
{
}

GLuint TextureCaps::getMaxSamples() const
{
    return !sampleCounts.empty() ? *sampleCounts.rbegin() : 0;
}

GLuint TextureCaps::getNearestSamples(GLuint requestedSamples) const
{
    if (requestedSamples == 0)
    {
        return 0;
    }

    for (SupportedSampleSet::const_iterator i = sampleCounts.begin(); i != sampleCounts.end(); i++)
    {
        GLuint samples = *i;
        if (samples >= requestedSamples)
        {
            return samples;
        }
    }

    return 0;
}

void TextureCapsMap::insert(GLenum internalFormat, const TextureCaps &caps)
{
    mCapsMap.insert(std::make_pair(internalFormat, caps));
}

void TextureCapsMap::remove(GLenum internalFormat)
{
    InternalFormatToCapsMap::iterator i = mCapsMap.find(internalFormat);
    if (i != mCapsMap.end())
    {
        mCapsMap.erase(i);
    }
}

const TextureCaps &TextureCapsMap::get(GLenum internalFormat) const
{
    static TextureCaps defaultUnsupportedTexture;
    InternalFormatToCapsMap::const_iterator iter = mCapsMap.find(internalFormat);
    return (iter != mCapsMap.end()) ? iter->second : defaultUnsupportedTexture;
}

TextureCapsMap::const_iterator TextureCapsMap::begin() const
{
    return mCapsMap.begin();
}

TextureCapsMap::const_iterator TextureCapsMap::end() const
{
    return mCapsMap.end();
}

size_t TextureCapsMap::size() const
{
    return mCapsMap.size();
}

Extensions::Extensions()
    : elementIndexUint(false),
      packedDepthStencil(false),
      getProgramBinary(false),
      rgb8rgba8(false),
      textureFormatBGRA8888(false),
      readFormatBGRA(false),
      pixelBufferObject(false),
      mapBuffer(false),
      mapBufferRange(false),
      textureHalfFloat(false),
      textureHalfFloatLinear(false),
      textureFloat(false),
      textureFloatLinear(false),
      textureRG(false),
      textureCompressionDXT1(false),
      textureCompressionDXT3(false),
      textureCompressionDXT5(false),
      depthTextures(false),
      textureNPOT(false),
      drawBuffers(false),
      textureStorage(false),
      textureFilterAnisotropic(false),
      maxTextureAnisotropy(false),
      occlusionQueryBoolean(false),
      fence(false),
      timerQuery(false),
      robustness(false),
      blendMinMax(false),
      framebufferBlit(false),
      framebufferMultisample(false),
      instancedArrays(false),
      packReverseRowOrder(false),
      standardDerivatives(false),
      shaderTextureLOD(false),
      fragDepth(false),
      textureUsage(false),
      translatedShaderSource(false),
      colorBufferFloat(false)
{
}

static void InsertExtensionString(const std::string &extension, bool supported, std::vector<std::string> *extensionVector)
{
    if (supported)
    {
        extensionVector->push_back(extension);
    }
}

std::vector<std::string> Extensions::getStrings() const
{
    std::vector<std::string> extensionStrings;

    //                   | Extension name                     | Supported flag          | Output vector   |
    InsertExtensionString("GL_OES_element_index_uint",         elementIndexUint,         &extensionStrings);
    InsertExtensionString("GL_OES_packed_depth_stencil",       packedDepthStencil,       &extensionStrings);
    InsertExtensionString("GL_OES_get_program_binary",         getProgramBinary,         &extensionStrings);
    InsertExtensionString("GL_OES_rgb8_rgba8",                 rgb8rgba8,                &extensionStrings);
    InsertExtensionString("GL_EXT_texture_format_BGRA8888",    textureFormatBGRA8888,    &extensionStrings);
    InsertExtensionString("GL_EXT_read_format_bgra",           readFormatBGRA,           &extensionStrings);
    InsertExtensionString("GL_NV_pixel_buffer_object",         pixelBufferObject,        &extensionStrings);
    InsertExtensionString("GL_OES_mapbuffer",                  mapBuffer,                &extensionStrings);
    InsertExtensionString("GL_EXT_map_buffer_range",           mapBufferRange,           &extensionStrings);
    InsertExtensionString("GL_OES_texture_half_float",         textureHalfFloat,         &extensionStrings);
    InsertExtensionString("GL_OES_texture_half_float_linear",  textureHalfFloatLinear,   &extensionStrings);
    InsertExtensionString("GL_OES_texture_float",              textureFloat,             &extensionStrings);
    InsertExtensionString("GL_OES_texture_float_linear",       textureFloatLinear,       &extensionStrings);
    InsertExtensionString("GL_EXT_texture_rg",                 textureRG,                &extensionStrings);
    InsertExtensionString("GL_EXT_texture_compression_dxt1",   textureCompressionDXT1,   &extensionStrings);
    InsertExtensionString("GL_ANGLE_texture_compression_dxt3", textureCompressionDXT3,   &extensionStrings);
    InsertExtensionString("GL_ANGLE_texture_compression_dxt5", textureCompressionDXT5,   &extensionStrings);
    InsertExtensionString("GL_EXT_sRGB",                       sRGB,                     &extensionStrings);
    InsertExtensionString("GL_ANGLE_depth_texture",            depthTextures,            &extensionStrings);
    InsertExtensionString("GL_EXT_texture_storage",            textureStorage,           &extensionStrings);
    InsertExtensionString("GL_OES_texture_npot",               textureNPOT,              &extensionStrings);
    InsertExtensionString("GL_EXT_draw_buffers",               drawBuffers,              &extensionStrings);
    InsertExtensionString("GL_EXT_texture_filter_anisotropic", textureFilterAnisotropic, &extensionStrings);
    InsertExtensionString("GL_EXT_occlusion_query_boolean",    occlusionQueryBoolean,    &extensionStrings);
    InsertExtensionString("GL_NV_fence",                       fence,                    &extensionStrings);
    InsertExtensionString("GL_ANGLE_timer_query",              timerQuery,               &extensionStrings);
    InsertExtensionString("GL_EXT_robustness",                 robustness,               &extensionStrings);
    InsertExtensionString("GL_EXT_blend_minmax",               blendMinMax,              &extensionStrings);
    InsertExtensionString("GL_ANGLE_framebuffer_blit",         framebufferBlit,          &extensionStrings);
    InsertExtensionString("GL_ANGLE_framebuffer_multisample",  framebufferMultisample,   &extensionStrings);
    InsertExtensionString("GL_ANGLE_instanced_arrays",         instancedArrays,          &extensionStrings);
    InsertExtensionString("GL_ANGLE_pack_reverse_row_order",   packReverseRowOrder,      &extensionStrings);
    InsertExtensionString("GL_OES_standard_derivatives",       standardDerivatives,      &extensionStrings);
    InsertExtensionString("GL_EXT_shader_texture_lod",         shaderTextureLOD,         &extensionStrings);
    InsertExtensionString("GL_EXT_frag_depth",                 fragDepth,                &extensionStrings);
    InsertExtensionString("GL_ANGLE_texture_usage",            textureUsage,             &extensionStrings);
    InsertExtensionString("GL_ANGLE_translated_shader_source", translatedShaderSource,   &extensionStrings);
    InsertExtensionString("GL_EXT_color_buffer_float",         colorBufferFloat,         &extensionStrings);

    return extensionStrings;
}

static bool GetFormatSupport(const TextureCapsMap &textureCaps, const std::vector<GLenum> &requiredFormats,
                             bool requiresTexturing, bool requiresFiltering, bool requiresRendering)
{
    for (size_t i = 0; i < requiredFormats.size(); i++)
    {
        const TextureCaps &cap = textureCaps.get(requiredFormats[i]);

        if (requiresTexturing && !cap.texturable)
        {
            return false;
        }

        if (requiresFiltering && !cap.filterable)
        {
            return false;
        }

        if (requiresRendering && !cap.renderable)
        {
            return false;
        }
    }

    return true;
}

// Checks for GL_OES_rgb8_rgba8 support
static bool DetermineRGB8AndRGBA8TextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_RGB8);
    requiredFormats.push_back(GL_RGBA8);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, true);
}

// Checks for GL_EXT_texture_format_BGRA8888 support
static bool DetermineBGRA8TextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_BGRA8_EXT);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, true);
}

// Checks for GL_OES_texture_half_float support
static bool DetermineHalfFloatTextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_RGB16F);
    requiredFormats.push_back(GL_RGBA16F);

    return GetFormatSupport(textureCaps, requiredFormats, true, false, true);
}

// Checks for GL_OES_texture_half_float_linear support
static bool DetermineHalfFloatTextureFilteringSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_RGB16F);
    requiredFormats.push_back(GL_RGBA16F);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, false);
}

// Checks for GL_OES_texture_float support
static bool DetermineFloatTextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_RGB32F);
    requiredFormats.push_back(GL_RGBA32F);

    return GetFormatSupport(textureCaps, requiredFormats, true, false, true);
}

// Checks for GL_OES_texture_float_linear support
static bool DetermineFloatTextureFilteringSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_RGB32F);
    requiredFormats.push_back(GL_RGBA32F);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, false);
}

// Checks for GL_EXT_texture_rg support
static bool DetermineRGTextureSupport(const TextureCapsMap &textureCaps, bool checkHalfFloatFormats, bool checkFloatFormats)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_R8);
    requiredFormats.push_back(GL_RG8);
    if (checkHalfFloatFormats)
    {
        requiredFormats.push_back(GL_R16F);
        requiredFormats.push_back(GL_RG16F);
    }
    if (checkFloatFormats)
    {
        requiredFormats.push_back(GL_R32F);
        requiredFormats.push_back(GL_RG32F);
    }

    return GetFormatSupport(textureCaps, requiredFormats, true, true, false);
}

// Check for GL_EXT_texture_compression_dxt1
static bool DetermineDXT1TextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
    requiredFormats.push_back(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, false);
}

// Check for GL_ANGLE_texture_compression_dxt3
static bool DetermineDXT3TextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, false);
}

// Check for GL_ANGLE_texture_compression_dxt5
static bool DetermineDXT5TextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, false);
}

// Check for GL_ANGLE_texture_compression_dxt5
static bool DetermineSRGBTextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFilterFormats;
    requiredFilterFormats.push_back(GL_SRGB8);
    requiredFilterFormats.push_back(GL_SRGB8_ALPHA8);

    std::vector<GLenum> requiredRenderFormats;
    requiredRenderFormats.push_back(GL_SRGB8_ALPHA8);

    return GetFormatSupport(textureCaps, requiredFilterFormats, true, true, false) &&
           GetFormatSupport(textureCaps, requiredRenderFormats, true, false, true);
}

// Check for GL_ANGLE_depth_texture
static bool DetermineDepthTextureSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_DEPTH_COMPONENT16);
    requiredFormats.push_back(GL_DEPTH_COMPONENT32_OES);
    requiredFormats.push_back(GL_DEPTH24_STENCIL8_OES);

    return GetFormatSupport(textureCaps, requiredFormats, true, true, true);
}

// Check for GL_EXT_color_buffer_float
static bool DetermineColorBufferFloatSupport(const TextureCapsMap &textureCaps)
{
    std::vector<GLenum> requiredFormats;
    requiredFormats.push_back(GL_R16F);
    requiredFormats.push_back(GL_RG16F);
    requiredFormats.push_back(GL_RGBA16F);
    requiredFormats.push_back(GL_R32F);
    requiredFormats.push_back(GL_RG32F);
    requiredFormats.push_back(GL_RGBA32F);
    requiredFormats.push_back(GL_R11F_G11F_B10F);

    return GetFormatSupport(textureCaps, requiredFormats, true, false, true);
}

void Extensions::setTextureExtensionSupport(const TextureCapsMap &textureCaps)
{
    rgb8rgba8 = DetermineRGB8AndRGBA8TextureSupport(textureCaps);
    textureFormatBGRA8888 = DetermineBGRA8TextureSupport(textureCaps);
    textureHalfFloat = DetermineHalfFloatTextureSupport(textureCaps);
    textureHalfFloatLinear = DetermineHalfFloatTextureFilteringSupport(textureCaps);
    textureFloat = DetermineFloatTextureSupport(textureCaps);
    textureFloatLinear = DetermineFloatTextureFilteringSupport(textureCaps);
    textureRG = DetermineRGTextureSupport(textureCaps, textureHalfFloat, textureFloat);
    textureCompressionDXT1 = DetermineDXT1TextureSupport(textureCaps);
    textureCompressionDXT3 = DetermineDXT3TextureSupport(textureCaps);
    textureCompressionDXT5 = DetermineDXT5TextureSupport(textureCaps);
    sRGB = DetermineSRGBTextureSupport(textureCaps);
    depthTextures = DetermineDepthTextureSupport(textureCaps);
    colorBufferFloat = DetermineColorBufferFloatSupport(textureCaps);
}

Caps::Caps()
    : maxElementIndex(0),
      max3DTextureSize(0),
      max2DTextureSize(0),
      maxArrayTextureLayers(0),
      maxLODBias(0),
      maxCubeMapTextureSize(0),
      maxRenderbufferSize(0),
      maxDrawBuffers(0),
      maxColorAttachments(0),
      maxViewportWidth(0),
      maxViewportHeight(0),
      minAliasedPointSize(0),
      maxAliasedPointSize(0),
      minAliasedLineWidth(0),
      // Table 6.29
      maxElementsIndices(0),
      maxElementsVertices(0),
      maxServerWaitTimeout(0),
      // Table 6.31
      maxVertexAttributes(0),
      maxVertexUniformComponents(0),
      maxVertexUniformVectors(0),
      maxVertexUniformBlocks(0),
      maxVertexOutputComponents(0),
      maxVertexTextureImageUnits(0),
      // Table 6.32
      maxFragmentUniformComponents(0),
      maxFragmentUniformVectors(0),
      maxFragmentUniformBlocks(0),
      maxFragmentInputComponents(0),
      maxTextureImageUnits(0),
      minProgramTexelOffset(0),
      maxProgramTexelOffset(0),

      maxUniformBufferBindings(0),
      maxUniformBlockSize(0),
      uniformBufferOffsetAlignment(0),
      maxCombinedUniformBlocks(0),
      maxCombinedVertexUniformComponents(0),
      maxCombinedFragmentUniformComponents(0),
      maxVaryingComponents(0),
      maxVaryingVectors(0),
      maxCombinedTextureImageUnits(0),

      maxTransformFeedbackInterleavedComponents(0),
      maxTransformFeedbackSeparateAttributes(0),
      maxTransformFeedbackSeparateComponents(0)
{
}

}
