//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// libGLESv2.cpp: Implements the exported OpenGL ES 2.0 functions.

#undef GL_APICALL
#define GL_APICALL
#define GL_GLEXT_PROTOTYPES

#include "common/version.h"
#include "common/utilities.h"

#include "libGLESv2/main.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/Fence.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Query.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/VertexArray.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/TransformFeedback.h"
#include "libGLESv2/FramebufferAttachment.h"

#include "libGLESv2/validationES.h"
#include "libGLESv2/validationES2.h"
#include "libGLESv2/validationES3.h"
#include "libGLESv2/queryconversions.h"

extern "C"
{

// OpenGL ES 2.0 functions

void GL_APIENTRY glActiveTexture(GLenum texture)
{
    EVENT("(GLenum texture = 0x%X)", texture);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (texture < GL_TEXTURE0 || texture > GL_TEXTURE0 + context->getCaps().maxCombinedTextureImageUnits - 1)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->getState().setActiveSampler(texture - GL_TEXTURE0);
    }
}

void GL_APIENTRY glAttachShader(GLuint program, GLuint shader)
{
    EVENT("(GLuint program = %d, GLuint shader = %d)", program, shader);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);
        gl::Shader *shaderObject = context->getShader(shader);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        if (!shaderObject)
        {
            if (context->getProgram(shader))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        if (!programObject->attachShader(shaderObject))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }
    }
}

void GL_APIENTRY glBeginQueryEXT(GLenum target, GLuint id)
{
    EVENT("(GLenum target = 0x%X, GLuint %d)", target, id);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateBeginQuery(context, target, id))
        {
            return;
        }

        gl::Error error = context->beginQuery(target, id);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name)
{
    EVENT("(GLuint program = %d, GLuint index = %d, const GLchar* name = 0x%0.8p)", program, index, name);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        if (strncmp(name, "gl_", 3) == 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        programObject->bindAttributeLocation(index, name);
    }
}

void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
    EVENT("(GLenum target = 0x%X, GLuint buffer = %d)", target, buffer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (target)
        {
          case GL_ARRAY_BUFFER:
            context->bindArrayBuffer(buffer);
            return;
          case GL_ELEMENT_ARRAY_BUFFER:
            context->bindElementArrayBuffer(buffer);
            return;
          case GL_COPY_READ_BUFFER:
            context->bindCopyReadBuffer(buffer);
            return;
          case GL_COPY_WRITE_BUFFER:
            context->bindCopyWriteBuffer(buffer);
            return;
          case GL_PIXEL_PACK_BUFFER:
            context->bindPixelPackBuffer(buffer);
            return;
          case GL_PIXEL_UNPACK_BUFFER:
            context->bindPixelUnpackBuffer(buffer);
            return;
          case GL_UNIFORM_BUFFER:
            context->bindGenericUniformBuffer(buffer);
            return;
          case GL_TRANSFORM_FEEDBACK_BUFFER:
            context->bindGenericTransformFeedbackBuffer(buffer);
            return;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    EVENT("(GLenum target = 0x%X, GLuint framebuffer = %d)", target, framebuffer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidFramebufferTarget(target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (target == GL_READ_FRAMEBUFFER_ANGLE || target == GL_FRAMEBUFFER)
        {
            context->bindReadFramebuffer(framebuffer);
        }

        if (target == GL_DRAW_FRAMEBUFFER_ANGLE || target == GL_FRAMEBUFFER)
        {
            context->bindDrawFramebuffer(framebuffer);
        }
    }
}

void GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    EVENT("(GLenum target = 0x%X, GLuint renderbuffer = %d)", target, renderbuffer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (target != GL_RENDERBUFFER)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->bindRenderbuffer(renderbuffer);
    }
}

void GL_APIENTRY glBindTexture(GLenum target, GLuint texture)
{
    EVENT("(GLenum target = 0x%X, GLuint texture = %d)", target, texture);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Texture *textureObject = context->getTexture(texture);

        if (textureObject && textureObject->getTarget() != target && texture != 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
          case GL_TEXTURE_CUBE_MAP:
            break;

          case GL_TEXTURE_3D:
          case GL_TEXTURE_2D_ARRAY:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->bindTexture(target, texture);
    }
}

void GL_APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    EVENT("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
          red, green, blue, alpha);

    gl::Context* context = gl::getNonLostContext();

    if (context)
    {
        context->getState().setBlendColor(gl::clamp01(red), gl::clamp01(green), gl::clamp01(blue), gl::clamp01(alpha));
    }
}

void GL_APIENTRY glBlendEquation(GLenum mode)
{
    glBlendEquationSeparate(mode, mode);
}

void GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    EVENT("(GLenum modeRGB = 0x%X, GLenum modeAlpha = 0x%X)", modeRGB, modeAlpha);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (modeRGB)
        {
          case GL_FUNC_ADD:
          case GL_FUNC_SUBTRACT:
          case GL_FUNC_REVERSE_SUBTRACT:
          case GL_MIN:
          case GL_MAX:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (modeAlpha)
        {
          case GL_FUNC_ADD:
          case GL_FUNC_SUBTRACT:
          case GL_FUNC_REVERSE_SUBTRACT:
          case GL_MIN:
          case GL_MAX:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->getState().setBlendEquation(modeRGB, modeAlpha);
    }
}

void GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    EVENT("(GLenum srcRGB = 0x%X, GLenum dstRGB = 0x%X, GLenum srcAlpha = 0x%X, GLenum dstAlpha = 0x%X)",
          srcRGB, dstRGB, srcAlpha, dstAlpha);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (srcRGB)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
          case GL_SRC_ALPHA_SATURATE:
            break;

          default:
              context->recordError(gl::Error(GL_INVALID_ENUM));
              return;
        }

        switch (dstRGB)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
            break;

          case GL_SRC_ALPHA_SATURATE:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (srcAlpha)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
          case GL_SRC_ALPHA_SATURATE:
            break;

          default:
              context->recordError(gl::Error(GL_INVALID_ENUM));
              return;
        }

        switch (dstAlpha)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
            break;

          case GL_SRC_ALPHA_SATURATE:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        bool constantColorUsed = (srcRGB == GL_CONSTANT_COLOR || srcRGB == GL_ONE_MINUS_CONSTANT_COLOR ||
                                  dstRGB == GL_CONSTANT_COLOR || dstRGB == GL_ONE_MINUS_CONSTANT_COLOR);

        bool constantAlphaUsed = (srcRGB == GL_CONSTANT_ALPHA || srcRGB == GL_ONE_MINUS_CONSTANT_ALPHA ||
                                  dstRGB == GL_CONSTANT_ALPHA || dstRGB == GL_ONE_MINUS_CONSTANT_ALPHA);

        if (constantColorUsed && constantAlphaUsed)
        {
            ERR("Simultaneous use of GL_CONSTANT_ALPHA/GL_ONE_MINUS_CONSTANT_ALPHA and GL_CONSTANT_COLOR/GL_ONE_MINUS_CONSTANT_COLOR invalid under WebGL");
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->getState().setBlendFactors(srcRGB, dstRGB, srcAlpha, dstAlpha);
    }
}

void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage)
{
    EVENT("(GLenum target = 0x%X, GLsizeiptr size = %d, const GLvoid* data = 0x%0.8p, GLenum usage = %d)",
          target, size, data, usage);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (size < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (usage)
        {
          case GL_STREAM_DRAW:
          case GL_STATIC_DRAW:
          case GL_DYNAMIC_DRAW:
            break;

          case GL_STREAM_READ:
          case GL_STREAM_COPY:
          case GL_STATIC_READ:
          case GL_STATIC_COPY:
          case GL_DYNAMIC_READ:
          case GL_DYNAMIC_COPY:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          default:
              context->recordError(gl::Error(GL_INVALID_ENUM));
              return;
        }

        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (!buffer)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Error error = buffer->bufferData(data, size, usage);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data)
{
    EVENT("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr size = %d, const GLvoid* data = 0x%0.8p)",
          target, offset, size, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (size < 0 || offset < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (data == NULL)
        {
            return;
        }

        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (!buffer)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (buffer->isMapped())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // Check for possible overflow of size + offset
        if (!rx::IsUnsignedAdditionSafe<size_t>(size, offset))
        {
            context->recordError(gl::Error(GL_OUT_OF_MEMORY));
            return;
        }

        if (size + offset > buffer->getSize())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Error error = buffer->bufferSubData(data, size, offset);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target)
{
    EVENT("(GLenum target = 0x%X)", target);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidFramebufferTarget(target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return 0;
        }

        gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
        ASSERT(framebuffer);

        return framebuffer->completeness(context->getData());
    }

    return 0;
}

void GL_APIENTRY glClear(GLbitfield mask)
{
    EVENT("(GLbitfield mask = 0x%X)", mask);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Framebuffer *framebufferObject = context->getState().getDrawFramebuffer();
        ASSERT(framebufferObject);

        if (framebufferObject->completeness(context->getData()) != GL_FRAMEBUFFER_COMPLETE)
        {
            context->recordError(gl::Error(GL_INVALID_FRAMEBUFFER_OPERATION));
            return;
        }

        if ((mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Error error = context->clear(mask);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    EVENT("(GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f)",
          red, green, blue, alpha);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setClearColor(red, green, blue, alpha);
    }
}

void GL_APIENTRY glClearDepthf(GLclampf depth)
{
    EVENT("(GLclampf depth = %f)", depth);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setClearDepth(depth);
    }
}

void GL_APIENTRY glClearStencil(GLint s)
{
    EVENT("(GLint s = %d)", s);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setClearStencil(s);
    }
}

void GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    EVENT("(GLboolean red = %d, GLboolean green = %u, GLboolean blue = %u, GLboolean alpha = %u)",
          red, green, blue, alpha);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setColorMask(red == GL_TRUE, green == GL_TRUE, blue == GL_TRUE, alpha == GL_TRUE);
    }
}

void GL_APIENTRY glCompileShader(GLuint shader)
{
    EVENT("(GLuint shader = %d)", shader);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Shader *shaderObject = context->getShader(shader);

        if (!shaderObject)
        {
            if (context->getProgram(shader))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        shaderObject->compile(context->getData());
    }
}

void GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                      GLint border, GLsizei imageSize, const GLvoid* data)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, "
          "GLsizei height = %d, GLint border = %d, GLsizei imageSize = %d, const GLvoid* data = 0x%0.8p)",
          target, level, internalformat, width, height, border, imageSize, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3 &&
            !ValidateES2TexImageParameters(context, target, level, internalformat, true, false,
                                           0, 0, width, height, border, GL_NONE, GL_NONE, data))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3TexImageParameters(context, target, level, internalformat, true, false,
                                           0, 0, 0, width, height, 1, border, GL_NONE, GL_NONE, data))
        {
            return;
        }

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalformat);
        if (imageSize < 0 || static_cast<GLuint>(imageSize) != formatInfo.computeBlockSize(GL_UNSIGNED_BYTE, width, height))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture = context->getTexture2D();
                gl::Error error = texture->setCompressedImage(level, internalformat, width, height, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
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
                gl::TextureCubeMap *texture = context->getTextureCubeMap();
                gl::Error error = texture->setCompressedImage(target, level, internalformat, width, height, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                         GLenum format, GLsizei imageSize, const GLvoid* data)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, "
          "GLsizei imageSize = %d, const GLvoid* data = 0x%0.8p)",
          target, level, xoffset, yoffset, width, height, format, imageSize, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3 &&
            !ValidateES2TexImageParameters(context, target, level, GL_NONE, true, true,
                                           xoffset, yoffset, width, height, 0, GL_NONE, GL_NONE, data))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3TexImageParameters(context, target, level, GL_NONE, true, true,
                                           xoffset, yoffset, 0, width, height, 1, 0, GL_NONE, GL_NONE, data))
        {
            return;
        }

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(format);
        if (imageSize < 0 || static_cast<GLuint>(imageSize) != formatInfo.computeBlockSize(GL_UNSIGNED_BYTE, width, height))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture = context->getTexture2D();
                gl::Error error = texture->subImageCompressed(level, xoffset, yoffset, width, height, format, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
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
                gl::TextureCubeMap *texture = context->getTextureCubeMap();
                gl::Error error = texture->subImageCompressed(target, level, xoffset, yoffset, width, height, format, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
          "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, GLint border = %d)",
          target, level, internalformat, x, y, width, height, border);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3 &&
            !ValidateES2CopyTexImageParameters(context, target, level, internalformat, false,
                                               0, 0, x, y, width, height, border))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3CopyTexImageParameters(context, target, level, internalformat, false,
                                               0, 0, 0, x, y, width, height, border))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture = context->getTexture2D();
                gl::Error error = texture->copyImage(level, internalformat, x, y, width, height, framebuffer);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
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
                gl::TextureCubeMap *texture = context->getTextureCubeMap();
                gl::Error error = texture->copyImage(target, level, internalformat, x, y, width, height, framebuffer);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
          target, level, xoffset, yoffset, x, y, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3 &&
            !ValidateES2CopyTexImageParameters(context, target, level, GL_NONE, true,
                                               xoffset, yoffset, x, y, width, height, 0))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3CopyTexImageParameters(context, target, level, GL_NONE, true,
                                               xoffset, yoffset, 0, x, y, width, height, 0))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture = context->getTexture2D();
                gl::Error error = texture->copySubImage(target, level, xoffset, yoffset, 0, x, y, width, height, framebuffer);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
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
                gl::TextureCubeMap *texture = context->getTextureCubeMap();
                gl::Error error = texture->copySubImage(target, level, xoffset, yoffset, 0, x, y, width, height, framebuffer);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

GLuint GL_APIENTRY glCreateProgram(void)
{
    EVENT("()");

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        return context->createProgram();
    }

    return 0;
}

GLuint GL_APIENTRY glCreateShader(GLenum type)
{
    EVENT("(GLenum type = 0x%X)", type);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (type)
        {
          case GL_FRAGMENT_SHADER:
          case GL_VERTEX_SHADER:
            return context->createShader(type);

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return 0;
        }
    }

    return 0;
}

void GL_APIENTRY glCullFace(GLenum mode)
{
    EVENT("(GLenum mode = 0x%X)", mode);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (mode)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->getState().setCullMode(mode);
    }
}

void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
    EVENT("(GLsizei n = %d, const GLuint* buffers = 0x%0.8p)", n, buffers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            context->deleteBuffer(buffers[i]);
        }
    }
}

void GL_APIENTRY glDeleteFencesNV(GLsizei n, const GLuint* fences)
{
    EVENT("(GLsizei n = %d, const GLuint* fences = 0x%0.8p)", n, fences);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            context->deleteFenceNV(fences[i]);
        }
    }
}

void GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
    EVENT("(GLsizei n = %d, const GLuint* framebuffers = 0x%0.8p)", n, framebuffers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            if (framebuffers[i] != 0)
            {
                context->deleteFramebuffer(framebuffers[i]);
            }
        }
    }
}

void GL_APIENTRY glDeleteProgram(GLuint program)
{
    EVENT("(GLuint program = %d)", program);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (program == 0)
        {
            return;
        }

        if (!context->getProgram(program))
        {
            if(context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        context->deleteProgram(program);
    }
}

void GL_APIENTRY glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
    EVENT("(GLsizei n = %d, const GLuint *ids = 0x%0.8p)", n, ids);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            context->deleteQuery(ids[i]);
        }
    }
}

void GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
    EVENT("(GLsizei n = %d, const GLuint* renderbuffers = 0x%0.8p)", n, renderbuffers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            context->deleteRenderbuffer(renderbuffers[i]);
        }
    }
}

void GL_APIENTRY glDeleteShader(GLuint shader)
{
    EVENT("(GLuint shader = %d)", shader);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (shader == 0)
        {
            return;
        }

        if (!context->getShader(shader))
        {
            if(context->getProgram(shader))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        context->deleteShader(shader);
    }
}

void GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
    EVENT("(GLsizei n = %d, const GLuint* textures = 0x%0.8p)", n, textures);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            if (textures[i] != 0)
            {
                context->deleteTexture(textures[i]);
            }
        }
    }
}

void GL_APIENTRY glDepthFunc(GLenum func)
{
    EVENT("(GLenum func = 0x%X)", func);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (func)
        {
          case GL_NEVER:
          case GL_ALWAYS:
          case GL_LESS:
          case GL_LEQUAL:
          case GL_EQUAL:
          case GL_GREATER:
          case GL_GEQUAL:
          case GL_NOTEQUAL:
            context->getState().setDepthFunc(func);
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glDepthMask(GLboolean flag)
{
    EVENT("(GLboolean flag = %u)", flag);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setDepthMask(flag != GL_FALSE);
    }
}

void GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar)
{
    EVENT("(GLclampf zNear = %f, GLclampf zFar = %f)", zNear, zFar);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setDepthRange(zNear, zFar);
    }
}

void GL_APIENTRY glDetachShader(GLuint program, GLuint shader)
{
    EVENT("(GLuint program = %d, GLuint shader = %d)", program, shader);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);
        gl::Shader *shaderObject = context->getShader(shader);

        if (!programObject)
        {
            gl::Shader *shaderByProgramHandle;
            shaderByProgramHandle = context->getShader(program);
            if (!shaderByProgramHandle)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
        }

        if (!shaderObject)
        {
            gl::Program *programByShaderHandle = context->getProgram(shader);
            if (!programByShaderHandle)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
        }

        if (!programObject->detachShader(shaderObject))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }
    }
}

void GL_APIENTRY glDisable(GLenum cap)
{
    EVENT("(GLenum cap = 0x%X)", cap);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidCap(context, cap))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->getState().setEnableFeature(cap, false);
    }
}

void GL_APIENTRY glDisableVertexAttribArray(GLuint index)
{
    EVENT("(GLuint index = %d)", index);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setEnableVertexAttribArray(index, false);
    }
}

void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    EVENT("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d)", mode, first, count);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateDrawArrays(context, mode, first, count, 0))
        {
            return;
        }

        gl::Error error = context->drawArrays(mode, first, count, 0);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glDrawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    EVENT("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d, GLsizei primcount = %d)", mode, first, count, primcount);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateDrawArraysInstancedANGLE(context, mode, first, count, primcount))
        {
            return;
        }

        gl::Error error = context->drawArrays(mode, first, count, primcount);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
    EVENT("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const GLvoid* indices = 0x%0.8p)",
          mode, count, type, indices);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        rx::RangeUI indexRange;
        if (!ValidateDrawElements(context, mode, count, type, indices, 0, &indexRange))
        {
            return;
        }

        gl::Error error = context->drawElements(mode, count, type, indices, 0, indexRange);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glDrawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei primcount)
{
    EVENT("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const GLvoid* indices = 0x%0.8p, GLsizei primcount = %d)",
          mode, count, type, indices, primcount);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        rx::RangeUI indexRange;
        if (!ValidateDrawElementsInstancedANGLE(context, mode, count, type, indices, primcount, &indexRange))
        {
            return;
        }

        gl::Error error = context->drawElements(mode, count, type, indices, primcount, indexRange);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glEnable(GLenum cap)
{
    EVENT("(GLenum cap = 0x%X)", cap);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidCap(context, cap))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        context->getState().setEnableFeature(cap, true);
    }
}

void GL_APIENTRY glEnableVertexAttribArray(GLuint index)
{
    EVENT("(GLuint index = %d)", index);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setEnableVertexAttribArray(index, true);
    }
}

void GL_APIENTRY glEndQueryEXT(GLenum target)
{
    EVENT("GLenum target = 0x%X)", target);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateEndQuery(context, target))
        {
            return;
        }

        gl::Error error = context->endQuery(target);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glFinishFenceNV(GLuint fence)
{
    EVENT("(GLuint fence = %d)", fence);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::FenceNV *fenceObject = context->getFenceNV(fence);

        if (fenceObject == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (fenceObject->isFence() != GL_TRUE)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        fenceObject->finishFence();
    }
}

void GL_APIENTRY glFinish(void)
{
    EVENT("()");

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Error error = context->sync(true);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glFlush(void)
{
    EVENT("()");

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Error error = context->sync(false);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    EVENT("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum renderbuffertarget = 0x%X, "
          "GLuint renderbuffer = %d)", target, attachment, renderbuffertarget, renderbuffer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidFramebufferTarget(target) || (renderbuffertarget != GL_RENDERBUFFER && renderbuffer != 0))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (!gl::ValidateFramebufferRenderbufferParameters(context, target, attachment, renderbuffertarget, renderbuffer))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
        ASSERT(framebuffer);

        if (renderbuffer != 0)
        {
            gl::Renderbuffer *renderbufferObject = context->getRenderbuffer(renderbuffer);
            framebuffer->setRenderbufferAttachment(attachment, renderbufferObject);
        }
        else
        {
            framebuffer->setNULLAttachment(attachment);
        }
    }
}

void GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    EVENT("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum textarget = 0x%X, "
          "GLuint texture = %d, GLint level = %d)", target, attachment, textarget, texture, level);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateFramebufferTexture2D(context, target, attachment, textarget, texture, level))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
        ASSERT(framebuffer);

        if (texture != 0)
        {
            gl::Texture *textureObj = context->getTexture(texture);
            gl::ImageIndex index(textarget, level, gl::ImageIndex::ENTIRE_LEVEL);
            framebuffer->setTextureAttachment(attachment, textureObj, index);
        }
        else
        {
            framebuffer->setNULLAttachment(attachment);
        }
    }
}

void GL_APIENTRY glFrontFace(GLenum mode)
{
    EVENT("(GLenum mode = 0x%X)", mode);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (mode)
        {
          case GL_CW:
          case GL_CCW:
            context->getState().setFrontFace(mode);
            break;
          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers)
{
    EVENT("(GLsizei n = %d, GLuint* buffers = 0x%0.8p)", n, buffers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            buffers[i] = context->createBuffer();
        }
    }
}

void GL_APIENTRY glGenerateMipmap(GLenum target)
{
    EVENT("(GLenum target = 0x%X)", target);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidTextureTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Texture *texture = context->getTargetTexture(target);

        if (texture == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        GLenum internalFormat = texture->getBaseLevelInternalFormat();
        const gl::TextureCaps &formatCaps = context->getTextureCaps().get(internalFormat);
        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);

        // GenerateMipmap should not generate an INVALID_OPERATION for textures created with
        // unsized formats or that are color renderable and filterable.  Since we do not track if
        // the texture was created with sized or unsized format (only sized formats are stored),
        // it is not possible to make sure the the LUMA formats can generate mipmaps (they should
        // be able to) because they aren't color renderable.  Simply do a special case for LUMA
        // textures since they're the only texture format that can be created with unsized formats
        // that is not color renderable.  New unsized formats are unlikely to be added, since ES2
        // was the last version to use add them.
        bool isLUMA = internalFormat == GL_LUMINANCE8_EXT ||
                      internalFormat == GL_LUMINANCE8_ALPHA8_EXT ||
                      internalFormat == GL_ALPHA8_EXT;

        if (formatInfo.depthBits > 0 || formatInfo.stencilBits > 0 || !formatCaps.filterable ||
            (!formatCaps.renderable && !isLUMA) || formatInfo.compressed)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // GL_EXT_sRGB does not support mipmap generation on sRGB textures
        if (context->getClientVersion() == 2 && formatInfo.colorEncoding == GL_SRGB)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // Non-power of 2 ES2 check
        if (!context->getExtensions().textureNPOT && (!gl::isPow2(texture->getBaseLevelWidth()) || !gl::isPow2(texture->getBaseLevelHeight())))
        {
            ASSERT(context->getClientVersion() <= 2 && (target == GL_TEXTURE_2D || target == GL_TEXTURE_CUBE_MAP));
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // Cube completeness check
        if (target == GL_TEXTURE_CUBE_MAP)
        {
            gl::TextureCubeMap *textureCube = static_cast<gl::TextureCubeMap *>(texture);
            if (!textureCube->isCubeComplete())
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
        }

        gl::Error error = texture->generateMipmaps();
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glGenFencesNV(GLsizei n, GLuint* fences)
{
    EVENT("(GLsizei n = %d, GLuint* fences = 0x%0.8p)", n, fences);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            fences[i] = context->createFenceNV();
        }
    }
}

void GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
    EVENT("(GLsizei n = %d, GLuint* framebuffers = 0x%0.8p)", n, framebuffers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            framebuffers[i] = context->createFramebuffer();
        }
    }
}

void GL_APIENTRY glGenQueriesEXT(GLsizei n, GLuint* ids)
{
    EVENT("(GLsizei n = %d, GLuint* ids = 0x%0.8p)", n, ids);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (GLsizei i = 0; i < n; i++)
        {
            ids[i] = context->createQuery();
        }
    }
}

void GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
    EVENT("(GLsizei n = %d, GLuint* renderbuffers = 0x%0.8p)", n, renderbuffers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            renderbuffers[i] = context->createRenderbuffer();
        }
    }
}

void GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
    EVENT("(GLsizei n = %d, GLuint* textures = 0x%0.8p)", n, textures);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            textures[i] = context->createTexture();
        }
    }
}

void GL_APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    EVENT("(GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, GLsizei *length = 0x%0.8p, "
          "GLint *size = 0x%0.8p, GLenum *type = %0.8p, GLchar *name = %0.8p)",
          program, index, bufsize, length, size, type, name);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (bufsize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        if (index >= (GLuint)programObject->getActiveAttributeCount())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        programObject->getActiveAttribute(index, bufsize, length, size, type, name);
    }
}

void GL_APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name)
{
    EVENT("(GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, "
          "GLsizei* length = 0x%0.8p, GLint* size = 0x%0.8p, GLenum* type = 0x%0.8p, GLchar* name = 0x%0.8p)",
          program, index, bufsize, length, size, type, name);


    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (bufsize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        if (index >= (GLuint)programObject->getActiveUniformCount())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        programObject->getActiveUniform(index, bufsize, length, size, type, name);
    }
}

void GL_APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    EVENT("(GLuint program = %d, GLsizei maxcount = %d, GLsizei* count = 0x%0.8p, GLuint* shaders = 0x%0.8p)",
          program, maxcount, count, shaders);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (maxcount < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        return programObject->getAttachedShaders(maxcount, count, shaders);
    }
}

GLint GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar* name)
{
    EVENT("(GLuint program = %d, const GLchar* name = %s)", program, name);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return -1;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return -1;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        if (!programObject->isLinked() || !programBinary)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return -1;
        }

        return programBinary->getAttributeLocation(name);
    }

    return -1;
}

void GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params)
{
    EVENT("(GLenum pname = 0x%X, GLboolean* params = 0x%0.8p)",  pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        GLenum nativeType;
        unsigned int numParams = 0;
        if (!ValidateStateQuery(context, pname, &nativeType, &numParams))
        {
            return;
        }

        if (nativeType == GL_BOOL)
        {
            context->getBooleanv(pname, params);
        }
        else
        {
            CastStateValues(context, nativeType, pname, numParams, params);
        }
    }
}

void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (!gl::ValidBufferParameter(context, pname))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (!buffer)
        {
            // A null buffer means that "0" is bound to the requested buffer target
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch (pname)
        {
          case GL_BUFFER_USAGE:
            *params = static_cast<GLint>(buffer->getUsage());
            break;
          case GL_BUFFER_SIZE:
            *params = gl::clampCast<GLint>(buffer->getSize());
            break;
          case GL_BUFFER_ACCESS_FLAGS:
            *params = buffer->getAccessFlags();
            break;
          case GL_BUFFER_MAPPED:
            *params = static_cast<GLint>(buffer->isMapped());
            break;
          case GL_BUFFER_MAP_OFFSET:
            *params = gl::clampCast<GLint>(buffer->getMapOffset());
            break;
          case GL_BUFFER_MAP_LENGTH:
            *params = gl::clampCast<GLint>(buffer->getMapLength());
            break;
          default: UNREACHABLE(); break;
        }
    }
}

GLenum GL_APIENTRY glGetError(void)
{
    EVENT("()");

    gl::Context *context = gl::getContext();

    if (context)
    {
        return context->getError();
    }

    return GL_NO_ERROR;
}

void GL_APIENTRY glGetFenceivNV(GLuint fence, GLenum pname, GLint *params)
{
    EVENT("(GLuint fence = %d, GLenum pname = 0x%X, GLint *params = 0x%0.8p)", fence, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::FenceNV *fenceObject = context->getFenceNV(fence);

        if (fenceObject == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (fenceObject->isFence() != GL_TRUE)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch (pname)
        {
          case GL_FENCE_STATUS_NV:
            {
                // GL_NV_fence spec:
                // Once the status of a fence has been finished (via FinishFenceNV) or tested and the returned status is TRUE (via either TestFenceNV
                // or GetFenceivNV querying the FENCE_STATUS_NV), the status remains TRUE until the next SetFenceNV of the fence.
                GLboolean status = GL_TRUE;
                if (fenceObject->getStatus() != GL_TRUE)
                {
                    gl::Error error = fenceObject->testFence(&status);
                    if (error.isError())
                    {
                        context->recordError(error);
                        return;
                    }
                }
                *params = status;
                break;
            }

          case GL_FENCE_CONDITION_NV:
            {
                *params = fenceObject->getCondition();
                break;
            }

          default:
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
        }
    }
}

void GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params)
{
    EVENT("(GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        GLenum nativeType;
        unsigned int numParams = 0;
        if (!ValidateStateQuery(context, pname, &nativeType, &numParams))
        {
            return;
        }

        if (nativeType == GL_FLOAT)
        {
            context->getFloatv(pname, params);
        }
        else
        {
            CastStateValues(context, nativeType, pname, numParams, params);
        }
    }
}

void GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
    EVENT("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)",
          target, attachment, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidFramebufferTarget(target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        int clientVersion = context->getClientVersion();

        switch (pname)
        {
          case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
          case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
          case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
          case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
            break;

          case GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING:
            if (clientVersion < 3 && !context->getExtensions().sRGB)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
          case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
          case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
          case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
          case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
          case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
          case GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE:
          case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER:
            if (clientVersion < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        // Determine if the attachment is a valid enum
        switch (attachment)
        {
          case GL_BACK:
          case GL_FRONT:
          case GL_DEPTH:
          case GL_STENCIL:
          case GL_DEPTH_STENCIL_ATTACHMENT:
            if (clientVersion < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          case GL_DEPTH_ATTACHMENT:
          case GL_STENCIL_ATTACHMENT:
            break;

          default:
            if (attachment < GL_COLOR_ATTACHMENT0_EXT ||
                (attachment - GL_COLOR_ATTACHMENT0_EXT) >= context->getCaps().maxColorAttachments)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;
        }

        GLuint framebufferHandle = context->getState().getTargetFramebuffer(target)->id();
        gl::Framebuffer *framebuffer = context->getFramebuffer(framebufferHandle);
        ASSERT(framebuffer);

        if (framebufferHandle == 0)
        {
            if (clientVersion < 3)
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }

            switch (attachment)
            {
              case GL_BACK:
              case GL_DEPTH:
              case GL_STENCIL:
                break;

              default:
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
        }
        else
        {
            if (attachment >= GL_COLOR_ATTACHMENT0_EXT && attachment <= GL_COLOR_ATTACHMENT15_EXT)
            {
                // Valid attachment query
            }
            else
            {
                switch (attachment)
                {
                  case GL_DEPTH_ATTACHMENT:
                  case GL_STENCIL_ATTACHMENT:
                    break;

                  case GL_DEPTH_STENCIL_ATTACHMENT:
                    if (framebuffer->hasValidDepthStencil())
                    {
                        context->recordError(gl::Error(GL_INVALID_OPERATION));
                        return;
                    }
                    break;

                  default:
                    context->recordError(gl::Error(GL_INVALID_OPERATION));
                    return;
                }
            }
        }

        GLenum attachmentType = GL_NONE;
        GLuint attachmentHandle = 0;
        GLuint attachmentLevel = 0;
        GLuint attachmentLayer = 0;

        const gl::FramebufferAttachment *attachmentObject = framebuffer->getAttachment(attachment);

        if (attachmentObject)
        {
            attachmentType = attachmentObject->type();
            attachmentHandle = attachmentObject->id();
            attachmentLevel = attachmentObject->mipLevel();
            attachmentLayer = attachmentObject->layer();
        }

        GLenum attachmentObjectType;   // Type category
        if (framebufferHandle == 0)
        {
            attachmentObjectType = GL_FRAMEBUFFER_DEFAULT;
        }
        else if (attachmentType == GL_NONE || attachmentType == GL_RENDERBUFFER)
        {
            attachmentObjectType = attachmentType;
        }
        else if (gl::ValidTexture2DDestinationTarget(context, attachmentType))
        {
            attachmentObjectType = GL_TEXTURE;
        }
        else
        {
            UNREACHABLE();
            return;
        }

        if (attachmentObjectType == GL_NONE)
        {
            // ES 2.0.25 spec pg 127 states that if the value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
            // is NONE, then querying any other pname will generate INVALID_ENUM.

            // ES 3.0.2 spec pg 235 states that if the attachment type is none,
            // GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME will return zero and be an
            // INVALID_OPERATION for all other pnames

            switch (pname)
            {
              case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                *params = attachmentObjectType;
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                if (clientVersion < 3)
                {
                    context->recordError(gl::Error(GL_INVALID_ENUM));
                    return;
                }
                *params = 0;
                break;

              default:
                if (clientVersion < 3)
                {
                    context->recordError(gl::Error(GL_INVALID_ENUM));
                    return;
                }
                else
                {
                    context->recordError(gl::Error(GL_INVALID_OPERATION));
                    return;
                }
            }
        }
        else
        {
            ASSERT(attachmentObjectType == GL_RENDERBUFFER || attachmentObjectType == GL_TEXTURE ||
                   attachmentObjectType == GL_FRAMEBUFFER_DEFAULT);
            ASSERT(attachmentObject != NULL);

            switch (pname)
            {
              case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                *params = attachmentObjectType;
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                if (attachmentObjectType != GL_RENDERBUFFER && attachmentObjectType != GL_TEXTURE)
                {
                    context->recordError(gl::Error(GL_INVALID_ENUM));
                    return;
                }
                *params = attachmentHandle;
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
                if (attachmentObjectType != GL_TEXTURE)
                {
                    context->recordError(gl::Error(GL_INVALID_ENUM));
                    return;
                }
                *params = attachmentLevel;
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
                if (attachmentObjectType != GL_TEXTURE)
                {
                    context->recordError(gl::Error(GL_INVALID_ENUM));
                    return;
                }
                *params = gl::IsCubemapTextureTarget(attachmentType) ? attachmentType : 0;
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
                *params = attachmentObject->getRedSize();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
                *params = attachmentObject->getGreenSize();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
                *params = attachmentObject->getBlueSize();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
                *params = attachmentObject->getAlphaSize();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
                *params = attachmentObject->getDepthSize();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
                *params = attachmentObject->getStencilSize();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE:
                if (attachment == GL_DEPTH_STENCIL_ATTACHMENT)
                {
                    context->recordError(gl::Error(GL_INVALID_OPERATION));
                    return;
                }
                *params = attachmentObject->getComponentType();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING:
                *params = attachmentObject->getColorEncoding();
                break;

              case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER:
                if (attachmentObjectType != GL_TEXTURE)
                {
                    context->recordError(gl::Error(GL_INVALID_ENUM));
                    return;
                }
                *params = attachmentLayer;
                break;

              default:
                UNREACHABLE();
                break;
            }
        }
    }
}

GLenum GL_APIENTRY glGetGraphicsResetStatusEXT(void)
{
    EVENT("()");

    gl::Context *context = gl::getContext();

    if (context)
    {
        return context->getResetStatus();
    }

    return GL_NO_ERROR;
}

void GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params)
{
    EVENT("(GLenum pname = 0x%X, GLint* params = 0x%0.8p)", pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        GLenum nativeType;
        unsigned int numParams = 0;

        if (!ValidateStateQuery(context, pname, &nativeType, &numParams))
        {
            return;
        }

        if (nativeType == GL_INT)
        {
            context->getIntegerv(pname, params);
        }
        else
        {
            CastStateValues(context, nativeType, pname, numParams, params);
        }
    }
}

void GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
    EVENT("(GLuint program = %d, GLenum pname = %d, GLint* params = 0x%0.8p)", program, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (context->getClientVersion() < 3)
        {
            switch (pname)
            {
              case GL_ACTIVE_UNIFORM_BLOCKS:
              case GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH:
              case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
              case GL_TRANSFORM_FEEDBACK_VARYINGS:
              case GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH:
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
        }

        switch (pname)
        {
          case GL_DELETE_STATUS:
            *params = programObject->isFlaggedForDeletion();
            return;
          case GL_LINK_STATUS:
            *params = programObject->isLinked();
            return;
          case GL_VALIDATE_STATUS:
            *params = programObject->isValidated();
            return;
          case GL_INFO_LOG_LENGTH:
            *params = programObject->getInfoLogLength();
            return;
          case GL_ATTACHED_SHADERS:
            *params = programObject->getAttachedShadersCount();
            return;
          case GL_ACTIVE_ATTRIBUTES:
            *params = programObject->getActiveAttributeCount();
            return;
          case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
            *params = programObject->getActiveAttributeMaxLength();
            return;
          case GL_ACTIVE_UNIFORMS:
            *params = programObject->getActiveUniformCount();
            return;
          case GL_ACTIVE_UNIFORM_MAX_LENGTH:
            *params = programObject->getActiveUniformMaxLength();
            return;
          case GL_PROGRAM_BINARY_LENGTH_OES:
            *params = programObject->getProgramBinaryLength();
            return;
          case GL_ACTIVE_UNIFORM_BLOCKS:
            *params = programObject->getActiveUniformBlockCount();
            return;
          case GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH:
            *params = programObject->getActiveUniformBlockMaxLength();
            break;
          case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
            *params = programObject->getTransformFeedbackBufferMode();
            break;
          case GL_TRANSFORM_FEEDBACK_VARYINGS:
            *params = programObject->getTransformFeedbackVaryingCount();
            break;
          case GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH:
            *params = programObject->getTransformFeedbackVaryingMaxLength();
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
    EVENT("(GLuint program = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* infolog = 0x%0.8p)",
          program, bufsize, length, infolog);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (bufsize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        programObject->getInfoLog(bufsize, length, infolog);
    }
}

void GL_APIENTRY glGetQueryivEXT(GLenum target, GLenum pname, GLint *params)
{
    EVENT("GLenum target = 0x%X, GLenum pname = 0x%X, GLint *params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidQueryType(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (pname)
        {
          case GL_CURRENT_QUERY_EXT:
            params[0] = context->getState().getActiveQueryId(target);
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetQueryObjectuivEXT(GLuint id, GLenum pname, GLuint *params)
{
    EVENT("(GLuint id = %d, GLenum pname = 0x%X, GLuint *params = 0x%0.8p)", id, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Query *queryObject = context->getQuery(id, false, GL_NONE);

        if (!queryObject)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (context->getState().getActiveQueryId(queryObject->getType()) == id)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch(pname)
        {
          case GL_QUERY_RESULT_EXT:
            {
                gl::Error error = queryObject->getResult(params);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_QUERY_RESULT_AVAILABLE_EXT:
            {
                gl::Error error = queryObject->isResultAvailable(params);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (target != GL_RENDERBUFFER)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (context->getState().getRenderbufferId() == 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Renderbuffer *renderbuffer = context->getRenderbuffer(context->getState().getRenderbufferId());

        switch (pname)
        {
          case GL_RENDERBUFFER_WIDTH:           *params = renderbuffer->getWidth();          break;
          case GL_RENDERBUFFER_HEIGHT:          *params = renderbuffer->getHeight();         break;
          case GL_RENDERBUFFER_INTERNAL_FORMAT: *params = renderbuffer->getInternalFormat(); break;
          case GL_RENDERBUFFER_RED_SIZE:        *params = renderbuffer->getRedSize();        break;
          case GL_RENDERBUFFER_GREEN_SIZE:      *params = renderbuffer->getGreenSize();      break;
          case GL_RENDERBUFFER_BLUE_SIZE:       *params = renderbuffer->getBlueSize();       break;
          case GL_RENDERBUFFER_ALPHA_SIZE:      *params = renderbuffer->getAlphaSize();      break;
          case GL_RENDERBUFFER_DEPTH_SIZE:      *params = renderbuffer->getDepthSize();      break;
          case GL_RENDERBUFFER_STENCIL_SIZE:    *params = renderbuffer->getStencilSize();    break;

          case GL_RENDERBUFFER_SAMPLES_ANGLE:
            if (!context->getExtensions().framebufferMultisample)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = renderbuffer->getSamples();
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
    EVENT("(GLuint shader = %d, GLenum pname = %d, GLint* params = 0x%0.8p)", shader, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Shader *shaderObject = context->getShader(shader);

        if (!shaderObject)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (pname)
        {
          case GL_SHADER_TYPE:
            *params = shaderObject->getType();
            return;
          case GL_DELETE_STATUS:
            *params = shaderObject->isFlaggedForDeletion();
            return;
          case GL_COMPILE_STATUS:
            *params = shaderObject->isCompiled() ? GL_TRUE : GL_FALSE;
            return;
          case GL_INFO_LOG_LENGTH:
            *params = shaderObject->getInfoLogLength();
            return;
          case GL_SHADER_SOURCE_LENGTH:
            *params = shaderObject->getSourceLength();
            return;
          case GL_TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE:
            *params = shaderObject->getTranslatedSourceLength();
            return;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog)
{
    EVENT("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* infolog = 0x%0.8p)",
          shader, bufsize, length, infolog);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (bufsize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Shader *shaderObject = context->getShader(shader);

        if (!shaderObject)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        shaderObject->getInfoLog(bufsize, length, infolog);
    }
}

void GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    EVENT("(GLenum shadertype = 0x%X, GLenum precisiontype = 0x%X, GLint* range = 0x%0.8p, GLint* precision = 0x%0.8p)",
          shadertype, precisiontype, range, precision);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (shadertype)
        {
          case GL_VERTEX_SHADER:
          case GL_FRAGMENT_SHADER:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (precisiontype)
        {
          case GL_LOW_FLOAT:
          case GL_MEDIUM_FLOAT:
          case GL_HIGH_FLOAT:
            // Assume IEEE 754 precision
            range[0] = 127;
            range[1] = 127;
            *precision = 23;
            break;

          case GL_LOW_INT:
          case GL_MEDIUM_INT:
          case GL_HIGH_INT:
            // Some (most) hardware only supports single-precision floating-point numbers,
            // which can accurately represent integers up to +/-16777216
            range[0] = 24;
            range[1] = 24;
            *precision = 0;
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
    EVENT("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* source = 0x%0.8p)",
          shader, bufsize, length, source);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (bufsize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Shader *shaderObject = context->getShader(shader);

        if (!shaderObject)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        shaderObject->getSource(bufsize, length, source);
    }
}

void GL_APIENTRY glGetTranslatedShaderSourceANGLE(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source)
{
    EVENT("(GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLchar* source = 0x%0.8p)",
          shader, bufsize, length, source);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (bufsize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Shader *shaderObject = context->getShader(shader);

        if (!shaderObject)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // Only returns extra info if ANGLE_GENERATE_SHADER_DEBUG_INFO is defined
        shaderObject->getTranslatedSourceWithDebugInfo(bufsize, length, source);
    }
}

const GLubyte* GL_APIENTRY glGetString(GLenum name)
{
    EVENT("(GLenum name = 0x%X)", name);

    gl::Context *context = gl::getNonLostContext();

    switch (name)
    {
      case GL_VENDOR:
        return (GLubyte*)"Google Inc.";

      case GL_RENDERER:
        return (GLubyte*)((context != NULL) ? context->getRendererString().c_str() : "ANGLE");

      case GL_VERSION:
        if (context->getClientVersion() == 2)
        {
            return (GLubyte*)"OpenGL ES 2.0 (ANGLE " ANGLE_VERSION_STRING ")";
        }
        else
        {
            return (GLubyte*)"OpenGL ES 3.0 (ANGLE " ANGLE_VERSION_STRING ")";
        }

      case GL_SHADING_LANGUAGE_VERSION:
        if (context->getClientVersion() == 2)
        {
            return (GLubyte*)"OpenGL ES GLSL ES 1.00 (ANGLE " ANGLE_VERSION_STRING ")";
        }
        else
        {
            return (GLubyte*)"OpenGL ES GLSL ES 3.00 (ANGLE " ANGLE_VERSION_STRING ")";
        }

      case GL_EXTENSIONS:
        return (GLubyte*)((context != NULL) ? context->getExtensionString().c_str() : "");

      default:
        if (context)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
        }
        return NULL;
    }
}

void GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Texture *texture = context->getTargetTexture(target);

        if (!texture)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (pname)
        {
          case GL_TEXTURE_MAG_FILTER:
            *params = (GLfloat)texture->getSamplerState().magFilter;
            break;
          case GL_TEXTURE_MIN_FILTER:
            *params = (GLfloat)texture->getSamplerState().minFilter;
            break;
          case GL_TEXTURE_WRAP_S:
            *params = (GLfloat)texture->getSamplerState().wrapS;
            break;
          case GL_TEXTURE_WRAP_T:
            *params = (GLfloat)texture->getSamplerState().wrapT;
            break;
          case GL_TEXTURE_WRAP_R:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().wrapR;
            break;
          case GL_TEXTURE_IMMUTABLE_FORMAT:
            // Exposed to ES2.0 through EXT_texture_storage, no client version validation.
            *params = (GLfloat)(texture->isImmutable() ? GL_TRUE : GL_FALSE);
            break;
          case GL_TEXTURE_IMMUTABLE_LEVELS:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->immutableLevelCount();
            break;
          case GL_TEXTURE_USAGE_ANGLE:
            *params = (GLfloat)texture->getUsage();
            break;
          case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (!context->getExtensions().textureFilterAnisotropic)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().maxAnisotropy;
            break;
          case GL_TEXTURE_SWIZZLE_R:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().swizzleRed;
            break;
          case GL_TEXTURE_SWIZZLE_G:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().swizzleGreen;
            break;
          case GL_TEXTURE_SWIZZLE_B:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().swizzleBlue;
            break;
          case GL_TEXTURE_SWIZZLE_A:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().swizzleAlpha;
            break;
          case GL_TEXTURE_BASE_LEVEL:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().baseLevel;
            break;
          case GL_TEXTURE_MAX_LEVEL:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLfloat)texture->getSamplerState().maxLevel;
            break;
          case GL_TEXTURE_MIN_LOD:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().minLod;
            break;
          case GL_TEXTURE_MAX_LOD:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().maxLod;
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Texture *texture = context->getTargetTexture(target);

        if (!texture)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (pname)
        {
          case GL_TEXTURE_MAG_FILTER:
            *params = texture->getSamplerState().magFilter;
            break;
          case GL_TEXTURE_MIN_FILTER:
            *params = texture->getSamplerState().minFilter;
            break;
          case GL_TEXTURE_WRAP_S:
            *params = texture->getSamplerState().wrapS;
            break;
          case GL_TEXTURE_WRAP_T:
            *params = texture->getSamplerState().wrapT;
            break;
          case GL_TEXTURE_WRAP_R:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().wrapR;
            break;
          case GL_TEXTURE_IMMUTABLE_FORMAT:
            // Exposed to ES2.0 through EXT_texture_storage, no client version validation.
            *params = texture->isImmutable() ? GL_TRUE : GL_FALSE;
            break;
          case GL_TEXTURE_IMMUTABLE_LEVELS:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = static_cast<GLint>(texture->immutableLevelCount());
            break;
          case GL_TEXTURE_USAGE_ANGLE:
            *params = texture->getUsage();
            break;
          case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (!context->getExtensions().textureFilterAnisotropic)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLint)texture->getSamplerState().maxAnisotropy;
            break;
          case GL_TEXTURE_SWIZZLE_R:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().swizzleRed;
            break;
          case GL_TEXTURE_SWIZZLE_G:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().swizzleGreen;
            break;
          case GL_TEXTURE_SWIZZLE_B:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().swizzleBlue;
            break;
          case GL_TEXTURE_SWIZZLE_A:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().swizzleAlpha;
            break;
          case GL_TEXTURE_BASE_LEVEL:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().baseLevel;
            break;
          case GL_TEXTURE_MAX_LEVEL:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = texture->getSamplerState().maxLevel;
            break;
          case GL_TEXTURE_MIN_LOD:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLint)texture->getSamplerState().minLod;
            break;
          case GL_TEXTURE_MAX_LOD:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            *params = (GLint)texture->getSamplerState().maxLod;
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat* params)
{
    EVENT("(GLuint program = %d, GLint location = %d, GLsizei bufSize = %d, GLfloat* params = 0x%0.8p)",
          program, location, bufSize, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateGetnUniformfvEXT(context, program, location, bufSize, params))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);
        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        ASSERT(programBinary);

        programBinary->getUniformfv(location, params);
    }
}

void GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    EVENT("(GLuint program = %d, GLint location = %d, GLfloat* params = 0x%0.8p)", program, location, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateGetUniformfv(context, program, location, params))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);
        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        ASSERT(programBinary);

        programBinary->getUniformfv(location, params);
    }
}

void GL_APIENTRY glGetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint* params)
{
    EVENT("(GLuint program = %d, GLint location = %d, GLsizei bufSize = %d, GLint* params = 0x%0.8p)",
          program, location, bufSize, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateGetnUniformivEXT(context, program, location, bufSize, params))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);
        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        ASSERT(programBinary);

        programBinary->getUniformiv(location, params);
    }
}

void GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params)
{
    EVENT("(GLuint program = %d, GLint location = %d, GLint* params = 0x%0.8p)", program, location, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateGetUniformiv(context, program, location, params))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);
        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        ASSERT(programBinary);

        programBinary->getUniformiv(location, params);
    }
}

GLint GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar* name)
{
    EVENT("(GLuint program = %d, const GLchar* name = 0x%0.8p)", program, name);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (strstr(name, "gl_") == name)
        {
            return -1;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return -1;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return -1;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        if (!programObject->isLinked() || !programBinary)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return -1;
        }

        return programBinary->getUniformLocation(name);
    }

    return -1;
}

void GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
    EVENT("(GLuint index = %d, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", index, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        const gl::VertexAttribute &attribState = context->getState().getVertexAttribState(index);
        if (!gl::ValidateGetVertexAttribParameters(context, pname))
        {
            return;
        }

        if (pname == GL_CURRENT_VERTEX_ATTRIB)
        {
            const gl::VertexAttribCurrentValueData &currentValueData = context->getState().getVertexAttribCurrentValue(index);
            for (int i = 0; i < 4; ++i)
            {
                params[i] = currentValueData.FloatValues[i];
            }
        }
        else
        {
            *params = gl::QuerySingleVertexAttributeParameter<GLfloat>(attribState, pname);
        }
    }
}

void GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
    EVENT("(GLuint index = %d, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", index, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        const gl::VertexAttribute &attribState = context->getState().getVertexAttribState(index);

        if (!gl::ValidateGetVertexAttribParameters(context, pname))
        {
            return;
        }

        if (pname == GL_CURRENT_VERTEX_ATTRIB)
        {
            const gl::VertexAttribCurrentValueData &currentValueData = context->getState().getVertexAttribCurrentValue(index);
            for (int i = 0; i < 4; ++i)
            {
                float currentValue = currentValueData.FloatValues[i];
                params[i] = gl::iround<GLint>(currentValue);
            }
        }
        else
        {
            *params = gl::QuerySingleVertexAttributeParameter<GLint>(attribState, pname);
        }
    }
}

void GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer)
{
    EVENT("(GLuint index = %d, GLenum pname = 0x%X, GLvoid** pointer = 0x%0.8p)", index, pname, pointer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        *pointer = const_cast<GLvoid*>(context->getState().getVertexAttribPointer(index));
    }
}

void GL_APIENTRY glHint(GLenum target, GLenum mode)
{
    EVENT("(GLenum target = 0x%X, GLenum mode = 0x%X)", target, mode);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (mode)
        {
          case GL_FASTEST:
          case GL_NICEST:
          case GL_DONT_CARE:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (target)
        {
          case GL_GENERATE_MIPMAP_HINT:
            context->getState().setGenerateMipmapHint(mode);
            break;

          case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
            context->getState().setFragmentShaderDerivativeHint(mode);
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
    EVENT("(GLuint buffer = %d)", buffer);

    gl::Context *context = gl::getNonLostContext();
    if (context && buffer)
    {
        gl::Buffer *bufferObject = context->getBuffer(buffer);

        if (bufferObject)
        {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsEnabled(GLenum cap)
{
    EVENT("(GLenum cap = 0x%X)", cap);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidCap(context, cap))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return GL_FALSE;
        }

        return context->getState().getEnableFeature(cap);
    }

    return false;
}

GLboolean GL_APIENTRY glIsFenceNV(GLuint fence)
{
    EVENT("(GLuint fence = %d)", fence);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::FenceNV *fenceObject = context->getFenceNV(fence);

        if (fenceObject == NULL)
        {
            return GL_FALSE;
        }

        return fenceObject->isFence();
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsFramebuffer(GLuint framebuffer)
{
    EVENT("(GLuint framebuffer = %d)", framebuffer);

    gl::Context *context = gl::getNonLostContext();
    if (context && framebuffer)
    {
        gl::Framebuffer *framebufferObject = context->getFramebuffer(framebuffer);

        if (framebufferObject)
        {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsProgram(GLuint program)
{
    EVENT("(GLuint program = %d)", program);

    gl::Context *context = gl::getNonLostContext();
    if (context && program)
    {
        gl::Program *programObject = context->getProgram(program);

        if (programObject)
        {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsQueryEXT(GLuint id)
{
    EVENT("(GLuint id = %d)", id);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        return (context->getQuery(id, false, GL_NONE) != NULL) ? GL_TRUE : GL_FALSE;
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer)
{
    EVENT("(GLuint renderbuffer = %d)", renderbuffer);

    gl::Context *context = gl::getNonLostContext();
    if (context && renderbuffer)
    {
        gl::Renderbuffer *renderbufferObject = context->getRenderbuffer(renderbuffer);

        if (renderbufferObject)
        {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsShader(GLuint shader)
{
    EVENT("(GLuint shader = %d)", shader);

    gl::Context *context = gl::getNonLostContext();
    if (context && shader)
    {
        gl::Shader *shaderObject = context->getShader(shader);

        if (shaderObject)
        {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

GLboolean GL_APIENTRY glIsTexture(GLuint texture)
{
    EVENT("(GLuint texture = %d)", texture);

    gl::Context *context = gl::getNonLostContext();
    if (context && texture)
    {
        gl::Texture *textureObject = context->getTexture(texture);

        if (textureObject)
        {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

void GL_APIENTRY glLineWidth(GLfloat width)
{
    EVENT("(GLfloat width = %f)", width);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (width <= 0.0f)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setLineWidth(width);
    }
}

void GL_APIENTRY glLinkProgram(GLuint program)
{
    EVENT("(GLuint program = %d)", program);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        gl::Error error = context->linkProgram(program);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glPixelStorei(GLenum pname, GLint param)
{
    EVENT("(GLenum pname = 0x%X, GLint param = %d)", pname, param);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (pname)
        {
          case GL_UNPACK_ALIGNMENT:
            if (param != 1 && param != 2 && param != 4 && param != 8)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }

            context->getState().setUnpackAlignment(param);
            break;

          case GL_PACK_ALIGNMENT:
            if (param != 1 && param != 2 && param != 4 && param != 8)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }

            context->getState().setPackAlignment(param);
            break;

          case GL_PACK_REVERSE_ROW_ORDER_ANGLE:
            context->getState().setPackReverseRowOrder(param != 0);
            break;

          case GL_UNPACK_IMAGE_HEIGHT:
          case GL_UNPACK_SKIP_IMAGES:
          case GL_UNPACK_ROW_LENGTH:
          case GL_UNPACK_SKIP_ROWS:
          case GL_UNPACK_SKIP_PIXELS:
          case GL_PACK_ROW_LENGTH:
          case GL_PACK_SKIP_ROWS:
          case GL_PACK_SKIP_PIXELS:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            UNIMPLEMENTED();
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
    EVENT("(GLfloat factor = %f, GLfloat units = %f)", factor, units);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        context->getState().setPolygonOffsetParams(factor, units);
    }
}

void GL_APIENTRY glReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height,
                                GLenum format, GLenum type, GLsizei bufSize,
                                GLvoid *data)
{
    EVENT("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%X, GLsizei bufSize = 0x%d, GLvoid *data = 0x%0.8p)",
          x, y, width, height, format, type, bufSize, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (width < 0 || height < 0 || bufSize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (!gl::ValidateReadPixelsParameters(context, x, y, width, height,
                                              format, type, &bufSize, data))
        {
            return;
        }

        gl::Error error = context->readPixels(x, y, width, height, format, type, &bufSize, data);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                            GLenum format, GLenum type, GLvoid* pixels)
{
    EVENT("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%X, GLvoid* pixels = 0x%0.8p)",
          x, y, width, height, format, type,  pixels);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (width < 0 || height < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (!gl::ValidateReadPixelsParameters(context, x, y, width, height,
                                              format, type, NULL, pixels))
        {
            return;
        }

        gl::Error error = context->readPixels(x, y, width, height, format, type, NULL, pixels);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glReleaseShaderCompiler(void)
{
    EVENT("()");

    gl::Context *context = gl::getNonLostContext();

    if (context)
    {
        context->releaseShaderCompiler();
    }
}

void GL_APIENTRY glRenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLsizei samples = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
          target, samples, internalformat, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateRenderbufferStorageParameters(context, target, samples, internalformat,
                                                   width, height, true))
        {
            return;
        }

        gl::Renderbuffer *renderbuffer = context->getState().getCurrentRenderbuffer();
        gl::Error error = renderbuffer->setStorage(width, height, internalformat, samples);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    glRenderbufferStorageMultisampleANGLE(target, 0, internalformat, width, height);
}

void GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert)
{
    EVENT("(GLclampf value = %f, GLboolean invert = %u)", value, invert);

    gl::Context* context = gl::getNonLostContext();

    if (context)
    {
        context->getState().setSampleCoverageParams(gl::clamp01(value), invert == GL_TRUE);
    }
}

void GL_APIENTRY glSetFenceNV(GLuint fence, GLenum condition)
{
    EVENT("(GLuint fence = %d, GLenum condition = 0x%X)", fence, condition);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (condition != GL_ALL_COMPLETED_NV)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::FenceNV *fenceObject = context->getFenceNV(fence);

        if (fenceObject == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Error error = fenceObject->setFence(condition);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    EVENT("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

    gl::Context* context = gl::getNonLostContext();
    if (context)
    {
        if (width < 0 || height < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setScissorParams(x, y, width, height);
    }
}

void GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length)
{
    EVENT("(GLsizei n = %d, const GLuint* shaders = 0x%0.8p, GLenum binaryformat = 0x%X, "
          "const GLvoid* binary = 0x%0.8p, GLsizei length = %d)",
          n, shaders, binaryformat, binary, length);

    gl::Context* context = gl::getNonLostContext();
    if (context)
    {
        const std::vector<GLenum> &shaderBinaryFormats = context->getCaps().shaderBinaryFormats;
        if (std::find(shaderBinaryFormats.begin(), shaderBinaryFormats.end(), binaryformat) == shaderBinaryFormats.end())
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        // No binary shader formats are supported.
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length)
{
    EVENT("(GLuint shader = %d, GLsizei count = %d, const GLchar** string = 0x%0.8p, const GLint* length = 0x%0.8p)",
          shader, count, string, length);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (count < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Shader *shaderObject = context->getShader(shader);

        if (!shaderObject)
        {
            if (context->getProgram(shader))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        shaderObject->setSource(count, string, length);
    }
}

void GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    glStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    EVENT("(GLenum face = 0x%X, GLenum func = 0x%X, GLint ref = %d, GLuint mask = %d)", face, func, ref, mask);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (func)
        {
          case GL_NEVER:
          case GL_ALWAYS:
          case GL_LESS:
          case GL_LEQUAL:
          case GL_EQUAL:
          case GL_GEQUAL:
          case GL_GREATER:
          case GL_NOTEQUAL:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
        {
            context->getState().setStencilParams(func, ref, mask);
        }

        if (face == GL_BACK || face == GL_FRONT_AND_BACK)
        {
            context->getState().setStencilBackParams(func, ref, mask);
        }
    }
}

void GL_APIENTRY glStencilMask(GLuint mask)
{
    glStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
}

void GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask)
{
    EVENT("(GLenum face = 0x%X, GLuint mask = %d)", face, mask);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
        {
            context->getState().setStencilWritemask(mask);
        }

        if (face == GL_BACK || face == GL_FRONT_AND_BACK)
        {
            context->getState().setStencilBackWritemask(mask);
        }
    }
}

void GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    glStencilOpSeparate(GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    EVENT("(GLenum face = 0x%X, GLenum fail = 0x%X, GLenum zfail = 0x%X, GLenum zpas = 0x%Xs)",
          face, fail, zfail, zpass);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        switch (face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (fail)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (zfail)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (zpass)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
        {
            context->getState().setStencilOperations(fail, zfail, zpass);
        }

        if (face == GL_BACK || face == GL_FRONT_AND_BACK)
        {
            context->getState().setStencilBackOperations(fail, zfail, zpass);
        }
    }
}

GLboolean GL_APIENTRY glTestFenceNV(GLuint fence)
{
    EVENT("(GLuint fence = %d)", fence);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::FenceNV *fenceObject = context->getFenceNV(fence);

        if (fenceObject == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_TRUE;
        }

        if (fenceObject->isFence() != GL_TRUE)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_TRUE;
        }

        GLboolean result;
        gl::Error error = fenceObject->testFence(&result);
        if (error.isError())
        {
            context->recordError(error);
            return GL_TRUE;
        }

        return result;
    }

    return GL_TRUE;
}

void GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                            GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint internalformat = %d, GLsizei width = %d, GLsizei height = %d, "
          "GLint border = %d, GLenum format = 0x%X, GLenum type = 0x%X, const GLvoid* pixels = 0x%0.8p)",
          target, level, internalformat, width, height, border, format, type, pixels);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3 &&
            !ValidateES2TexImageParameters(context, target, level, internalformat, false, false,
                                           0, 0, width, height, border, format, type, pixels))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3TexImageParameters(context, target, level, internalformat, false, false,
                                           0, 0, 0, width, height, 1, border, format, type, pixels))
        {
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture = context->getTexture2D();
                gl::Error error = texture->setImage(level, width, height, internalformat, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
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
                gl::TextureCubeMap *texture = context->getTextureCubeMap();
                gl::Error error = texture->setImage(target, level, width, height, internalformat, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default: UNREACHABLE();
        }
    }
}

void GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %f)", target, pname, param);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateTexParamParameters(context, pname, static_cast<GLint>(param)))
        {
            return;
        }

        gl::Texture *texture = context->getTargetTexture(target);

        if (!texture)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (pname)
        {
          case GL_TEXTURE_WRAP_S:               texture->getSamplerState().wrapS = gl::uiround<GLenum>(param);        break;
          case GL_TEXTURE_WRAP_T:               texture->getSamplerState().wrapT = gl::uiround<GLenum>(param);        break;
          case GL_TEXTURE_WRAP_R:               texture->getSamplerState().wrapR = gl::uiround<GLenum>(param);        break;
          case GL_TEXTURE_MIN_FILTER:           texture->getSamplerState().minFilter = gl::uiround<GLenum>(param);    break;
          case GL_TEXTURE_MAG_FILTER:           texture->getSamplerState().magFilter = gl::uiround<GLenum>(param);    break;
          case GL_TEXTURE_USAGE_ANGLE:          texture->setUsage(gl::uiround<GLenum>(param));                        break;
          case GL_TEXTURE_MAX_ANISOTROPY_EXT:   texture->getSamplerState().maxAnisotropy = std::min(param, context->getExtensions().maxTextureAnisotropy); break;
          case GL_TEXTURE_COMPARE_MODE:         texture->getSamplerState().compareMode = gl::uiround<GLenum>(param);  break;
          case GL_TEXTURE_COMPARE_FUNC:         texture->getSamplerState().compareFunc = gl::uiround<GLenum>(param);  break;
          case GL_TEXTURE_SWIZZLE_R:            texture->getSamplerState().swizzleRed = gl::uiround<GLenum>(param);   break;
          case GL_TEXTURE_SWIZZLE_G:            texture->getSamplerState().swizzleGreen = gl::uiround<GLenum>(param); break;
          case GL_TEXTURE_SWIZZLE_B:            texture->getSamplerState().swizzleBlue = gl::uiround<GLenum>(param);  break;
          case GL_TEXTURE_SWIZZLE_A:            texture->getSamplerState().swizzleAlpha = gl::uiround<GLenum>(param); break;
          case GL_TEXTURE_BASE_LEVEL:           texture->getSamplerState().baseLevel = gl::iround<GLint>(param);      break;
          case GL_TEXTURE_MAX_LEVEL:            texture->getSamplerState().maxLevel = gl::iround<GLint>(param);       break;
          case GL_TEXTURE_MIN_LOD:              texture->getSamplerState().minLod = param;                            break;
          case GL_TEXTURE_MAX_LOD:              texture->getSamplerState().maxLod = param;                            break;
          default: UNREACHABLE(); break;
        }
    }
}

void GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
    glTexParameterf(target, pname, (GLfloat)*params);
}

void GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint param = %d)", target, pname, param);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateTexParamParameters(context, pname, param))
        {
            return;
        }

        gl::Texture *texture = context->getTargetTexture(target);

        if (!texture)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (pname)
        {
          case GL_TEXTURE_WRAP_S:               texture->getSamplerState().wrapS = (GLenum)param;        break;
          case GL_TEXTURE_WRAP_T:               texture->getSamplerState().wrapT = (GLenum)param;        break;
          case GL_TEXTURE_WRAP_R:               texture->getSamplerState().wrapR = (GLenum)param;        break;
          case GL_TEXTURE_MIN_FILTER:           texture->getSamplerState().minFilter = (GLenum)param;    break;
          case GL_TEXTURE_MAG_FILTER:           texture->getSamplerState().magFilter = (GLenum)param;    break;
          case GL_TEXTURE_USAGE_ANGLE:          texture->setUsage((GLenum)param);                        break;
          case GL_TEXTURE_MAX_ANISOTROPY_EXT:   texture->getSamplerState().maxAnisotropy = std::min((float)param, context->getExtensions().maxTextureAnisotropy); break;
          case GL_TEXTURE_COMPARE_MODE:         texture->getSamplerState().compareMode = (GLenum)param;  break;
          case GL_TEXTURE_COMPARE_FUNC:         texture->getSamplerState().compareFunc = (GLenum)param;  break;
          case GL_TEXTURE_SWIZZLE_R:            texture->getSamplerState().swizzleRed = (GLenum)param;   break;
          case GL_TEXTURE_SWIZZLE_G:            texture->getSamplerState().swizzleGreen = (GLenum)param; break;
          case GL_TEXTURE_SWIZZLE_B:            texture->getSamplerState().swizzleBlue = (GLenum)param;  break;
          case GL_TEXTURE_SWIZZLE_A:            texture->getSamplerState().swizzleAlpha = (GLenum)param; break;
          case GL_TEXTURE_BASE_LEVEL:           texture->getSamplerState().baseLevel = param;            break;
          case GL_TEXTURE_MAX_LEVEL:            texture->getSamplerState().maxLevel = param;             break;
          case GL_TEXTURE_MIN_LOD:              texture->getSamplerState().minLod = (GLfloat)param;      break;
          case GL_TEXTURE_MAX_LOD:              texture->getSamplerState().maxLod = (GLfloat)param;      break;
          default: UNREACHABLE(); break;
        }
    }
}

void GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
    glTexParameteri(target, pname, *params);
}

void GL_APIENTRY glTexStorage2DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLsizei levels = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
           target, levels, internalformat, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!context->getExtensions().textureStorage)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (context->getClientVersion() < 3 &&
            !ValidateES2TexStorageParameters(context, target, levels, internalformat, width, height))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3TexStorageParameters(context, target, levels, internalformat, width, height, 1))
        {
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture2d = context->getTexture2D();
                gl::Error error = texture2d->storage(levels, internalformat, width, height);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_CUBE_MAP:
            {
                gl::TextureCubeMap *textureCube = context->getTextureCubeMap();
                gl::Error error = textureCube->storage(levels, internalformat, width);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               GLenum format, GLenum type, const GLvoid* pixels)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLenum type = 0x%X, "
          "const GLvoid* pixels = 0x%0.8p)",
           target, level, xoffset, yoffset, width, height, format, type, pixels);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3 &&
            !ValidateES2TexImageParameters(context, target, level, GL_NONE, false, true,
                                           xoffset, yoffset, width, height, 0, format, type, pixels))
        {
            return;
        }

        if (context->getClientVersion() >= 3 &&
            !ValidateES3TexImageParameters(context, target, level, GL_NONE, false, true,
                                           xoffset, yoffset, 0, width, height, 1, 0, format, type, pixels))
        {
            return;
        }

        // Zero sized uploads are valid but no-ops
        if (width == 0 || height == 0)
        {
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture = context->getTexture2D();
                gl::Error error = texture->subImage(level, xoffset, yoffset, width, height, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
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
                gl::TextureCubeMap *texture = context->getTextureCubeMap();
                gl::Error error = texture->subImage(target, level, xoffset, yoffset, width, height, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            UNREACHABLE();
        }
    }
}

void GL_APIENTRY glUniform1f(GLint location, GLfloat x)
{
    glUniform1fv(location, 1, &x);
}

void GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_FLOAT, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform1fv(location, count, v);
    }
}

void GL_APIENTRY glUniform1i(GLint location, GLint x)
{
    glUniform1iv(location, 1, &x);
}

void GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_INT, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform1iv(location, count, v);
    }
}

void GL_APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y)
{
    GLfloat xy[2] = {x, y};

    glUniform2fv(location, 1, xy);
}

void GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_FLOAT_VEC2, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform2fv(location, count, v);
    }
}

void GL_APIENTRY glUniform2i(GLint location, GLint x, GLint y)
{
    GLint xy[2] = {x, y};

    glUniform2iv(location, 1, xy);
}

void GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_INT_VEC2, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform2iv(location, count, v);
    }
}

void GL_APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat xyz[3] = {x, y, z};

    glUniform3fv(location, 1, xyz);
}

void GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_FLOAT_VEC3, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform3fv(location, count, v);
    }
}

void GL_APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
    GLint xyz[3] = {x, y, z};

    glUniform3iv(location, 1, xyz);
}

void GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_INT_VEC3, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform3iv(location, count, v);
    }
}

void GL_APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLfloat xyzw[4] = {x, y, z, w};

    glUniform4fv(location, 1, xyzw);
}

void GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_FLOAT_VEC4, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform4fv(location, count, v);
    }
}

void GL_APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
    GLint xyzw[4] = {x, y, z, w};

    glUniform4iv(location, 1, xyzw);
}

void GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p)", location, count, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_INT_VEC4, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform4iv(location, count, v);
    }
}

void GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT2, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix2fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT3, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix3fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT4, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix4fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUseProgram(GLuint program)
{
    EVENT("(GLuint program = %d)", program);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);

        if (!programObject && program != 0)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        if (program != 0 && !programObject->isLinked())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->useProgram(program);
    }
}

void GL_APIENTRY glValidateProgram(GLuint program)
{
    EVENT("(GLuint program = %d)", program);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        programObject->validate(context->getCaps());
    }
}

void GL_APIENTRY glVertexAttrib1f(GLuint index, GLfloat x)
{
    EVENT("(GLuint index = %d, GLfloat x = %f)", index, x);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { x, 0, 0, 1 };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib1fv(GLuint index, const GLfloat* values)
{
    EVENT("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { values[0], 0, 0, 1 };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    EVENT("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f)", index, x, y);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { x, y, 0, 1 };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib2fv(GLuint index, const GLfloat* values)
{
    EVENT("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { values[0], values[1], 0, 1 };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    EVENT("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f)", index, x, y, z);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { x, y, z, 1 };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib3fv(GLuint index, const GLfloat* values)
{
    EVENT("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { values[0], values[1], values[2], 1 };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    EVENT("(GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f, GLfloat w = %f)", index, x, y, z, w);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLfloat vals[4] = { x, y, z, w };
        context->getState().setVertexAttribf(index, vals);
    }
}

void GL_APIENTRY glVertexAttrib4fv(GLuint index, const GLfloat* values)
{
    EVENT("(GLuint index = %d, const GLfloat* values = 0x%0.8p)", index, values);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setVertexAttribf(index, values);
    }
}

void GL_APIENTRY glVertexAttribDivisorANGLE(GLuint index, GLuint divisor)
{
    EVENT("(GLuint index = %d, GLuint divisor = %d)", index, divisor);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->setVertexAttribDivisor(index, divisor);
    }
}

void GL_APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr)
{
    EVENT("(GLuint index = %d, GLint size = %d, GLenum type = 0x%X, "
          "GLboolean normalized = %u, GLsizei stride = %d, const GLvoid* ptr = 0x%0.8p)",
          index, size, type, normalized, stride, ptr);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (size < 1 || size > 4)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (type)
        {
          case GL_BYTE:
          case GL_UNSIGNED_BYTE:
          case GL_SHORT:
          case GL_UNSIGNED_SHORT:
          case GL_FIXED:
          case GL_FLOAT:
            break;

          case GL_HALF_FLOAT:
          case GL_INT:
          case GL_UNSIGNED_INT:
          case GL_INT_2_10_10_10_REV:
          case GL_UNSIGNED_INT_2_10_10_10_REV:
            if (context->getClientVersion() < 3)
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (stride < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if ((type == GL_INT_2_10_10_10_REV || type == GL_UNSIGNED_INT_2_10_10_10_REV) && size != 4)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // [OpenGL ES 3.0.2] Section 2.8 page 24:
        // An INVALID_OPERATION error is generated when a non-zero vertex array object
        // is bound, zero is bound to the ARRAY_BUFFER buffer object binding point,
        // and the pointer argument is not NULL.
        if (context->getState().getVertexArray()->id() != 0 && context->getState().getArrayBufferId() == 0 && ptr != NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->getState().setVertexAttribState(index, context->getState().getTargetBuffer(GL_ARRAY_BUFFER), size, type,
                                                 normalized == GL_TRUE, false, stride, ptr);
    }
}

void GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    EVENT("(GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)", x, y, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (width < 0 || height < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setViewportParams(x, y, width, height);
    }
}

// OpenGL ES 3.0 functions

void GL_APIENTRY glReadBuffer(GLenum mode)
{
    EVENT("(GLenum mode = 0x%X)", mode);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glReadBuffer
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices)
{
    EVENT("(GLenum mode = 0x%X, GLuint start = %u, GLuint end = %u, GLsizei count = %d, GLenum type = 0x%X, "
          "const GLvoid* indices = 0x%0.8p)", mode, start, end, count, type, indices);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glDrawRangeElements
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint internalformat = %d, GLsizei width = %d, "
          "GLsizei height = %d, GLsizei depth = %d, GLint border = %d, GLenum format = 0x%X, "
          "GLenum type = 0x%X, const GLvoid* pixels = 0x%0.8p)",
          target, level, internalformat, width, height, depth, border, format, type, pixels);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // validateES3TexImageFormat sets the error code if there is an error
        if (!ValidateES3TexImageParameters(context, target, level, internalformat, false, false,
                                           0, 0, 0, width, height, depth, border, format, type, pixels))
        {
            return;
        }

        switch(target)
        {
          case GL_TEXTURE_3D:
            {
                gl::Texture3D *texture = context->getTexture3D();
                gl::Error error = texture->setImage(level, width, height, depth, internalformat, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_2D_ARRAY:
            {
                gl::Texture2DArray *texture = context->getTexture2DArray();
                gl::Error error = texture->setImage(level, width, height, depth, internalformat, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLint zoffset = %d, GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%X, const GLvoid* pixels = 0x%0.8p)",
          target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // validateES3TexImageFormat sets the error code if there is an error
        if (!ValidateES3TexImageParameters(context, target, level, GL_NONE, false, true,
                                           xoffset, yoffset, zoffset, width, height, depth, 0,
                                           format, type, pixels))
        {
            return;
        }

        // Zero sized uploads are valid but no-ops
        if (width == 0 || height == 0 || depth == 0)
        {
            return;
        }

        switch(target)
        {
          case GL_TEXTURE_3D:
            {
                gl::Texture3D *texture = context->getTexture3D();
                gl::Error error = texture->subImage(level, xoffset, yoffset, zoffset, width, height, depth, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_2D_ARRAY:
            {
                gl::Texture2DArray *texture = context->getTexture2DArray();
                gl::Error error = texture->subImage(level, xoffset, yoffset, zoffset, width, height, depth, format, type, context->getState().getUnpackState(), pixels);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
          "GLint zoffset = %d, GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
          target, level, xoffset, yoffset, zoffset, x, y, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateES3CopyTexImageParameters(context, target, level, GL_NONE, true, xoffset, yoffset, zoffset,
                                               x, y, width, height, 0))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();
        gl::Texture *texture = NULL;
        switch (target)
        {
          case GL_TEXTURE_3D:
            texture = context->getTexture3D();
            break;

          case GL_TEXTURE_2D_ARRAY:
            texture = context->getTexture2DArray();
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Error error = texture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, framebuffer);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, "
          "GLsizei height = %d, GLsizei depth = %d, GLint border = %d, GLsizei imageSize = %d, "
          "const GLvoid* data = 0x%0.8p)",
          target, level, internalformat, width, height, depth, border, imageSize, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalformat);
        if (imageSize < 0 || static_cast<GLuint>(imageSize) != formatInfo.computeBlockSize(GL_UNSIGNED_BYTE, width, height))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        // validateES3TexImageFormat sets the error code if there is an error
        if (!ValidateES3TexImageParameters(context, target, level, internalformat, true, false,
                                           0, 0, 0, width, height, depth, border, GL_NONE, GL_NONE, data))
        {
            return;
        }

        switch(target)
        {
          case GL_TEXTURE_3D:
            {
                gl::Texture3D *texture = context->getTexture3D();
                gl::Error error = texture->setCompressedImage(level, internalformat, width, height, depth, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_2D_ARRAY:
            {
                gl::Texture2DArray *texture = context->getTexture2DArray();
                gl::Error error = texture->setCompressedImage(level, internalformat, width, height, depth, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, "
        "GLint zoffset = %d, GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, "
        "GLenum format = 0x%X, GLsizei imageSize = %d, const GLvoid* data = 0x%0.8p)",
        target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(format);
        if (imageSize < 0 || static_cast<GLuint>(imageSize) != formatInfo.computeBlockSize(GL_UNSIGNED_BYTE, width, height))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (!data)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        // validateES3TexImageFormat sets the error code if there is an error
        if (!ValidateES3TexImageParameters(context, target, level, GL_NONE, true, true,
                                           0, 0, 0, width, height, depth, 0, GL_NONE, GL_NONE, data))
        {
            return;
        }

        // Zero sized uploads are valid but no-ops
        if (width == 0 || height == 0)
        {
            return;
        }

        switch(target)
        {
          case GL_TEXTURE_3D:
            {
                gl::Texture3D *texture = context->getTexture3D();
                gl::Error error = texture->subImageCompressed(level, xoffset, yoffset, zoffset, width, height, depth,
                                                              format, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_2D_ARRAY:
            {
                gl::Texture2DArray *texture = context->getTexture2DArray();
                gl::Error error = texture->subImageCompressed(level, xoffset, yoffset, zoffset, width, height, depth,
                                                              format, imageSize, context->getState().getUnpackState(), data);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGenQueries(GLsizei n, GLuint* ids)
{
    EVENT("(GLsizei n = %d, GLuint* ids = 0x%0.8p)", n, ids);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (GLsizei i = 0; i < n; i++)
        {
            ids[i] = context->createQuery();
        }
    }
}

void GL_APIENTRY glDeleteQueries(GLsizei n, const GLuint* ids)
{
    EVENT("(GLsizei n = %d, GLuint* ids = 0x%0.8p)", n, ids);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (GLsizei i = 0; i < n; i++)
        {
            context->deleteQuery(ids[i]);
        }
    }
}

GLboolean GL_APIENTRY glIsQuery(GLuint id)
{
    EVENT("(GLuint id = %u)", id);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        return (context->getQuery(id, false, GL_NONE) != NULL) ? GL_TRUE : GL_FALSE;
    }

    return GL_FALSE;
}

void GL_APIENTRY glBeginQuery(GLenum target, GLuint id)
{
    EVENT("(GLenum target = 0x%X, GLuint id = %u)", target, id);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateBeginQuery(context, target, id))
        {
            return;
        }

        gl::Error error = context->beginQuery(target, id);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glEndQuery(GLenum target)
{
    EVENT("(GLenum target = 0x%X)", target);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateEndQuery(context, target))
        {
            return;
        }

        gl::Error error = context->endQuery(target);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glGetQueryiv(GLenum target, GLenum pname, GLint* params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidQueryType(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (pname)
        {
          case GL_CURRENT_QUERY:
            params[0] = static_cast<GLint>(context->getState().getActiveQueryId(target));
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)
{
    EVENT("(GLuint id = %u, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", id, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Query *queryObject = context->getQuery(id, false, GL_NONE);

        if (!queryObject)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (context->getState().getActiveQueryId(queryObject->getType()) == id)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch(pname)
        {
          case GL_QUERY_RESULT_EXT:
            {
                gl::Error error = queryObject->getResult(params);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_QUERY_RESULT_AVAILABLE_EXT:
            {
                gl::Error error = queryObject->isResultAvailable(params);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

GLboolean GL_APIENTRY glUnmapBuffer(GLenum target)
{
    EVENT("(GLenum target = 0x%X)", target);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        return glUnmapBufferOES(target);
    }

    return GL_FALSE;
}

void GL_APIENTRY glGetBufferPointerv(GLenum target, GLenum pname, GLvoid** params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLvoid** params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        glGetBufferPointervOES(target, pname, params);
    }
}

void GL_APIENTRY glDrawBuffers(GLsizei n, const GLenum* bufs)
{
    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        glDrawBuffersEXT(n, bufs);
    }
}

void GL_APIENTRY glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT2x3, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix2x3fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT3x2, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix3x2fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT2x4, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix2x4fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT4x2, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix4x2fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT3x4, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix3x4fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, GLboolean transpose = %u, const GLfloat* value = 0x%0.8p)",
          location, count, transpose, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniformMatrix(context, GL_FLOAT_MAT4x3, location, count, transpose))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniformMatrix4x3fv(location, count, transpose, value);
    }
}

void GL_APIENTRY glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    EVENT("(GLint srcX0 = %d, GLint srcY0 = %d, GLint srcX1 = %d, GLint srcY1 = %d, GLint dstX0 = %d, "
          "GLint dstY0 = %d, GLint dstX1 = %d, GLint dstY1 = %d, GLbitfield mask = 0x%X, GLenum filter = 0x%X)",
          srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateBlitFramebufferParameters(context, srcX0, srcY0, srcX1, srcY1,
                                               dstX0, dstY0, dstX1, dstY1, mask, filter,
                                               false))
        {
            return;
        }

        gl::Error error = context->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                                                   mask, filter);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLsizei samples = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
        target, samples, internalformat, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateRenderbufferStorageParameters(context, target, samples, internalformat,
                                                   width, height, false))
        {
            return;
        }

        gl::Renderbuffer *renderbuffer = context->getState().getCurrentRenderbuffer();
        renderbuffer->setStorage(width, height, internalformat, samples);
    }
}

void GL_APIENTRY glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    EVENT("(GLenum target = 0x%X, GLenum attachment = 0x%X, GLuint texture = %u, GLint level = %d, GLint layer = %d)",
        target, attachment, texture, level, layer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateFramebufferTextureLayer(context, target, attachment, texture,
                                             level, layer))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
        ASSERT(framebuffer);

        if (texture != 0)
        {
            gl::Texture *textureObject = context->getTexture(texture);
            gl::ImageIndex index(textureObject->getTarget(), level, layer);
            framebuffer->setTextureAttachment(attachment, textureObject, index);
        }
        else
        {
            framebuffer->setNULLAttachment(attachment);
        }
    }
}

GLvoid* GL_APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    EVENT("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr length = %d, GLbitfield access = 0x%X)",
          target, offset, length, access);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        return glMapBufferRangeEXT(target, offset, length, access);
    }

    return NULL;
}

void GL_APIENTRY glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
    EVENT("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr length = %d)", target, offset, length);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        glFlushMappedBufferRangeEXT(target, offset, length);
    }
}

void GL_APIENTRY glBindVertexArray(GLuint array)
{
    EVENT("(GLuint array = %u)", array);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::VertexArray *vao = context->getVertexArray(array);

        if (!vao)
        {
            // The default VAO should always exist
            ASSERT(array != 0);
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->bindVertexArray(array);
    }
}

void GL_APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint* arrays)
{
    EVENT("(GLsizei n = %d, const GLuint* arrays = 0x%0.8p)", n, arrays);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int arrayIndex = 0; arrayIndex < n; arrayIndex++)
        {
            if (arrays[arrayIndex] != 0)
            {
                context->deleteVertexArray(arrays[arrayIndex]);
            }
        }
    }
}

void GL_APIENTRY glGenVertexArrays(GLsizei n, GLuint* arrays)
{
    EVENT("(GLsizei n = %d, GLuint* arrays = 0x%0.8p)", n, arrays);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (n < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int arrayIndex = 0; arrayIndex < n; arrayIndex++)
        {
            arrays[arrayIndex] = context->createVertexArray();
        }
    }
}

GLboolean GL_APIENTRY glIsVertexArray(GLuint array)
{
    EVENT("(GLuint array = %u)", array);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        if (array == 0)
        {
            return GL_FALSE;
        }

        gl::VertexArray *vao = context->getVertexArray(array);

        return (vao != NULL ? GL_TRUE : GL_FALSE);
    }

    return GL_FALSE;
}

void GL_APIENTRY glGetIntegeri_v(GLenum target, GLuint index, GLint* data)
{
    EVENT("(GLenum target = 0x%X, GLuint index = %u, GLint* data = 0x%0.8p)",
          target, index, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::Caps &caps = context->getCaps();
        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK_BUFFER_START:
          case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
          case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
            if (index >= caps.maxTransformFeedbackSeparateAttributes)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          case GL_UNIFORM_BUFFER_START:
          case GL_UNIFORM_BUFFER_SIZE:
          case GL_UNIFORM_BUFFER_BINDING:
            if (index >= caps.maxCombinedUniformBlocks)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (!(context->getIndexedIntegerv(target, index, data)))
        {
            GLenum nativeType;
            unsigned int numParams = 0;
            if (!context->getIndexedQueryParameterInfo(target, &nativeType, &numParams))
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }

            if (numParams == 0)
            {
                return; // it is known that pname is valid, but there are no parameters to return
            }

            if (nativeType == GL_INT_64_ANGLEX)
            {
                GLint64 minIntValue = static_cast<GLint64>(std::numeric_limits<int>::min());
                GLint64 maxIntValue = static_cast<GLint64>(std::numeric_limits<int>::max());
                GLint64 *int64Params = new GLint64[numParams];

                context->getIndexedInteger64v(target, index, int64Params);

                for (unsigned int i = 0; i < numParams; ++i)
                {
                    GLint64 clampedValue = std::max(std::min(int64Params[i], maxIntValue), minIntValue);
                    data[i] = static_cast<GLint>(clampedValue);
                }

                delete [] int64Params;
            }
            else
            {
                UNREACHABLE();
            }
        }
    }
}

void GL_APIENTRY glBeginTransformFeedback(GLenum primitiveMode)
{
    EVENT("(GLenum primitiveMode = 0x%X)", primitiveMode);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch (primitiveMode)
        {
          case GL_TRIANGLES:
          case GL_LINES:
          case GL_POINTS:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::TransformFeedback *transformFeedback = context->getState().getCurrentTransformFeedback();
        ASSERT(transformFeedback != NULL);

        if (transformFeedback->isStarted())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (transformFeedback->isPaused())
        {
            transformFeedback->resume();
        }
        else
        {
            transformFeedback->start(primitiveMode);
        }
    }
}

void GL_APIENTRY glEndTransformFeedback(void)
{
    EVENT("(void)");

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::TransformFeedback *transformFeedback = context->getState().getCurrentTransformFeedback();
        ASSERT(transformFeedback != NULL);

        if (!transformFeedback->isStarted())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        transformFeedback->stop();
    }
}

void GL_APIENTRY glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    EVENT("(GLenum target = 0x%X, GLuint index = %u, GLuint buffer = %u, GLintptr offset = %d, GLsizeiptr size = %d)",
          target, index, buffer, offset, size);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::Caps &caps = context->getCaps();
        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK_BUFFER:
            if (index >= caps.maxTransformFeedbackSeparateAttributes)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          case GL_UNIFORM_BUFFER:
            if (index >= caps.maxUniformBufferBindings)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (buffer != 0 && size <= 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK_BUFFER:

            // size and offset must be a multiple of 4
            if (buffer != 0 && ((offset % 4) != 0 || (size % 4) != 0))
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }

            context->bindIndexedTransformFeedbackBuffer(buffer, index, offset, size);
            context->bindGenericTransformFeedbackBuffer(buffer);
            break;

          case GL_UNIFORM_BUFFER:

            // it is an error to bind an offset not a multiple of the alignment
            if (buffer != 0 && (offset % caps.uniformBufferOffsetAlignment) != 0)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }

            context->bindIndexedUniformBuffer(buffer, index, offset, size);
            context->bindGenericUniformBuffer(buffer);
            break;

          default:
            UNREACHABLE();
        }
    }
}

void GL_APIENTRY glBindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    EVENT("(GLenum target = 0x%X, GLuint index = %u, GLuint buffer = %u)",
          target, index, buffer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::Caps &caps = context->getCaps();
        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK_BUFFER:
            if (index >= caps.maxTransformFeedbackSeparateAttributes)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          case GL_UNIFORM_BUFFER:
            if (index >= caps.maxUniformBufferBindings)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK_BUFFER:
            context->bindIndexedTransformFeedbackBuffer(buffer, index, 0, 0);
            context->bindGenericTransformFeedbackBuffer(buffer);
            break;

          case GL_UNIFORM_BUFFER:
            context->bindIndexedUniformBuffer(buffer, index, 0, 0);
            context->bindGenericUniformBuffer(buffer);
            break;

          default:
            UNREACHABLE();
        }
    }
}

void GL_APIENTRY glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar* const* varyings, GLenum bufferMode)
{
    EVENT("(GLuint program = %u, GLsizei count = %d, const GLchar* const* varyings = 0x%0.8p, GLenum bufferMode = 0x%X)",
          program, count, varyings, bufferMode);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (count < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        const gl::Caps &caps = context->getCaps();
        switch (bufferMode)
        {
          case GL_INTERLEAVED_ATTRIBS:
            break;
          case GL_SEPARATE_ATTRIBS:
            if (static_cast<GLuint>(count) > caps.maxTransformFeedbackSeparateAttributes)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;
          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (!gl::ValidProgram(context, program))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);

        programObject->setTransformFeedbackVaryings(count, varyings, bufferMode);
    }
}

void GL_APIENTRY glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name)
{
    EVENT("(GLuint program = %u, GLuint index = %u, GLsizei bufSize = %d, GLsizei* length = 0x%0.8p, "
          "GLsizei* size = 0x%0.8p, GLenum* type = 0x%0.8p, GLchar* name = 0x%0.8p)",
          program, index, bufSize, length, size, type, name);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (bufSize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (!gl::ValidProgram(context, program))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);

        if (index >= static_cast<GLuint>(programObject->getTransformFeedbackVaryingCount()))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        programObject->getTransformFeedbackVarying(index, bufSize, length, size, type, name);
    }
}

void GL_APIENTRY glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer)
{
    EVENT("(GLuint index = %u, GLint size = %d, GLenum type = 0x%X, GLsizei stride = %d, const GLvoid* pointer = 0x%0.8p)",
          index, size, type, stride, pointer);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (size < 1 || size > 4)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (type)
        {
          case GL_BYTE:
          case GL_UNSIGNED_BYTE:
          case GL_SHORT:
          case GL_UNSIGNED_SHORT:
          case GL_INT:
          case GL_UNSIGNED_INT:
          case GL_INT_2_10_10_10_REV:
          case GL_UNSIGNED_INT_2_10_10_10_REV:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (stride < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if ((type == GL_INT_2_10_10_10_REV || type == GL_UNSIGNED_INT_2_10_10_10_REV) && size != 4)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // [OpenGL ES 3.0.2] Section 2.8 page 24:
        // An INVALID_OPERATION error is generated when a non-zero vertex array object
        // is bound, zero is bound to the ARRAY_BUFFER buffer object binding point,
        // and the pointer argument is not NULL.
        if (context->getState().getVertexArray()->id() != 0 && context->getState().getArrayBufferId() == 0 && pointer != NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->getState().setVertexAttribState(index, context->getState().getTargetBuffer(GL_ARRAY_BUFFER), size, type, false, true,
                                                 stride, pointer);
    }
}

void GL_APIENTRY glGetVertexAttribIiv(GLuint index, GLenum pname, GLint* params)
{
    EVENT("(GLuint index = %u, GLenum pname = 0x%X, GLint* params = 0x%0.8p)",
          index, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        const gl::VertexAttribute &attribState = context->getState().getVertexAttribState(index);

        if (!gl::ValidateGetVertexAttribParameters(context, pname))
        {
            return;
        }

        if (pname == GL_CURRENT_VERTEX_ATTRIB)
        {
            const gl::VertexAttribCurrentValueData &currentValueData = context->getState().getVertexAttribCurrentValue(index);
            for (int i = 0; i < 4; ++i)
            {
                params[i] = currentValueData.IntValues[i];
            }
        }
        else
        {
            *params = gl::QuerySingleVertexAttributeParameter<GLint>(attribState, pname);
        }
    }
}

void GL_APIENTRY glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params)
{
    EVENT("(GLuint index = %u, GLenum pname = 0x%X, GLuint* params = 0x%0.8p)",
          index, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        const gl::VertexAttribute &attribState = context->getState().getVertexAttribState(index);

        if (!gl::ValidateGetVertexAttribParameters(context, pname))
        {
            return;
        }

        if (pname == GL_CURRENT_VERTEX_ATTRIB)
        {
            const gl::VertexAttribCurrentValueData &currentValueData = context->getState().getVertexAttribCurrentValue(index);
            for (int i = 0; i < 4; ++i)
            {
                params[i] = currentValueData.UnsignedIntValues[i];
            }
        }
        else
        {
            *params = gl::QuerySingleVertexAttributeParameter<GLuint>(attribState, pname);
        }
    }
}

void GL_APIENTRY glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    EVENT("(GLuint index = %u, GLint x = %d, GLint y = %d, GLint z = %d, GLint w = %d)",
          index, x, y, z, w);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLint vals[4] = { x, y, z, w };
        context->getState().setVertexAttribi(index, vals);
    }
}

void GL_APIENTRY glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    EVENT("(GLuint index = %u, GLuint x = %u, GLuint y = %u, GLuint z = %u, GLuint w = %u)",
          index, x, y, z, w);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        GLuint vals[4] = { x, y, z, w };
        context->getState().setVertexAttribu(index, vals);
    }
}

void GL_APIENTRY glVertexAttribI4iv(GLuint index, const GLint* v)
{
    EVENT("(GLuint index = %u, const GLint* v = 0x%0.8p)", index, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setVertexAttribi(index, v);
    }
}

void GL_APIENTRY glVertexAttribI4uiv(GLuint index, const GLuint* v)
{
    EVENT("(GLuint index = %u, const GLuint* v = 0x%0.8p)", index, v);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->getState().setVertexAttribu(index, v);
    }
}

void GL_APIENTRY glGetUniformuiv(GLuint program, GLint location, GLuint* params)
{
    EVENT("(GLuint program = %u, GLint location = %d, GLuint* params = 0x%0.8p)",
          program, location, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateGetUniformuiv(context, program, location, params))
        {
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        ASSERT(programObject);
        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        ASSERT(programBinary);

        programBinary->getUniformuiv(location, params);
    }
}

GLint GL_APIENTRY glGetFragDataLocation(GLuint program, const GLchar *name)
{
    EVENT("(GLuint program = %u, const GLchar *name = 0x%0.8p)",
          program, name);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return -1;
        }

        if (program == 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return -1;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject || !programObject->isLinked())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return -1;
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        if (!programBinary)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return -1;
        }

        return programBinary->getFragDataLocation(name);
    }

    return 0;
}

void GL_APIENTRY glUniform1ui(GLint location, GLuint v0)
{
    glUniform1uiv(location, 1, &v0);
}

void GL_APIENTRY glUniform2ui(GLint location, GLuint v0, GLuint v1)
{
    const GLuint xy[] = { v0, v1 };
    glUniform2uiv(location, 1, xy);
}

void GL_APIENTRY glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
    const GLuint xyz[] = { v0, v1, v2 };
    glUniform3uiv(location, 1, xyz);
}

void GL_APIENTRY glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
    const GLuint xyzw[] = { v0, v1, v2, v3 };
    glUniform4uiv(location, 1, xyzw);
}

void GL_APIENTRY glUniform1uiv(GLint location, GLsizei count, const GLuint* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLuint* value = 0x%0.8p)",
          location, count, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_UNSIGNED_INT, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform1uiv(location, count, value);
    }
}

void GL_APIENTRY glUniform2uiv(GLint location, GLsizei count, const GLuint* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLuint* value = 0x%0.8p)",
          location, count, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_UNSIGNED_INT_VEC2, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform2uiv(location, count, value);
    }
}

void GL_APIENTRY glUniform3uiv(GLint location, GLsizei count, const GLuint* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLuint* value)",
          location, count, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_UNSIGNED_INT_VEC3, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform3uiv(location, count, value);
    }
}

void GL_APIENTRY glUniform4uiv(GLint location, GLsizei count, const GLuint* value)
{
    EVENT("(GLint location = %d, GLsizei count = %d, const GLuint* value = 0x%0.8p)",
          location, count, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateUniform(context, GL_UNSIGNED_INT_VEC4, location, count))
        {
            return;
        }

        gl::ProgramBinary *programBinary = context->getState().getCurrentProgramBinary();
        programBinary->setUniform4uiv(location, count, value);
    }
}

void GL_APIENTRY glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value)
{
    EVENT("(GLenum buffer = 0x%X, GLint drawbuffer = %d, const GLint* value = 0x%0.8p)",
          buffer, drawbuffer, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateClearBuffer(context))
        {
            return;
        }

        switch (buffer)
        {
          case GL_COLOR:
            if (drawbuffer < 0 || static_cast<GLuint>(drawbuffer) >= context->getCaps().maxDrawBuffers)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          case GL_STENCIL:
            if (drawbuffer != 0)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Error error = context->clearBufferiv(buffer, drawbuffer, value);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value)
{
    EVENT("(GLenum buffer = 0x%X, GLint drawbuffer = %d, const GLuint* value = 0x%0.8p)",
          buffer, drawbuffer, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateClearBuffer(context))
        {
            return;
        }

        switch (buffer)
        {
          case GL_COLOR:
            if (drawbuffer < 0 || static_cast<GLuint>(drawbuffer) >= context->getCaps().maxDrawBuffers)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Error error = context->clearBufferuiv(buffer, drawbuffer, value);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value)
{
    EVENT("(GLenum buffer = 0x%X, GLint drawbuffer = %d, const GLfloat* value = 0x%0.8p)",
          buffer, drawbuffer, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateClearBuffer(context))
        {
            return;
        }

        switch (buffer)
        {
          case GL_COLOR:
            if (drawbuffer < 0 || static_cast<GLuint>(drawbuffer) >= context->getCaps().maxDrawBuffers)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          case GL_DEPTH:
            if (drawbuffer != 0)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Error error = context->clearBufferfv(buffer, drawbuffer, value);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    EVENT("(GLenum buffer = 0x%X, GLint drawbuffer = %d, GLfloat depth, GLint stencil = %d)",
          buffer, drawbuffer, depth, stencil);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateClearBuffer(context))
        {
            return;
        }

        switch (buffer)
        {
          case GL_DEPTH_STENCIL:
            if (drawbuffer != 0)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Error error = context->clearBufferfi(buffer, drawbuffer, depth, stencil);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

const GLubyte* GL_APIENTRY glGetStringi(GLenum name, GLuint index)
{
    EVENT("(GLenum name = 0x%X, GLuint index = %u)", name, index);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        if (name != GL_EXTENSIONS)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return NULL;
        }

        if (index >= context->getExtensionStringCount())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return NULL;
        }

        return reinterpret_cast<const GLubyte*>(context->getExtensionString(index).c_str());
    }

    return NULL;
}

void GL_APIENTRY glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    EVENT("(GLenum readTarget = 0x%X, GLenum writeTarget = 0x%X, GLintptr readOffset = %d, GLintptr writeOffset = %d, GLsizeiptr size = %d)",
          readTarget, writeTarget, readOffset, writeOffset, size);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!gl::ValidBufferTarget(context, readTarget) || !gl::ValidBufferTarget(context, writeTarget))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *readBuffer = context->getState().getTargetBuffer(readTarget);
        gl::Buffer *writeBuffer = context->getState().getTargetBuffer(writeTarget);

        if (!readBuffer || !writeBuffer)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // Verify that readBuffer and writeBuffer are not currently mapped
        if (readBuffer->isMapped() || writeBuffer->isMapped())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (readOffset < 0 || writeOffset < 0 || size < 0 ||
            static_cast<unsigned int>(readOffset + size) > readBuffer->getSize() ||
            static_cast<unsigned int>(writeOffset + size) > writeBuffer->getSize())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (readBuffer == writeBuffer && std::abs(readOffset - writeOffset) < size)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        // if size is zero, the copy is a successful no-op
        if (size > 0)
        {
            gl::Error error = writeBuffer->copyBufferSubData(readBuffer, readOffset, writeOffset, size);
            if (error.isError())
            {
                context->recordError(error);
                return;
            }
        }
    }
}

void GL_APIENTRY glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames, GLuint* uniformIndices)
{
    EVENT("(GLuint program = %u, GLsizei uniformCount = %d, const GLchar* const* uniformNames = 0x%0.8p, GLuint* uniformIndices = 0x%0.8p)",
          program, uniformCount, uniformNames, uniformIndices);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (uniformCount < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        if (!programObject->isLinked() || !programBinary)
        {
            for (int uniformId = 0; uniformId < uniformCount; uniformId++)
            {
                uniformIndices[uniformId] = GL_INVALID_INDEX;
            }
        }
        else
        {
            for (int uniformId = 0; uniformId < uniformCount; uniformId++)
            {
                uniformIndices[uniformId] = programBinary->getUniformIndex(uniformNames[uniformId]);
            }
        }
    }
}

void GL_APIENTRY glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params)
{
    EVENT("(GLuint program = %u, GLsizei uniformCount = %d, const GLuint* uniformIndices = 0x%0.8p, GLenum pname = 0x%X, GLint* params = 0x%0.8p)",
          program, uniformCount, uniformIndices, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (uniformCount < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        switch (pname)
        {
          case GL_UNIFORM_TYPE:
          case GL_UNIFORM_SIZE:
          case GL_UNIFORM_NAME_LENGTH:
          case GL_UNIFORM_BLOCK_INDEX:
          case GL_UNIFORM_OFFSET:
          case GL_UNIFORM_ARRAY_STRIDE:
          case GL_UNIFORM_MATRIX_STRIDE:
          case GL_UNIFORM_IS_ROW_MAJOR:
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();

        if (!programBinary && uniformCount > 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int uniformId = 0; uniformId < uniformCount; uniformId++)
        {
            const GLuint index = uniformIndices[uniformId];

            if (index >= (GLuint)programBinary->getActiveUniformCount())
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        for (int uniformId = 0; uniformId < uniformCount; uniformId++)
        {
            const GLuint index = uniformIndices[uniformId];
            params[uniformId] = programBinary->getActiveUniformi(index, pname);
        }
    }
}

GLuint GL_APIENTRY glGetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName)
{
    EVENT("(GLuint program = %u, const GLchar* uniformBlockName = 0x%0.8p)", program, uniformBlockName);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_INVALID_INDEX;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return GL_INVALID_INDEX;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return GL_INVALID_INDEX;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();
        if (!programBinary)
        {
            return GL_INVALID_INDEX;
        }

        return programBinary->getUniformBlockIndex(uniformBlockName);
    }

    return 0;
}

void GL_APIENTRY glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params)
{
    EVENT("(GLuint program = %u, GLuint uniformBlockIndex = %u, GLenum pname = 0x%X, GLint* params = 0x%0.8p)",
          program, uniformBlockIndex, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }
        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();

        if (!programBinary || uniformBlockIndex >= programBinary->getActiveUniformBlockCount())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (pname)
        {
          case GL_UNIFORM_BLOCK_BINDING:
            *params = static_cast<GLint>(programObject->getUniformBlockBinding(uniformBlockIndex));
            break;

          case GL_UNIFORM_BLOCK_DATA_SIZE:
          case GL_UNIFORM_BLOCK_NAME_LENGTH:
          case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
          case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
          case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
          case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
            programBinary->getActiveUniformBlockiv(uniformBlockIndex, pname, params);
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName)
{
    EVENT("(GLuint program = %u, GLuint uniformBlockIndex = %u, GLsizei bufSize = %d, GLsizei* length = 0x%0.8p, GLchar* uniformBlockName = 0x%0.8p)",
          program, uniformBlockIndex, bufSize, length, uniformBlockName);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();

        if (!programBinary || uniformBlockIndex >= programBinary->getActiveUniformBlockCount())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        programBinary->getActiveUniformBlockName(uniformBlockIndex, bufSize, length, uniformBlockName);
    }
}

void GL_APIENTRY glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    EVENT("(GLuint program = %u, GLuint uniformBlockIndex = %u, GLuint uniformBlockBinding = %u)",
          program, uniformBlockIndex, uniformBlockBinding);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (uniformBlockBinding >= context->getCaps().maxUniformBufferBindings)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Program *programObject = context->getProgram(program);

        if (!programObject)
        {
            if (context->getShader(program))
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
            else
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();

        // if never linked, there won't be any uniform blocks
        if (!programBinary || uniformBlockIndex >= programBinary->getActiveUniformBlockCount())
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        programObject->bindUniformBlock(uniformBlockIndex, uniformBlockBinding);
    }
}

void GL_APIENTRY glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount)
{
    EVENT("(GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d, GLsizei instanceCount = %d)",
          mode, first, count, instanceCount);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glDrawArraysInstanced
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instanceCount)
{
    EVENT("(GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const GLvoid* indices = 0x%0.8p, GLsizei instanceCount = %d)",
          mode, count, type, indices, instanceCount);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glDrawElementsInstanced
        UNIMPLEMENTED();
    }
}

GLsync GL_APIENTRY glFenceSync(GLenum condition, GLbitfield flags)
{
    EVENT("(GLenum condition = 0x%X, GLbitfield flags = 0x%X)", condition, flags);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return 0;
        }

        if (condition != GL_SYNC_GPU_COMMANDS_COMPLETE)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return 0;
        }

        if (flags != 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return 0;
        }

        GLsync fenceSync = context->createFenceSync();

        gl::FenceSync *fenceSyncObject = context->getFenceSync(fenceSync);
        gl::Error error = fenceSyncObject->set(condition);
        if (error.isError())
        {
            context->deleteFenceSync(fenceSync);
            context->recordError(error);
            return NULL;
        }

        return fenceSync;
    }

    return NULL;
}

GLboolean GL_APIENTRY glIsSync(GLsync sync)
{
    EVENT("(GLsync sync = 0x%0.8p)", sync);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        return (context->getFenceSync(sync) != NULL);
    }

    return GL_FALSE;
}

void GL_APIENTRY glDeleteSync(GLsync sync)
{
    EVENT("(GLsync sync = 0x%0.8p)", sync);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (sync != static_cast<GLsync>(0) && !context->getFenceSync(sync))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->deleteFenceSync(sync);
    }
}

GLenum GL_APIENTRY glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    EVENT("(GLsync sync = 0x%0.8p, GLbitfield flags = 0x%X, GLuint64 timeout = %llu)",
          sync, flags, timeout);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_WAIT_FAILED;
        }

        if ((flags & ~(GL_SYNC_FLUSH_COMMANDS_BIT)) != 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return GL_WAIT_FAILED;
        }

        gl::FenceSync *fenceSync = context->getFenceSync(sync);

        if (!fenceSync)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return GL_WAIT_FAILED;
        }

        GLenum result = GL_WAIT_FAILED;
        gl::Error error = fenceSync->clientWait(flags, timeout, &result);
        if (error.isError())
        {
            context->recordError(error);
            return GL_WAIT_FAILED;
        }

        return result;
    }

    return GL_FALSE;
}

void GL_APIENTRY glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    EVENT("(GLsync sync = 0x%0.8p, GLbitfield flags = 0x%X, GLuint64 timeout = %llu)",
          sync, flags, timeout);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (flags != 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (timeout != GL_TIMEOUT_IGNORED)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::FenceSync *fenceSync = context->getFenceSync(sync);

        if (!fenceSync)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::Error error = fenceSync->serverWait(flags, timeout);
        if (error.isError())
        {
            context->recordError(error);
        }
    }
}

void GL_APIENTRY glGetInteger64v(GLenum pname, GLint64* params)
{
    EVENT("(GLenum pname = 0x%X, GLint64* params = 0x%0.8p)",
          pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        GLenum nativeType;
        unsigned int numParams = 0;
        if (!ValidateStateQuery(context, pname, &nativeType, &numParams))
        {
            return;
        }

        if (nativeType == GL_INT_64_ANGLEX)
        {
            context->getInteger64v(pname, params);
        }
        else
        {
            CastStateValues(context, nativeType, pname, numParams, params);
        }
    }
}

void GL_APIENTRY glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values)
{
    EVENT("(GLsync sync = 0x%0.8p, GLenum pname = 0x%X, GLsizei bufSize = %d, GLsizei* length = 0x%0.8p, GLint* values = 0x%0.8p)",
          sync, pname, bufSize, length, values);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (bufSize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        gl::FenceSync *fenceSync = context->getFenceSync(sync);

        if (!fenceSync)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (pname)
        {
          case GL_OBJECT_TYPE:     values[0] = static_cast<GLint>(GL_SYNC_FENCE);              break;
          case GL_SYNC_CONDITION:  values[0] = static_cast<GLint>(fenceSync->getCondition());  break;
          case GL_SYNC_FLAGS:      values[0] = 0;                                              break;

          case GL_SYNC_STATUS:
            {
                gl::Error error = fenceSync->getStatus(values);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
                break;
            }

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glGetInteger64i_v(GLenum target, GLuint index, GLint64* data)
{
    EVENT("(GLenum target = 0x%X, GLuint index = %u, GLint64* data = 0x%0.8p)",
          target, index, data);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::Caps &caps = context->getCaps();
        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK_BUFFER_START:
          case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
          case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
            if (index >= caps.maxTransformFeedbackSeparateAttributes)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          case GL_UNIFORM_BUFFER_START:
          case GL_UNIFORM_BUFFER_SIZE:
          case GL_UNIFORM_BUFFER_BINDING:
            if (index >= caps.maxUniformBufferBindings)
            {
                context->recordError(gl::Error(GL_INVALID_VALUE));
                return;
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (!(context->getIndexedInteger64v(target, index, data)))
        {
            GLenum nativeType;
            unsigned int numParams = 0;
            if (!context->getIndexedQueryParameterInfo(target, &nativeType, &numParams))
            {
                context->recordError(gl::Error(GL_INVALID_ENUM));
                return;
            }

            if (numParams == 0)
                return; // it is known that pname is valid, but there are no parameters to return

            if (nativeType == GL_INT)
            {
                GLint *intParams = new GLint[numParams];

                context->getIndexedIntegerv(target, index, intParams);

                for (unsigned int i = 0; i < numParams; ++i)
                {
                    data[i] = static_cast<GLint64>(intParams[i]);
                }

                delete [] intParams;
            }
            else
            {
                UNREACHABLE();
            }
        }
    }
}

void GL_APIENTRY glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLint64* params = 0x%0.8p)",
          target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (!gl::ValidBufferParameter(context, pname))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (!buffer)
        {
            // A null buffer means that "0" is bound to the requested buffer target
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch (pname)
        {
          case GL_BUFFER_USAGE:
            *params = static_cast<GLint64>(buffer->getUsage());
            break;
          case GL_BUFFER_SIZE:
            *params = buffer->getSize();
            break;
          case GL_BUFFER_ACCESS_FLAGS:
            *params = static_cast<GLint64>(buffer->getAccessFlags());
            break;
          case GL_BUFFER_MAPPED:
            *params = static_cast<GLint64>(buffer->isMapped());
            break;
          case GL_BUFFER_MAP_OFFSET:
            *params = buffer->getMapOffset();
            break;
          case GL_BUFFER_MAP_LENGTH:
            *params = buffer->getMapLength();
            break;
          default: UNREACHABLE(); break;
        }
    }
}

void GL_APIENTRY glGenSamplers(GLsizei count, GLuint* samplers)
{
    EVENT("(GLsizei count = %d, GLuint* samplers = 0x%0.8p)", count, samplers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (count < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < count; i++)
        {
            samplers[i] = context->createSampler();
        }
    }
}

void GL_APIENTRY glDeleteSamplers(GLsizei count, const GLuint* samplers)
{
    EVENT("(GLsizei count = %d, const GLuint* samplers = 0x%0.8p)", count, samplers);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (count < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        for (int i = 0; i < count; i++)
        {
            context->deleteSampler(samplers[i]);
        }
    }
}

GLboolean GL_APIENTRY glIsSampler(GLuint sampler)
{
    EVENT("(GLuint sampler = %u)", sampler);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        return context->isSampler(sampler);
    }

    return GL_FALSE;
}

void GL_APIENTRY glBindSampler(GLuint unit, GLuint sampler)
{
    EVENT("(GLuint unit = %u, GLuint sampler = %u)", unit, sampler);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (sampler != 0 && !context->isSampler(sampler))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (unit >= context->getCaps().maxCombinedTextureImageUnits)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->bindSampler(unit, sampler);
    }
}

void GL_APIENTRY glSamplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
    EVENT("(GLuint sampler = %u, GLenum pname = 0x%X, GLint param = %d)", sampler, pname, param);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!gl::ValidateSamplerObjectParameter(context, pname))
        {
            return;
        }

        if (!gl::ValidateTexParamParameters(context, pname, param))
        {
            return;
        }

        if (!context->isSampler(sampler))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->samplerParameteri(sampler, pname, param);
    }
}

void GL_APIENTRY glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint* param)
{
    glSamplerParameteri(sampler, pname, *param);
}

void GL_APIENTRY glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
    EVENT("(GLuint sampler = %u, GLenum pname = 0x%X, GLfloat param = %g)", sampler, pname, param);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!gl::ValidateSamplerObjectParameter(context, pname))
        {
            return;
        }

        if (!gl::ValidateTexParamParameters(context, pname, static_cast<GLint>(param)))
        {
            return;
        }

        if (!context->isSampler(sampler))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        context->samplerParameterf(sampler, pname, param);
    }
}

void GL_APIENTRY glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* param)
{
    glSamplerParameterf(sampler, pname, *param);
}

void GL_APIENTRY glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params)
{
    EVENT("(GLuint sampler = %u, GLenum pname = 0x%X, GLint* params = 0x%0.8p)", sampler, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!gl::ValidateSamplerObjectParameter(context, pname))
        {
            return;
        }

        if (!context->isSampler(sampler))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        *params = context->getSamplerParameteri(sampler, pname);
    }
}

void GL_APIENTRY glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params)
{
    EVENT("(GLuint sample = %ur, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p)", sampler, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!gl::ValidateSamplerObjectParameter(context, pname))
        {
            return;
        }

        if (!context->isSampler(sampler))
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        *params = context->getSamplerParameterf(sampler, pname);
    }
}

void GL_APIENTRY glVertexAttribDivisor(GLuint index, GLuint divisor)
{
    EVENT("(GLuint index = %u, GLuint divisor = %u)", index, divisor);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        context->setVertexAttribDivisor(index, divisor);
    }
}

void GL_APIENTRY glBindTransformFeedback(GLenum target, GLuint id)
{
    EVENT("(GLenum target = 0x%X, GLuint id = %u)", target, id);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        switch (target)
        {
          case GL_TRANSFORM_FEEDBACK:
            {
                // Cannot bind a transform feedback object if the current one is started and not paused (3.0.2 pg 85 section 2.14.1)
                gl::TransformFeedback *curTransformFeedback = context->getState().getCurrentTransformFeedback();
                if (curTransformFeedback && curTransformFeedback->isStarted() && !curTransformFeedback->isPaused())
                {
                    context->recordError(gl::Error(GL_INVALID_OPERATION));
                    return;
                }

                // Cannot bind a transform feedback object that does not exist (3.0.2 pg 85 section 2.14.1)
                if (context->getTransformFeedback(id) == NULL)
                {
                    context->recordError(gl::Error(GL_INVALID_OPERATION));
                    return;
                }

                context->bindTransformFeedback(id);
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glDeleteTransformFeedbacks(GLsizei n, const GLuint* ids)
{
    EVENT("(GLsizei n = %d, const GLuint* ids = 0x%0.8p)", n, ids);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            context->deleteTransformFeedback(ids[i]);
        }
    }
}

void GL_APIENTRY glGenTransformFeedbacks(GLsizei n, GLuint* ids)
{
    EVENT("(GLsizei n = %d, GLuint* ids = 0x%0.8p)", n, ids);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        for (int i = 0; i < n; i++)
        {
            ids[i] = context->createTransformFeedback();
        }
    }
}

GLboolean GL_APIENTRY glIsTransformFeedback(GLuint id)
{
    EVENT("(GLuint id = %u)", id);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        return ((context->getTransformFeedback(id) != NULL) ? GL_TRUE : GL_FALSE);
    }

    return GL_FALSE;
}

void GL_APIENTRY glPauseTransformFeedback(void)
{
    EVENT("(void)");

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::TransformFeedback *transformFeedback = context->getState().getCurrentTransformFeedback();
        ASSERT(transformFeedback != NULL);

        // Current transform feedback must be started and not paused in order to pause (3.0.2 pg 86)
        if (!transformFeedback->isStarted() || transformFeedback->isPaused())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        transformFeedback->pause();
    }
}

void GL_APIENTRY glResumeTransformFeedback(void)
{
    EVENT("(void)");

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::TransformFeedback *transformFeedback = context->getState().getCurrentTransformFeedback();
        ASSERT(transformFeedback != NULL);

        // Current transform feedback must be started and paused in order to resume (3.0.2 pg 86)
        if (!transformFeedback->isStarted() || !transformFeedback->isPaused())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        transformFeedback->resume();
    }
}

void GL_APIENTRY glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary)
{
    EVENT("(GLuint program = %u, GLsizei bufSize = %d, GLsizei* length = 0x%0.8p, GLenum* binaryFormat = 0x%0.8p, GLvoid* binary = 0x%0.8p)",
          program, bufSize, length, binaryFormat, binary);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glGetProgramBinary
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glProgramBinary(GLuint program, GLenum binaryFormat, const GLvoid* binary, GLsizei length)
{
    EVENT("(GLuint program = %u, GLenum binaryFormat = 0x%X, const GLvoid* binary = 0x%0.8p, GLsizei length = %d)",
          program, binaryFormat, binary, length);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glProgramBinary
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glProgramParameteri(GLuint program, GLenum pname, GLint value)
{
    EVENT("(GLuint program = %u, GLenum pname = 0x%X, GLint value = %d)",
          program, pname, value);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // glProgramParameteri
        UNIMPLEMENTED();
    }
}

void GL_APIENTRY glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments)
{
    EVENT("(GLenum target = 0x%X, GLsizei numAttachments = %d, const GLenum* attachments = 0x%0.8p)",
          target, numAttachments, attachments);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateInvalidateFramebufferParameters(context, target, numAttachments, attachments))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
        ASSERT(framebuffer);

        if (framebuffer->completeness(context->getData()) == GL_FRAMEBUFFER_COMPLETE)
        {
            gl::Error error = framebuffer->invalidate(context->getCaps(), numAttachments, attachments);
            if (error.isError())
            {
                context->recordError(error);
                return;
            }
        }
    }
}

void GL_APIENTRY glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments, GLint x, GLint y, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLsizei numAttachments = %d, const GLenum* attachments = 0x%0.8p, GLint x = %d, "
          "GLint y = %d, GLsizei width = %d, GLsizei height = %d)",
          target, numAttachments, attachments, x, y, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateInvalidateFramebufferParameters(context, target, numAttachments, attachments))
        {
            return;
        }

        gl::Framebuffer *framebuffer = context->getState().getTargetFramebuffer(target);
        ASSERT(framebuffer);

        if (framebuffer->completeness(context->getData()) == GL_FRAMEBUFFER_COMPLETE)
        {
            gl::Error error = framebuffer->invalidateSub(numAttachments, attachments, x, y, width, height);
            if (error.isError())
            {
                context->recordError(error);
                return;
            }
        }
    }
}

void GL_APIENTRY glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    EVENT("(GLenum target = 0x%X, GLsizei levels = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d)",
          target, levels, internalformat, width, height);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateES3TexStorageParameters(context, target, levels, internalformat, width, height, 1))
        {
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            {
                gl::Texture2D *texture2d = context->getTexture2D();
                gl::Error error = texture2d->storage(levels, internalformat, width, height);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_CUBE_MAP:
            {
                gl::TextureCubeMap *textureCube = context->getTextureCubeMap();
                gl::Error error = textureCube->storage(levels, internalformat, width);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

void GL_APIENTRY glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    EVENT("(GLenum target = 0x%X, GLsizei levels = %d, GLenum internalformat = 0x%X, GLsizei width = %d, "
          "GLsizei height = %d, GLsizei depth = %d)",
          target, levels, internalformat, width, height, depth);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!ValidateES3TexStorageParameters(context, target, levels, internalformat, width, height, depth))
        {
            return;
        }

        switch (target)
        {
          case GL_TEXTURE_3D:
            {
                gl::Texture3D *texture3d = context->getTexture3D();
                gl::Error error = texture3d->storage(levels, internalformat, width, height, depth);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          case GL_TEXTURE_2D_ARRAY:
            {
                gl::Texture2DArray *texture2darray = context->getTexture2DArray();
                gl::Error error = texture2darray->storage(levels, internalformat, width, height, depth);
                if (error.isError())
                {
                    context->recordError(error);
                    return;
                }
            }
            break;

          default:
            UNREACHABLE();
        }
    }
}

void GL_APIENTRY glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params)
{
    EVENT("(GLenum target = 0x%X, GLenum internalformat = 0x%X, GLenum pname = 0x%X, GLsizei bufSize = %d, "
          "GLint* params = 0x%0.8p)",
          target, internalformat, pname, bufSize, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (context->getClientVersion() < 3)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        const gl::TextureCaps &formatCaps = context->getTextureCaps().get(internalformat);
        if (!formatCaps.renderable)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (target != GL_RENDERBUFFER)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (bufSize < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        switch (pname)
        {
          case GL_NUM_SAMPLE_COUNTS:
            if (bufSize != 0)
            {
                *params = formatCaps.sampleCounts.size();
            }
            break;

          case GL_SAMPLES:
            std::copy_n(formatCaps.sampleCounts.rbegin(), std::min<size_t>(bufSize, formatCaps.sampleCounts.size()), params);
            break;

          default:
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }
    }
}

// Extension functions

void GL_APIENTRY glBlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                      GLbitfield mask, GLenum filter)
{
    EVENT("(GLint srcX0 = %d, GLint srcY0 = %d, GLint srcX1 = %d, GLint srcY1 = %d, "
          "GLint dstX0 = %d, GLint dstY0 = %d, GLint dstX1 = %d, GLint dstY1 = %d, "
          "GLbitfield mask = 0x%X, GLenum filter = 0x%X)",
          srcX0, srcY0, srcX1, srcX1, dstX0, dstY0, dstX1, dstY1, mask, filter);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!ValidateBlitFramebufferParameters(context, srcX0, srcY0, srcX1, srcY1,
                                               dstX0, dstY0, dstX1, dstY1, mask, filter,
                                               true))
        {
            return;
        }

        gl::Error error = context->blitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1,
                                                   mask, filter);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
                               GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
    EVENT("(GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, "
          "GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, GLint border = %d, "
          "GLenum format = 0x%X, GLenum type = 0x%x, const GLvoid* pixels = 0x%0.8p)",
          target, level, internalformat, width, height, depth, border, format, type, pixels);

    UNIMPLEMENTED();   // FIXME
}

void GL_APIENTRY glGetProgramBinaryOES(GLuint program, GLsizei bufSize, GLsizei *length,
                                     GLenum *binaryFormat, void *binary)
{
    EVENT("(GLenum program = 0x%X, bufSize = %d, length = 0x%0.8p, binaryFormat = 0x%0.8p, binary = 0x%0.8p)",
          program, bufSize, length, binaryFormat, binary);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Program *programObject = context->getProgram(program);

        if (!programObject || !programObject->isLinked())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::ProgramBinary *programBinary = programObject->getProgramBinary();

        if (!programBinary)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Error error = programBinary->save(binaryFormat, binary, bufSize, length);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glProgramBinaryOES(GLuint program, GLenum binaryFormat,
                                  const void *binary, GLint length)
{
    EVENT("(GLenum program = 0x%X, binaryFormat = 0x%x, binary = 0x%0.8p, length = %d)",
          program, binaryFormat, binary, length);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        const std::vector<GLenum> &programBinaryFormats = context->getCaps().programBinaryFormats;
        if (std::find(programBinaryFormats.begin(), programBinaryFormats.end(), binaryFormat) == programBinaryFormats.end())
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Program *programObject = context->getProgram(program);
        if (!programObject)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        gl::Error error = context->setProgramBinary(program, binaryFormat, binary, length);
        if (error.isError())
        {
            context->recordError(error);
            return;
        }
    }
}

void GL_APIENTRY glDrawBuffersEXT(GLsizei n, const GLenum *bufs)
{
    EVENT("(GLenum n = %d, bufs = 0x%0.8p)", n, bufs);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (n < 0 || static_cast<GLuint>(n) > context->getCaps().maxDrawBuffers)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        ASSERT(context->getState().getDrawFramebuffer());

        if (context->getState().getDrawFramebuffer()->id() == 0)
        {
            if (n != 1)
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }

            if (bufs[0] != GL_NONE && bufs[0] != GL_BACK)
            {
                context->recordError(gl::Error(GL_INVALID_OPERATION));
                return;
            }
        }
        else
        {
            for (int colorAttachment = 0; colorAttachment < n; colorAttachment++)
            {
                const GLenum attachment = GL_COLOR_ATTACHMENT0_EXT + colorAttachment;
                if (bufs[colorAttachment] != GL_NONE && bufs[colorAttachment] != attachment)
                {
                    context->recordError(gl::Error(GL_INVALID_OPERATION));
                    return;
                }
            }
        }

        gl::Framebuffer *framebuffer = context->getState().getDrawFramebuffer();
        ASSERT(framebuffer);

        for (unsigned int colorAttachment = 0; colorAttachment < static_cast<unsigned int>(n); colorAttachment++)
        {
            framebuffer->setDrawBufferState(colorAttachment, bufs[colorAttachment]);
        }

        for (unsigned int colorAttachment = n; colorAttachment < context->getCaps().maxDrawBuffers; colorAttachment++)
        {
            framebuffer->setDrawBufferState(colorAttachment, GL_NONE);
        }
    }
}

void GL_APIENTRY glGetBufferPointervOES(GLenum target, GLenum pname, void** params)
{
    EVENT("(GLenum target = 0x%X, GLenum pname = 0x%X, GLvoid** params = 0x%0.8p)", target, pname, params);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        if (pname != GL_BUFFER_MAP_POINTER)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (!buffer || !buffer->isMapped())
        {
            *params = NULL;
        }
        else
        {
            *params = buffer->getMapPointer();
        }
    }
}

void * GL_APIENTRY glMapBufferOES(GLenum target, GLenum access)
{
    EVENT("(GLenum target = 0x%X, GLbitfield access = 0x%X)", target, access);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return NULL;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (buffer == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        if (access != GL_WRITE_ONLY_OES)
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return NULL;
        }

        if (buffer->isMapped())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        gl::Error error = buffer->mapRange(0, buffer->getSize(), GL_MAP_WRITE_BIT);
        if (error.isError())
        {
            context->recordError(error);
            return NULL;
        }

        return buffer->getMapPointer();
    }

    return NULL;
}

GLboolean GL_APIENTRY glUnmapBufferOES(GLenum target)
{
    EVENT("(GLenum target = 0x%X)", target);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return GL_FALSE;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (buffer == NULL || !buffer->isMapped())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return GL_FALSE;
        }

        // TODO: detect if we had corruption. if so, throw an error and return false.

        gl::Error error = buffer->unmap();
        if (error.isError())
        {
            context->recordError(error);
            return GL_FALSE;
        }

        return GL_TRUE;
    }

    return GL_FALSE;
}

void* GL_APIENTRY glMapBufferRangeEXT (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    EVENT("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr length = %d, GLbitfield access = 0x%X)",
          target, offset, length, access);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return NULL;
        }

        if (offset < 0 || length < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return NULL;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (buffer == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        // Check for buffer overflow
        size_t offsetSize = static_cast<size_t>(offset);
        size_t lengthSize = static_cast<size_t>(length);

        if (!rx::IsUnsignedAdditionSafe(offsetSize, lengthSize) ||
            offsetSize + lengthSize > static_cast<size_t>(buffer->getSize()))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return NULL;
        }

        // Check for invalid bits in the mask
        GLbitfield allAccessBits = GL_MAP_READ_BIT |
                                   GL_MAP_WRITE_BIT |
                                   GL_MAP_INVALIDATE_RANGE_BIT |
                                   GL_MAP_INVALIDATE_BUFFER_BIT |
                                   GL_MAP_FLUSH_EXPLICIT_BIT |
                                   GL_MAP_UNSYNCHRONIZED_BIT;

        if (access & ~(allAccessBits))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return NULL;
        }

        if (length == 0 || buffer->isMapped())
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        // Check for invalid bit combinations
        if ((access & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) == 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        GLbitfield writeOnlyBits = GL_MAP_INVALIDATE_RANGE_BIT |
                                   GL_MAP_INVALIDATE_BUFFER_BIT |
                                   GL_MAP_UNSYNCHRONIZED_BIT;

        if ((access & GL_MAP_READ_BIT) != 0 && (access & writeOnlyBits) != 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        if ((access & GL_MAP_WRITE_BIT) == 0 && (access & GL_MAP_FLUSH_EXPLICIT_BIT) != 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return NULL;
        }

        gl::Error error = buffer->mapRange(offset, length, access);
        if (error.isError())
        {
            context->recordError(error);
            return NULL;
        }

        return buffer->getMapPointer();
    }

    return NULL;
}

void GL_APIENTRY glFlushMappedBufferRangeEXT (GLenum target, GLintptr offset, GLsizeiptr length)
{
    EVENT("(GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr length = %d)", target, offset, length);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        if (offset < 0 || length < 0)
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        if (!gl::ValidBufferTarget(context, target))
        {
            context->recordError(gl::Error(GL_INVALID_ENUM));
            return;
        }

        gl::Buffer *buffer = context->getState().getTargetBuffer(target);

        if (buffer == NULL)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        if (!buffer->isMapped() || (buffer->getAccessFlags() & GL_MAP_FLUSH_EXPLICIT_BIT) == 0)
        {
            context->recordError(gl::Error(GL_INVALID_OPERATION));
            return;
        }

        // Check for buffer overflow
        size_t offsetSize = static_cast<size_t>(offset);
        size_t lengthSize = static_cast<size_t>(length);

        if (!rx::IsUnsignedAdditionSafe(offsetSize, lengthSize) ||
            offsetSize + lengthSize > static_cast<size_t>(buffer->getMapLength()))
        {
            context->recordError(gl::Error(GL_INVALID_VALUE));
            return;
        }

        // We do not currently support a non-trivial implementation of FlushMappedBufferRange
    }
}

__eglMustCastToProperFunctionPointerType EGLAPIENTRY glGetProcAddress(const char *procname)
{
    struct Extension
    {
        const char *name;
        __eglMustCastToProperFunctionPointerType address;
    };

    static const Extension glExtensions[] =
    {
        {"glTexImage3DOES", (__eglMustCastToProperFunctionPointerType)glTexImage3DOES},
        {"glBlitFramebufferANGLE", (__eglMustCastToProperFunctionPointerType)glBlitFramebufferANGLE},
        {"glRenderbufferStorageMultisampleANGLE", (__eglMustCastToProperFunctionPointerType)glRenderbufferStorageMultisampleANGLE},
        {"glDeleteFencesNV", (__eglMustCastToProperFunctionPointerType)glDeleteFencesNV},
        {"glGenFencesNV", (__eglMustCastToProperFunctionPointerType)glGenFencesNV},
        {"glIsFenceNV", (__eglMustCastToProperFunctionPointerType)glIsFenceNV},
        {"glTestFenceNV", (__eglMustCastToProperFunctionPointerType)glTestFenceNV},
        {"glGetFenceivNV", (__eglMustCastToProperFunctionPointerType)glGetFenceivNV},
        {"glFinishFenceNV", (__eglMustCastToProperFunctionPointerType)glFinishFenceNV},
        {"glSetFenceNV", (__eglMustCastToProperFunctionPointerType)glSetFenceNV},
        {"glGetTranslatedShaderSourceANGLE", (__eglMustCastToProperFunctionPointerType)glGetTranslatedShaderSourceANGLE},
        {"glTexStorage2DEXT", (__eglMustCastToProperFunctionPointerType)glTexStorage2DEXT},
        {"glGetGraphicsResetStatusEXT", (__eglMustCastToProperFunctionPointerType)glGetGraphicsResetStatusEXT},
        {"glReadnPixelsEXT", (__eglMustCastToProperFunctionPointerType)glReadnPixelsEXT},
        {"glGetnUniformfvEXT", (__eglMustCastToProperFunctionPointerType)glGetnUniformfvEXT},
        {"glGetnUniformivEXT", (__eglMustCastToProperFunctionPointerType)glGetnUniformivEXT},
        {"glGenQueriesEXT", (__eglMustCastToProperFunctionPointerType)glGenQueriesEXT},
        {"glDeleteQueriesEXT", (__eglMustCastToProperFunctionPointerType)glDeleteQueriesEXT},
        {"glIsQueryEXT", (__eglMustCastToProperFunctionPointerType)glIsQueryEXT},
        {"glBeginQueryEXT", (__eglMustCastToProperFunctionPointerType)glBeginQueryEXT},
        {"glEndQueryEXT", (__eglMustCastToProperFunctionPointerType)glEndQueryEXT},
        {"glGetQueryivEXT", (__eglMustCastToProperFunctionPointerType)glGetQueryivEXT},
        {"glGetQueryObjectuivEXT", (__eglMustCastToProperFunctionPointerType)glGetQueryObjectuivEXT},
        {"glDrawBuffersEXT", (__eglMustCastToProperFunctionPointerType)glDrawBuffersEXT},
        {"glVertexAttribDivisorANGLE", (__eglMustCastToProperFunctionPointerType)glVertexAttribDivisorANGLE},
        {"glDrawArraysInstancedANGLE", (__eglMustCastToProperFunctionPointerType)glDrawArraysInstancedANGLE},
        {"glDrawElementsInstancedANGLE", (__eglMustCastToProperFunctionPointerType)glDrawElementsInstancedANGLE},
        {"glGetProgramBinaryOES", (__eglMustCastToProperFunctionPointerType)glGetProgramBinaryOES},
        {"glProgramBinaryOES", (__eglMustCastToProperFunctionPointerType)glProgramBinaryOES},
        {"glGetBufferPointervOES", (__eglMustCastToProperFunctionPointerType)glGetBufferPointervOES},
        {"glMapBufferOES", (__eglMustCastToProperFunctionPointerType)glMapBufferOES},
        {"glUnmapBufferOES", (__eglMustCastToProperFunctionPointerType)glUnmapBufferOES},
        {"glMapBufferRangeEXT", (__eglMustCastToProperFunctionPointerType)glMapBufferRangeEXT},
        {"glFlushMappedBufferRangeEXT", (__eglMustCastToProperFunctionPointerType)glFlushMappedBufferRangeEXT},    };

    for (unsigned int ext = 0; ext < ArraySize(glExtensions); ext++)
    {
        if (strcmp(procname, glExtensions[ext].name) == 0)
        {
            return (__eglMustCastToProperFunctionPointerType)glExtensions[ext].address;
        }
    }

    return NULL;
}

// Non-public functions used by EGL

bool EGLAPIENTRY glBindTexImage(egl::Surface *surface)
{
    EVENT("(egl::Surface* surface = 0x%0.8p)",
          surface);

    gl::Context *context = gl::getNonLostContext();
    if (context)
    {
        gl::Texture2D *textureObject = context->getTexture2D();
        ASSERT(textureObject != NULL);

        if (textureObject->isImmutable())
        {
            return false;
        }

        textureObject->bindTexImage(surface);
    }

    return true;
}

}
