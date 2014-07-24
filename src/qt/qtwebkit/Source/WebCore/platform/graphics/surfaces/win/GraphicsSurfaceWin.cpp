/*

 Copyright (C) 2012 Zeno Albisser <zeno@webkit.org>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "GraphicsSurface.h"

#if USE(GRAPHICS_SURFACE)
#include "TextureMapperGL.h"

#define EGL_EGLEXT_PROTOTYPES // This must be defined before including egl.h and eglext.h.
#include <EGL/egl.h>
#include <EGL/eglext.h>

#if PLATFORM(QT)
#include <QGuiApplication>
#include <QOpenGLContext>
#include <qpa/qplatformnativeinterface.h>
#endif

namespace WebCore {

#define STRINGIFY(...) #__VA_ARGS__

static GLuint loadShader(GLenum type, const GLchar *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);
    if (!shader)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, 0);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

struct GraphicsSurfacePrivate {
public:
    GraphicsSurfacePrivate(const IntSize& size, GraphicsSurfaceToken token)
        : m_token(token)
        , m_detachedContext(0)
        , m_detachedReadSurface(0)
        , m_detachedDrawSurface(0)
        , m_size(size)
        , m_eglDisplay(0)
        , m_eglContext(0)
        , m_eglConfig(0)
        , m_eglFrontBufferSurface(0)
        , m_eglBackBufferSurface(0)
        , m_initialFrontBufferShareHandle(token.frontBufferHandle)
        , m_frontBufferShareHandle(token.frontBufferHandle)
        , m_backBufferShareHandle(token.backBufferHandle)
        , m_frontBufferTexture(0)
        , m_shaderProgram(0)
    {
        initializeShaderProgram();
    }


    GraphicsSurfacePrivate(const PlatformGraphicsContext3D shareContext, const IntSize& size, GraphicsSurface::Flags flags)
        : m_detachedContext(0)
        , m_detachedReadSurface(0)
        , m_detachedDrawSurface(0)
        , m_size(size)
        , m_eglDisplay(0)
        , m_eglContext(0)
        , m_eglConfig(0)
        , m_eglFrontBufferSurface(0)
        , m_eglBackBufferSurface(0)
        , m_initialFrontBufferShareHandle(0)
        , m_frontBufferShareHandle(0)
        , m_backBufferShareHandle(0)
        , m_frontBufferTexture(0)
        , m_shaderProgram(0)
    {
        initializeShaderProgram();

        static PFNEGLQUERYSURFACEPOINTERANGLEPROC eglQuerySurfacePointerANGLE = 0;
        if (!eglQuerySurfacePointerANGLE) {
            eglQuerySurfacePointerANGLE = reinterpret_cast<PFNEGLQUERYSURFACEPOINTERANGLEPROC>(eglGetProcAddress("eglQuerySurfacePointerANGLE"));
            if (!eglQuerySurfacePointerANGLE)
                return;
        }

        if (!m_eglDisplay || !m_eglContext || !m_eglConfig) {
            m_eglDisplay = eglGetCurrentDisplay();

#if PLATFORM(QT)
            QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
            m_eglConfig = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext(QByteArrayLiteral("eglConfig"), shareContext));
            EGLContext eglShareContext = static_cast<EGLContext>(nativeInterface->nativeResourceForContext(QByteArrayLiteral("eglContext"), shareContext));
#endif
            EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
            m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, eglShareContext, contextAttributes);
        }

        EGLint attributes[] = {
            EGL_WIDTH, size.width(),
            EGL_HEIGHT, size.height(),
            EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB,
            EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
            EGL_NONE
        };

        m_eglFrontBufferSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, attributes);
        m_eglBackBufferSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, attributes);
        if (m_eglFrontBufferSurface == EGL_NO_SURFACE || m_eglBackBufferSurface == EGL_NO_SURFACE)
            return;

        eglQuerySurfacePointerANGLE(m_eglDisplay, m_eglFrontBufferSurface, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, &m_frontBufferShareHandle);
        eglQuerySurfacePointerANGLE(m_eglDisplay, m_eglBackBufferSurface, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, &m_backBufferShareHandle);

        m_initialFrontBufferShareHandle = m_frontBufferShareHandle;

        m_token = GraphicsSurfaceToken(m_frontBufferShareHandle, m_backBufferShareHandle);
    }

    ~GraphicsSurfacePrivate()
    {
        releaseFrontBufferTexture();

        if (m_eglBackBufferSurface)
            eglDestroySurface(m_eglDisplay, m_eglBackBufferSurface);

        if (m_shaderProgram)
            glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;

        if (m_eglDisplay && m_eglContext)
            eglDestroyContext(m_eglDisplay, m_eglContext);
    }

    void copyFromTexture(uint32_t texture, const IntRect& sourceRect)
    {
        if (!makeCurrent())
            return;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        drawTexture(texture);
        glFinish();
        doneCurrent();
    }

    bool makeCurrent()
    {
        m_detachedContext = eglGetCurrentContext();
        m_detachedReadSurface = eglGetCurrentSurface(EGL_READ);
        m_detachedDrawSurface = eglGetCurrentSurface(EGL_DRAW);

        return eglMakeCurrent(m_eglDisplay, m_eglBackBufferSurface, m_eglBackBufferSurface, m_eglContext);
    }

    bool doneCurrent()
    {
        if (!m_detachedContext || !m_detachedDrawSurface || !m_detachedReadSurface)
            return false;

        bool success = eglMakeCurrent(m_eglDisplay, m_detachedDrawSurface, m_detachedReadSurface, m_detachedContext);
        m_detachedContext = 0;
        m_detachedReadSurface = 0;
        m_detachedDrawSurface = 0;
        return success;
    }

    PlatformGraphicsSurface swapBuffers()
    {
        if (m_frontBufferTexture)
            releaseFrontBufferTexture();
        std::swap(m_eglFrontBufferSurface, m_eglBackBufferSurface);
        std::swap(m_frontBufferShareHandle, m_backBufferShareHandle);

        return m_frontBufferShareHandle;
    }

    GraphicsSurfaceToken token() const
    {
        return m_token;
    }

    uint32_t frontBufferTextureID()
    {
        if (!m_eglFrontBufferSurface) {
            m_eglFrontBufferSurface = createSurfaceFromShareHandle(m_size, m_frontBufferShareHandle);

            glGenTextures(1, &m_frontBufferTexture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_frontBufferTexture);
            eglBindTexImage(m_eglDisplay, m_eglFrontBufferSurface, EGL_BACK_BUFFER);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        return m_frontBufferTexture;
    }

    PlatformGraphicsSurface initialFrontBufferShareHandle() const
    {
        return m_initialFrontBufferShareHandle;
    }

    PlatformGraphicsSurface frontBufferShareHandle() const
    {
        return m_frontBufferShareHandle;
    }

    PlatformGraphicsSurface backBufferShareHandle() const
    {
        return m_backBufferShareHandle;
    }

    void releaseFrontBufferTexture()
    {
        if (m_frontBufferTexture) {
            eglReleaseTexImage(m_eglDisplay, m_eglFrontBufferSurface, EGL_BACK_BUFFER);
            glDeleteTextures(1, &m_frontBufferTexture);
            m_frontBufferTexture = 0;
        }

        if (m_eglFrontBufferSurface)
            eglDestroySurface(m_eglDisplay, m_eglFrontBufferSurface);
        m_eglFrontBufferSurface = 0;
    }

    IntSize size() const
    {
        return m_size;
    }

protected:
    void initializeShaderProgram()
    {
        if (m_shaderProgram)
            return;

        GLchar vShaderStr[] =
            STRINGIFY(
                attribute highp vec2 vertex;
                attribute highp vec2 textureCoordinates;
                varying highp vec2 textureCoords;
                void main(void)
                {
                    gl_Position = vec4(vertex, 0.0, 1.0);
                    textureCoords = textureCoordinates;
                }
            );

        GLchar fShaderStr[] =
            STRINGIFY(
                varying highp vec2 textureCoords;
                uniform sampler2D texture;
                void main(void)
                {
                    highp vec3 color = texture2D(texture, textureCoords).rgb;
                    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
                }
            );

        GLuint vertexShader;
        GLuint fragmentShader;
        GLint linked;

        vertexShader = loadShader(GL_VERTEX_SHADER, vShaderStr);
        fragmentShader = loadShader(GL_FRAGMENT_SHADER, fShaderStr);
        if (!vertexShader || !fragmentShader)
            return;

        m_shaderProgram = glCreateProgram();
        if (!m_shaderProgram)
            return;

        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);

        glLinkProgram(m_shaderProgram);
        glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &linked);
        if (!linked) {
            glDeleteProgram(m_shaderProgram);
            m_shaderProgram = 0;
        }

        m_vertexAttr = glGetAttribLocation(m_shaderProgram, "vertex");
        m_textureCoordinatesAttr = glGetAttribLocation(m_shaderProgram, "textureCoordinates");
        m_textureUniform = glGetAttribLocation(m_shaderProgram, "texture");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    EGLSurface createSurfaceFromShareHandle(const IntSize& size, HANDLE shareHandle)
    {
        if (!m_eglDisplay  || !m_eglConfig) {
            m_eglDisplay = eglGetCurrentDisplay();

#if PLATFORM(QT)
            QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
            m_eglConfig = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext(QByteArrayLiteral("eglConfig"), QOpenGLContext::currentContext()));
#endif
        }

        EGLint attributes[] = {
            EGL_WIDTH, size.width(),
            EGL_HEIGHT, size.height(),
            EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB,
            EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
            EGL_NONE
        };

        EGLSurface surface = eglCreatePbufferFromClientBuffer(m_eglDisplay, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE, reinterpret_cast<EGLClientBuffer>(shareHandle), m_eglConfig, attributes);
        ASSERT(surface);

        return surface;
    }

    void drawTexture(uint32_t texture)
    {
        glUseProgram(m_shaderProgram);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLfloat afVertices[] = {
            -1, -1,
             1, -1,
            -1,  1,
             1,  1
        };
        glVertexAttribPointer(m_vertexAttr, 2, GL_FLOAT, GL_FALSE, 0, afVertices);

        GLfloat aftextureCoordinates[] = {
            0, 1,
            1, 1,
            0, 0,
            1, 0
        };
        glVertexAttribPointer(m_textureCoordinatesAttr, 2, GL_FLOAT, GL_FALSE, 0, aftextureCoordinates);

        glUniform1i(m_textureUniform, 0);

        glEnableVertexAttribArray(m_vertexAttr);
        glEnableVertexAttribArray(m_textureCoordinatesAttr);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(m_vertexAttr);
        glDisableVertexAttribArray(m_textureCoordinatesAttr);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:
    GraphicsSurfaceToken m_token;
    EGLContext m_detachedContext;
    EGLSurface m_detachedReadSurface;
    EGLSurface m_detachedDrawSurface;
    IntSize m_size;
    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
    EGLConfig m_eglConfig;
    EGLSurface m_eglFrontBufferSurface;
    EGLSurface m_eglBackBufferSurface;
    PlatformGraphicsSurface m_initialFrontBufferShareHandle;
    PlatformGraphicsSurface m_frontBufferShareHandle;
    PlatformGraphicsSurface m_backBufferShareHandle;
    GLuint m_frontBufferTexture;
    GLint m_shaderProgram;

    GLuint m_vertexAttr;
    GLuint m_textureCoordinatesAttr;
    GLuint m_textureUniform;
};

GraphicsSurfaceToken GraphicsSurface::platformExport()
{
    return m_private->token();
}

uint32_t GraphicsSurface::platformGetTextureID()
{
    return m_private->frontBufferTextureID();
}

void GraphicsSurface::platformCopyToGLTexture(uint32_t target, uint32_t id, const IntRect& targetRect, const IntPoint& offset)
{
    // Currently not implemented.
}

void GraphicsSurface::platformCopyFromTexture(uint32_t texture, const IntRect& sourceRect)
{
    m_private->copyFromTexture(texture, sourceRect);
}

void GraphicsSurface::platformPaintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    IntSize size = m_private->size();
    FloatRect rectOnContents(FloatPoint::zero(), size);
    TransformationMatrix adjustedTransform = transform;
    adjustedTransform.multiply(TransformationMatrix::rectToRect(rectOnContents, targetRect));
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(platformGetTextureID(), 0, size, rectOnContents, adjustedTransform, opacity);
}

uint32_t GraphicsSurface::platformFrontBuffer() const
{
    if (m_private->initialFrontBufferShareHandle() == m_private->frontBufferShareHandle())
        return 0;
    if (m_private->initialFrontBufferShareHandle() == m_private->backBufferShareHandle())
        return 1;
    return 0xFFFF;
}

uint32_t GraphicsSurface::platformSwapBuffers()
{
    m_private->swapBuffers();
    return platformFrontBuffer();
}

IntSize GraphicsSurface::platformSize() const
{
    return m_private->size();
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(const IntSize& size, Flags flags, const PlatformGraphicsContext3D shareContext)
{
    // Single buffered GraphicsSurface is currently not supported.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate(shareContext, size, flags);

    if (!surface->m_private->frontBufferShareHandle() || !surface->m_private->backBufferShareHandle())
        return PassRefPtr<GraphicsSurface>();

    return surface;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, const GraphicsSurfaceToken& token)
{
    // Single buffered GraphicsSurface is currently not supported.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate(size, token);

    if (!surface->m_private->frontBufferShareHandle() || !surface->m_private->backBufferShareHandle())
        return PassRefPtr<GraphicsSurface>();

    return surface;
}

char* GraphicsSurface::platformLock(const IntRect& rect, int* outputStride, LockOptions lockOptions)
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
    return 0;
}

void GraphicsSurface::platformUnlock()
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
}

void GraphicsSurface::platformDestroy()
{
    delete m_private;
    m_private = 0;
}

}
#endif
