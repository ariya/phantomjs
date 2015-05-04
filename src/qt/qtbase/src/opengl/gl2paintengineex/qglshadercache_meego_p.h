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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QGLSHADERCACHE_MEEGO_P_H
#define QGLSHADERCACHE_MEEGO_P_H

#include <QtCore/qglobal.h>

#if defined(QT_MEEGO_EXPERIMENTAL_SHADERCACHE) && defined(QT_OPENGL_ES_2)

#include <QtCore/qcryptographichash.h>
#include <QtCore/qsharedmemory.h>
#include <QtCore/qsystemsemaphore.h>

#ifndef QT_BOOTSTRAPPED
#  include <GLES2/gl2ext.h>
#endif
#if defined(QT_DEBUG) || defined(QT_MEEGO_EXPERIMENTAL_SHADERCACHE_TRACE)
#  include <syslog.h>
#endif

/*
    This cache stores internal Qt shader programs in shared memory.

    This header file is ugly on purpose and can only be included once. It is only to be used
    for the internal shader cache, not as a generic cache for anyone's shaders.

    The cache stores either ShaderCacheMaxEntries shader programs or ShaderCacheDataSize kilobytes
    of shader programs, whatever limit is reached first.

    The layout of the cache is as outlined in the CachedShaders struct. After some
    integers, an array of headers is reserved, then comes the space for the actual binaries.

    Shader Programs are identified by the md5sum of their frag and vertex shader source code.

    Shader Programs are never removed. The cache never shrinks or re-shuffles. This is done
    on purpose to ensure minimum amount of locking, no alignment problems and very few write
    operations.

    Note: Locking the shader cache could be expensive, because the entire system might hang.
    That's why the cache is immutable to minimize the time we need to keep it locked.

    Why is it Meego specific?

    First, the size is chosen so that it fits to generic meego usage. Second, on Meego, there's
    always at least one Qt application active (the launcher), so the cache will never be destroyed.
    Only when the last Qt app exits, the cache dies, which should only be when someone kills the
    X11 server. And last but not least it was only tested with Meego's SGX driver.

    There's a small tool in src/opengl/util/meego that dumps the contents of the cache.
 */

// anonymous namespace, prevent exporting of the private symbols
namespace
{

struct CachedShaderHeader
{
    /* the index in the data[] member of CachedShaders */
    int index;
    /* the size of the binary shader */
    GLsizei size;
    /* the format of the binary shader */
    GLenum format;
    /* the md5sum of the frag+vertex shaders */
    char md5Sum[16];
};

enum
{
    /* The maximum amount of shader programs the cache can hold */
    ShaderCacheMaxEntries = 20
};

typedef CachedShaderHeader CachedShaderHeaders[ShaderCacheMaxEntries];

enum
{
    // ShaderCacheDataSize is 20k minus the other data members of CachedShaders
    ShaderCacheDataSize = 1024 * ShaderCacheMaxEntries - sizeof(CachedShaderHeaders) - 2 * sizeof(int)
};

struct CachedShaders
{
    /* How much space is still available in the cache */
    inline int availableSize() const { return ShaderCacheDataSize - dataSize; }

    /* The current amount of cached shaders */
    int shaderCount;

    /* The current amount (in bytes) of cached data */
    int dataSize;

    /* The headers describing the shaders */
    CachedShaderHeaders headers;

    /* The actual binary data of the shader programs */
    char data[ShaderCacheDataSize];
};

//#define QT_DEBUG_SHADER_CACHE
#ifdef QT_DEBUG_SHADER_CACHE
static QDebug shaderCacheDebug()
{
    return QDebug(QtDebugMsg);
}
#else
static inline QNoDebug shaderCacheDebug() { return QNoDebug(); }
#endif

class ShaderCacheSharedMemory
{
public:
    ShaderCacheSharedMemory()
        : shm(QLatin1String("qt_gles2_shadercache_" QT_VERSION_STR))
    {
        // we need a system semaphore here, since cache creation and initialization must be atomic
        QSystemSemaphore attachSemaphore(QLatin1String("qt_gles2_shadercache_mutex_" QT_VERSION_STR), 1);

        if (!attachSemaphore.acquire()) {
            shaderCacheDebug() << "Unable to require shader cache semaphore:" << attachSemaphore.errorString();
            return;
        }

        if (shm.attach()) {
            // success!
            shaderCacheDebug() << "Attached to shader cache";
        } else {

            // no cache exists - create and initialize it
            if (shm.create(sizeof(CachedShaders))) {
                shaderCacheDebug() << "Created new shader cache";
                initializeCache();
            } else {
                shaderCacheDebug() << "Unable to create shader cache:" << shm.errorString();
            }
        }

        attachSemaphore.release();
    }

    inline bool isAttached() const { return shm.isAttached(); }

    inline bool lock() { return shm.lock(); }
    inline bool unlock() { return shm.unlock(); }
    inline void *data() { return shm.data(); }
    inline QString errorString() { return shm.errorString(); }

    ~ShaderCacheSharedMemory()
    {
        if (!shm.detach())
            shaderCacheDebug() << "Unable to detach shader cache" << shm.errorString();
    }

private:
    void initializeCache()
    {
        // no need to lock the shared memory since we're already protected by the
        // attach system semaphore.

        void *data = shm.data();
        Q_ASSERT(data);

        memset(data, 0, sizeof(CachedShaders));
    }

    QSharedMemory shm;
};

class ShaderCacheLocker
{
public:
    inline ShaderCacheLocker(ShaderCacheSharedMemory *cache)
        : shm(cache->lock() ? cache : (ShaderCacheSharedMemory *)0)
    {
        if (!shm)
            shaderCacheDebug() << "Unable to lock shader cache" << cache->errorString();
    }

    inline bool isLocked() const { return shm; }

    inline ~ShaderCacheLocker()
    {
        if (!shm)
            return;
        if (!shm->unlock())
            shaderCacheDebug() << "Unable to unlock shader cache" << shm->errorString();
    }

private:
    ShaderCacheSharedMemory *shm;
};

#ifdef QT_BOOTSTRAPPED
} // end namespace
#else

static void traceCacheOverflow(const char *message)
{
#if defined(QT_DEBUG) || defined (QT_MEEGO_EXPERIMENTAL_SHADERCACHE_TRACE)
    openlog(qPrintable(QCoreApplication::applicationName()), LOG_PID | LOG_ODELAY, LOG_USER);
    syslog(LOG_DEBUG, message);
    closelog();
#endif
    shaderCacheDebug() << message;
}

Q_GLOBAL_STATIC(ShaderCacheSharedMemory, shaderCacheSharedMemory)

/*
   Finds the index of the shader program identified by md5Sum in the cache.
   Note: Does NOT lock the cache for reading, the cache must already be locked!

   Returns -1 when no shader was found.
 */
static int qt_cache_index_unlocked(const QByteArray &md5Sum, CachedShaders *cache)
{
    for (int i = 0; i < cache->shaderCount; ++i) {
        if (qstrncmp(md5Sum.constData(), cache->headers[i].md5Sum, 16) == 0) {
            return i;
        }
    }
    return -1;
}

/* Returns the index of the shader identified by md5Sum */
static int qt_cache_index(const QByteArray &md5Sum)
{
    ShaderCacheSharedMemory *shm = shaderCacheSharedMemory();
    if (!shm || !shm->isAttached())
        return false;

    Q_ASSERT(md5Sum.length() == 16);

    ShaderCacheLocker locker(shm);
    if (!locker.isLocked())
        return false;

    void *data = shm->data();
    Q_ASSERT(data);

    CachedShaders *cache = reinterpret_cast<CachedShaders *>(data);

    return qt_cache_index_unlocked(md5Sum, cache);
}

/* Loads the cached shader at index \a shaderIndex into \a program
 * Note: Since the cache is immutable, this operation doesn't lock the shared memory.
 */
static bool qt_cached_shader(QGLShaderProgram *program, const QGLContext *ctx, int shaderIndex)
{
    Q_ASSERT(shaderIndex >= 0 && shaderIndex <= ShaderCacheMaxEntries);
    Q_ASSERT(program);

    ShaderCacheSharedMemory *shm = shaderCacheSharedMemory();
    if (!shm || !shm->isAttached())
        return false;

    void *data = shm->data();
    Q_ASSERT(data);

    CachedShaders *cache = reinterpret_cast<CachedShaders *>(data);

    shaderCacheDebug() << "fetching cached shader at index" << shaderIndex
                 << "dataIndex" << cache->headers[shaderIndex].index
                 << "size" << cache->headers[shaderIndex].size
                 << "format" << cache->headers[shaderIndex].format;

    // call program->programId first, since that resolves the glProgramBinaryOES symbol
    GLuint programId = program->programId();
    glProgramBinaryOES(programId, cache->headers[shaderIndex].format,
                       cache->data + cache->headers[shaderIndex].index,
                       cache->headers[shaderIndex].size);

    return true;
}

/* Stores the shader program in the cache. Returns false if there's an error with the cache, or
   if the cache is too small to hold the shader. */
static bool qt_cache_shader(const QGLShaderProgram *shader, const QGLContext *ctx, const QByteArray &md5Sum)
{
    ShaderCacheSharedMemory *shm = shaderCacheSharedMemory();
    if (!shm || !shm->isAttached())
        return false;

    void *data = shm->data();
    Q_ASSERT(data);

    CachedShaders *cache = reinterpret_cast<CachedShaders *>(data);

    ShaderCacheLocker locker(shm);
    if (!locker.isLocked())
        return false;

    int cacheIdx = cache->shaderCount;
    if (cacheIdx >= ShaderCacheMaxEntries) {
        traceCacheOverflow("Qt OpenGL shader cache index overflow!");
        return false;
    }

    // now that we have the lock on the shared memory, make sure no one
    // inserted the shader already while we were unlocked
    if (qt_cache_index_unlocked(md5Sum, cache) != -1)
        return true; // already cached

    shaderCacheDebug() << "Caching shader at index" << cacheIdx;

    GLint binaryLength = 0;
    glGetProgramiv(shader->programId(), GL_PROGRAM_BINARY_LENGTH_OES, &binaryLength);

    if (!binaryLength) {
        shaderCacheDebug() << "Unable to determine binary shader size!";
        return false;
    }

    if (binaryLength > cache->availableSize()) {
        traceCacheOverflow("Qt OpenGL shader cache data overflow!");
        return false;
    }

    GLsizei size = 0;
    GLenum format = 0;
    glGetProgramBinaryOES(shader->programId(), binaryLength, &size, &format,
            cache->data + cache->dataSize);

    if (!size) {
        shaderCacheDebug() << "Unable to get binary shader!";
        return false;
    }

    cache->headers[cacheIdx].index = cache->dataSize;
    cache->dataSize += binaryLength;
    ++cache->shaderCount;
    cache->headers[cacheIdx].size = binaryLength;
    cache->headers[cacheIdx].format = format;

    memcpy(cache->headers[cacheIdx].md5Sum, md5Sum.constData(), 16);

    shaderCacheDebug() << "cached shader size" << size
                       << "format" << format
                       << "binarySize" << binaryLength
                       << "cache index" << cacheIdx
                       << "data index" << cache->headers[cacheIdx].index;

    return true;
}

} // namespace

QT_BEGIN_NAMESPACE


class CachedShader
{
public:
    CachedShader(const QByteArray &fragSource, const QByteArray &vertexSource)
        : cacheIdx(-1)
    {
        QCryptographicHash md5Hash(QCryptographicHash::Md5);

        md5Hash.addData(fragSource);
        md5Hash.addData(vertexSource);

        md5Sum = md5Hash.result();
    }

    bool isCached()
    {
        return cacheIndex() != -1;
    }

    int cacheIndex()
    {
        if (cacheIdx != -1)
            return cacheIdx;
        cacheIdx = qt_cache_index(md5Sum);
        return cacheIdx;
    }

    bool load(QGLShaderProgram *program, const QGLContext *ctx)
    {
        if (cacheIndex() == -1)
            return false;
        return qt_cached_shader(program, ctx, cacheIdx);
    }

    bool store(QGLShaderProgram *program, const QGLContext *ctx)
    {
        return qt_cache_shader(program, ctx, md5Sum);
    }

private:
    QByteArray md5Sum;
    int cacheIdx;
};


QT_END_NAMESPACE

#endif

#endif
#endif
