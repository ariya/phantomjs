/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef Extensions3DOpenGLES_h
#define Extensions3DOpenGLES_h

#include "Extensions3DOpenGLCommon.h"

#if PLATFORM(QT)
// Takes care of declaring the GLES extensions.
#include <qopengl.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#if OS(QNX) || PLATFORM(QT)
// See https://bugs.webkit.org/show_bug.cgi?id=91030.
// Newer Khorons headers do define these with a PROC suffix, but older headers don't.
#define PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC
#define PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC
#endif

#ifndef GL_EXT_robustness
/* reuse GL_NO_ERROR */
#define GL_GUILTY_CONTEXT_RESET_EXT 0x8253
#define GL_INNOCENT_CONTEXT_RESET_EXT 0x8254
#define GL_UNKNOWN_CONTEXT_RESET_EXT 0x8255
#define GL_CONTEXT_ROBUST_ACCESS_EXT 0x90F3
#define GL_RESET_NOTIFICATION_STRATEGY_EXT 0x8256
#define GL_LOSE_CONTEXT_ON_RESET_EXT 0x8252
#define GL_NO_RESET_NOTIFICATION_EXT 0x8261
#endif

#ifndef GL_EXT_robustness
#define GL_EXT_robustness 1
#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL GC3Denum GL_APIENTRY glGetGraphicsResetStatusEXT(void);
GL_APICALL void GL_APIENTRY glReadnPixelsEXT(GLint x, GLint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, GC3Dsizei bufSize, void *data);
GL_APICALL void GL_APIENTRY glGetnUniformfvEXT(GLuint program, GLint location, GC3Dsizei bufSize, float *params);
GL_APICALL void GL_APIENTRY glGetnUniformivEXT(GLuint program, GLint location, GC3Dsizei bufSize, GLint *params);
#endif
typedef GC3Denum (GL_APIENTRYP PFNGLGETGRAPHICSRESETSTATUSEXTPROC) (void);
typedef void (GL_APIENTRYP PFNGLREADNPIXELSEXTPROC) (GLint x, GLint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, GC3Dsizei bufSize, void *data);
typedef void (GL_APIENTRYP PFNGLGETNUNIFORMFVEXTPROC) (GLuint program, GLint location, GC3Dsizei bufSize, float *params);
typedef void (GL_APIENTRYP PFNGLGETNUNIFORMIVEXTPROC) (GLuint program, GLint location, GC3Dsizei bufSize, GLint *params);
#endif

namespace WebCore {

class Extensions3DOpenGLES : public Extensions3DOpenGLCommon {
public:
    virtual ~Extensions3DOpenGLES();

    virtual void framebufferTexture2DMultisampleIMG(unsigned long target, unsigned long attachment, unsigned long textarget, unsigned int texture, int level, unsigned long samples);
    virtual void renderbufferStorageMultisampleIMG(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height);

    // Extension3D methods
    virtual void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter);
    virtual void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height);
    virtual void copyTextureCHROMIUM(GC3Denum, Platform3DObject, Platform3DObject, GC3Dint, GC3Denum);
    virtual void insertEventMarkerEXT(const String&);
    virtual void pushGroupMarkerEXT(const String&);
    virtual void popGroupMarkerEXT(void);

    virtual Platform3DObject createVertexArrayOES();
    virtual void deleteVertexArrayOES(Platform3DObject);
    virtual GC3Dboolean isVertexArrayOES(Platform3DObject);
    virtual void bindVertexArrayOES(Platform3DObject);
    virtual void drawBuffersEXT(GC3Dsizei, const GC3Denum*);

    // EXT Robustness - reset
    virtual int getGraphicsResetStatusARB();
    void setEXTContextLostCallback(PassOwnPtr<GraphicsContext3D::ContextLostCallback>);

    // EXT Robustness - etc
    virtual void readnPixelsEXT(int x, int y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, GC3Dsizei bufSize, void *data);
    virtual void getnUniformfvEXT(GC3Duint program, int location, GC3Dsizei bufSize, float *params);
    virtual void getnUniformivEXT(GC3Duint program, int location, GC3Dsizei bufSize, int *params);

protected:
    // This class only needs to be instantiated by GraphicsContext3D implementations.
    friend class GraphicsContext3D;
    Extensions3DOpenGLES(GraphicsContext3D*);

    virtual bool supportsExtension(const String&);
    virtual String getExtensions();

    GC3Denum m_contextResetStatus;

    bool m_supportsOESvertexArrayObject;
    bool m_supportsIMGMultisampledRenderToTexture;

    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG m_glFramebufferTexture2DMultisampleIMG;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG m_glRenderbufferStorageMultisampleIMG;
    PFNGLBINDVERTEXARRAYOESPROC m_glBindVertexArrayOES;
    PFNGLDELETEVERTEXARRAYSOESPROC m_glDeleteVertexArraysOES;
    PFNGLGENVERTEXARRAYSOESPROC m_glGenVertexArraysOES;
    PFNGLISVERTEXARRAYOESPROC m_glIsVertexArrayOES;
    PFNGLGETGRAPHICSRESETSTATUSEXTPROC m_glGetGraphicsResetStatusEXT;
    PFNGLREADNPIXELSEXTPROC m_glReadnPixelsEXT;
    PFNGLGETNUNIFORMFVEXTPROC m_glGetnUniformfvEXT;
    PFNGLGETNUNIFORMIVEXTPROC m_glGetnUniformivEXT;

    OwnPtr<GraphicsContext3D::ContextLostCallback> m_contextLostCallback;
};

} // namespace WebCore

#endif // Extensions3DOpenGLES_h
