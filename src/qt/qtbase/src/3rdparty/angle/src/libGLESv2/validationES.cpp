//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES.h: Validation functions for generic OpenGL ES entry point parameters

#include "libGLESv2/validationES.h"
#include "libGLESv2/validationES2.h"
#include "libGLESv2/validationES3.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/main.h"
#include "libGLESv2/Query.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/TransformFeedback.h"
#include "libGLESv2/VertexArray.h"
#include "libGLESv2/renderer/BufferImpl.h"

#include "common/mathutil.h"
#include "common/utilities.h"

// FIXME(jmadill): remove this when we support buffer data caching
#include "libGLESv2/renderer/d3d/BufferD3D.h"

namespace gl
{

bool ValidCap(const Context *context, GLenum cap)
{
    switch (cap)
    {
      case GL_CULL_FACE:
      case GL_POLYGON_OFFSET_FILL:
      case GL_SAMPLE_ALPHA_TO_COVERAGE:
      case GL_SAMPLE_COVERAGE:
      case GL_SCISSOR_TEST:
      case GL_STENCIL_TEST:
      case GL_DEPTH_TEST:
      case GL_BLEND:
      case GL_DITHER:
        return true;
      case GL_PRIMITIVE_RESTART_FIXED_INDEX:
      case GL_RASTERIZER_DISCARD:
        return (context->getClientVersion() >= 3);
      default:
        return false;
    }
}

bool ValidTextureTarget(const Context *context, GLenum target)
{
    switch (target)
    {
      case GL_TEXTURE_2D:
      case GL_TEXTURE_CUBE_MAP:
        return true;

      case GL_TEXTURE_3D:
      case GL_TEXTURE_2D_ARRAY:
        return (context->getClientVersion() >= 3);

      default:
        return false;
    }
}

// This function differs from ValidTextureTarget in that the target must be
// usable as the destination of a 2D operation-- so a cube face is valid, but
// GL_TEXTURE_CUBE_MAP is not.
// Note: duplicate of IsInternalTextureTarget
bool ValidTexture2DDestinationTarget(const Context *context, GLenum target)
{
    switch (target)
    {
      case GL_TEXTURE_2D:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        return true;
      case GL_TEXTURE_2D_ARRAY:
      case GL_TEXTURE_3D:
        return (context->getClientVersion() >= 3);
      default:
        return false;
    }
}

bool ValidFramebufferTarget(GLenum target)
{
    META_ASSERT(GL_DRAW_FRAMEBUFFER_ANGLE == GL_DRAW_FRAMEBUFFER && GL_READ_FRAMEBUFFER_ANGLE == GL_READ_FRAMEBUFFER);

    switch (target)
    {
      case GL_FRAMEBUFFER:      return true;
      case GL_READ_FRAMEBUFFER: return true;
      case GL_DRAW_FRAMEBUFFER: return true;
      default:                  return false;
    }
}

bool ValidBufferTarget(const Context *context, GLenum target)
{
    switch (target)
    {
      case GL_ARRAY_BUFFER:
      case GL_ELEMENT_ARRAY_BUFFER:
        return true;

      case GL_PIXEL_PACK_BUFFER:
      case GL_PIXEL_UNPACK_BUFFER:
        return context->getExtensions().pixelBufferObject;

      case GL_COPY_READ_BUFFER:
      case GL_COPY_WRITE_BUFFER:
      case GL_TRANSFORM_FEEDBACK_BUFFER:
      case GL_UNIFORM_BUFFER:
        return (context->getClientVersion() >= 3);

      default:
        return false;
    }
}

bool ValidBufferParameter(const Context *context, GLenum pname)
{
    switch (pname)
    {
      case GL_BUFFER_USAGE:
      case GL_BUFFER_SIZE:
        return true;

      // GL_BUFFER_MAP_POINTER is a special case, and may only be
      // queried with GetBufferPointerv
      case GL_BUFFER_ACCESS_FLAGS:
      case GL_BUFFER_MAPPED:
      case GL_BUFFER_MAP_OFFSET:
      case GL_BUFFER_MAP_LENGTH:
        return (context->getClientVersion() >= 3);

      default:
        return false;
    }
}

bool ValidMipLevel(const Context *context, GLenum target, GLint level)
{
    size_t maxDimension = 0;
    switch (target)
    {
      case GL_TEXTURE_2D:                  maxDimension = context->getCaps().max2DTextureSize;       break;
      case GL_TEXTURE_CUBE_MAP:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: maxDimension = context->getCaps().maxCubeMapTextureSize;  break;
      case GL_TEXTURE_3D:                  maxDimension = context->getCaps().max3DTextureSize;       break;
      case GL_TEXTURE_2D_ARRAY:            maxDimension = context->getCaps().max2DTextureSize;       break;
      default: UNREACHABLE();
    }

    return level <= gl::log2(maxDimension);
}

bool ValidImageSize(const Context *context, GLenum target, GLint level,
                    GLsizei width, GLsizei height, GLsizei depth)
{
    if (level < 0 || width < 0 || height < 0 || depth < 0)
    {
        return false;
    }

    if (!context->getExtensions().textureNPOT &&
        (level != 0 && (!gl::isPow2(width) || !gl::isPow2(height) || !gl::isPow2(depth))))
    {
        return false;
    }

    if (!ValidMipLevel(context, target, level))
    {
        return false;
    }

    return true;
}

bool ValidCompressedImageSize(const Context *context, GLenum internalFormat, GLsizei width, GLsizei height)
{
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
    if (!formatInfo.compressed)
    {
        return false;
    }

    if (width  < 0 || (static_cast<GLuint>(width)  > formatInfo.compressedBlockWidth  && width  % formatInfo.compressedBlockWidth != 0) ||
        height < 0 || (static_cast<GLuint>(height) > formatInfo.compressedBlockHeight && height % formatInfo.compressedBlockHeight != 0))
    {
        return false;
    }

    return true;
}

bool ValidQueryType(const Context *context, GLenum queryType)
{
    META_ASSERT(GL_ANY_SAMPLES_PASSED == GL_ANY_SAMPLES_PASSED_EXT);
    META_ASSERT(GL_ANY_SAMPLES_PASSED_CONSERVATIVE == GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT);

    switch (queryType)
    {
      case GL_ANY_SAMPLES_PASSED:
      case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
        return true;
      case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
        return (context->getClientVersion() >= 3);
      default:
        return false;
    }
}

bool ValidProgram(Context *context, GLuint id)
{
    // ES3 spec (section 2.11.1) -- "Commands that accept shader or program object names will generate the
    // error INVALID_VALUE if the provided name is not the name of either a shader or program object and
    // INVALID_OPERATION if the provided name identifies an object that is not the expected type."

    if (context->getProgram(id) != NULL)
    {
        return true;
    }
    else if (context->getShader(id) != NULL)
    {
        // ID is the wrong type
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }
    else
    {
        // No shader/program object has this ID
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }
}

bool ValidateAttachmentTarget(gl::Context *context, GLenum attachment)
{
    if (attachment >= GL_COLOR_ATTACHMENT0_EXT && attachment <= GL_COLOR_ATTACHMENT15_EXT)
    {
        const unsigned int colorAttachment = (attachment - GL_COLOR_ATTACHMENT0_EXT);

        if (colorAttachment >= context->getCaps().maxColorAttachments)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
    }
    else
    {
        switch (attachment)
        {
          case GL_DEPTH_ATTACHMENT:
          case GL_STENCIL_ATTACHMENT:
            break;

          case GL_DEPTH_STENCIL_ATTACHMENT:
            if (context->getClientVersion() < 3)
            {
                context->recordError(Error(GL_INVALID_ENUM));
                return false;
            }
            break;

          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
    }

    return true;
}

bool ValidateRenderbufferStorageParameters(gl::Context *context, GLenum target, GLsizei samples,
                                           GLenum internalformat, GLsizei width, GLsizei height,
                                           bool angleExtension)
{
    switch (target)
    {
      case GL_RENDERBUFFER:
        break;
      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (width < 0 || height < 0 || samples < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    const TextureCaps &formatCaps = context->getTextureCaps().get(internalformat);
    if (!formatCaps.renderable)
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    // ANGLE_framebuffer_multisample does not explicitly state that the internal format must be
    // sized but it does state that the format must be in the ES2.0 spec table 4.5 which contains
    // only sized internal formats. The ES3 spec (section 4.4.2) does, however, state that the
    // internal format must be sized and not an integer format if samples is greater than zero.
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalformat);
    if (formatInfo.pixelBytes == 0)
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if ((formatInfo.componentType == GL_UNSIGNED_INT || formatInfo.componentType == GL_INT) && samples > 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (static_cast<GLuint>(std::max(width, height)) > context->getCaps().maxRenderbufferSize)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    // ANGLE_framebuffer_multisample states that the value of samples must be less than or equal
    // to MAX_SAMPLES_ANGLE (Context::getMaxSupportedSamples) while the ES3.0 spec (section 4.4.2)
    // states that samples must be less than or equal to the maximum samples for the specified
    // internal format.
    if (angleExtension)
    {
        ASSERT(context->getExtensions().framebufferMultisample);
        if (static_cast<GLuint>(samples) > context->getExtensions().maxSamples)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }

        // Check if this specific format supports enough samples
        if (static_cast<GLuint>(samples) > formatCaps.getMaxSamples())
        {
            context->recordError(Error(GL_OUT_OF_MEMORY));
            return false;
        }
    }
    else
    {
        if (static_cast<GLuint>(samples) > formatCaps.getMaxSamples())
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
    }

    GLuint handle = context->getState().getRenderbufferId();
    if (handle == 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateFramebufferRenderbufferParameters(gl::Context *context, GLenum target, GLenum attachment,
                                               GLenum renderbuffertarget, GLuint renderbuffer)
{
    if (!ValidFramebufferTarget(target))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
    GLuint framebufferHandle = context->getState().getTargetFramebuffer(target)->id();

    if (!framebuffer || (framebufferHandle == 0 && renderbuffer != 0))
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (!ValidateAttachmentTarget(context, attachment))
    {
        return false;
    }

    // [OpenGL ES 2.0.25] Section 4.4.3 page 112
    // [OpenGL ES 3.0.2] Section 4.4.2 page 201
    // 'renderbuffer' must be either zero or the name of an existing renderbuffer object of
    // type 'renderbuffertarget', otherwise an INVALID_OPERATION error is generated.
    if (renderbuffer != 0)
    {
        if (!context->getRenderbuffer(renderbuffer))
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    return true;
}

static bool IsPartialBlit(gl::Context *context, gl::FramebufferAttachment *readBuffer, gl::FramebufferAttachment *writeBuffer,
                          GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                          GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1)
{
    if (srcX0 != 0 || srcY0 != 0 || dstX0 != 0 || dstY0 != 0 ||
        dstX1 != writeBuffer->getWidth() || dstY1 != writeBuffer->getHeight() ||
        srcX1 != readBuffer->getWidth() || srcY1 != readBuffer->getHeight())
    {
        return true;
    }
    else if (context->getState().isScissorTestEnabled())
    {
        const Rectangle &scissor = context->getState().getScissor();

        return scissor.x > 0 || scissor.y > 0 ||
               scissor.width < writeBuffer->getWidth() ||
               scissor.height < writeBuffer->getHeight();
    }
    else
    {
        return false;
    }
}

bool ValidateBlitFramebufferParameters(gl::Context *context, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                       GLenum filter, bool fromAngleExtension)
{
    switch (filter)
    {
      case GL_NEAREST:
        break;
      case GL_LINEAR:
        if (fromAngleExtension)
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;
      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if ((mask & ~(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) != 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (mask == 0)
    {
        // ES3.0 spec, section 4.3.2 specifies that a mask of zero is valid and no
        // buffers are copied.
        return false;
    }

    if (fromAngleExtension && (srcX1 - srcX0 != dstX1 - dstX0 || srcY1 - srcY0 != dstY1 - dstY0))
    {
        ERR("Scaling and flipping in BlitFramebufferANGLE not supported by this implementation.");
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    // ES3.0 spec, section 4.3.2 states that linear filtering is only available for the
    // color buffer, leaving only nearest being unfiltered from above
    if ((mask & ~GL_COLOR_BUFFER_BIT) != 0 && filter != GL_NEAREST)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (context->getState().getReadFramebuffer()->id() == context->getState().getDrawFramebuffer()->id())
    {
        if (fromAngleExtension)
        {
            ERR("Blits with the same source and destination framebuffer are not supported by this "
                "implementation.");
        }
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    gl::Framebuffer *readFramebuffer = context->getState().getReadFramebuffer();
    gl::Framebuffer *drawFramebuffer = context->getState().getDrawFramebuffer();

    if (!readFramebuffer || !drawFramebuffer)
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (!readFramebuffer->completeness(context->getData()))
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (!drawFramebuffer->completeness(context->getData()))
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (drawFramebuffer->getSamples(context->getData()) != 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    bool sameBounds = srcX0 == dstX0 && srcY0 == dstY0 && srcX1 == dstX1 && srcY1 == dstY1;

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        gl::FramebufferAttachment *readColorBuffer = readFramebuffer->getReadColorbuffer();
        gl::FramebufferAttachment *drawColorBuffer = drawFramebuffer->getFirstColorbuffer();

        if (readColorBuffer && drawColorBuffer)
        {
            GLenum readInternalFormat = readColorBuffer->getActualFormat();
            const InternalFormat &readFormatInfo = GetInternalFormatInfo(readInternalFormat);

            for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; i++)
            {
                if (drawFramebuffer->isEnabledColorAttachment(i))
                {
                    GLenum drawInternalFormat = drawFramebuffer->getColorbuffer(i)->getActualFormat();
                    const InternalFormat &drawFormatInfo = GetInternalFormatInfo(drawInternalFormat);

                    // The GL ES 3.0.2 spec (pg 193) states that:
                    // 1) If the read buffer is fixed point format, the draw buffer must be as well
                    // 2) If the read buffer is an unsigned integer format, the draw buffer must be as well
                    // 3) If the read buffer is a signed integer format, the draw buffer must be as well
                    if ( (readFormatInfo.componentType == GL_UNSIGNED_NORMALIZED || readFormatInfo.componentType == GL_SIGNED_NORMALIZED) &&
                        !(drawFormatInfo.componentType == GL_UNSIGNED_NORMALIZED || drawFormatInfo.componentType == GL_SIGNED_NORMALIZED))
                    {
                        context->recordError(Error(GL_INVALID_OPERATION));
                        return false;
                    }

                    if (readFormatInfo.componentType == GL_UNSIGNED_INT && drawFormatInfo.componentType != GL_UNSIGNED_INT)
                    {
                        context->recordError(Error(GL_INVALID_OPERATION));
                        return false;
                    }

                    if (readFormatInfo.componentType == GL_INT && drawFormatInfo.componentType != GL_INT)
                    {
                        context->recordError(Error(GL_INVALID_OPERATION));
                        return false;
                    }

                    if (readColorBuffer->getSamples() > 0 && (readInternalFormat != drawInternalFormat || !sameBounds))
                    {
                        context->recordError(Error(GL_INVALID_OPERATION));
                        return false;
                    }
                }
            }

            if ((readFormatInfo.componentType == GL_INT || readFormatInfo.componentType == GL_UNSIGNED_INT) && filter == GL_LINEAR)
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }

            if (fromAngleExtension)
            {
                const GLenum readColorbufferType = readFramebuffer->getReadColorbufferType();
                if (readColorbufferType != GL_TEXTURE_2D && readColorbufferType != GL_RENDERBUFFER)
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }

                for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
                {
                    if (drawFramebuffer->isEnabledColorAttachment(colorAttachment))
                    {
                        FramebufferAttachment *attachment = drawFramebuffer->getColorbuffer(colorAttachment);
                        ASSERT(attachment);

                        if (attachment->type() != GL_TEXTURE_2D && attachment->type() != GL_RENDERBUFFER)
                        {
                            context->recordError(Error(GL_INVALID_OPERATION));
                            return false;
                        }

                        // Return an error if the destination formats do not match
                        if (attachment->getInternalFormat() != readColorBuffer->getInternalFormat())
                        {
                            context->recordError(Error(GL_INVALID_OPERATION));
                            return false;
                        }
                    }
                }

                int readSamples = readFramebuffer->getSamples(context->getData());

                if (readSamples != 0 && IsPartialBlit(context, readColorBuffer, drawColorBuffer,
                                                      srcX0, srcY0, srcX1, srcY1,
                                                      dstX0, dstY0, dstX1, dstY1))
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
        }
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        gl::FramebufferAttachment *readDepthBuffer = readFramebuffer->getDepthbuffer();
        gl::FramebufferAttachment *drawDepthBuffer = drawFramebuffer->getDepthbuffer();

        if (readDepthBuffer && drawDepthBuffer)
        {
            if (readDepthBuffer->getActualFormat() != drawDepthBuffer->getActualFormat())
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }

            if (readDepthBuffer->getSamples() > 0 && !sameBounds)
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }

            if (fromAngleExtension)
            {
                if (IsPartialBlit(context, readDepthBuffer, drawDepthBuffer,
                                  srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1))
                {
                    ERR("Only whole-buffer depth and stencil blits are supported by this implementation.");
                    context->recordError(Error(GL_INVALID_OPERATION)); // only whole-buffer copies are permitted
                    return false;
                }

                if (readDepthBuffer->getSamples() != 0 || drawDepthBuffer->getSamples() != 0)
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
        }
    }

    if (mask & GL_STENCIL_BUFFER_BIT)
    {
        gl::FramebufferAttachment *readStencilBuffer = readFramebuffer->getStencilbuffer();
        gl::FramebufferAttachment *drawStencilBuffer = drawFramebuffer->getStencilbuffer();

        if (readStencilBuffer && drawStencilBuffer)
        {
            if (readStencilBuffer->getActualFormat() != drawStencilBuffer->getActualFormat())
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }

            if (readStencilBuffer->getSamples() > 0 && !sameBounds)
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }

            if (fromAngleExtension)
            {
                if (IsPartialBlit(context, readStencilBuffer, drawStencilBuffer,
                                  srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1))
                {
                    ERR("Only whole-buffer depth and stencil blits are supported by this implementation.");
                    context->recordError(Error(GL_INVALID_OPERATION)); // only whole-buffer copies are permitted
                    return false;
                }

                if (readStencilBuffer->getSamples() != 0 || drawStencilBuffer->getSamples() != 0)
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
        }
    }

    return true;
}

bool ValidateGetVertexAttribParameters(Context *context, GLenum pname)
{
    switch (pname)
    {
      case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
      case GL_VERTEX_ATTRIB_ARRAY_SIZE:
      case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
      case GL_VERTEX_ATTRIB_ARRAY_TYPE:
      case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
      case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
      case GL_CURRENT_VERTEX_ATTRIB:
        return true;

      case GL_VERTEX_ATTRIB_ARRAY_DIVISOR:
        // Don't verify ES3 context because GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE uses
        // the same constant.
        META_ASSERT(GL_VERTEX_ATTRIB_ARRAY_DIVISOR == GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE);
        return true;

      case GL_VERTEX_ATTRIB_ARRAY_INTEGER:
        if (context->getClientVersion() < 3)
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        return true;

      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }
}

bool ValidateTexParamParameters(gl::Context *context, GLenum pname, GLint param)
{
    switch (pname)
    {
      case GL_TEXTURE_WRAP_R:
      case GL_TEXTURE_SWIZZLE_R:
      case GL_TEXTURE_SWIZZLE_G:
      case GL_TEXTURE_SWIZZLE_B:
      case GL_TEXTURE_SWIZZLE_A:
      case GL_TEXTURE_BASE_LEVEL:
      case GL_TEXTURE_MAX_LEVEL:
      case GL_TEXTURE_COMPARE_MODE:
      case GL_TEXTURE_COMPARE_FUNC:
      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
        if (context->getClientVersion() < 3)
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      default: break;
    }

    switch (pname)
    {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_WRAP_R:
        switch (param)
        {
          case GL_REPEAT:
          case GL_CLAMP_TO_EDGE:
          case GL_MIRRORED_REPEAT:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }

      case GL_TEXTURE_MIN_FILTER:
        switch (param)
        {
          case GL_NEAREST:
          case GL_LINEAR:
          case GL_NEAREST_MIPMAP_NEAREST:
          case GL_LINEAR_MIPMAP_NEAREST:
          case GL_NEAREST_MIPMAP_LINEAR:
          case GL_LINEAR_MIPMAP_LINEAR:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      case GL_TEXTURE_MAG_FILTER:
        switch (param)
        {
          case GL_NEAREST:
          case GL_LINEAR:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      case GL_TEXTURE_USAGE_ANGLE:
        switch (param)
        {
          case GL_NONE:
          case GL_FRAMEBUFFER_ATTACHMENT_ANGLE:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        if (!context->getExtensions().textureFilterAnisotropic)
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }

        // we assume the parameter passed to this validation method is truncated, not rounded
        if (param < 1)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
        return true;

      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
        // any value is permissible
        return true;

      case GL_TEXTURE_COMPARE_MODE:
        // Acceptable mode parameters from GLES 3.0.2 spec, table 3.17
        switch (param)
        {
          case GL_NONE:
          case GL_COMPARE_REF_TO_TEXTURE:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      case GL_TEXTURE_COMPARE_FUNC:
        // Acceptable function parameters from GLES 3.0.2 spec, table 3.17
        switch (param)
        {
          case GL_LEQUAL:
          case GL_GEQUAL:
          case GL_LESS:
          case GL_GREATER:
          case GL_EQUAL:
          case GL_NOTEQUAL:
          case GL_ALWAYS:
          case GL_NEVER:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      case GL_TEXTURE_SWIZZLE_R:
      case GL_TEXTURE_SWIZZLE_G:
      case GL_TEXTURE_SWIZZLE_B:
      case GL_TEXTURE_SWIZZLE_A:
        switch (param)
        {
          case GL_RED:
          case GL_GREEN:
          case GL_BLUE:
          case GL_ALPHA:
          case GL_ZERO:
          case GL_ONE:
            return true;
          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;

      case GL_TEXTURE_BASE_LEVEL:
      case GL_TEXTURE_MAX_LEVEL:
        if (param < 0)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
        return true;

      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }
}

bool ValidateSamplerObjectParameter(gl::Context *context, GLenum pname)
{
    switch (pname)
    {
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_WRAP_R:
      case GL_TEXTURE_MIN_LOD:
      case GL_TEXTURE_MAX_LOD:
      case GL_TEXTURE_COMPARE_MODE:
      case GL_TEXTURE_COMPARE_FUNC:
        return true;

      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }
}

bool ValidateReadPixelsParameters(gl::Context *context, GLint x, GLint y, GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, GLsizei *bufSize, GLvoid *pixels)
{
    gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();
    ASSERT(framebuffer);

    if (framebuffer->completeness(context->getData()) != GL_FRAMEBUFFER_COMPLETE)
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (context->getState().getReadFramebuffer()->id() != 0 &&
        framebuffer->getSamples(context->getData()) != 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (!framebuffer->getReadColorbuffer())
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    GLenum currentInternalFormat, currentFormat, currentType;
    GLuint clientVersion = context->getClientVersion();

    context->getCurrentReadFormatType(&currentInternalFormat, &currentFormat, &currentType);

    bool validReadFormat = (clientVersion < 3) ? ValidES2ReadFormatType(context, format, type) :
                                                 ValidES3ReadFormatType(context, currentInternalFormat, format, type);

    if (!(currentFormat == format && currentType == type) && !validReadFormat)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    GLenum sizedInternalFormat = GetSizedInternalFormat(format, type);
    const InternalFormat &sizedFormatInfo = GetInternalFormatInfo(sizedInternalFormat);

    GLsizei outputPitch = sizedFormatInfo.computeRowPitch(type, width, context->getState().getPackAlignment());
    // sized query sanity check
    if (bufSize)
    {
        int requiredSize = outputPitch * height;
        if (requiredSize > *bufSize)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    return true;
}

bool ValidateBeginQuery(gl::Context *context, GLenum target, GLuint id)
{
    if (!ValidQueryType(context, target))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (id == 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    // From EXT_occlusion_query_boolean: If BeginQueryEXT is called with an <id>
    // of zero, if the active query object name for <target> is non-zero (for the
    // targets ANY_SAMPLES_PASSED_EXT and ANY_SAMPLES_PASSED_CONSERVATIVE_EXT, if
    // the active query for either target is non-zero), if <id> is the name of an
    // existing query object whose type does not match <target>, or if <id> is the
    // active query object name for any query type, the error INVALID_OPERATION is
    // generated.

    // Ensure no other queries are active
    // NOTE: If other queries than occlusion are supported, we will need to check
    // separately that:
    //    a) The query ID passed is not the current active query for any target/type
    //    b) There are no active queries for the requested target (and in the case
    //       of GL_ANY_SAMPLES_PASSED_EXT and GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT,
    //       no query may be active for either if glBeginQuery targets either.
    if (context->getState().isQueryActive())
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    Query *queryObject = context->getQuery(id, true, target);

    // check that name was obtained with glGenQueries
    if (!queryObject)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    // check for type mismatch
    if (queryObject->getType() != target)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateEndQuery(gl::Context *context, GLenum target)
{
    if (!ValidQueryType(context, target))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    const Query *queryObject = context->getState().getActiveQuery(target);

    if (queryObject == NULL)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

static bool ValidateUniformCommonBase(gl::Context *context, GLenum targetUniformType,
                                      GLint location, GLsizei count, LinkedUniform **uniformOut)
{
    if (count < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
    if (!programBinary)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (location == -1)
    {
        // Silently ignore the uniform command
        return false;
    }

    if (!programBinary->isValidUniformLocation(location))
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    LinkedUniform *uniform = programBinary->getUniformByLocation(location);

    // attempting to write an array to a non-array uniform is an INVALID_OPERATION
    if (uniform->elementCount() == 1 && count > 1)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    *uniformOut = uniform;
    return true;
}

bool ValidateUniform(gl::Context *context, GLenum uniformType, GLint location, GLsizei count)
{
    // Check for ES3 uniform entry points
    if (VariableComponentType(uniformType) == GL_UNSIGNED_INT && context->getClientVersion() < 3)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    LinkedUniform *uniform = NULL;
    if (!ValidateUniformCommonBase(context, uniformType, location, count, &uniform))
    {
        return false;
    }

    GLenum targetBoolType = VariableBoolVectorType(uniformType);
    bool samplerUniformCheck = (IsSampler(uniform->type) && uniformType == GL_INT);
    if (!samplerUniformCheck && uniformType != uniform->type && targetBoolType != uniform->type)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateUniformMatrix(gl::Context *context, GLenum matrixType, GLint location, GLsizei count,
                           GLboolean transpose)
{
    // Check for ES3 uniform entry points
    int rows = VariableRowCount(matrixType);
    int cols = VariableColumnCount(matrixType);
    if (rows != cols && context->getClientVersion() < 3)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (transpose != GL_FALSE && context->getClientVersion() < 3)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    LinkedUniform *uniform = NULL;
    if (!ValidateUniformCommonBase(context, matrixType, location, count, &uniform))
    {
        return false;
    }

    if (uniform->type != matrixType)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateStateQuery(gl::Context *context, GLenum pname, GLenum *nativeType, unsigned int *numParams)
{
    if (!context->getQueryParameterInfo(pname, nativeType, numParams))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (pname >= GL_DRAW_BUFFER0 && pname <= GL_DRAW_BUFFER15)
    {
        unsigned int colorAttachment = (pname - GL_DRAW_BUFFER0);

        if (colorAttachment >= context->getCaps().maxDrawBuffers)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    switch (pname)
    {
      case GL_TEXTURE_BINDING_2D:
      case GL_TEXTURE_BINDING_CUBE_MAP:
      case GL_TEXTURE_BINDING_3D:
      case GL_TEXTURE_BINDING_2D_ARRAY:
        if (context->getState().getActiveSampler() >= context->getCaps().maxCombinedTextureImageUnits)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
        break;

      case GL_IMPLEMENTATION_COLOR_READ_TYPE:
      case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
        {
            Framebuffer *framebuffer = context->getState().getReadFramebuffer();
            ASSERT(framebuffer);
            if (framebuffer->completeness(context->getData()) != GL_FRAMEBUFFER_COMPLETE)
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }

            FramebufferAttachment *attachment = framebuffer->getReadColorbuffer();
            if (!attachment)
            {
                context->recordError(Error(GL_INVALID_OPERATION));
                return false;
            }
        }
        break;

      default:
        break;
    }

    // pname is valid, but there are no parameters to return
    if (numParams == 0)
    {
        return false;
    }

    return true;
}

bool ValidateCopyTexImageParametersBase(gl::Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                        GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                        GLint border, GLenum *textureFormatOut)
{

    if (!ValidTexture2DDestinationTarget(context, target))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (level < 0 || xoffset < 0 || yoffset < 0 || zoffset < 0 || width < 0 || height < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (border != 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (!ValidMipLevel(context, target, level))
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();
    if (framebuffer->completeness(context->getData()) != GL_FRAMEBUFFER_COMPLETE)
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (context->getState().getReadFramebuffer()->id() != 0 && framebuffer->getSamples(context->getData()) != 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::Caps &caps = context->getCaps();

    gl::Texture *texture = NULL;
    GLenum textureInternalFormat = GL_NONE;
    GLint textureLevelWidth = 0;
    GLint textureLevelHeight = 0;
    GLint textureLevelDepth = 0;
    GLuint maxDimension = 0;

    switch (target)
    {
      case GL_TEXTURE_2D:
        {
            gl::Texture2D *texture2d = context->getTexture2D();
            if (texture2d)
            {
                textureInternalFormat = texture2d->getInternalFormat(level);
                textureLevelWidth = texture2d->getWidth(level);
                textureLevelHeight = texture2d->getHeight(level);
                textureLevelDepth = 1;
                texture = texture2d;
                maxDimension = caps.max2DTextureSize;
            }
        }
        break;

      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            gl::TextureCubeMap *textureCube = context->getTextureCubeMap();
            if (textureCube)
            {
                textureInternalFormat = textureCube->getInternalFormat(target, level);
                textureLevelWidth = textureCube->getWidth(target, level);
                textureLevelHeight = textureCube->getHeight(target, level);
                textureLevelDepth = 1;
                texture = textureCube;
                maxDimension = caps.maxCubeMapTextureSize;
            }
        }
        break;

      case GL_TEXTURE_2D_ARRAY:
        {
            gl::Texture2DArray *texture2dArray = context->getTexture2DArray();
            if (texture2dArray)
            {
                textureInternalFormat = texture2dArray->getInternalFormat(level);
                textureLevelWidth = texture2dArray->getWidth(level);
                textureLevelHeight = texture2dArray->getHeight(level);
                textureLevelDepth = texture2dArray->getLayers(level);
                texture = texture2dArray;
                maxDimension = caps.max2DTextureSize;
            }
        }
        break;

      case GL_TEXTURE_3D:
        {
            gl::Texture3D *texture3d = context->getTexture3D();
            if (texture3d)
            {
                textureInternalFormat = texture3d->getInternalFormat(level);
                textureLevelWidth = texture3d->getWidth(level);
                textureLevelHeight = texture3d->getHeight(level);
                textureLevelDepth = texture3d->getDepth(level);
                texture = texture3d;
                maxDimension = caps.max3DTextureSize;
            }
        }
        break;

      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (!texture)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (texture->isImmutable() && !isSubImage)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalformat);

    if (formatInfo.depthBits > 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (formatInfo.compressed)
    {
        if (((width % formatInfo.compressedBlockWidth) != 0 && width != textureLevelWidth) ||
            ((height % formatInfo.compressedBlockHeight) != 0 && height != textureLevelHeight))
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }

    if (isSubImage)
    {
        if (xoffset + width > textureLevelWidth ||
            yoffset + height > textureLevelHeight ||
            zoffset >= textureLevelDepth)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
    }
    else
    {
        if (IsCubemapTextureTarget(target) && width != height)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }

        if (!formatInfo.textureSupport(context->getClientVersion(), context->getExtensions()))
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }

        int maxLevelDimension = (maxDimension >> level);
        if (static_cast<int>(width) > maxLevelDimension || static_cast<int>(height) > maxLevelDimension)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
    }

    *textureFormatOut = textureInternalFormat;
    return true;
}

static bool ValidateDrawBase(Context *context, GLenum mode, GLsizei count, GLsizei maxVertex, GLsizei primcount)
{
    switch (mode)
    {
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
      case GL_TRIANGLES:
      case GL_TRIANGLE_STRIP:
      case GL_TRIANGLE_FAN:
        break;
      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (count < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    const State &state = context->getState();

    // Check for mapped buffers
    if (state.hasMappedBuffer(GL_ARRAY_BUFFER))
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::DepthStencilState &depthStencilState = state.getDepthStencilState();
    if (depthStencilState.stencilWritemask != depthStencilState.stencilBackWritemask ||
        state.getStencilRef() != state.getStencilBackRef() ||
        depthStencilState.stencilMask != depthStencilState.stencilBackMask)
    {
        // Note: these separate values are not supported in WebGL, due to D3D's limitations.
        // See Section 6.10 of the WebGL 1.0 spec
        ERR("This ANGLE implementation does not support separate front/back stencil "
            "writemasks, reference values, or stencil mask values.");
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::Framebuffer *fbo = state.getDrawFramebuffer();
    if (!fbo || fbo->completeness(context->getData()) != GL_FRAMEBUFFER_COMPLETE)
    {
        context->recordError(Error(GL_INVALID_FRAMEBUFFER_OPERATION));
        return false;
    }

    if (state.getCurrentProgramId() == 0)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    gl::ProgramBinary *programBinary = state.getCurrentProgramBinary();
    if (!programBinary->validateSamplers(NULL, context->getCaps()))
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    // Buffer validations
    const VertexArray *vao = state.getVertexArray();
    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        const VertexAttribute &attrib = vao->getVertexAttribute(attributeIndex);
        bool attribActive = (programBinary->getSemanticIndex(attributeIndex) != -1);
        if (attribActive && attrib.enabled)
        {
            gl::Buffer *buffer = attrib.buffer.get();

            if (buffer)
            {
                GLint64 attribStride = static_cast<GLint64>(ComputeVertexAttributeStride(attrib));
                GLint64 maxVertexElement = 0;

                if (attrib.divisor > 0)
                {
                    maxVertexElement = static_cast<GLint64>(primcount) / static_cast<GLint64>(attrib.divisor);
                }
                else
                {
                    maxVertexElement = static_cast<GLint64>(maxVertex);
                }

                GLint64 attribDataSize = maxVertexElement * attribStride;

                // [OpenGL ES 3.0.2] section 2.9.4 page 40:
                // We can return INVALID_OPERATION if our vertex attribute does not have
                // enough backing data.
                if (attribDataSize > buffer->getSize())
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
            else if (attrib.pointer == NULL)
            {
                // This is an application error that would normally result in a crash,
                // but we catch it and return an error
                context->recordError(Error(GL_INVALID_OPERATION, "An enabled vertex array has no buffer and no pointer."));
                return false;
            }
        }
    }

    // No-op if zero count
    return (count > 0);
}

bool ValidateDrawArrays(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    if (first < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    const State &state = context->getState();
    gl::TransformFeedback *curTransformFeedback = state.getCurrentTransformFeedback();
    if (curTransformFeedback && curTransformFeedback->isStarted() && !curTransformFeedback->isPaused() &&
        curTransformFeedback->getDrawMode() != mode)
    {
        // It is an invalid operation to call DrawArrays or DrawArraysInstanced with a draw mode
        // that does not match the current transform feedback object's draw mode (if transform feedback
        // is active), (3.0.2, section 2.14, pg 86)
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (!ValidateDrawBase(context, mode, count, count, primcount))
    {
        return false;
    }

    return true;
}

bool ValidateDrawArraysInstanced(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    if (primcount < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (!ValidateDrawArrays(context, mode, first, count, primcount))
    {
        return false;
    }

    // No-op if zero primitive count
    return (primcount > 0);
}

static bool ValidateDrawInstancedANGLE(Context *context)
{
    // Verify there is at least one active attribute with a divisor of zero
    const gl::State& state = context->getState();

    gl::ProgramBinary *programBinary = state.getCurrentProgramBinary();

    const VertexArray *vao = state.getVertexArray();
    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        const VertexAttribute &attrib = vao->getVertexAttribute(attributeIndex);
        bool active = (programBinary->getSemanticIndex(attributeIndex) != -1);
        if (active && attrib.divisor == 0)
        {
            return true;
        }
    }

    context->recordError(Error(GL_INVALID_OPERATION, "ANGLE_instanced_arrays requires that at least one active attribute"
                                                     "has a divisor of zero."));
    return false;
}

bool ValidateDrawArraysInstancedANGLE(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    if (!ValidateDrawInstancedANGLE(context))
    {
        return false;
    }

    return ValidateDrawArraysInstanced(context, mode, first, count, primcount);
}

bool ValidateDrawElements(Context *context, GLenum mode, GLsizei count, GLenum type,
                          const GLvoid* indices, GLsizei primcount, rx::RangeUI *indexRangeOut)
{
    switch (type)
    {
      case GL_UNSIGNED_BYTE:
      case GL_UNSIGNED_SHORT:
        break;
      case GL_UNSIGNED_INT:
        if (!context->getExtensions().elementIndexUint)
        {
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
        break;
      default:
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    const State &state = context->getState();

    gl::TransformFeedback *curTransformFeedback = state.getCurrentTransformFeedback();
    if (curTransformFeedback && curTransformFeedback->isStarted() && !curTransformFeedback->isPaused())
    {
        // It is an invalid operation to call DrawElements, DrawRangeElements or DrawElementsInstanced
        // while transform feedback is active, (3.0.2, section 2.14, pg 86)
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    // Check for mapped buffers
    if (state.hasMappedBuffer(GL_ELEMENT_ARRAY_BUFFER))
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    const gl::VertexArray *vao = state.getVertexArray();
    const gl::Buffer *elementArrayBuffer = vao->getElementArrayBuffer();
    if (!indices && !elementArrayBuffer)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (elementArrayBuffer)
    {
        const gl::Type &typeInfo = gl::GetTypeInfo(type);

        GLint64 offset = reinterpret_cast<GLint64>(indices);
        GLint64 byteCount = static_cast<GLint64>(typeInfo.bytes) * static_cast<GLint64>(count)+offset;

        // check for integer overflows
        if (static_cast<GLuint>(count) > (std::numeric_limits<GLuint>::max() / typeInfo.bytes) ||
            byteCount > static_cast<GLint64>(std::numeric_limits<GLuint>::max()))
        {
            context->recordError(Error(GL_OUT_OF_MEMORY));
            return false;
        }

        // Check for reading past the end of the bound buffer object
        if (byteCount > elementArrayBuffer->getSize())
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }
    }
    else if (!indices)
    {
        // Catch this programming error here
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    // Use max index to validate if our vertex buffers are large enough for the pull.
    // TODO: offer fast path, with disabled index validation.
    // TODO: also disable index checking on back-ends that are robust to out-of-range accesses.
    if (elementArrayBuffer)
    {
        uintptr_t offset = reinterpret_cast<uintptr_t>(indices);
        if (!elementArrayBuffer->getIndexRangeCache()->findRange(type, offset, count, indexRangeOut, NULL))
        {
            // FIXME(jmadill): Use buffer data caching instead of the D3D back-end
            rx::BufferD3D *bufferD3D = rx::BufferD3D::makeBufferD3D(elementArrayBuffer->getImplementation());
            const uint8_t *dataPointer = NULL;
            Error error = bufferD3D->getData(&dataPointer);
            if (error.isError())
            {
                context->recordError(error);
                return false;
            }

            const uint8_t *offsetPointer = dataPointer + offset;
            *indexRangeOut = rx::IndexRangeCache::ComputeRange(type, offsetPointer, count);
        }
    }
    else
    {
        *indexRangeOut = rx::IndexRangeCache::ComputeRange(type, indices, count);
    }

    if (!ValidateDrawBase(context, mode, count, static_cast<GLsizei>(indexRangeOut->end), primcount))
    {
        return false;
    }

    return true;
}

bool ValidateDrawElementsInstanced(Context *context,
                                   GLenum mode, GLsizei count, GLenum type,
                                   const GLvoid *indices, GLsizei primcount,
                                   rx::RangeUI *indexRangeOut)
{
    if (primcount < 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (!ValidateDrawElements(context, mode, count, type, indices, primcount, indexRangeOut))
    {
        return false;
    }

    // No-op zero primitive count
    return (primcount > 0);
}

bool ValidateDrawElementsInstancedANGLE(Context *context, GLenum mode, GLsizei count, GLenum type,
                                        const GLvoid *indices, GLsizei primcount, rx::RangeUI *indexRangeOut)
{
    if (!ValidateDrawInstancedANGLE(context))
    {
        return false;
    }

    return ValidateDrawElementsInstanced(context, mode, count, type, indices, primcount, indexRangeOut);
}

bool ValidateFramebufferTextureBase(Context *context, GLenum target, GLenum attachment,
                                    GLuint texture, GLint level)
{
    if (!ValidFramebufferTarget(target))
    {
        context->recordError(Error(GL_INVALID_ENUM));
        return false;
    }

    if (!ValidateAttachmentTarget(context, attachment))
    {
        return false;
    }

    if (texture != 0)
    {
        gl::Texture *tex = context->getTexture(texture);

        if (tex == NULL)
        {
            context->recordError(Error(GL_INVALID_OPERATION));
            return false;
        }

        if (level < 0)
        {
            context->recordError(Error(GL_INVALID_VALUE));
            return false;
        }
    }

    const gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
    GLuint framebufferHandle = context->getState().getTargetFramebuffer(target)->id();

    if (framebufferHandle == 0 || !framebuffer)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateFramebufferTexture2D(Context *context, GLenum target, GLenum attachment,
                                  GLenum textarget, GLuint texture, GLint level)
{
    // Attachments are required to be bound to level 0 in ES2
    if (context->getClientVersion() < 3 && level != 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (!ValidateFramebufferTextureBase(context, target, attachment, texture, level))
    {
        return false;
    }

    if (texture != 0)
    {
        gl::Texture *tex = context->getTexture(texture);
        ASSERT(tex);

        const gl::Caps &caps = context->getCaps();

        switch (textarget)
        {
          case GL_TEXTURE_2D:
            {
                if (level > gl::log2(caps.max2DTextureSize))
                {
                    context->recordError(Error(GL_INVALID_VALUE));
                    return false;
                }
                if (tex->getTarget() != GL_TEXTURE_2D)
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
                gl::Texture2D *tex2d = static_cast<gl::Texture2D *>(tex);
                if (tex2d->isCompressed(level))
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
            break;

          case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
          case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
          case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
          case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
          case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
          case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            {
                if (level > gl::log2(caps.maxCubeMapTextureSize))
                {
                    context->recordError(Error(GL_INVALID_VALUE));
                    return false;
                }
                if (tex->getTarget() != GL_TEXTURE_CUBE_MAP)
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
                gl::TextureCubeMap *texcube = static_cast<gl::TextureCubeMap *>(tex);
                if (texcube->isCompressed(textarget, level))
                {
                    context->recordError(Error(GL_INVALID_OPERATION));
                    return false;
                }
            }
            break;

          default:
            context->recordError(Error(GL_INVALID_ENUM));
            return false;
        }
    }

    return true;
}

bool ValidateGetUniformBase(Context *context, GLuint program, GLint location)
{
    if (program == 0)
    {
        context->recordError(Error(GL_INVALID_VALUE));
        return false;
    }

    if (!ValidProgram(context, program))
    {
        return false;
    }

    gl::Program *programObject = context->getProgram(program);

    if (!programObject || !programObject->isLinked())
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    gl::ProgramBinary *programBinary = programObject->getProgramBinary();
    if (!programBinary)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    if (!programBinary->isValidUniformLocation(location))
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateGetUniformfv(Context *context, GLuint program, GLint location, GLfloat* params)
{
    return ValidateGetUniformBase(context, program, location);
}

bool ValidateGetUniformiv(Context *context, GLuint program, GLint location, GLint* params)
{
    return ValidateGetUniformBase(context, program, location);
}

static bool ValidateSizedGetUniform(Context *context, GLuint program, GLint location, GLsizei bufSize)
{
    if (!ValidateGetUniformBase(context, program, location))
    {
        return false;
    }

    gl::Program *programObject = context->getProgram(program);
    ASSERT(programObject);
    gl::ProgramBinary *programBinary = programObject->getProgramBinary();

    // sized queries -- ensure the provided buffer is large enough
    LinkedUniform *uniform = programBinary->getUniformByLocation(location);
    size_t requiredBytes = VariableExternalSize(uniform->type);
    if (static_cast<size_t>(bufSize) < requiredBytes)
    {
        context->recordError(Error(GL_INVALID_OPERATION));
        return false;
    }

    return true;
}

bool ValidateGetnUniformfvEXT(Context *context, GLuint program, GLint location, GLsizei bufSize, GLfloat* params)
{
    return ValidateSizedGetUniform(context, program, location, bufSize);
}

bool ValidateGetnUniformivEXT(Context *context, GLuint program, GLint location, GLsizei bufSize, GLint* params)
{
    return ValidateSizedGetUniform(context, program, location, bufSize);
}

}
