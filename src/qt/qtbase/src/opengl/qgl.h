/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGL_H
#define QGL_H

#ifndef QT_NO_OPENGL

#include <QtGui/qopengl.h>
#include <QtWidgets/qwidget.h>
#include <QtGui/qpaintengine.h>
#include <QtOpenGL/qglcolormap.h>
#include <QtCore/qmap.h>
#include <QtCore/qscopedpointer.h>

#include <QtGui/QSurfaceFormat>


QT_BEGIN_NAMESPACE




class QPixmap;
class QGLWidgetPrivate;
class QGLContextPrivate;

// Namespace class:
namespace QGL
{
    enum FormatOption {
        DoubleBuffer            = 0x0001,
        DepthBuffer             = 0x0002,
        Rgba                    = 0x0004,
        AlphaChannel            = 0x0008,
        AccumBuffer             = 0x0010,
        StencilBuffer           = 0x0020,
        StereoBuffers           = 0x0040,
        DirectRendering         = 0x0080,
        HasOverlay              = 0x0100,
        SampleBuffers           = 0x0200,
        DeprecatedFunctions     = 0x0400,
        SingleBuffer            = DoubleBuffer    << 16,
        NoDepthBuffer           = DepthBuffer     << 16,
        ColorIndex              = Rgba            << 16,
        NoAlphaChannel          = AlphaChannel    << 16,
        NoAccumBuffer           = AccumBuffer     << 16,
        NoStencilBuffer         = StencilBuffer   << 16,
        NoStereoBuffers         = StereoBuffers   << 16,
        IndirectRendering       = DirectRendering << 16,
        NoOverlay               = HasOverlay      << 16,
        NoSampleBuffers         = SampleBuffers   << 16,
        NoDeprecatedFunctions   = DeprecatedFunctions << 16
    };
    Q_DECLARE_FLAGS(FormatOptions, FormatOption)
}

Q_DECLARE_OPERATORS_FOR_FLAGS(QGL::FormatOptions)

class QGLFormatPrivate;

class Q_OPENGL_EXPORT QGLFormat
{
public:
    QGLFormat();
    QGLFormat(QGL::FormatOptions options, int plane = 0);
    QGLFormat(const QGLFormat &other);
    QGLFormat &operator=(const QGLFormat &other);
    ~QGLFormat();

    void setDepthBufferSize(int size);
    int  depthBufferSize() const;

    void setAccumBufferSize(int size);
    int  accumBufferSize() const;

    void setRedBufferSize(int size);
    int  redBufferSize() const;

    void setGreenBufferSize(int size);
    int  greenBufferSize() const;

    void setBlueBufferSize(int size);
    int  blueBufferSize() const;

    void setAlphaBufferSize(int size);
    int  alphaBufferSize() const;

    void setStencilBufferSize(int size);
    int  stencilBufferSize() const;

    void setSampleBuffers(bool enable);
    bool sampleBuffers() const;

    void setSamples(int numSamples);
    int  samples() const;

    void setSwapInterval(int interval);
    int  swapInterval() const;

    bool doubleBuffer() const;
    void setDoubleBuffer(bool enable);
    bool depth() const;
    void setDepth(bool enable);
    bool rgba() const;
    void setRgba(bool enable);
    bool alpha() const;
    void setAlpha(bool enable);
    bool accum() const;
    void setAccum(bool enable);
    bool stencil() const;
    void setStencil(bool enable);
    bool stereo() const;
    void setStereo(bool enable);
    bool directRendering() const;
    void setDirectRendering(bool enable);
    bool hasOverlay() const;
    void setOverlay(bool enable);

    int plane() const;
    void setPlane(int plane);

    void setOption(QGL::FormatOptions opt);
    bool testOption(QGL::FormatOptions opt) const;

    static QGLFormat defaultFormat();
    static void setDefaultFormat(const QGLFormat& f);

    static QGLFormat defaultOverlayFormat();
    static void setDefaultOverlayFormat(const QGLFormat& f);

    static bool hasOpenGL();
    static bool hasOpenGLOverlays();

    void setVersion(int major, int minor);
    int majorVersion() const;
    int minorVersion() const;

    enum OpenGLContextProfile {
        NoProfile,
        CoreProfile,
        CompatibilityProfile
    };

    void setProfile(OpenGLContextProfile profile);
    OpenGLContextProfile profile() const;

    enum OpenGLVersionFlag {
        OpenGL_Version_None               = 0x00000000,
        OpenGL_Version_1_1                = 0x00000001,
        OpenGL_Version_1_2                = 0x00000002,
        OpenGL_Version_1_3                = 0x00000004,
        OpenGL_Version_1_4                = 0x00000008,
        OpenGL_Version_1_5                = 0x00000010,
        OpenGL_Version_2_0                = 0x00000020,
        OpenGL_Version_2_1                = 0x00000040,
        OpenGL_ES_Common_Version_1_0      = 0x00000080,
        OpenGL_ES_CommonLite_Version_1_0  = 0x00000100,
        OpenGL_ES_Common_Version_1_1      = 0x00000200,
        OpenGL_ES_CommonLite_Version_1_1  = 0x00000400,
        OpenGL_ES_Version_2_0             = 0x00000800,
        OpenGL_Version_3_0                = 0x00001000,
        OpenGL_Version_3_1                = 0x00002000,
        OpenGL_Version_3_2                = 0x00004000,
        OpenGL_Version_3_3                = 0x00008000,
        OpenGL_Version_4_0                = 0x00010000,
        OpenGL_Version_4_1                = 0x00020000,
        OpenGL_Version_4_2                = 0x00040000,
        OpenGL_Version_4_3                = 0x00080000
    };
    Q_DECLARE_FLAGS(OpenGLVersionFlags, OpenGLVersionFlag)

    static OpenGLVersionFlags openGLVersionFlags();

    static QGLFormat fromSurfaceFormat(const QSurfaceFormat &format);
    static QSurfaceFormat toSurfaceFormat(const QGLFormat &format);
private:
    QGLFormatPrivate *d;

    void detach();

    friend Q_OPENGL_EXPORT bool operator==(const QGLFormat&, const QGLFormat&);
    friend Q_OPENGL_EXPORT bool operator!=(const QGLFormat&, const QGLFormat&);
#ifndef QT_NO_DEBUG_STREAM
    friend Q_OPENGL_EXPORT QDebug operator<<(QDebug, const QGLFormat &);
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGLFormat::OpenGLVersionFlags)

Q_OPENGL_EXPORT bool operator==(const QGLFormat&, const QGLFormat&);
Q_OPENGL_EXPORT bool operator!=(const QGLFormat&, const QGLFormat&);

#ifndef QT_NO_DEBUG_STREAM
Q_OPENGL_EXPORT QDebug operator<<(QDebug, const QGLFormat &);
#endif

class QGLFunctions;

class Q_OPENGL_EXPORT QGLContext
{
    Q_DECLARE_PRIVATE(QGLContext)
public:
    QGLContext(const QGLFormat& format, QPaintDevice* device);
    QGLContext(const QGLFormat& format);
    virtual ~QGLContext();

    virtual bool create(const QGLContext* shareContext = 0);
    bool isValid() const;
    bool isSharing() const;
    void reset();

    static bool areSharing(const QGLContext *context1, const QGLContext *context2);

    QGLFormat format() const;
    QGLFormat requestedFormat() const;
    void setFormat(const QGLFormat& format);

    void moveToThread(QThread *thread);

    virtual void makeCurrent();
    virtual void doneCurrent();

    virtual void swapBuffers() const;

    QGLFunctions *functions() const;

    enum BindOption {
        NoBindOption                            = 0x0000,
        InvertedYBindOption                     = 0x0001,
        MipmapBindOption                        = 0x0002,
        PremultipliedAlphaBindOption            = 0x0004,
        LinearFilteringBindOption               = 0x0008,

        MemoryManagedBindOption                 = 0x0010, // internal flag
        CanFlipNativePixmapBindOption           = 0x0020, // internal flag
        TemporarilyCachedBindOption             = 0x0040, // internal flag

        DefaultBindOption                       = LinearFilteringBindOption
                                                  | InvertedYBindOption
                                                  | MipmapBindOption,
        InternalBindOption                      = MemoryManagedBindOption
                                                  | PremultipliedAlphaBindOption
    };
    Q_DECLARE_FLAGS(BindOptions, BindOption)

    GLuint bindTexture(const QImage &image, GLenum target, GLint format,
                       BindOptions options);
    GLuint bindTexture(const QPixmap &pixmap, GLenum target, GLint format,
                       BindOptions options);

    GLuint bindTexture(const QImage &image, GLenum target = GL_TEXTURE_2D,
                       GLint format = GL_RGBA);
    GLuint bindTexture(const QPixmap &pixmap, GLenum target = GL_TEXTURE_2D,
                       GLint format = GL_RGBA);
    GLuint bindTexture(const QString &fileName);

    void deleteTexture(GLuint tx_id);

    void drawTexture(const QRectF &target, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
    void drawTexture(const QPointF &point, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);

    static void setTextureCacheLimit(int size);
    static int textureCacheLimit();

    QFunctionPointer getProcAddress(const QString &proc) const;
    QPaintDevice* device() const;
    QColor overlayTransparentColor() const;

    static const QGLContext* currentContext();

    static QGLContext *fromOpenGLContext(QOpenGLContext *platformContext);
    QOpenGLContext *contextHandle() const;

protected:
    virtual bool chooseContext(const QGLContext* shareContext = 0);

    bool deviceIsPixmap() const;
    bool windowCreated() const;
    void setWindowCreated(bool on);
    bool initialized() const;
    void setInitialized(bool on);

    uint colorIndex(const QColor& c) const;
    void setValid(bool valid);
    void setDevice(QPaintDevice *pDev);

protected:
    static QGLContext* currentCtx;

private:
    QGLContext(QOpenGLContext *windowContext);

    QScopedPointer<QGLContextPrivate> d_ptr;

    friend class QGLPixelBuffer;
    friend class QGLPixelBufferPrivate;
    friend class QGLWidget;
    friend class QGLWidgetPrivate;
    friend class QGLGlyphCache;
    friend class QGL2PaintEngineEx;
    friend class QGL2PaintEngineExPrivate;
    friend class QGLEngineShaderManager;
    friend class QGLTextureGlyphCache;
    friend struct QGLGlyphTexture;
    friend class QGLContextGroup;
    friend class QGLPixmapBlurFilter;
    friend class QGLTexture;
    friend QGLFormat::OpenGLVersionFlags QGLFormat::openGLVersionFlags();
    friend class QGLFramebufferObject;
    friend class QGLFramebufferObjectPrivate;
    friend class QGLFBOGLPaintDevice;
    friend class QGLPaintDevice;
    friend class QGLWidgetGLPaintDevice;
    friend class QX11GLSharedContexts;
    friend class QGLContextResourceBase;
    friend class QSGDistanceFieldGlyphCache;
private:
    Q_DISABLE_COPY(QGLContext)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGLContext::BindOptions)

class Q_OPENGL_EXPORT QGLWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGLWidget)
public:
    explicit QGLWidget(QWidget* parent=0,
                       const QGLWidget* shareWidget = 0, Qt::WindowFlags f=0);
    explicit QGLWidget(QGLContext *context, QWidget* parent=0,
                       const QGLWidget* shareWidget = 0, Qt::WindowFlags f=0);
    explicit QGLWidget(const QGLFormat& format, QWidget* parent=0,
                       const QGLWidget* shareWidget = 0, Qt::WindowFlags f=0);
    ~QGLWidget();

    void qglColor(const QColor& c) const;
    void qglClearColor(const QColor& c) const;

    bool isValid() const;
    bool isSharing() const;

    void makeCurrent();
    void doneCurrent();

    bool doubleBuffer() const;
    void swapBuffers();

    QGLFormat format() const;
    void setFormat(const QGLFormat& format);

    QGLContext* context() const;
    void setContext(QGLContext* context, const QGLContext* shareContext = 0,
                    bool deleteOldContext = true);

    QPixmap renderPixmap(int w = 0, int h = 0, bool useContext = false);
    QImage grabFrameBuffer(bool withAlpha = false);

    void makeOverlayCurrent();
    const QGLContext* overlayContext() const;

    static QImage convertToGLFormat(const QImage& img);

    const QGLColormap & colormap() const;
    void  setColormap(const QGLColormap & map);

    void renderText(int x, int y, const QString & str,
                    const QFont & fnt = QFont());
    void renderText(double x, double y, double z, const QString & str,
                    const QFont & fnt = QFont());
    QPaintEngine *paintEngine() const;

    GLuint bindTexture(const QImage &image, GLenum target, GLint format,
                       QGLContext::BindOptions options);
    GLuint bindTexture(const QPixmap &pixmap, GLenum target, GLint format,
                       QGLContext::BindOptions options);

    GLuint bindTexture(const QImage &image, GLenum target = GL_TEXTURE_2D,
                       GLint format = GL_RGBA);
    GLuint bindTexture(const QPixmap &pixmap, GLenum target = GL_TEXTURE_2D,
                       GLint format = GL_RGBA);

    GLuint bindTexture(const QString &fileName);

    void deleteTexture(GLuint tx_id);

    void drawTexture(const QRectF &target, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
    void drawTexture(const QPointF &point, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);

public Q_SLOTS:
    virtual void updateGL();
    virtual void updateOverlayGL();

protected:
    bool event(QEvent *);
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    virtual void initializeOverlayGL();
    virtual void resizeOverlayGL(int w, int h);
    virtual void paintOverlayGL();

    void setAutoBufferSwap(bool on);
    bool autoBufferSwap() const;

    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

    virtual void glInit();
    virtual void glDraw();

    QGLWidget(QGLWidgetPrivate &dd,
              const QGLFormat &format = QGLFormat(),
              QWidget *parent = 0,
              const QGLWidget* shareWidget = 0,
              Qt::WindowFlags f = 0);
private:
    Q_DISABLE_COPY(QGLWidget)

    friend class QGLDrawable;
    friend class QGLPixelBuffer;
    friend class QGLPixelBufferPrivate;
    friend class QGLContext;
    friend class QGLContextPrivate;
    friend class QGLOverlayWidget;
    friend class QGLPaintDevice;
    friend class QGLWidgetGLPaintDevice;
};


//
// QGLFormat inline functions
//

inline bool QGLFormat::doubleBuffer() const
{
    return testOption(QGL::DoubleBuffer);
}

inline bool QGLFormat::depth() const
{
    return testOption(QGL::DepthBuffer);
}

inline bool QGLFormat::rgba() const
{
    return testOption(QGL::Rgba);
}

inline bool QGLFormat::alpha() const
{
    return testOption(QGL::AlphaChannel);
}

inline bool QGLFormat::accum() const
{
    return testOption(QGL::AccumBuffer);
}

inline bool QGLFormat::stencil() const
{
    return testOption(QGL::StencilBuffer);
}

inline bool QGLFormat::stereo() const
{
    return testOption(QGL::StereoBuffers);
}

inline bool QGLFormat::directRendering() const
{
    return testOption(QGL::DirectRendering);
}

inline bool QGLFormat::hasOverlay() const
{
    return testOption(QGL::HasOverlay);
}

inline bool QGLFormat::sampleBuffers() const
{
    return testOption(QGL::SampleBuffers);
}

QT_END_NAMESPACE

#endif // QT_NO_OPENGL
#endif // QGL_H
