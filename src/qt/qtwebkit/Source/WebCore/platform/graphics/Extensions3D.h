/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Extensions3D_h
#define Extensions3D_h

#include "GraphicsTypes3D.h"

#include <wtf/text/WTFString.h>

namespace WebCore {

// This is a base class containing only pure virtual functions.
// Implementations must provide a subclass.
//
// The supported extensions are defined below and in subclasses,
// possibly platform-specific ones.
//
// Calling any extension function not supported by the current context
// must be a no-op; in particular, it may not have side effects. In
// this situation, if the function has a return value, 0 is returned.
class Extensions3D {
public:
    virtual ~Extensions3D() {}

    // Supported extensions:
    //   GL_EXT_texture_format_BGRA8888
    //   GL_EXT_read_format_bgra
    //   GL_ARB_robustness
    //   GL_ARB_texture_non_power_of_two / GL_OES_texture_npot
    //   GL_EXT_packed_depth_stencil / GL_OES_packed_depth_stencil
    //   GL_ANGLE_framebuffer_blit / GL_ANGLE_framebuffer_multisample
    //   GL_OES_texture_float
    //   GL_OES_texture_half_float
    //   GL_OES_standard_derivatives
    //   GL_OES_rgb8_rgba8
    //   GL_OES_vertex_array_object
    //   GL_OES_element_index_uint
    //   GL_ANGLE_translated_shader_source
    //   GL_ARB_texture_rectangle (only the subset required to
    //     implement IOSurface binding; it's recommended to support
    //     this only on Mac OS X to limit the amount of code dependent
    //     on this extension)
    //   GL_EXT_texture_compression_dxt1
    //   GL_EXT_texture_compression_s3tc
    //   GL_OES_compressed_ETC1_RGB8_texture
    //   GL_IMG_texture_compression_pvrtc
    //   EXT_texture_filter_anisotropic
    //   GL_EXT_debug_marker
    //   GL_CHROMIUM_copy_texture
    //   GL_CHROMIUM_flipy
    //   GL_ARB_draw_buffers / GL_EXT_draw_buffers

    // Takes full name of extension; for example,
    // "GL_EXT_texture_format_BGRA8888".
    virtual bool supports(const String&) = 0;

    // Certain OpenGL and WebGL implementations may support enabling
    // extensions lazily. This method may only be called with
    // extension names for which supports returns true.
    virtual void ensureEnabled(const String&) = 0;

    // Takes full name of extension: for example, "GL_EXT_texture_format_BGRA8888".
    // Checks to see whether the given extension is actually enabled (see ensureEnabled).
    // Has no other side-effects.
    virtual bool isEnabled(const String&) = 0;

    enum ExtensionsEnumType {
        // GL_EXT_texture_format_BGRA8888 enums
        BGRA_EXT = 0x80E1,

        // GL_ARB_robustness enums
        GUILTY_CONTEXT_RESET_ARB = 0x8253,
        INNOCENT_CONTEXT_RESET_ARB = 0x8254,
        UNKNOWN_CONTEXT_RESET_ARB = 0x8255,

        // GL_EXT/OES_packed_depth_stencil enums
        DEPTH24_STENCIL8 = 0x88F0,
        
        // GL_ANGLE_framebuffer_blit names
        READ_FRAMEBUFFER = 0x8CA8,
        DRAW_FRAMEBUFFER = 0x8CA9,
        DRAW_FRAMEBUFFER_BINDING = 0x8CA6, 
        READ_FRAMEBUFFER_BINDING = 0x8CAA,
        
        // GL_ANGLE_framebuffer_multisample names
        RENDERBUFFER_SAMPLES = 0x8CAB,
        FRAMEBUFFER_INCOMPLETE_MULTISAMPLE = 0x8D56,
        MAX_SAMPLES = 0x8D57,

        // GL_OES_standard_derivatives names
        FRAGMENT_SHADER_DERIVATIVE_HINT_OES = 0x8B8B,

        // GL_OES_rgb8_rgba8 names
        RGB8_OES = 0x8051,
        RGBA8_OES = 0x8058,
        
        // GL_OES_vertex_array_object names
        VERTEX_ARRAY_BINDING_OES = 0x85B5,

        // GL_ANGLE_translated_shader_source
        TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE = 0x93A0,

        // GL_ARB_texture_rectangle
        TEXTURE_RECTANGLE_ARB =  0x84F5,
        TEXTURE_BINDING_RECTANGLE_ARB = 0x84F6,

        // GL_EXT_texture_compression_dxt1
        // GL_EXT_texture_compression_s3tc
        COMPRESSED_RGB_S3TC_DXT1_EXT = 0x83F0,
        COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1,
        COMPRESSED_RGBA_S3TC_DXT3_EXT = 0x83F2,
        COMPRESSED_RGBA_S3TC_DXT5_EXT = 0x83F3,

        // GL_OES_compressed_ETC1_RGB8_texture
        ETC1_RGB8_OES = 0x8D64,

        // GL_IMG_texture_compression_pvrtc
        COMPRESSED_RGB_PVRTC_4BPPV1_IMG = 0x8C00,
        COMPRESSED_RGB_PVRTC_2BPPV1_IMG = 0x8C01,
        COMPRESSED_RGBA_PVRTC_4BPPV1_IMG = 0x8C02,
        COMPRESSED_RGBA_PVRTC_2BPPV1_IMG = 0x8C03,

        // GL_AMD_compressed_ATC_texture
        COMPRESSED_ATC_RGB_AMD = 0x8C92,
        COMPRESSED_ATC_RGBA_EXPLICIT_ALPHA_AMD = 0x8C93,
        COMPRESSED_ATC_RGBA_INTERPOLATED_ALPHA_AMD = 0x87EE,

        // GL_EXT_texture_filter_anisotropic
        TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE,
        MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF,

        // GL_CHROMIUM_flipy
        UNPACK_FLIP_Y_CHROMIUM = 0x9240,

        // GL_CHROMIUM_copy_texture
        UNPACK_PREMULTIPLY_ALPHA_CHROMIUM = 0x9241,
        UNPACK_UNPREMULTIPLY_ALPHA_CHROMIUM = 0x9242,

        // GL_ARB_draw_buffers / GL_EXT_draw_buffers
        MAX_DRAW_BUFFERS_EXT = 0x8824,
        DRAW_BUFFER0_EXT = 0x8825,
        DRAW_BUFFER1_EXT = 0x8826,
        DRAW_BUFFER2_EXT = 0x8827,
        DRAW_BUFFER3_EXT = 0x8828,
        DRAW_BUFFER4_EXT = 0x8829,
        DRAW_BUFFER5_EXT = 0x882A,
        DRAW_BUFFER6_EXT = 0x882B,
        DRAW_BUFFER7_EXT = 0x882C,
        DRAW_BUFFER8_EXT = 0x882D,
        DRAW_BUFFER9_EXT = 0x882E,
        DRAW_BUFFER10_EXT = 0x882F,
        DRAW_BUFFER11_EXT = 0x8830,
        DRAW_BUFFER12_EXT = 0x8831,
        DRAW_BUFFER13_EXT = 0x8832,
        DRAW_BUFFER14_EXT = 0x8833,
        DRAW_BUFFER15_EXT = 0x8834,
        MAX_COLOR_ATTACHMENTS_EXT = 0x8CDF,
        COLOR_ATTACHMENT0_EXT = 0x8CE0,
        COLOR_ATTACHMENT1_EXT = 0x8CE1,
        COLOR_ATTACHMENT2_EXT = 0x8CE2,
        COLOR_ATTACHMENT3_EXT = 0x8CE3,
        COLOR_ATTACHMENT4_EXT = 0x8CE4,
        COLOR_ATTACHMENT5_EXT = 0x8CE5,
        COLOR_ATTACHMENT6_EXT = 0x8CE6,
        COLOR_ATTACHMENT7_EXT = 0x8CE7,
        COLOR_ATTACHMENT8_EXT = 0x8CE8,
        COLOR_ATTACHMENT9_EXT = 0x8CE9,
        COLOR_ATTACHMENT10_EXT = 0x8CEA,
        COLOR_ATTACHMENT11_EXT = 0x8CEB,
        COLOR_ATTACHMENT12_EXT = 0x8CEC,
        COLOR_ATTACHMENT13_EXT = 0x8CED,
        COLOR_ATTACHMENT14_EXT = 0x8CEE,
        COLOR_ATTACHMENT15_EXT = 0x8CEF
    };

    // GL_ARB_robustness
    // Note: This method's behavior differs from the GL_ARB_robustness
    // specification in the following way:
    // The implementation must not reset the error state during this call.
    // If getGraphicsResetStatusARB returns an error, it should continue
    // returning the same error. Restoring the GraphicsContext3D is handled
    // externally.
    virtual int getGraphicsResetStatusARB() = 0;
    
    // GL_ANGLE_framebuffer_blit
    virtual void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter) = 0;
    
    // GL_ANGLE_framebuffer_multisample
    virtual void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height) = 0;
    
    // GL_OES_vertex_array_object
    virtual Platform3DObject createVertexArrayOES() = 0;
    virtual void deleteVertexArrayOES(Platform3DObject) = 0;
    virtual GC3Dboolean isVertexArrayOES(Platform3DObject) = 0;
    virtual void bindVertexArrayOES(Platform3DObject) = 0;

    // GL_ANGLE_translated_shader_source
    virtual String getTranslatedShaderSourceANGLE(Platform3DObject) = 0;

    // GL_CHROMIUM_copy_texture
    virtual void copyTextureCHROMIUM(GC3Denum, Platform3DObject, Platform3DObject, GC3Dint, GC3Denum) = 0;

    // EXT Robustness - uses getGraphicsResetStatusARB
    virtual void readnPixelsEXT(int x, int y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, GC3Dsizei bufSize, void *data) = 0;
    virtual void getnUniformfvEXT(GC3Duint program, int location, GC3Dsizei bufSize, float *params) = 0;
    virtual void getnUniformivEXT(GC3Duint program, int location, GC3Dsizei bufSize, int *params) = 0;

    // GL_EXT_debug_marker
    virtual void insertEventMarkerEXT(const String&) = 0;
    virtual void pushGroupMarkerEXT(const String&) = 0;
    virtual void popGroupMarkerEXT(void) = 0;

    // GL_ARB_draw_buffers / GL_EXT_draw_buffers
    virtual void drawBuffersEXT(GC3Dsizei n, const GC3Denum* bufs) = 0;

    virtual bool isNVIDIA() = 0;
    virtual bool isAMD() = 0;
    virtual bool isIntel() = 0;
    virtual String vendor() = 0;

    // If this method returns false then the system *definitely* does not support multisampling.
    // It does not necessarily say the system does support it - callers must attempt to construct
    // multisampled renderbuffers and check framebuffer completeness.
    // Ports should implement this to return false on configurations where it is known
    // that multisampling is not available.
    virtual bool maySupportMultisampling() = 0;

    // Some configurations have bugs regarding built-in functions in their OpenGL drivers
    // that must be avoided. Ports should implement this flag such configurations.
    virtual bool requiresBuiltInFunctionEmulation() = 0;
};

} // namespace WebCore

#endif // Extensions3D_h
