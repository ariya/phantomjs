//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES3.h: Validation functions for OpenGL ES 3.0 entry point parameters

#ifndef LIBGLESV2_VALIDATION_ES3_H
#define LIBGLESV2_VALIDATION_ES3_H

#include <GLES3/gl3.h>

namespace gl
{

class Context;

bool ValidateES3TexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                   GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels);

bool ValidateES3CopyTexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat,
                                       bool isSubImage, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y,
                                       GLsizei width, GLsizei height, GLint border);

bool ValidateES3TexStorageParameters(Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                     GLsizei width, GLsizei height, GLsizei depth);

bool ValidateFramebufferTextureLayer(Context *context, GLenum target, GLenum attachment,
                                     GLuint texture, GLint level, GLint layer);

bool ValidES3ReadFormatType(Context *context, GLenum internalFormat, GLenum format, GLenum type);

bool ValidateInvalidateFramebufferParameters(Context *context, GLenum target, GLsizei numAttachments,
                                             const GLenum* attachments);

bool ValidateClearBuffer(Context *context);

bool ValidateGetUniformuiv(Context *context, GLuint program, GLint location, GLuint* params);

}

#endif // LIBGLESV2_VALIDATION_ES3_H
