//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES.h: Validation functions for generic OpenGL ES entry point parameters

#ifndef LIBGLESV2_VALIDATION_ES_H
#define LIBGLESV2_VALIDATION_ES_H

#include "common/mathutil.h"

#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

namespace gl
{

class Context;

bool ValidCap(const Context *context, GLenum cap);
bool ValidTextureTarget(const Context *context, GLenum target);
bool ValidTexture2DDestinationTarget(const Context *context, GLenum target);
bool ValidFramebufferTarget(GLenum target);
bool ValidBufferTarget(const Context *context, GLenum target);
bool ValidBufferParameter(const Context *context, GLenum pname);
bool ValidMipLevel(const Context *context, GLenum target, GLint level);
bool ValidImageSize(const Context *context, GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth);
bool ValidCompressedImageSize(const Context *context, GLenum internalFormat, GLsizei width, GLsizei height);
bool ValidQueryType(const Context *context, GLenum queryType);
bool ValidProgram(Context *context, GLuint id);

bool ValidateAttachmentTarget(Context *context, GLenum attachment);
bool ValidateRenderbufferStorageParameters(Context *context, GLenum target, GLsizei samples,
                                           GLenum internalformat, GLsizei width, GLsizei height,
                                           bool angleExtension);
bool ValidateFramebufferRenderbufferParameters(Context *context, GLenum target, GLenum attachment,
                                               GLenum renderbuffertarget, GLuint renderbuffer);

bool ValidateBlitFramebufferParameters(Context *context, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                       GLenum filter, bool fromAngleExtension);

bool ValidateGetVertexAttribParameters(Context *context, GLenum pname);

bool ValidateTexParamParameters(Context *context, GLenum pname, GLint param);

bool ValidateSamplerObjectParameter(Context *context, GLenum pname);

bool ValidateReadPixelsParameters(Context *context, GLint x, GLint y, GLsizei width, GLsizei height,
                                  GLenum format, GLenum type, GLsizei *bufSize, GLvoid *pixels);

bool ValidateBeginQuery(Context *context, GLenum target, GLuint id);
bool ValidateEndQuery(Context *context, GLenum target);

bool ValidateUniform(Context *context, GLenum uniformType, GLint location, GLsizei count);
bool ValidateUniformMatrix(Context *context, GLenum matrixType, GLint location, GLsizei count,
                           GLboolean transpose);

bool ValidateStateQuery(Context *context, GLenum pname, GLenum *nativeType, unsigned int *numParams);

bool ValidateCopyTexImageParametersBase(Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                        GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                        GLint border, GLenum *textureInternalFormatOut);

bool ValidateDrawArrays(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount);
bool ValidateDrawArraysInstanced(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount);
bool ValidateDrawArraysInstancedANGLE(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount);

bool ValidateDrawElements(Context *context, GLenum mode, GLsizei count, GLenum type,
                          const GLvoid* indices, GLsizei primcount, rx::RangeUI *indexRangeOut);

bool ValidateDrawElementsInstanced(Context *context, GLenum mode, GLsizei count, GLenum type,
                                   const GLvoid *indices, GLsizei primcount, rx::RangeUI *indexRangeOut);
bool ValidateDrawElementsInstancedANGLE(Context *context, GLenum mode, GLsizei count, GLenum type,
                                        const GLvoid *indices, GLsizei primcount, rx::RangeUI *indexRangeOut);

bool ValidateFramebufferTextureBase(Context *context, GLenum target, GLenum attachment,
                                    GLuint texture, GLint level);
bool ValidateFramebufferTexture2D(Context *context, GLenum target, GLenum attachment,
                                  GLenum textarget, GLuint texture, GLint level);

bool ValidateGetUniformBase(Context *context, GLuint program, GLint location);
bool ValidateGetUniformfv(Context *context, GLuint program, GLint location, GLfloat* params);
bool ValidateGetUniformiv(Context *context, GLuint program, GLint location, GLint* params);
bool ValidateGetnUniformfvEXT(Context *context, GLuint program, GLint location, GLsizei bufSize, GLfloat* params);
bool ValidateGetnUniformivEXT(Context *context, GLuint program, GLint location, GLsizei bufSize, GLint* params);

}

#endif // LIBGLESV2_VALIDATION_ES_H
