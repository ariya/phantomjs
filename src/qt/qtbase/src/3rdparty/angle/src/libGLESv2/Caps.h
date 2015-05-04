#ifndef LIBGLESV2_CAPS_H
#define LIBGLESV2_CAPS_H

//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"

#include <set>
#include <unordered_map>
#include <vector>
#include <string>

namespace gl
{

typedef std::set<GLuint> SupportedSampleSet;

struct TextureCaps
{
    TextureCaps();

    // Supports for basic texturing: glTexImage, glTexSubImage, etc
    bool texturable;

    // Support for linear or anisotropic filtering
    bool filterable;

    // Support for being used as a framebuffer attachment or renderbuffer format
    bool renderable;

    SupportedSampleSet sampleCounts;

    // Get the maximum number of samples supported
    GLuint getMaxSamples() const;

    // Get the number of supported samples that is at least as many as requested.  Returns 0 if
    // there are no sample counts available
    GLuint getNearestSamples(GLuint requestedSamples) const;
};

class TextureCapsMap
{
  public:
    typedef std::unordered_map<GLenum, TextureCaps>::const_iterator const_iterator;

    void insert(GLenum internalFormat, const TextureCaps &caps);
    void remove(GLenum internalFormat);

    const TextureCaps &get(GLenum internalFormat) const;

    const_iterator begin() const;
    const_iterator end() const;

    size_t size() const;

  private:
    typedef std::unordered_map<GLenum, TextureCaps> InternalFormatToCapsMap;
    InternalFormatToCapsMap mCapsMap;
};

struct Extensions
{
    Extensions();

    // Generate a vector of supported extension strings
    std::vector<std::string> getStrings() const;

    // Set all texture related extension support based on the supported textures.
    // Determines support for:
    // GL_OES_rgb8_rgba8
    // GL_EXT_texture_format_BGRA8888
    // GL_OES_texture_half_float, GL_OES_texture_half_float_linear
    // GL_OES_texture_float, GL_OES_texture_float_linear
    // GL_EXT_texture_rg
    // GL_EXT_texture_compression_dxt1, GL_ANGLE_texture_compression_dxt3, GL_ANGLE_texture_compression_dxt5
    // GL_EXT_sRGB
    // GL_ANGLE_depth_texture
    // GL_EXT_color_buffer_float
    void setTextureExtensionSupport(const TextureCapsMap &textureCaps);

    // ES2 Extension support

    // GL_OES_element_index_uint
    bool elementIndexUint;

    // GL_OES_packed_depth_stencil
    bool packedDepthStencil;

    // GL_OES_get_program_binary
    bool getProgramBinary;

    // GL_OES_rgb8_rgba8
    // Implies that TextureCaps for GL_RGB8 and GL_RGBA8 exist
    bool rgb8rgba8;

    // GL_EXT_texture_format_BGRA8888
    // Implies that TextureCaps for GL_BGRA8 exist
    bool textureFormatBGRA8888;

    // GL_EXT_read_format_bgra
    bool readFormatBGRA;

    // GL_NV_pixel_buffer_object
    bool pixelBufferObject;

    // GL_OES_mapbuffer and GL_EXT_map_buffer_range
    bool mapBuffer;
    bool mapBufferRange;

    // GL_OES_texture_half_float and GL_OES_texture_half_float_linear
    // Implies that TextureCaps for GL_RGB16F, GL_RGBA16F, GL_ALPHA32F_EXT, GL_LUMINANCE32F_EXT and
    // GL_LUMINANCE_ALPHA32F_EXT exist
    bool textureHalfFloat;
    bool textureHalfFloatLinear;

    // GL_OES_texture_float and GL_OES_texture_float_linear
    // Implies that TextureCaps for GL_RGB32F, GL_RGBA32F, GL_ALPHA16F_EXT, GL_LUMINANCE16F_EXT and
    // GL_LUMINANCE_ALPHA16F_EXT exist
    bool textureFloat;
    bool textureFloatLinear;

    // GL_EXT_texture_rg
    // Implies that TextureCaps for GL_R8, GL_RG8 (and floating point R/RG texture formats if floating point extensions
    // are also present) exist
    bool textureRG;

    // GL_EXT_texture_compression_dxt1, GL_ANGLE_texture_compression_dxt3 and GL_ANGLE_texture_compression_dxt5
    // Implies that TextureCaps for GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    // GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE and GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE
    bool textureCompressionDXT1;
    bool textureCompressionDXT3;
    bool textureCompressionDXT5;

    // GL_EXT_sRGB
    // Implies that TextureCaps for GL_SRGB8_ALPHA8 and GL_SRGB8 exist
    // TODO: Don't advertise this extension in ES3
    bool sRGB;

    // GL_ANGLE_depth_texture
    bool depthTextures;

    // GL_EXT_texture_storage
    bool textureStorage;

    // GL_OES_texture_npot
    bool textureNPOT;

    // GL_EXT_draw_buffers
    bool drawBuffers;

    // GL_EXT_texture_filter_anisotropic
    bool textureFilterAnisotropic;
    GLfloat maxTextureAnisotropy;

    // GL_EXT_occlusion_query_boolean
    bool occlusionQueryBoolean;

    // GL_NV_fence
    bool fence;

    // GL_ANGLE_timer_query
    bool timerQuery;

    // GL_EXT_robustness
    bool robustness;

    // GL_EXT_blend_minmax
    bool blendMinMax;

    // GL_ANGLE_framebuffer_blit
    bool framebufferBlit;

    // GL_ANGLE_framebuffer_multisample
    bool framebufferMultisample;
    GLuint maxSamples;

    // GL_ANGLE_instanced_arrays
    bool instancedArrays;

    // GL_ANGLE_pack_reverse_row_order
    bool packReverseRowOrder;

    // GL_OES_standard_derivatives
    bool standardDerivatives;

    // GL_EXT_shader_texture_lod
    bool shaderTextureLOD;

    // GL_EXT_frag_depth
    bool fragDepth;

    // GL_ANGLE_texture_usage
    bool textureUsage;

    // GL_ANGLE_translated_shader_source
    bool translatedShaderSource;

    // ES3 Extension support

    // GL_EXT_color_buffer_float
    bool colorBufferFloat;
};

struct Caps
{
    Caps();

    // Table 6.28, implementation dependent values
    GLuint64 maxElementIndex;
    GLuint max3DTextureSize;
    GLuint max2DTextureSize;
    GLuint maxArrayTextureLayers;
    GLfloat maxLODBias;
    GLuint maxCubeMapTextureSize;
    GLuint maxRenderbufferSize;
    GLuint maxDrawBuffers;
    GLuint maxColorAttachments;
    GLuint maxViewportWidth;
    GLuint maxViewportHeight;
    GLfloat minAliasedPointSize;
    GLfloat maxAliasedPointSize;
    GLfloat minAliasedLineWidth;
    GLfloat maxAliasedLineWidth;

    // Table 6.29, implementation dependent values (cont.)
    GLuint maxElementsIndices;
    GLuint maxElementsVertices;
    std::vector<GLenum> compressedTextureFormats;
    std::vector<GLenum> programBinaryFormats;
    std::vector<GLenum> shaderBinaryFormats;
    GLuint64 maxServerWaitTimeout;

    // Table 6.31, implementation dependent vertex shader limits
    GLuint maxVertexAttributes;
    GLuint maxVertexUniformComponents;
    GLuint maxVertexUniformVectors;
    GLuint maxVertexUniformBlocks;
    GLuint maxVertexOutputComponents;
    GLuint maxVertexTextureImageUnits;

    // Table 6.32, implementation dependent fragment shader limits
    GLuint maxFragmentUniformComponents;
    GLuint maxFragmentUniformVectors;
    GLuint maxFragmentUniformBlocks;
    GLuint maxFragmentInputComponents;
    GLuint maxTextureImageUnits;
    GLint minProgramTexelOffset;
    GLint maxProgramTexelOffset;

    // Table 6.33, implementation dependent aggregate shader limits
    GLuint maxUniformBufferBindings;
    GLuint64 maxUniformBlockSize;
    GLuint uniformBufferOffsetAlignment;
    GLuint maxCombinedUniformBlocks;
    GLuint64 maxCombinedVertexUniformComponents;
    GLuint64 maxCombinedFragmentUniformComponents;
    GLuint maxVaryingComponents;
    GLuint maxVaryingVectors;
    GLuint maxCombinedTextureImageUnits;

    // Table 6.34, implementation dependent transform feedback limits
    GLuint maxTransformFeedbackInterleavedComponents;
    GLuint maxTransformFeedbackSeparateAttributes;
    GLuint maxTransformFeedbackSeparateComponents;
};

}

#endif // LIBGLESV2_CAPS_H
