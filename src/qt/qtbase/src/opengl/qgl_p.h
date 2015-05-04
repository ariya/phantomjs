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

#ifndef QGL_P_H
#define QGL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QGLWidget class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtOpenGL/qgl.h"
#include "QtOpenGL/qglcolormap.h"
#include "QtCore/qmap.h"
#include "QtCore/qthread.h"
#include "QtCore/qthreadstorage.h"
#include "QtCore/qhash.h"
#include "QtCore/qatomic.h"
#include "QtWidgets/private/qwidget_p.h"
#include "QtGui/private/qopenglcontext_p.h"
#include "QtGui/private/qopenglextensions_p.h"
#include "qcache.h"
#include "qglpaintdevice_p.h"

#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

class QGLContext;
class QGLOverlayWidget;
class QPixmap;
class QOpenGLExtensions;

class QGLFormatPrivate
{
public:
    QGLFormatPrivate()
        : ref(1)
    {
        opts = QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba | QGL::DirectRendering
             | QGL::StencilBuffer | QGL::DeprecatedFunctions;
        pln = 0;
        depthSize = accumSize = stencilSize = redSize = greenSize = blueSize = alphaSize = -1;
        numSamples = -1;
        swapInterval = -1;
        majorVersion = 2;
        minorVersion = 0;
        profile = QGLFormat::NoProfile;
    }
    QGLFormatPrivate(const QGLFormatPrivate *other)
        : ref(1),
          opts(other->opts),
          pln(other->pln),
          depthSize(other->depthSize),
          accumSize(other->accumSize),
          stencilSize(other->stencilSize),
          redSize(other->redSize),
          greenSize(other->greenSize),
          blueSize(other->blueSize),
          alphaSize(other->alphaSize),
          numSamples(other->numSamples),
          swapInterval(other->swapInterval),
          majorVersion(other->majorVersion),
          minorVersion(other->minorVersion),
          profile(other->profile)
    {
    }
    QAtomicInt ref;
    QGL::FormatOptions opts;
    int pln;
    int depthSize;
    int accumSize;
    int stencilSize;
    int redSize;
    int greenSize;
    int blueSize;
    int alphaSize;
    int numSamples;
    int swapInterval;
    int majorVersion;
    int minorVersion;
    QGLFormat::OpenGLContextProfile profile;
};

class Q_OPENGL_EXPORT QGLWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QGLWidget)
public:
    QGLWidgetPrivate() : QWidgetPrivate()
                       , disable_clear_on_painter_begin(false)
                       , parent_changing(false)
    {
    }

    ~QGLWidgetPrivate() {}

    void init(QGLContext *context, const QGLWidget* shareWidget);
    void initContext(QGLContext *context, const QGLWidget* shareWidget);
    bool renderCxPm(QPixmap *pixmap);
    void cleanupColormaps();
    void aboutToDestroy() {
        if (glcx && !parent_changing)
            glcx->reset();
    }

    QGLContext *glcx;
    QGLWidgetGLPaintDevice glDevice;
    bool autoSwap;

    QGLColormap cmap;

    bool disable_clear_on_painter_begin;
    bool parent_changing;
};

// QGLContextPrivate has the responsibility of creating context groups.
// QGLContextPrivate maintains the reference counter and destroys
// context groups when needed.
class QGLContextGroup
{
public:
    ~QGLContextGroup();

    const QGLContext *context() const {return m_context;}
    bool isSharing() const { return m_shares.size() >= 2; }
    QList<const QGLContext *> shares() const { return m_shares; }

    static void addShare(const QGLContext *context, const QGLContext *share);
    static void removeShare(const QGLContext *context);

private:
    QGLContextGroup(const QGLContext *context);

    const QGLContext *m_context; // context group's representative
    QList<const QGLContext *> m_shares;
    QAtomicInt m_refs;

    friend class QGLContext;
    friend class QGLContextPrivate;
    friend class QGLContextGroupResourceBase;
};

// Get the context that resources for "ctx" will transfer to once
// "ctx" is destroyed.  Returns null if nothing is sharing with ctx.
const QGLContext *qt_gl_transfer_context(const QGLContext *);

/*
    QGLTemporaryContext - the main objective of this class is to have a way of
    creating a GL context and making it current, without going via QGLWidget
    and friends. At certain points during GL initialization we need a current
    context in order decide what GL features are available, and to resolve GL
    extensions. Having a light-weight way of creating such a context saves
    initial application startup time, and it doesn't wind up creating recursive
    conflicts.
    The class currently uses a private d pointer to hide the platform specific
    types. This could possibly been done inline with #ifdef'ery, but it causes
    major headaches on e.g. X11 due to namespace pollution.
*/
class QGLTemporaryContextPrivate;
class QGLTemporaryContext {
public:
    explicit QGLTemporaryContext(bool directRendering = true, QWidget *parent = 0);
    ~QGLTemporaryContext();

private:
    QScopedPointer<QGLTemporaryContextPrivate> d;
};

class QGLTexture;
class QGLTextureDestroyer;

// This probably needs to grow to GL_MAX_VERTEX_ATTRIBS, but 3 is ok for now as that's
// all the GL2 engine uses:
#define QT_GL_VERTEX_ARRAY_TRACKED_COUNT 3

class QGLContextResourceBase;

class QGLContextPrivate
{
    Q_DECLARE_PUBLIC(QGLContext)
public:
    explicit QGLContextPrivate(QGLContext *context);
    ~QGLContextPrivate();
    QGLTexture *bindTexture(const QImage &image, GLenum target, GLint format,
                            QGLContext::BindOptions options);
    QGLTexture *bindTexture(const QImage &image, GLenum target, GLint format, const qint64 key,
                            QGLContext::BindOptions options);
    QGLTexture *bindTexture(const QPixmap &pixmap, GLenum target, GLint format,
                            QGLContext::BindOptions options);
    QGLTexture *textureCacheLookup(const qint64 key, GLenum target);
    void init(QPaintDevice *dev, const QGLFormat &format);
    int maxTextureSize();

    void cleanup();

    void setVertexAttribArrayEnabled(int arrayIndex, bool enabled = true);
    void syncGlState(); // Makes sure the GL context's state is what we think it is
    void swapRegion(const QRegion &region);

    QOpenGLContext *guiGlContext;
    // true if QGLContext owns the QOpenGLContext (for who deletes who)
    bool ownContext;

    void setupSharing();
    void refreshCurrentFbo();
    void setCurrentFbo(GLuint fbo);

    QGLFormat glFormat;
    QGLFormat reqFormat;
    GLuint fbo;

    uint valid : 1;
    uint sharing : 1;
    uint initDone : 1;
    uint crWin : 1;
    uint internal_context : 1;
    uint version_flags_cached : 1;

    // workarounds for driver/hw bugs on different platforms
    uint workaround_needsFullClearOnEveryFrame : 1;
    uint workaround_brokenFBOReadBack : 1;
    uint workaround_brokenTexSubImage : 1;
    uint workaroundsCached : 1;

    uint workaround_brokenTextureFromPixmap : 1;
    uint workaround_brokenTextureFromPixmap_init : 1;

    uint workaround_brokenAlphaTexSubImage : 1;
    uint workaround_brokenAlphaTexSubImage_init : 1;

    QPaintDevice *paintDevice;
    QSize readback_target_size;
    QColor transpColor;
    QGLContext *q_ptr;
    QGLFormat::OpenGLVersionFlags version_flags;

    QGLContextGroup *group;
    GLint max_texture_size;

    GLuint current_fbo;
    GLuint default_fbo;
    QPaintEngine *active_engine;
    QGLTextureDestroyer *texture_destroyer;

    QGLFunctions *functions;

    bool vertexAttributeArraysEnabledState[QT_GL_VERTEX_ARRAY_TRACKED_COUNT];

    static inline QGLContextGroup *contextGroup(const QGLContext *ctx) { return ctx->d_ptr->group; }

    static void setCurrentContext(QGLContext *context);
};

// Temporarily make a context current if not already current or
// shared with the current contex.  The previous context is made
// current when the object goes out of scope.
class Q_OPENGL_EXPORT QGLShareContextScope
{
public:
    QGLShareContextScope(const QGLContext *ctx)
        : m_oldContext(0)
    {
        QGLContext *currentContext = const_cast<QGLContext *>(QGLContext::currentContext());
        if (currentContext != ctx && !QGLContext::areSharing(ctx, currentContext)) {
            m_oldContext = currentContext;
            m_ctx = const_cast<QGLContext *>(ctx);
            m_ctx->makeCurrent();
        } else {
            m_ctx = currentContext;
        }
    }

    operator QGLContext *()
    {
        return m_ctx;
    }

    QGLContext *operator->()
    {
        return m_ctx;
    }

    ~QGLShareContextScope()
    {
        if (m_oldContext)
            m_oldContext->makeCurrent();
    }

private:
    QGLContext *m_oldContext;
    QGLContext *m_ctx;
};

QT_END_NAMESPACE
Q_DECLARE_METATYPE(GLuint)
QT_BEGIN_NAMESPACE

class Q_OPENGL_EXPORT QGLTextureDestroyer
{
public:
    void emitFreeTexture(QGLContext *context, QPlatformPixmap *, GLuint id) {
        if (context->contextHandle())
            (new QOpenGLSharedResourceGuard(context->contextHandle(), id, freeTextureFunc))->free();
    }

private:
    static void freeTextureFunc(QOpenGLFunctions *, GLuint id) {
        QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &id);
    }
};

class Q_OPENGL_EXPORT QGLSignalProxy : public QObject
{
    Q_OBJECT
public:
    void emitAboutToDestroyContext(const QGLContext *context) {
        emit aboutToDestroyContext(context);
    }
    static QGLSignalProxy *instance();
Q_SIGNALS:
    void aboutToDestroyContext(const QGLContext *context);
};

class QGLTexture {
public:
    explicit QGLTexture(QGLContext *ctx = 0, GLuint tx_id = 0, GLenum tx_target = GL_TEXTURE_2D,
               QGLContext::BindOptions opt = QGLContext::DefaultBindOption)
        : context(ctx),
          id(tx_id),
          target(tx_target),
          options(opt)
    {}

    ~QGLTexture() {
        if (options & QGLContext::MemoryManagedBindOption) {
            Q_ASSERT(context);
            QPlatformPixmap *boundPixmap = 0;
            context->d_ptr->texture_destroyer->emitFreeTexture(context, boundPixmap, id);
        }
     }

    QGLContext *context;
    GLuint id;
    GLenum target;

    QGLContext::BindOptions options;

    bool canBindCompressedTexture
        (const char *buf, int len, const char *format, bool *hasAlpha);
    QSize bindCompressedTexture
        (const QString& fileName, const char *format = 0);
    QSize bindCompressedTexture
        (const char *buf, int len, const char *format = 0);
    QSize bindCompressedTextureDDS(const char *buf, int len);
    QSize bindCompressedTexturePVR(const char *buf, int len);
};

struct QGLTextureCacheKey {
    qint64 key;
    QGLContextGroup *group;
};

inline bool operator==(const QGLTextureCacheKey &a, const QGLTextureCacheKey &b)
{
    return a.key == b.key && a.group == b.group;
}

inline uint qHash(const QGLTextureCacheKey &key)
{
    return qHash(key.key) ^ qHash(key.group);
}


class Q_AUTOTEST_EXPORT QGLTextureCache {
public:
    QGLTextureCache();
    ~QGLTextureCache();

    void insert(QGLContext *ctx, qint64 key, QGLTexture *texture, int cost);
    void remove(qint64 key);
    inline int size();
    inline void setMaxCost(int newMax);
    inline int maxCost();
    inline QGLTexture* getTexture(QGLContext *ctx, qint64 key);

    bool remove(QGLContext *ctx, GLuint textureId);
    void removeContextTextures(QGLContext *ctx);
    static QGLTextureCache *instance();
    static void cleanupTexturesForCacheKey(qint64 cacheKey);
    static void cleanupTexturesForPixampData(QPlatformPixmap* pixmap);
    static void cleanupBeforePixmapDestruction(QPlatformPixmap* pixmap);

private:
    QCache<QGLTextureCacheKey, QGLTexture> m_cache;
    QReadWriteLock m_lock;
};

int QGLTextureCache::size() {
    QReadLocker locker(&m_lock);
    return m_cache.size();
}

void QGLTextureCache::setMaxCost(int newMax)
{
    QWriteLocker locker(&m_lock);
    m_cache.setMaxCost(newMax);
}

int QGLTextureCache::maxCost()
{
    QReadLocker locker(&m_lock);
    return m_cache.maxCost();
}

QGLTexture* QGLTextureCache::getTexture(QGLContext *ctx, qint64 key)
{
    // Can't be a QReadLocker since QCache::object() modifies the cache (reprioritizes the object)
    QWriteLocker locker(&m_lock);
    const QGLTextureCacheKey cacheKey = {key, QGLContextPrivate::contextGroup(ctx)};
    return m_cache.object(cacheKey);
}

Q_OPENGL_EXPORT extern QPaintEngine* qt_qgl_paint_engine();

QOpenGLExtensions* qgl_extensions();
bool qgl_hasFeature(QOpenGLFunctions::OpenGLFeature feature);
bool qgl_hasExtension(QOpenGLExtensions::OpenGLExtension extension);

// Put a guard around a GL object identifier and its context.
// When the context goes away, a shared context will be used
// in its place.  If there are no more shared contexts, then
// the identifier is returned as zero - it is assumed that the
// context destruction cleaned up the identifier in this case.
class QGLSharedResourceGuardBase : public QOpenGLSharedResource
{
public:
    QGLSharedResourceGuardBase(QGLContext *context, GLuint id)
        : QOpenGLSharedResource(context->contextHandle()->shareGroup())
        , m_id(id)
    {
    }

    GLuint id() const
    {
        return m_id;
    }

protected:
    void invalidateResource()
    {
        m_id = 0;
    }

    void freeResource(QOpenGLContext *context)
    {
        if (m_id) {
            freeResource(QGLContext::fromOpenGLContext(context), m_id);
        }
    }

    virtual void freeResource(QGLContext *ctx, GLuint id) = 0;

private:
    GLuint m_id;
};

template <typename Func>
class QGLSharedResourceGuard : public QGLSharedResourceGuardBase
{
public:
    QGLSharedResourceGuard(QGLContext *context, GLuint id, Func func)
        : QGLSharedResourceGuardBase(context, id)
        , m_func(func)
    {
    }

protected:
    void freeResource(QGLContext *ctx, GLuint id)
    {
        m_func(ctx, id);
    }

private:
    Func m_func;
};

template <typename Func>
QGLSharedResourceGuardBase *createSharedResourceGuard(QGLContext *context, GLuint id, Func cleanupFunc)
{
    return new QGLSharedResourceGuard<Func>(context, id, cleanupFunc);
}

// this is a class that wraps a QThreadStorage object for storing
// thread local instances of the GL 1 and GL 2 paint engines

template <class T>
class QGLEngineThreadStorage
{
public:
    QPaintEngine *engine() {
        QPaintEngine *&localEngine = storage.localData();
        if (!localEngine)
            localEngine = new T;
        return localEngine;
    }

private:
    QThreadStorage<QPaintEngine *> storage;
};
QT_END_NAMESPACE

#endif // QGL_P_H
