/*
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#if USE(3D_GRAPHICS) || defined(QT_OPENGL_SHIMS)

#define DISABLE_SHIMS
#include "OpenGLShims.h"

#if !PLATFORM(QT) && !PLATFORM(WIN)
#include <dlfcn.h>
#endif

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

OpenGLFunctionTable* openGLFunctionTable()
{
    static OpenGLFunctionTable table;
    return &table;
}

#if PLATFORM(QT)
static void* getProcAddress(const char* procName)
{
    if (QOpenGLContext* context = QOpenGLContext::currentContext())
        return reinterpret_cast<void*>(context->getProcAddress(procName));
    return 0;
}
#elif PLATFORM(WIN)
static void* getProcAddress(const char* procName)
{
    return GetProcAddress(GetModuleHandleA("libGLESv2"), procName);
}
#else
typedef void* (*glGetProcAddressType) (const char* procName);
static void* getProcAddress(const char* procName)
{
    static bool initialized = false;
    static glGetProcAddressType getProcAddressFunction = 0;

    if (!initialized) {
        getProcAddressFunction = reinterpret_cast<glGetProcAddressType>(dlsym(RTLD_DEFAULT, "glXGetProcAddress"));
        if (!getProcAddressFunction)
            getProcAddressFunction = reinterpret_cast<glGetProcAddressType>(dlsym(RTLD_DEFAULT, "glXGetProcAddressARB"));
    }

    if (!getProcAddressFunction)
        return dlsym(RTLD_DEFAULT, procName);
    return getProcAddressFunction(procName);
}
#endif

static void* lookupOpenGLFunctionAddress(const char* functionName, bool* success = 0)
{
    if (success && !*success)
        return 0;

    void* target = getProcAddress(functionName);
    if (target)
        return target;

    String fullFunctionName(functionName);
    fullFunctionName.append("ARB");
    target = getProcAddress(fullFunctionName.utf8().data());
    if (target)
        return target;

    fullFunctionName = functionName;
    fullFunctionName.append("EXT");
    target = getProcAddress(fullFunctionName.utf8().data());

#if defined(GL_ES_VERSION_2_0)
    fullFunctionName = functionName;
    fullFunctionName.append("ANGLE");
    target = getProcAddress(fullFunctionName.utf8().data());
    if (target)
        return target;

    fullFunctionName = functionName;
    fullFunctionName.append("APPLE");
    target = getProcAddress(fullFunctionName.utf8().data());
#endif

    // A null address is still a failure case.
    if (!target && success)
        *success = false;

    return target;
}

#if PLATFORM(QT) && defined(QT_OPENGL_ES_2)

// With Angle only EGL/GLES2 extensions are available through eglGetProcAddress, not the regular standardized functions.
#define ASSIGN_FUNCTION_TABLE_ENTRY(FunctionName, success) \
    openGLFunctionTable()->FunctionName = reinterpret_cast<FunctionName##Type>(::FunctionName)

#else

#define ASSIGN_FUNCTION_TABLE_ENTRY(FunctionName, success) \
    openGLFunctionTable()->FunctionName = reinterpret_cast<FunctionName##Type>(lookupOpenGLFunctionAddress(#FunctionName, &success))

#endif

#define ASSIGN_FUNCTION_TABLE_ENTRY_EXT(FunctionName) \
    openGLFunctionTable()->FunctionName = reinterpret_cast<FunctionName##Type>(lookupOpenGLFunctionAddress(#FunctionName))

bool initializeOpenGLShims()
{
    static bool success = true;
    static bool initialized = false;
    if (initialized)
        return success;

    initialized = true;
    ASSIGN_FUNCTION_TABLE_ENTRY(glActiveTexture, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glAttachShader, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBindAttribLocation, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBindBuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBindFramebuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBindRenderbuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY_EXT(glBindVertexArray);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBlendColor, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBlendEquation, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBlendEquationSeparate, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBlendFuncSeparate, success);
    // In GLES2 there is optional an ANGLE extension for glBlitFramebuffer
#if defined(GL_ES_VERSION_2_0)
    ASSIGN_FUNCTION_TABLE_ENTRY_EXT(glBlitFramebuffer);
#else
    ASSIGN_FUNCTION_TABLE_ENTRY(glBlitFramebuffer, success);
#endif
    ASSIGN_FUNCTION_TABLE_ENTRY(glBufferData, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glBufferSubData, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glCheckFramebufferStatus, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glCompileShader, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glCompressedTexImage2D, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glCompressedTexSubImage2D, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glCreateProgram, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glCreateShader, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDeleteBuffers, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDeleteFramebuffers, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDeleteProgram, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDeleteRenderbuffers, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDeleteShader, success);
    ASSIGN_FUNCTION_TABLE_ENTRY_EXT(glDeleteVertexArrays);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDetachShader, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glDisableVertexAttribArray, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glEnableVertexAttribArray, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glFramebufferRenderbuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glFramebufferTexture2D, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGenBuffers, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGenerateMipmap, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGenFramebuffers, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGenRenderbuffers, success);
    ASSIGN_FUNCTION_TABLE_ENTRY_EXT(glGenVertexArrays);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetActiveAttrib, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetActiveUniform, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetAttachedShaders, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetAttribLocation, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetBufferParameteriv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetFramebufferAttachmentParameteriv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetProgramInfoLog, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetProgramiv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetRenderbufferParameteriv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetShaderInfoLog, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetShaderiv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetShaderSource, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetUniformfv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetUniformiv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetUniformLocation, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetVertexAttribfv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetVertexAttribiv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glGetVertexAttribPointerv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glIsBuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glIsFramebuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glIsProgram, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glIsRenderbuffer, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glIsShader, success);
    ASSIGN_FUNCTION_TABLE_ENTRY_EXT(glIsVertexArray);
    ASSIGN_FUNCTION_TABLE_ENTRY(glLinkProgram, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glRenderbufferStorage, success);
    // In GLES2 there are optional ANGLE and APPLE extensions for glRenderbufferStorageMultisample.
#if defined(GL_ES_VERSION_2_0)
    ASSIGN_FUNCTION_TABLE_ENTRY_EXT(glRenderbufferStorageMultisample);
#else
    ASSIGN_FUNCTION_TABLE_ENTRY(glRenderbufferStorageMultisample, success);
#endif
    ASSIGN_FUNCTION_TABLE_ENTRY(glSampleCoverage, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glShaderSource, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glStencilFuncSeparate, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glStencilMaskSeparate, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glStencilOpSeparate, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform1f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform1fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform1i, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform1iv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform2f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform2fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform2i, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform2iv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform3f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform3fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform3i, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform3iv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform4f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform4fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform4i, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniform4iv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniformMatrix2fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniformMatrix3fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUniformMatrix4fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glUseProgram, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glValidateProgram, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib1f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib1fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib2f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib2fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib3f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib3fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib4f, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttrib4fv, success);
    ASSIGN_FUNCTION_TABLE_ENTRY(glVertexAttribPointer, success);

    if (!success)
        LOG_ERROR("Could not initialize OpenGL shims");
    return success;
}

} // namespace WebCore

#endif // USE(3D_GRAPHICS)
