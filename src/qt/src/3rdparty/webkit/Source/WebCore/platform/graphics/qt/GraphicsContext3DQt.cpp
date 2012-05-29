/*
    Copyright (C) 2010 Tieto Corporation.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
     
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
     
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"

#include "GraphicsContext3D.h"

#include "WebGLObject.h"
#include "CanvasRenderingContext.h"
#include "Extensions3DQt.h"
#include "GraphicsContext.h"
#include "HTMLCanvasElement.h"
#include "HostWindow.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "NotImplemented.h"
#include "QWebPageClient.h"
#include "SharedBuffer.h"
#include "qwebpage.h"
#include <QAbstractScrollArea>
#include <QGraphicsObject>
#include <QGLContext>
#include <QStyleOptionGraphicsItem>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>

#if ENABLE(WEBGL)

namespace WebCore {

#if !defined(GLchar)
typedef char GLchar;
#endif

#if !defined(GL_DEPTH24_STENCIL8)
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

#if !defined(APIENTRY)
#define APIENTRY
#endif

#ifdef QT_OPENGL_ES_2
typedef GLsizeiptr GLsizeiptrType;
typedef GLintptr GLintptrType;
#else
typedef ptrdiff_t GLsizeiptrType;
typedef ptrdiff_t GLintptrType;
#endif

typedef void (APIENTRY* glActiveTextureType) (GLenum);
typedef void (APIENTRY* glAttachShaderType) (GLuint, GLuint);
typedef void (APIENTRY* glBindAttribLocationType) (GLuint, GLuint, const char*);
typedef void (APIENTRY* glBindBufferType) (GLenum, GLuint);
typedef void (APIENTRY* glBindFramebufferType) (GLenum, GLuint);
typedef void (APIENTRY* glBindRenderbufferType) (GLenum, GLuint);
typedef void (APIENTRY* glBlendColorType) (GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (APIENTRY* glBlendEquationType) (GLenum);
typedef void (APIENTRY* glBlendEquationSeparateType)(GLenum, GLenum);
typedef void (APIENTRY* glBlendFuncSeparateType)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef void (APIENTRY* glBufferDataType) (GLenum, GLsizeiptrType, const GLvoid*, GLenum);
typedef void (APIENTRY* glBufferSubDataType) (GLenum, GLintptrType, GLsizeiptrType, const GLvoid*);
typedef GLenum (APIENTRY* glCheckFramebufferStatusType) (GLenum);
typedef void (APIENTRY* glCompileShaderType) (GLuint);
typedef GLuint (APIENTRY* glCreateProgramType) ();
typedef GLuint (APIENTRY* glCreateShaderType) (GLenum);
typedef void (APIENTRY* glDeleteBuffersType) (GLsizei, const GLuint*);
typedef void (APIENTRY* glDeleteFramebuffersType) (GLsizei n, const GLuint*);
typedef void (APIENTRY* glDeleteProgramType) (GLuint);
typedef void (APIENTRY* glDeleteRenderbuffersType) (GLsizei n, const GLuint*);
typedef void (APIENTRY* glDeleteShaderType) (GLuint);
typedef void (APIENTRY* glDetachShaderType) (GLuint, GLuint);
typedef void (APIENTRY* glDisableVertexAttribArrayType) (GLuint);
typedef void (APIENTRY* glEnableVertexAttribArrayType) (GLuint);
typedef void (APIENTRY* glFramebufferRenderbufferType) (GLenum, GLenum, GLenum, GLuint);
typedef void (APIENTRY* glFramebufferTexture2DType) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY* glGenBuffersType) (GLsizei, GLuint*);
typedef void (APIENTRY* glGenerateMipmapType) (GLenum target);
typedef void (APIENTRY* glGenFramebuffersType) (GLsizei, GLuint*);
typedef void (APIENTRY* glGenRenderbuffersType) (GLsizei, GLuint*);
typedef void (APIENTRY* glGetActiveAttribType) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef void (APIENTRY* glGetActiveUniformType) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef void (APIENTRY* glGetAttachedShadersType) (GLuint, GLsizei, GLsizei*, GLuint*);
typedef GLint (APIENTRY* glGetAttribLocationType) (GLuint, const char*);
typedef void (APIENTRY* glGetBufferParameterivType) (GLenum, GLenum, GLint*);
typedef void (APIENTRY* glGetFramebufferAttachmentParameterivType) (GLenum, GLenum, GLenum, GLint* params);
typedef void (APIENTRY* glGetProgramInfoLogType) (GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY* glGetProgramivType) (GLuint, GLenum, GLint*);
typedef void (APIENTRY* glGetRenderbufferParameterivType) (GLenum, GLenum, GLint*);
typedef void (APIENTRY* glGetShaderInfoLogType) (GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY* glGetShaderivType) (GLuint, GLenum, GLint*);
typedef void (APIENTRY* glGetShaderSourceType) (GLuint, GLsizei, GLsizei*, char*);
typedef GLint (APIENTRY* glGetUniformLocationType) (GLuint, const char*);
typedef void (APIENTRY* glGetUniformfvType) (GLuint, GLint, GLfloat*);
typedef void (APIENTRY* glGetUniformivType) (GLuint, GLint, GLint*);
typedef void (APIENTRY* glGetVertexAttribfvType) (GLuint, GLenum, GLfloat*);
typedef void (APIENTRY* glGetVertexAttribivType) (GLuint, GLenum, GLint*);
typedef void (APIENTRY* glGetVertexAttribPointervType) (GLuint, GLenum, GLvoid**);
typedef GLboolean (APIENTRY* glIsBufferType) (GLuint);
typedef GLboolean (APIENTRY* glIsFramebufferType) (GLuint);
typedef GLboolean (APIENTRY* glIsProgramType) (GLuint);
typedef GLboolean (APIENTRY* glIsRenderbufferType) (GLuint);
typedef GLboolean (APIENTRY* glIsShaderType) (GLuint);
typedef void (APIENTRY* glLinkProgramType) (GLuint);
typedef void (APIENTRY* glRenderbufferStorageType) (GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY* glSampleCoverageType) (GLclampf, GLboolean);
typedef void (APIENTRY* glShaderSourceType) (GLuint, GLsizei, const char**, const GLint*);
typedef void (APIENTRY* glStencilFuncSeparateType) (GLenum, GLenum, GLint, GLuint);
typedef void (APIENTRY* glStencilMaskSeparateType) (GLenum, GLuint);
typedef void (APIENTRY* glStencilOpSeparateType) (GLenum, GLenum, GLenum, GLenum);
typedef void (APIENTRY* glUniform1fType) (GLint, GLfloat);
typedef void (APIENTRY* glUniform1fvType) (GLint, GLsizei, const GLfloat*);
typedef void (APIENTRY* glUniform1iType) (GLint, GLint);
typedef void (APIENTRY* glUniform1ivType) (GLint, GLsizei, const GLint*);
typedef void (APIENTRY* glUniform2fType) (GLint, GLfloat, GLfloat);
typedef void (APIENTRY* glUniform2fvType) (GLint, GLsizei, const GLfloat*);
typedef void (APIENTRY* glUniform2iType) (GLint, GLint, GLint);
typedef void (APIENTRY* glUniform2ivType) (GLint, GLsizei, const GLint*);
typedef void (APIENTRY* glUniform3fType) (GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY* glUniform3fvType) (GLint, GLsizei, const GLfloat*);
typedef void (APIENTRY* glUniform3iType) (GLint, GLint, GLint, GLint);
typedef void (APIENTRY* glUniform3ivType) (GLint, GLsizei, const GLint*);
typedef void (APIENTRY* glUniform4fType) (GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY* glUniform4fvType) (GLint, GLsizei, const GLfloat*);
typedef void (APIENTRY* glUniform4iType) (GLint, GLint, GLint, GLint, GLint);
typedef void (APIENTRY* glUniform4ivType) (GLint, GLsizei, const GLint*);
typedef void (APIENTRY* glUniformMatrix2fvType) (GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY* glUniformMatrix3fvType) (GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY* glUniformMatrix4fvType) (GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (APIENTRY* glUseProgramType) (GLuint);
typedef void (APIENTRY* glValidateProgramType) (GLuint);
typedef void (APIENTRY* glVertexAttrib1fType) (GLuint, const GLfloat);
typedef void (APIENTRY* glVertexAttrib1fvType) (GLuint, const GLfloat*);
typedef void (APIENTRY* glVertexAttrib2fType) (GLuint, const GLfloat, const GLfloat);
typedef void (APIENTRY* glVertexAttrib2fvType) (GLuint, const GLfloat*);
typedef void (APIENTRY* glVertexAttrib3fType) (GLuint, const GLfloat, const GLfloat, const GLfloat);
typedef void (APIENTRY* glVertexAttrib3fvType) (GLuint, const GLfloat*);
typedef void (APIENTRY* glVertexAttrib4fType) (GLuint, const GLfloat, const GLfloat, const GLfloat, const GLfloat);
typedef void (APIENTRY* glVertexAttrib4fvType) (GLuint, const GLfloat*);
typedef void (APIENTRY* glVertexAttribPointerType) (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);

class GraphicsContext3DInternal : public QGraphicsObject {
public:
    GraphicsContext3DInternal(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow);
    ~GraphicsContext3DInternal();

    bool isValid() { return m_valid; }

    QGLWidget* getViewportGLWidget();
    void reshape(int width, int height);
    void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    QRectF boundingRect() const;

    glActiveTextureType activeTexture;
    glAttachShaderType attachShader;
    glBindAttribLocationType bindAttribLocation;
    glBindBufferType bindBuffer;
    glBindFramebufferType bindFramebuffer;
    glBindRenderbufferType bindRenderbuffer;
    glBlendColorType blendColor;
    glBlendEquationType blendEquation;
    glBlendEquationSeparateType blendEquationSeparate;
    glBlendFuncSeparateType blendFuncSeparate;
    glBufferDataType bufferData;
    glBufferSubDataType bufferSubData;
    glCheckFramebufferStatusType checkFramebufferStatus;
    glCompileShaderType compileShader;
    glCreateProgramType createProgram;
    glCreateShaderType createShader;
    glDeleteBuffersType deleteBuffers;
    glDeleteFramebuffersType deleteFramebuffers;
    glDeleteProgramType deleteProgram;
    glDeleteRenderbuffersType deleteRenderbuffers;
    glDeleteShaderType deleteShader;
    glDetachShaderType detachShader;
    glDisableVertexAttribArrayType disableVertexAttribArray;
    glEnableVertexAttribArrayType enableVertexAttribArray;
    glFramebufferRenderbufferType framebufferRenderbuffer;
    glFramebufferTexture2DType framebufferTexture2D;
    glGenBuffersType genBuffers;
    glGenerateMipmapType generateMipmap;
    glGenFramebuffersType genFramebuffers;
    glGenRenderbuffersType genRenderbuffers;
    glGetActiveAttribType getActiveAttrib;
    glGetActiveUniformType getActiveUniform;
    glGetAttachedShadersType getAttachedShaders;
    glGetAttribLocationType getAttribLocation;
    glGetBufferParameterivType getBufferParameteriv;
    glGetFramebufferAttachmentParameterivType getFramebufferAttachmentParameteriv;
    glGetProgramInfoLogType getProgramInfoLog;
    glGetProgramivType getProgramiv;
    glGetRenderbufferParameterivType getRenderbufferParameteriv;
    glGetShaderInfoLogType getShaderInfoLog;
    glGetShaderivType getShaderiv;
    glGetShaderSourceType getShaderSource;
    glGetUniformfvType getUniformfv;
    glGetUniformivType getUniformiv;
    glGetUniformLocationType getUniformLocation;
    glGetVertexAttribfvType getVertexAttribfv;
    glGetVertexAttribivType getVertexAttribiv;
    glGetVertexAttribPointervType getVertexAttribPointerv;
    glIsBufferType isBuffer;
    glIsFramebufferType isFramebuffer;
    glIsProgramType isProgram;
    glIsRenderbufferType isRenderbuffer;
    glIsShaderType isShader;
    glLinkProgramType linkProgram;
    glRenderbufferStorageType renderbufferStorage;
    glSampleCoverageType sampleCoverage;
    glShaderSourceType shaderSource;
    glStencilFuncSeparateType stencilFuncSeparate;
    glStencilMaskSeparateType stencilMaskSeparate;
    glStencilOpSeparateType stencilOpSeparate;
    glUniform1fType uniform1f;
    glUniform1fvType uniform1fv;
    glUniform1iType uniform1i;
    glUniform1ivType uniform1iv;
    glUniform2fType uniform2f;
    glUniform2fvType uniform2fv;
    glUniform2iType uniform2i;
    glUniform2ivType uniform2iv;
    glUniform3fType uniform3f;
    glUniform3fvType uniform3fv;
    glUniform3iType uniform3i;
    glUniform3ivType uniform3iv;
    glUniform4fType uniform4f;
    glUniform4fvType uniform4fv;
    glUniform4iType uniform4i;
    glUniform4ivType uniform4iv;
    glUniformMatrix2fvType uniformMatrix2fv;
    glUniformMatrix3fvType uniformMatrix3fv;
    glUniformMatrix4fvType uniformMatrix4fv;
    glUseProgramType useProgram;
    glValidateProgramType validateProgram;
    glVertexAttrib1fType vertexAttrib1f;
    glVertexAttrib1fvType vertexAttrib1fv;
    glVertexAttrib2fType vertexAttrib2f;
    glVertexAttrib2fvType vertexAttrib2fv;
    glVertexAttrib3fType vertexAttrib3f;
    glVertexAttrib3fvType vertexAttrib3fv;
    glVertexAttrib4fType vertexAttrib4f;
    glVertexAttrib4fvType vertexAttrib4fv;
    glVertexAttribPointerType vertexAttribPointer;

    GraphicsContext3D::Attributes m_attrs;
    HostWindow* m_hostWindow;
    QGLWidget* m_glWidget;
    QGLWidget* m_viewportGLWidget;
    QRectF m_boundingRect;
    GLuint m_texture;
    GLuint m_canvasFbo;
    GLuint m_currentFbo;
    GLuint m_depthBuffer;
    bool m_layerComposited;
    ListHashSet<unsigned int> m_syntheticErrors;

    OwnPtr<Extensions3DQt> m_extensions;

private:

    void* getProcAddress(const String& proc);
    bool m_valid;
};

#if defined (QT_OPENGL_ES_2) 
#define GET_PROC_ADDRESS(Proc) Proc
#else
#define GET_PROC_ADDRESS(Proc) reinterpret_cast<Proc##Type>(getProcAddress(#Proc));
#endif

bool GraphicsContext3D::isGLES2Compliant() const
{
#if defined (QT_OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

GraphicsContext3DInternal::GraphicsContext3DInternal(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow)
    : m_attrs(attrs)
    , m_hostWindow(hostWindow)
    , m_glWidget(0)
    , m_viewportGLWidget(0)
    , m_texture(0)
    , m_canvasFbo(0)
    , m_currentFbo(0)
    , m_depthBuffer(0)
    , m_layerComposited(false)
    , m_valid(true)
{
    m_viewportGLWidget = getViewportGLWidget();

    if (m_viewportGLWidget)
        m_glWidget = new QGLWidget(0, m_viewportGLWidget);
    else
        m_glWidget = new QGLWidget();

    if (!m_glWidget->isValid()) {
        LOG_ERROR("GraphicsContext3D: QGLWidget initialization failed.");
        m_valid = false;
        return;
    }

    // Geometry can be set to zero because m_glWidget is used only for its QGLContext.
    m_glWidget->setGeometry(0, 0, 0, 0);

#if defined(QT_OPENGL_ES_2)
    m_attrs.stencil = false;
#else
    if (m_attrs.stencil)
        m_attrs.depth = true;
#endif
    m_attrs.antialias = false;

    m_glWidget->makeCurrent();

    activeTexture = GET_PROC_ADDRESS(glActiveTexture);
    attachShader = GET_PROC_ADDRESS(glAttachShader);
    bindAttribLocation = GET_PROC_ADDRESS(glBindAttribLocation);
    bindBuffer = GET_PROC_ADDRESS(glBindBuffer);
    bindFramebuffer = GET_PROC_ADDRESS(glBindFramebuffer);
    bindRenderbuffer = GET_PROC_ADDRESS(glBindRenderbuffer);
    blendColor = GET_PROC_ADDRESS(glBlendColor);
    blendEquation = GET_PROC_ADDRESS(glBlendEquation);
    blendEquationSeparate = GET_PROC_ADDRESS(glBlendEquationSeparate);
    blendFuncSeparate = GET_PROC_ADDRESS(glBlendFuncSeparate);
    bufferData = GET_PROC_ADDRESS(glBufferData);
    bufferSubData = GET_PROC_ADDRESS(glBufferSubData);
    checkFramebufferStatus = GET_PROC_ADDRESS(glCheckFramebufferStatus);
    compileShader = GET_PROC_ADDRESS(glCompileShader);
    createProgram = GET_PROC_ADDRESS(glCreateProgram);
    createShader = GET_PROC_ADDRESS(glCreateShader);
    deleteBuffers = GET_PROC_ADDRESS(glDeleteBuffers);
    deleteFramebuffers = GET_PROC_ADDRESS(glDeleteFramebuffers);
    deleteProgram = GET_PROC_ADDRESS(glDeleteProgram);
    deleteRenderbuffers = GET_PROC_ADDRESS(glDeleteRenderbuffers);
    deleteShader = GET_PROC_ADDRESS(glDeleteShader);
    detachShader = GET_PROC_ADDRESS(glDetachShader);
    disableVertexAttribArray = GET_PROC_ADDRESS(glDisableVertexAttribArray);
    enableVertexAttribArray = GET_PROC_ADDRESS(glEnableVertexAttribArray);
    framebufferRenderbuffer = GET_PROC_ADDRESS(glFramebufferRenderbuffer);
    framebufferTexture2D = GET_PROC_ADDRESS(glFramebufferTexture2D);
    genBuffers = GET_PROC_ADDRESS(glGenBuffers);
    generateMipmap = GET_PROC_ADDRESS(glGenerateMipmap);
    genFramebuffers = GET_PROC_ADDRESS(glGenFramebuffers);
    genRenderbuffers = GET_PROC_ADDRESS(glGenRenderbuffers);
    getActiveAttrib = GET_PROC_ADDRESS(glGetActiveAttrib);
    getActiveUniform = GET_PROC_ADDRESS(glGetActiveUniform);
    getAttachedShaders = GET_PROC_ADDRESS(glGetAttachedShaders);
    getAttribLocation = GET_PROC_ADDRESS(glGetAttribLocation);
    getBufferParameteriv = GET_PROC_ADDRESS(glGetBufferParameteriv);
    getFramebufferAttachmentParameteriv = GET_PROC_ADDRESS(glGetFramebufferAttachmentParameteriv);
    getProgramInfoLog = GET_PROC_ADDRESS(glGetProgramInfoLog);
    getProgramiv = GET_PROC_ADDRESS(glGetProgramiv);
    getRenderbufferParameteriv = GET_PROC_ADDRESS(glGetRenderbufferParameteriv);
    getShaderInfoLog = GET_PROC_ADDRESS(glGetShaderInfoLog);
    getShaderiv = GET_PROC_ADDRESS(glGetShaderiv);
    getShaderSource = GET_PROC_ADDRESS(glGetShaderSource);
    getUniformfv = GET_PROC_ADDRESS(glGetUniformfv);
    getUniformiv = GET_PROC_ADDRESS(glGetUniformiv);
    getUniformLocation = GET_PROC_ADDRESS(glGetUniformLocation);
    getVertexAttribfv = GET_PROC_ADDRESS(glGetVertexAttribfv);
    getVertexAttribiv = GET_PROC_ADDRESS(glGetVertexAttribiv);
    getVertexAttribPointerv = GET_PROC_ADDRESS(glGetVertexAttribPointerv);
    isBuffer = GET_PROC_ADDRESS(glIsBuffer);
    isFramebuffer = GET_PROC_ADDRESS(glIsFramebuffer);
    isProgram = GET_PROC_ADDRESS(glIsProgram);
    isRenderbuffer = GET_PROC_ADDRESS(glIsRenderbuffer);
    isShader = GET_PROC_ADDRESS(glIsShader);
    linkProgram = GET_PROC_ADDRESS(glLinkProgram);
    renderbufferStorage = GET_PROC_ADDRESS(glRenderbufferStorage);
    sampleCoverage = GET_PROC_ADDRESS(glSampleCoverage);
    shaderSource = GET_PROC_ADDRESS(glShaderSource);
    stencilFuncSeparate = GET_PROC_ADDRESS(glStencilFuncSeparate);
    stencilMaskSeparate = GET_PROC_ADDRESS(glStencilMaskSeparate);
    stencilOpSeparate = GET_PROC_ADDRESS(glStencilOpSeparate);
    uniform1f = GET_PROC_ADDRESS(glUniform1f);
    uniform1fv = GET_PROC_ADDRESS(glUniform1fv);
    uniform1i = GET_PROC_ADDRESS(glUniform1i);
    uniform1iv = GET_PROC_ADDRESS(glUniform1iv);
    uniform2f = GET_PROC_ADDRESS(glUniform2f);
    uniform2fv = GET_PROC_ADDRESS(glUniform2fv);
    uniform2i = GET_PROC_ADDRESS(glUniform2i);
    uniform2iv = GET_PROC_ADDRESS(glUniform2iv);
    uniform3f = GET_PROC_ADDRESS(glUniform3f);
    uniform3fv = GET_PROC_ADDRESS(glUniform3fv);
    uniform3i = GET_PROC_ADDRESS(glUniform3i);
    uniform3iv = GET_PROC_ADDRESS(glUniform3iv);
    uniform4f = GET_PROC_ADDRESS(glUniform4f);
    uniform4fv = GET_PROC_ADDRESS(glUniform4fv);
    uniform4i = GET_PROC_ADDRESS(glUniform4i);
    uniform4iv = GET_PROC_ADDRESS(glUniform4iv);
    uniformMatrix2fv = GET_PROC_ADDRESS(glUniformMatrix2fv);
    uniformMatrix3fv = GET_PROC_ADDRESS(glUniformMatrix3fv);
    uniformMatrix4fv = GET_PROC_ADDRESS(glUniformMatrix4fv);
    useProgram = GET_PROC_ADDRESS(glUseProgram);
    validateProgram = GET_PROC_ADDRESS(glValidateProgram);
    vertexAttrib1f = GET_PROC_ADDRESS(glVertexAttrib1f);
    vertexAttrib1fv = GET_PROC_ADDRESS(glVertexAttrib1fv);
    vertexAttrib2f = GET_PROC_ADDRESS(glVertexAttrib2f);
    vertexAttrib2fv = GET_PROC_ADDRESS(glVertexAttrib2fv);
    vertexAttrib3f = GET_PROC_ADDRESS(glVertexAttrib3f);
    vertexAttrib3fv = GET_PROC_ADDRESS(glVertexAttrib3fv);
    vertexAttrib4f = GET_PROC_ADDRESS(glVertexAttrib4f);
    vertexAttrib4fv = GET_PROC_ADDRESS(glVertexAttrib4fv);
    vertexAttribPointer = GET_PROC_ADDRESS(glVertexAttribPointer);

    if (!m_valid) {
        LOG_ERROR("GraphicsContext3D: All needed OpenGL extensions are not available");
        return;
    }

    // Create buffers for the canvas FBO.
    genFramebuffers(/* count */ 1, &m_canvasFbo);

    glGenTextures(1, &m_texture);
    glBindTexture(GraphicsContext3D::TEXTURE_2D, m_texture);
    glTexParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    glTexParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    glTexParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    glTexParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    glBindTexture(GraphicsContext3D::TEXTURE_2D, 0);

    if (m_attrs.depth)
        genRenderbuffers(/* count */ 1, &m_depthBuffer);

    // Bind canvas FBO and set initial clear color to black.
    m_currentFbo = m_canvasFbo;
    bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_canvasFbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

GraphicsContext3DInternal::~GraphicsContext3DInternal()
{
    m_glWidget->makeCurrent();
    if (m_glWidget->isValid()) {
        ::glDeleteTextures(1, &m_texture);
        deleteRenderbuffers(1, &m_depthBuffer);
        deleteFramebuffers(1, &m_canvasFbo);
    }
    delete m_glWidget;
    m_glWidget = 0;
}

QGLWidget* GraphicsContext3DInternal::getViewportGLWidget()
{
    QWebPageClient* webPageClient = m_hostWindow->platformPageClient();
    if (!webPageClient)
        return 0;

    QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>(webPageClient->ownerWidget());
    if (scrollArea)
        return qobject_cast<QGLWidget*>(scrollArea->viewport());

    return 0;
}

static inline quint32 swapBgrToRgb(quint32 pixel)
{
    return ((pixel << 16) & 0xff0000) | ((pixel >> 16) & 0xff) | (pixel & 0xff00ff00);
}

void GraphicsContext3DInternal::reshape(int width, int height)
{
    if (width == m_boundingRect.width() && height == m_boundingRect.height())
        return;

    m_boundingRect = QRectF(QPointF(0, 0), QSizeF(width, height));

    m_glWidget->makeCurrent();

    // Create color buffer
    glBindTexture(GraphicsContext3D::TEXTURE_2D, m_texture);
    if (m_attrs.alpha)
        glTexImage2D(GraphicsContext3D::TEXTURE_2D, /* level */ 0, GraphicsContext3D::RGBA, width, height, /* border */ 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, /* data */ 0);
    else
        glTexImage2D(GraphicsContext3D::TEXTURE_2D, /* level */ 0, GraphicsContext3D::RGB, width, height, /* border */ 0, GraphicsContext3D::RGB, GraphicsContext3D::UNSIGNED_BYTE, /* data */ 0);
    glBindTexture(GraphicsContext3D::TEXTURE_2D, 0);

    if (m_attrs.depth) {
        // Create depth and stencil buffers.
        bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
#if defined(QT_OPENGL_ES_2)
        renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_COMPONENT16, width, height);
#else
        if (m_attrs.stencil)
            renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        else
            renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_COMPONENT, width, height);
#endif
        bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
    }

    // Construct canvas FBO.
    bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_canvasFbo);
    framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, m_texture, 0);
    if (m_attrs.depth)
        framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
#if !defined(QT_OPENGL_ES_2)
    if (m_attrs.stencil)
        framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
#endif

    GLenum status = checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER);
    if (status != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("GraphicsContext3D: Canvas FBO initialization failed.");
        return;
    }

    int clearFlags = GraphicsContext3D::COLOR_BUFFER_BIT;
    if (m_attrs.depth)
        clearFlags |= GraphicsContext3D::DEPTH_BUFFER_BIT;
    if (m_attrs.stencil)
        clearFlags |= GraphicsContext3D::STENCIL_BUFFER_BIT;

    glClear(clearFlags);
    glFlush();
}

void GraphicsContext3DInternal::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    QRectF rect = option ? option->rect : boundingRect();

    // Use direct texture mapping if WebGL canvas has a shared OpenGL context
    // with browsers OpenGL context.
    QGLWidget* viewportGLWidget = getViewportGLWidget();
    if (viewportGLWidget && viewportGLWidget == m_viewportGLWidget && viewportGLWidget == painter->device()) {
        viewportGLWidget->drawTexture(rect, m_texture);
        return;
    }

    // Alternatively read pixels to a memory buffer.
    QImage offscreenImage(rect.width(), rect.height(), QImage::Format_ARGB32);
    quint32* imagePixels = reinterpret_cast<quint32*>(offscreenImage.bits());

    m_glWidget->makeCurrent();
    bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_canvasFbo);
    glReadPixels(/* x */ 0, /* y */ 0, rect.width(), rect.height(), GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, imagePixels);

    bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_currentFbo);

    // OpenGL gives us ABGR on 32 bits, and with the origin at the bottom left
    // We need RGB32 or ARGB32_PM, with the origin at the top left.
    quint32* pixelsSrc = imagePixels;
    const int height = static_cast<int>(rect.height());
    const int width = static_cast<int>(rect.width());
    const int halfHeight = height / 2;
    for (int row = 0; row < halfHeight; ++row) {
        const int targetIdx = (height - 1 - row) * width;
        quint32* pixelsDst = imagePixels + targetIdx;
        for (int column = 0; column < width; ++column) {
            quint32 tempPixel = *pixelsSrc;
            *pixelsSrc = swapBgrToRgb(*pixelsDst);
            *pixelsDst = swapBgrToRgb(tempPixel);
            ++pixelsSrc;
            ++pixelsDst;
        }
    }
    if (static_cast<int>(height) % 2) {
        for (int column = 0; column < width; ++column) {
            *pixelsSrc = swapBgrToRgb(*pixelsSrc);
            ++pixelsSrc;
        }
    }
    painter->drawImage(/* x */ 0, /* y */ 0, offscreenImage);
}

QRectF GraphicsContext3DInternal::boundingRect() const
{
    return m_boundingRect;
}

void* GraphicsContext3DInternal::getProcAddress(const String& proc)
{
    String ext[3] = { "", "ARB", "EXT" };

    for (int i = 0; i < 3; i++) {
        String nameWithExt = proc + ext[i];

        void* addr = m_glWidget->context()->getProcAddress(QString(nameWithExt));
        if (addr) 
            return addr;
    }

    LOG_ERROR("GraphicsContext3D: Did not find GL function %s", proc.utf8().data());
    m_valid = false;
    return 0;
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to the HostWindow.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attrs, hostWindow, false));
    return context->m_internal ? context.release() : 0;
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, bool)
    : m_internal(adoptPtr(new GraphicsContext3DInternal(attrs, hostWindow)))
{
    if (!m_internal->isValid())
        m_internal.clear();
}

GraphicsContext3D::~GraphicsContext3D()
{
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D()
{
    return m_internal->m_glWidget;
}

Platform3DObject GraphicsContext3D::platformTexture() const
{
    return m_internal->m_texture;
}

PlatformLayer* GraphicsContext3D::platformLayer() const
{
    return m_internal.get();
}

void GraphicsContext3D::makeContextCurrent()
{
    m_internal->m_glWidget->makeCurrent();
}

void GraphicsContext3D::paintRenderingResultsToCanvas(CanvasRenderingContext* context)
{
    m_internal->m_glWidget->makeCurrent();
    HTMLCanvasElement* canvas = context->canvas();
    ImageBuffer* imageBuffer = canvas->buffer();
    QPainter* painter = imageBuffer->context()->platformContext();
    m_internal->paint(painter, 0, 0);
}

PassRefPtr<ImageData> GraphicsContext3D::paintRenderingResultsToImageData()
{
    // FIXME: This needs to be implemented for proper non-premultiplied-alpha
    // support.
    return 0;
}

void GraphicsContext3D::reshape(int width, int height)
{
    if ((width == m_currentWidth && height == m_currentHeight) || (!m_internal))
        return;

    m_currentWidth = width;
    m_currentHeight = height;

    m_internal->reshape(width, height);
}

IntSize GraphicsContext3D::getInternalFramebufferSize()
{
    return IntSize(m_currentWidth, m_currentHeight);
}

void GraphicsContext3D::activeTexture(GC3Denum texture)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->activeTexture(texture);
}

void GraphicsContext3D::attachShader(Platform3DObject program, Platform3DObject shader)
{
    ASSERT(program);
    ASSERT(shader);
    m_internal->m_glWidget->makeCurrent();
    m_internal->attachShader(program, shader);
}

void GraphicsContext3D::getAttachedShaders(Platform3DObject program, GC3Dsizei maxCount, GC3Dsizei* count, Platform3DObject* shaders)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return;
    }

    m_internal->m_glWidget->makeCurrent();
    m_internal->getAttachedShaders(program, maxCount, count, shaders);
}

void GraphicsContext3D::bindAttribLocation(Platform3DObject program, GC3Duint index, const String& name)
{
    ASSERT(program);
    m_internal->m_glWidget->makeCurrent();
    m_internal->bindAttribLocation(program, index, name.utf8().data());
}

void GraphicsContext3D::bindBuffer(GC3Denum target, Platform3DObject buffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->bindBuffer(target, buffer);
}

void GraphicsContext3D::bindFramebuffer(GC3Denum target, Platform3DObject buffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->m_currentFbo = buffer ? buffer : m_internal->m_canvasFbo;
    m_internal->bindFramebuffer(target, m_internal->m_currentFbo);
}

void GraphicsContext3D::bindRenderbuffer(GC3Denum target, Platform3DObject renderbuffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->bindRenderbuffer(target, renderbuffer);
}

void GraphicsContext3D::bindTexture(GC3Denum target, Platform3DObject texture)
{
    m_internal->m_glWidget->makeCurrent();
    glBindTexture(target, texture);
}

void GraphicsContext3D::blendColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->blendColor(red, green, blue, alpha);
}

void GraphicsContext3D::blendEquation(GC3Denum mode)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->blendEquation(mode);
}

void GraphicsContext3D::blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->blendEquationSeparate(modeRGB, modeAlpha);
}

void GraphicsContext3D::blendFunc(GC3Denum sfactor, GC3Denum dfactor)
{
    m_internal->m_glWidget->makeCurrent();
    glBlendFunc(sfactor, dfactor);
}       

void GraphicsContext3D::blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->blendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GraphicsContext3D::bufferData(GC3Denum target, GC3Dsizeiptr size, GC3Denum usage)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->bufferData(target, size, /* data */ 0, usage);
}

void GraphicsContext3D::bufferData(GC3Denum target, GC3Dsizeiptr size, const void* data, GC3Denum usage)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->bufferData(target, size, data, usage);
}

void GraphicsContext3D::bufferSubData(GC3Denum target, GC3Dintptr offset, GC3Dsizeiptr size, const void* data)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->bufferSubData(target, offset, size, data);
}

GC3Denum GraphicsContext3D::checkFramebufferStatus(GC3Denum target)
{
    m_internal->m_glWidget->makeCurrent();
    return m_internal->checkFramebufferStatus(target);
}

void GraphicsContext3D::clearColor(GC3Dclampf r, GC3Dclampf g, GC3Dclampf b, GC3Dclampf a)
{
    m_internal->m_glWidget->makeCurrent();
    glClearColor(r, g, b, a);
}

void GraphicsContext3D::clear(GC3Dbitfield mask)
{
    m_internal->m_glWidget->makeCurrent();
    glClear(mask);
}

void GraphicsContext3D::clearDepth(GC3Dclampf depth)
{
    m_internal->m_glWidget->makeCurrent();
#if defined(QT_OPENGL_ES_2)
    glClearDepthf(depth);
#else
    glClearDepth(depth);
#endif
}

void GraphicsContext3D::clearStencil(GC3Dint s)
{
    m_internal->m_glWidget->makeCurrent();
    glClearStencil(s);
}

void GraphicsContext3D::colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha)
{
    m_internal->m_glWidget->makeCurrent();
    glColorMask(red, green, blue, alpha);
}

void GraphicsContext3D::compileShader(Platform3DObject shader)
{
    ASSERT(shader);
    m_internal->m_glWidget->makeCurrent();
    m_internal->compileShader(shader);
}

void GraphicsContext3D::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    m_internal->m_glWidget->makeCurrent();
    glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

void GraphicsContext3D::copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    m_internal->m_glWidget->makeCurrent();
    glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void GraphicsContext3D::cullFace(GC3Denum mode)
{
    m_internal->m_glWidget->makeCurrent();
    glCullFace(mode);
}

void GraphicsContext3D::depthFunc(GC3Denum func)
{
    m_internal->m_glWidget->makeCurrent();
    glDepthFunc(func);
}

void GraphicsContext3D::depthMask(GC3Dboolean flag)
{
    m_internal->m_glWidget->makeCurrent();
    glDepthMask(flag);
}

void GraphicsContext3D::depthRange(GC3Dclampf zNear, GC3Dclampf zFar)
{
    m_internal->m_glWidget->makeCurrent();
#if defined(QT_OPENGL_ES_2)
    glDepthRangef(zNear, zFar);
#else
    glDepthRange(zNear, zFar);
#endif
}

void GraphicsContext3D::detachShader(Platform3DObject program, Platform3DObject shader)
{
    ASSERT(program);
    ASSERT(shader);
    m_internal->m_glWidget->makeCurrent();
    m_internal->detachShader(program, shader);
}

void GraphicsContext3D::disable(GC3Denum cap)
{
    m_internal->m_glWidget->makeCurrent();
    glDisable(cap);
}

void GraphicsContext3D::disableVertexAttribArray(GC3Duint index)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->disableVertexAttribArray(index);
}

void GraphicsContext3D::drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count)
{
    m_internal->m_glWidget->makeCurrent();
    glDrawArrays(mode, first, count);
}

void GraphicsContext3D::drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset)
{
    m_internal->m_glWidget->makeCurrent();
    glDrawElements(mode, count, type, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(offset)));
}

void GraphicsContext3D::enable(GC3Denum cap)
{
    m_internal->m_glWidget->makeCurrent();
    glEnable(cap);
}

void GraphicsContext3D::enableVertexAttribArray(GC3Duint index)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->enableVertexAttribArray(index);
}

void GraphicsContext3D::finish()
{
    m_internal->m_glWidget->makeCurrent();
    glFinish();
}

void GraphicsContext3D::flush()
{
    m_internal->m_glWidget->makeCurrent();
    glFlush();
}

void GraphicsContext3D::framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, Platform3DObject buffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->framebufferRenderbuffer(target, attachment, renderbuffertarget, buffer);
}

void GraphicsContext3D::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject texture, GC3Dint level)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->framebufferTexture2D(target, attachment, textarget, texture, level);
}

void GraphicsContext3D::frontFace(GC3Denum mode)
{
    m_internal->m_glWidget->makeCurrent();
    glFrontFace(mode);
}

void GraphicsContext3D::generateMipmap(GC3Denum target)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->generateMipmap(target);
}

bool GraphicsContext3D::getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }

    m_internal->m_glWidget->makeCurrent();

    GLint maxLength = 0;
    m_internal->getProgramiv(program, GraphicsContext3D::ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);

    GLchar* name = (GLchar*) fastMalloc(maxLength);
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;

    m_internal->getActiveAttrib(program, index, maxLength, &nameLength, &size, &type, name);

    if (!nameLength) {
        fastFree(name);
        return false;
    }

    info.name = String(name, nameLength);
    info.type = type;
    info.size = size;

    fastFree(name);
    return true;
}
    
bool GraphicsContext3D::getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }

    m_internal->m_glWidget->makeCurrent();

    GLint maxLength = 0;
    m_internal->getProgramiv(static_cast<GLuint>(program), GraphicsContext3D::ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLchar* name = (GLchar*) fastMalloc(maxLength);
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;

    m_internal->getActiveUniform(static_cast<GLuint>(program), index, maxLength, &nameLength, &size, &type, name);

    if (!nameLength) {
        fastFree(name);
        return false;
    }

    info.name = String(name, nameLength);
    info.type = type;
    info.size = size;

    fastFree(name);
    return true;
}

int GraphicsContext3D::getAttribLocation(Platform3DObject program, const String& name)
{
    if (!program)
        return -1;
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->getAttribLocation(program, name.utf8().data());
}

GraphicsContext3D::Attributes GraphicsContext3D::getContextAttributes()
{
    return m_internal->m_attrs;
}

GC3Denum GraphicsContext3D::getError()
{
    if (m_internal->m_syntheticErrors.size() > 0) {
        ListHashSet<GC3Denum>::iterator iter = m_internal->m_syntheticErrors.begin();
        GC3Denum err = *iter;
        m_internal->m_syntheticErrors.remove(iter);
        return err;
    }

    m_internal->m_glWidget->makeCurrent();
    return glGetError();
}

String GraphicsContext3D::getString(GC3Denum name)
{
    m_internal->m_glWidget->makeCurrent();
    return String((const char*) glGetString(name));
}

void GraphicsContext3D::hint(GC3Denum target, GC3Denum mode)
{
    m_internal->m_glWidget->makeCurrent();
    glHint(target, mode);
}

GC3Dboolean GraphicsContext3D::isBuffer(Platform3DObject buffer)
{
    if (!buffer)
        return GL_FALSE;
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->isBuffer(buffer);
}

GC3Dboolean GraphicsContext3D::isEnabled(GC3Denum cap)
{
    m_internal->m_glWidget->makeCurrent();
    return glIsEnabled(cap);
}

GC3Dboolean GraphicsContext3D::isFramebuffer(Platform3DObject framebuffer)
{
    if (!framebuffer)
        return GL_FALSE;
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->isFramebuffer(framebuffer);
}

GC3Dboolean GraphicsContext3D::isProgram(Platform3DObject program)
{
    if (!program)
        return GL_FALSE;
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->isProgram(program);
}

GC3Dboolean GraphicsContext3D::isRenderbuffer(Platform3DObject renderbuffer)
{
    if (!renderbuffer)
        return GL_FALSE;
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->isRenderbuffer(renderbuffer);
}

GC3Dboolean GraphicsContext3D::isShader(Platform3DObject shader)
{
    if (!shader)
        return GL_FALSE;
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->isShader(shader);
}

GC3Dboolean GraphicsContext3D::isTexture(Platform3DObject texture)
{
    if (!texture)
        return GL_FALSE;
    
    m_internal->m_glWidget->makeCurrent();
    return glIsTexture(texture);
}

void GraphicsContext3D::lineWidth(GC3Dfloat width)
{
    m_internal->m_glWidget->makeCurrent();
    glLineWidth(static_cast<float>(width));
}

void GraphicsContext3D::linkProgram(Platform3DObject program)
{
    ASSERT(program);
    m_internal->m_glWidget->makeCurrent();
    m_internal->linkProgram(program);
}

void GraphicsContext3D::pixelStorei(GC3Denum paramName, GC3Dint param)
{
    m_internal->m_glWidget->makeCurrent();
    glPixelStorei(paramName, param);
}

void GraphicsContext3D::polygonOffset(GC3Dfloat factor, GC3Dfloat units)
{
    m_internal->m_glWidget->makeCurrent();
    glPolygonOffset(factor, units);
}

void GraphicsContext3D::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    m_internal->m_glWidget->makeCurrent();
    
    if (type != GraphicsContext3D::UNSIGNED_BYTE || format != GraphicsContext3D::RGBA)
        return;
        
    glReadPixels(x, y, width, height, format, type, data);
}

void GraphicsContext3D::releaseShaderCompiler()
{
    m_internal->m_glWidget->makeCurrent();
    notImplemented();
}

void GraphicsContext3D::renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height)
{
    m_internal->m_glWidget->makeCurrent();
#if !defined(QT_OPENGL_ES_2)
    switch (internalformat) {
    case DEPTH_STENCIL:
        internalformat = GL_DEPTH24_STENCIL8;
        break;
    case DEPTH_COMPONENT16:
        internalformat = DEPTH_COMPONENT;
        break;
    case RGBA4:
    case RGB5_A1:
        internalformat = RGBA;
        break;
    case RGB565:
        internalformat = RGB;
        break;
    }
#endif
    m_internal->renderbufferStorage(target, internalformat, width, height);
}

void GraphicsContext3D::sampleCoverage(GC3Dclampf value, GC3Dboolean invert)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->sampleCoverage(value, invert);
}

void GraphicsContext3D::scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    m_internal->m_glWidget->makeCurrent();
    glScissor(x, y, width, height);
}

void GraphicsContext3D::shaderSource(Platform3DObject shader, const String& source)
{
    ASSERT(shader);

    m_internal->m_glWidget->makeCurrent();

    String prefixedSource;

#if defined (QT_OPENGL_ES_2)
    prefixedSource.append("precision mediump float;\n");
#endif

    prefixedSource.append(source);

    CString sourceCS = prefixedSource.utf8();
    const char* data = sourceCS.data();
    int length = prefixedSource.length();
    m_internal->shaderSource((GLuint) shader, /* count */ 1, &data, &length);
}

void GraphicsContext3D::stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    m_internal->m_glWidget->makeCurrent();
    glStencilFunc(func, ref, mask);
}

void GraphicsContext3D::stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->stencilFuncSeparate(face, func, ref, mask);
}

void GraphicsContext3D::stencilMask(GC3Duint mask)
{
    m_internal->m_glWidget->makeCurrent();
    glStencilMask(mask);
}

void GraphicsContext3D::stencilMaskSeparate(GC3Denum face, GC3Duint mask)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->stencilMaskSeparate(face, mask);
}

void GraphicsContext3D::stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    m_internal->m_glWidget->makeCurrent();
    glStencilOp(fail, zfail, zpass);
}

void GraphicsContext3D::stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->stencilOpSeparate(face, fail, zfail, zpass);
}

void GraphicsContext3D::texParameterf(GC3Denum target, GC3Denum paramName, GC3Dfloat value)
{
    m_internal->m_glWidget->makeCurrent();
    glTexParameterf(target, paramName, value);
}

void GraphicsContext3D::texParameteri(GC3Denum target, GC3Denum paramName, GC3Dint value)
{
    m_internal->m_glWidget->makeCurrent();
    glTexParameteri(target, paramName, value);
}

void GraphicsContext3D::uniform1f(GC3Dint location, GC3Dfloat v0)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform1f(location, v0);
}

void GraphicsContext3D::uniform1fv(GC3Dint location, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform1fv(location, size, array);
}

void GraphicsContext3D::uniform2f(GC3Dint location, GC3Dfloat v0, GC3Dfloat v1)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform2f(location, v0, v1);
}

void GraphicsContext3D::uniform2fv(GC3Dint location, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform2fv(location, size, array);
}

void GraphicsContext3D::uniform3f(GC3Dint location, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform3f(location, v0, v1, v2);
}

void GraphicsContext3D::uniform3fv(GC3Dint location, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform3fv(location, size, array);
}

void GraphicsContext3D::uniform4f(GC3Dint location, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform4f(location, v0, v1, v2, v3);
}

void GraphicsContext3D::uniform4fv(GC3Dint location, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform4fv(location, size, array);
}

void GraphicsContext3D::uniform1i(GC3Dint location, GC3Dint v0)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform1i(location, v0);
}

void GraphicsContext3D::uniform1iv(GC3Dint location, GC3Dint* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform1iv(location, size, array);
}

void GraphicsContext3D::uniform2i(GC3Dint location, GC3Dint v0, GC3Dint v1)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform2i(location, v0, v1);
}

void GraphicsContext3D::uniform2iv(GC3Dint location, GC3Dint* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform2iv(location, size, array);
}

void GraphicsContext3D::uniform3i(GC3Dint location, GC3Dint v0, GC3Dint v1, GC3Dint v2)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform3i(location, v0, v1, v2);
}

void GraphicsContext3D::uniform3iv(GC3Dint location, GC3Dint* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform3iv(location, size, array);
}

void GraphicsContext3D::uniform4i(GC3Dint location, GC3Dint v0, GC3Dint v1, GC3Dint v2, GC3Dint v3)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform4i(location, v0, v1, v2, v3);
}

void GraphicsContext3D::uniform4iv(GC3Dint location, GC3Dint* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniform4iv(location, size, array);
}

void GraphicsContext3D::uniformMatrix2fv(GC3Dint location, GC3Dboolean transpose, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniformMatrix2fv(location, size, transpose, array);
}

void GraphicsContext3D::uniformMatrix3fv(GC3Dint location, GC3Dboolean transpose, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniformMatrix3fv(location, size, transpose, array);
}

void GraphicsContext3D::uniformMatrix4fv(GC3Dint location, GC3Dboolean transpose, GC3Dfloat* array, GC3Dsizei size)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->uniformMatrix4fv(location, size, transpose, array);
}

void GraphicsContext3D::useProgram(Platform3DObject program)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->useProgram(program);
}

void GraphicsContext3D::validateProgram(Platform3DObject program)
{
    ASSERT(program);
    
    m_internal->m_glWidget->makeCurrent();
    m_internal->validateProgram(program);
}

void GraphicsContext3D::vertexAttrib1f(GC3Duint index, GC3Dfloat v0)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib1f(index, v0);
}

void GraphicsContext3D::vertexAttrib1fv(GC3Duint index, GC3Dfloat* array)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib1fv(index, array);
}

void GraphicsContext3D::vertexAttrib2f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib2f(index, v0, v1);
}

void GraphicsContext3D::vertexAttrib2fv(GC3Duint index, GC3Dfloat* array)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib2fv(index, array);
}

void GraphicsContext3D::vertexAttrib3f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib3f(index, v0, v1, v2);
}

void GraphicsContext3D::vertexAttrib3fv(GC3Duint index, GC3Dfloat* array)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib3fv(index, array);
}

void GraphicsContext3D::vertexAttrib4f(GC3Duint index, GC3Dfloat v0, GC3Dfloat v1, GC3Dfloat v2, GC3Dfloat v3)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib4f(index, v0, v1, v2, v3);
}

void GraphicsContext3D::vertexAttrib4fv(GC3Duint index, GC3Dfloat* array)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttrib4fv(index, array);
}

void GraphicsContext3D::vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, GC3Dintptr offset)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->vertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(offset)));
}

void GraphicsContext3D::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    m_internal->m_glWidget->makeCurrent();
    glViewport(x, y, width, height);
}

void GraphicsContext3D::getBooleanv(GC3Denum paramName, GC3Dboolean* value)
{
    m_internal->m_glWidget->makeCurrent();
    glGetBooleanv(paramName, value);
}

void GraphicsContext3D::getBufferParameteriv(GC3Denum target, GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getBufferParameteriv(target, paramName, value);
}

void GraphicsContext3D::getFloatv(GC3Denum paramName, GC3Dfloat* value)
{
    m_internal->m_glWidget->makeCurrent();
    glGetFloatv(paramName, value);
}

void GraphicsContext3D::getFramebufferAttachmentParameteriv(GC3Denum target, GC3Denum attachment, GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getFramebufferAttachmentParameteriv(target, attachment, paramName, value);
}

void GraphicsContext3D::getIntegerv(GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    glGetIntegerv(paramName, value);
}

void GraphicsContext3D::getProgramiv(Platform3DObject program, GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getProgramiv(program, paramName, value);
}

String GraphicsContext3D::getProgramInfoLog(Platform3DObject program)
{
    m_internal->m_glWidget->makeCurrent();

    GLint length = 0;
    m_internal->getProgramiv(program, GraphicsContext3D::INFO_LOG_LENGTH, &length);

    GLsizei size = 0;

    GLchar* info = (GLchar*) fastMalloc(length);
    if (!info)
        return "";

    m_internal->getProgramInfoLog(program, length, &size, info);

    String result(info);
    fastFree(info);

    return result;
}

void GraphicsContext3D::getRenderbufferParameteriv(GC3Denum target, GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getRenderbufferParameteriv(target, paramName, value);
}

void GraphicsContext3D::getShaderiv(Platform3DObject shader, GC3Denum paramName, GC3Dint* value)
{
    ASSERT(shader);
    m_internal->m_glWidget->makeCurrent();
    m_internal->getShaderiv(shader, paramName, value);
}

String GraphicsContext3D::getShaderInfoLog(Platform3DObject shader)
{
    m_internal->m_glWidget->makeCurrent();

    GLint length = 0;
    m_internal->getShaderiv(shader, GraphicsContext3D::INFO_LOG_LENGTH, &length);

    GLsizei size = 0;
    GLchar* info = (GLchar*) fastMalloc(length);
    if (!info)
        return "";

    m_internal->getShaderInfoLog(shader, length, &size, info);

    String result(info);
    fastFree(info);

    return result;
}

String GraphicsContext3D::getShaderSource(Platform3DObject shader)
{
    m_internal->m_glWidget->makeCurrent();

    GLint length = 0;
    m_internal->getShaderiv(shader, GraphicsContext3D::SHADER_SOURCE_LENGTH, &length);

    GLsizei size = 0;
    GLchar* info = (GLchar*) fastMalloc(length);
    if (!info)
        return "";

    m_internal->getShaderSource(shader, length, &size, info);

    String result(info);
    fastFree(info);

    return result;
}

void GraphicsContext3D::getTexParameterfv(GC3Denum target, GC3Denum paramName, GC3Dfloat* value)
{
    m_internal->m_glWidget->makeCurrent();
    glGetTexParameterfv(target, paramName, value);
}

void GraphicsContext3D::getTexParameteriv(GC3Denum target, GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    glGetTexParameteriv(target, paramName, value);
}

void GraphicsContext3D::getUniformfv(Platform3DObject program, GC3Dint location, GC3Dfloat* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getUniformfv(program, location, value);
}

void GraphicsContext3D::getUniformiv(Platform3DObject program, GC3Dint location, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getUniformiv(program, location, value);
}

GC3Dint GraphicsContext3D::getUniformLocation(Platform3DObject program, const String& name)
{
    ASSERT(program);
    
    m_internal->m_glWidget->makeCurrent();
    return m_internal->getUniformLocation(program, name.utf8().data());
}

void GraphicsContext3D::getVertexAttribfv(GC3Duint index, GC3Denum paramName, GC3Dfloat* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getVertexAttribfv(index, paramName, value);
}

void GraphicsContext3D::getVertexAttribiv(GC3Duint index, GC3Denum paramName, GC3Dint* value)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->getVertexAttribiv(index, paramName, value);
}

GC3Dsizeiptr GraphicsContext3D::getVertexAttribOffset(GC3Duint index, GC3Denum paramName)
{
    m_internal->m_glWidget->makeCurrent();
    
    GLvoid* pointer = 0;
    m_internal->getVertexAttribPointerv(index, paramName, &pointer);
    return static_cast<GC3Dsizeiptr>(reinterpret_cast<intptr_t>(pointer));
}

bool GraphicsContext3D::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    m_internal->m_glWidget->makeCurrent();
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    return true;
}

void GraphicsContext3D::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoff, GC3Dint yoff, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels)
{
    m_internal->m_glWidget->makeCurrent();
    glTexSubImage2D(target, level, xoff, yoff, width, height, format, type, pixels);
}

Platform3DObject GraphicsContext3D::createBuffer()
{
    m_internal->m_glWidget->makeCurrent();
    GLuint handle = 0;
    m_internal->genBuffers(/* count */ 1, &handle);
    return handle;
}

Platform3DObject GraphicsContext3D::createFramebuffer()
{
    m_internal->m_glWidget->makeCurrent();
    GLuint handle = 0;
    m_internal->genFramebuffers(/* count */ 1, &handle);
    return handle;
}

Platform3DObject GraphicsContext3D::createProgram()
{
    m_internal->m_glWidget->makeCurrent();
    return m_internal->createProgram();
}

Platform3DObject GraphicsContext3D::createRenderbuffer()
{
    m_internal->m_glWidget->makeCurrent();
    GLuint handle = 0;
    m_internal->genRenderbuffers(/* count */ 1, &handle);
    return handle;
}

Platform3DObject GraphicsContext3D::createShader(GC3Denum type)
{
    m_internal->m_glWidget->makeCurrent();
    return m_internal->createShader(type);
}

Platform3DObject GraphicsContext3D::createTexture()
{
    m_internal->m_glWidget->makeCurrent();
    GLuint handle = 0;
    glGenTextures(1, &handle);
    return handle;
}

void GraphicsContext3D::deleteBuffer(Platform3DObject buffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->deleteBuffers(1, &buffer);
}

void GraphicsContext3D::deleteFramebuffer(Platform3DObject framebuffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->deleteFramebuffers(1, &framebuffer);
}

void GraphicsContext3D::deleteProgram(Platform3DObject program)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->deleteProgram(program);
}

void GraphicsContext3D::deleteRenderbuffer(Platform3DObject renderbuffer)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->deleteRenderbuffers(1, &renderbuffer);
}

void GraphicsContext3D::deleteShader(Platform3DObject shader)
{
    m_internal->m_glWidget->makeCurrent();
    m_internal->deleteShader(shader);
}

void GraphicsContext3D::deleteTexture(Platform3DObject texture)
{
    m_internal->m_glWidget->makeCurrent();
    glDeleteTextures(1, &texture);
}

void GraphicsContext3D::synthesizeGLError(GC3Denum error)
{
    m_internal->m_syntheticErrors.add(error);
}

void GraphicsContext3D::markLayerComposited()
{
    m_internal->m_layerComposited = true;
}

void GraphicsContext3D::markContextChanged()
{
    // FIXME: Any accelerated compositor needs to be told to re-read from here.
    m_internal->m_layerComposited = false;
}

bool GraphicsContext3D::layerComposited() const
{
    return m_internal->m_layerComposited;
}

Extensions3D* GraphicsContext3D::getExtensions()
{
    if (!m_internal->m_extensions)
        m_internal->m_extensions = adoptPtr(new Extensions3DQt);
    return m_internal->m_extensions.get();
}

bool GraphicsContext3D::getImageData(Image* image,
                                     GC3Denum format,
                                     GC3Denum type,
                                     bool premultiplyAlpha,
                                     bool ignoreGammaAndColorProfile,
                                     Vector<uint8_t>& outputVector)
{
    UNUSED_PARAM(ignoreGammaAndColorProfile);
    if (!image)
        return false;
    QImage nativeImage;
    // Is image already loaded? If not, load it.
    if (image->data())
        nativeImage = QImage::fromData(reinterpret_cast<const uchar*>(image->data()->data()), image->data()->size()).convertToFormat(QImage::Format_ARGB32);
    else {
        QPixmap* nativePixmap = image->nativeImageForCurrentFrame();
        nativeImage = nativePixmap->toImage().convertToFormat(QImage::Format_ARGB32);
    }
    AlphaOp neededAlphaOp = AlphaDoNothing;
    if (premultiplyAlpha)
        neededAlphaOp = AlphaDoPremultiply;
    outputVector.resize(nativeImage.byteCount());
    return packPixels(nativeImage.bits(), SourceFormatBGRA8, image->width(), image->height(), 0, format, type, neededAlphaOp, outputVector.data());
}

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<ContextLostCallback>)
{
}

}

#endif // ENABLE(WEBGL)
