/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QOPENGLCONTEXT_H
#define QOPENGLCONTEXT_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtCore/qnamespace.h>
#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <QtGui/QSurfaceFormat>

#ifdef __GLEW_H__
#if defined(Q_CC_GNU)
#warning qopenglfunctions.h is not compatible with GLEW, GLEW defines will be undefined
#warning To use GLEW with Qt, do not include <qopengl.h> or <QOpenGLFunctions> after glew.h
#endif
#endif

#include <QtGui/qopengl.h>
#include <QtGui/qopenglversionfunctions.h>

#include <QtCore/qhash.h>
#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QOpenGLContextPrivate;
class QOpenGLContextGroupPrivate;
class QOpenGLFunctions;
class QPlatformOpenGLContext;

class QScreen;
class QSurface;

class QOpenGLVersionProfilePrivate;

class Q_GUI_EXPORT QOpenGLVersionProfile
{
public:
    QOpenGLVersionProfile();
    explicit QOpenGLVersionProfile(const QSurfaceFormat &format);
    QOpenGLVersionProfile(const QOpenGLVersionProfile &other);
    ~QOpenGLVersionProfile();

    QOpenGLVersionProfile &operator=(const QOpenGLVersionProfile &rhs);

    QPair<int, int> version() const;
    void setVersion(int majorVersion, int minorVersion);

    QSurfaceFormat::OpenGLContextProfile profile() const;
    void setProfile(QSurfaceFormat::OpenGLContextProfile profile);

    bool hasProfiles() const;
    bool isLegacyVersion() const;
    bool isValid() const;

private:
    QOpenGLVersionProfilePrivate* d;
};

inline uint qHash(const QOpenGLVersionProfile &v, uint seed = 0)
{
    return qHash(static_cast<int>(v.profile() * 1000)
               + v.version().first * 100 + v.version().second * 10, seed);
}

inline bool operator==(const QOpenGLVersionProfile &lhs, const QOpenGLVersionProfile &rhs)
{
    if (lhs.profile() != rhs.profile())
        return false;
    return lhs.version() == rhs.version();
}

inline bool operator!=(const QOpenGLVersionProfile &lhs, const QOpenGLVersionProfile &rhs)
{
    return !operator==(lhs, rhs);
}

class Q_GUI_EXPORT QOpenGLContextGroup : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenGLContextGroup)
public:
    ~QOpenGLContextGroup();

    QList<QOpenGLContext *> shares() const;

    static QOpenGLContextGroup *currentContextGroup();

private:
    QOpenGLContextGroup();

    friend class QOpenGLContext;
    friend class QOpenGLContextGroupResourceBase;
    friend class QOpenGLSharedResource;
    friend class QOpenGLMultiGroupSharedResource;
};


class QOpenGLTextureHelper;

class Q_GUI_EXPORT QOpenGLContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenGLContext)
public:
    explicit QOpenGLContext(QObject *parent = 0);
    ~QOpenGLContext();

    void setFormat(const QSurfaceFormat &format);
    void setShareContext(QOpenGLContext *shareContext);
    void setScreen(QScreen *screen);

    bool create();
    bool isValid() const;

    QSurfaceFormat format() const;
    QOpenGLContext *shareContext() const;
    QOpenGLContextGroup *shareGroup() const;
    QScreen *screen() const;

    GLuint defaultFramebufferObject() const;

    bool makeCurrent(QSurface *surface);
    void doneCurrent();

    void swapBuffers(QSurface *surface);
    QFunctionPointer getProcAddress(const QByteArray &procName) const;

    QSurface *surface() const;

    static QOpenGLContext *currentContext();
    static bool areSharing(QOpenGLContext *first, QOpenGLContext *second);

    QPlatformOpenGLContext *handle() const;
    QPlatformOpenGLContext *shareHandle() const;

    QOpenGLFunctions *functions() const;

    QAbstractOpenGLFunctions *versionFunctions(const QOpenGLVersionProfile &versionProfile = QOpenGLVersionProfile()) const;

    template<class TYPE>
    TYPE *versionFunctions() const
    {
        QOpenGLVersionProfile v = TYPE::versionProfile();
        return static_cast<TYPE*>(versionFunctions(v));
    }

    QSet<QByteArray> extensions() const;
    bool hasExtension(const QByteArray &extension) const;

    static void *openGLModuleHandle();

    enum OpenGLModuleType {
        LibGL,
        LibGLES
    };

    static OpenGLModuleType openGLModuleType();

    bool isOpenGLES() const;

Q_SIGNALS:
    void aboutToBeDestroyed();

private:
    friend class QGLContext;
    friend class QGLPixelBuffer;
    friend class QOpenGLContextResourceBase;
    friend class QOpenGLPaintDevice;
    friend class QOpenGLGlyphTexture;
    friend class QOpenGLTextureGlyphCache;
    friend class QOpenGLEngineShaderManager;
    friend class QOpenGLFramebufferObject;
    friend class QOpenGLFramebufferObjectPrivate;
    friend class QOpenGL2PaintEngineEx;
    friend class QOpenGL2PaintEngineExPrivate;
    friend class QSGDistanceFieldGlyphCache;
    friend class QWidgetPrivate;
    friend class QAbstractOpenGLFunctionsPrivate;
    friend class QOpenGLTexturePrivate;

    void *qGLContextHandle() const;
    void setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *));
    void deleteQGLContext();

    QOpenGLVersionFunctionsBackend* functionsBackend(const QOpenGLVersionStatus &v) const;
    void insertFunctionsBackend(const QOpenGLVersionStatus &v,
                                QOpenGLVersionFunctionsBackend *backend);
    void removeFunctionsBackend(const QOpenGLVersionStatus &v);

    QOpenGLTextureHelper* textureFunctions() const;
    void setTextureFunctions(QOpenGLTextureHelper* textureFuncs);

    void destroy();
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QGUIGLCONTEXT_H
