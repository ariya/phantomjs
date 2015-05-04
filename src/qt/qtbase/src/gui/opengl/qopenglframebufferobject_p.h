/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOPENGLFRAMEBUFFEROBJECT_P_H
#define QOPENGLFRAMEBUFFEROBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qopenglframebufferobject.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObjectFormatPrivate
{
public:
    QOpenGLFramebufferObjectFormatPrivate()
        : ref(1),
          samples(0),
          attachment(QOpenGLFramebufferObject::NoAttachment),
          target(GL_TEXTURE_2D),
          mipmap(false)
    {
#ifndef QT_OPENGL_ES_2
        // There is nothing that says QOpenGLFramebufferObjectFormat needs a current
        // context, so we need a fallback just to be safe, even though in pratice there
        // will usually be a context current.
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        const bool isES = ctx ? ctx->isOpenGLES() : QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL;
        internal_format = isES ? GL_RGBA : GL_RGBA8;
#else
        internal_format = GL_RGBA;
#endif
    }
    QOpenGLFramebufferObjectFormatPrivate
            (const QOpenGLFramebufferObjectFormatPrivate *other)
        : ref(1),
          samples(other->samples),
          attachment(other->attachment),
          target(other->target),
          internal_format(other->internal_format),
          mipmap(other->mipmap)
    {
    }
    bool equals(const QOpenGLFramebufferObjectFormatPrivate *other)
    {
        return samples == other->samples &&
               attachment == other->attachment &&
               target == other->target &&
               internal_format == other->internal_format &&
               mipmap == other->mipmap;
    }

    QAtomicInt ref;
    int samples;
    QOpenGLFramebufferObject::Attachment attachment;
    GLenum target;
    GLenum internal_format;
    uint mipmap : 1;
};

class QOpenGLFramebufferObjectPrivate
{
public:
    QOpenGLFramebufferObjectPrivate() : fbo_guard(0), texture_guard(0), depth_buffer_guard(0)
                                  , stencil_buffer_guard(0), color_buffer_guard(0)
                                  , valid(false) {}
    ~QOpenGLFramebufferObjectPrivate() {}

    void init(QOpenGLFramebufferObject *q, const QSize& sz,
              QOpenGLFramebufferObject::Attachment attachment,
              GLenum internal_format, GLenum texture_target,
              GLint samples = 0, bool mipmap = false);
    void initTexture(GLenum target, GLenum internal_format, const QSize &size, bool mipmap);
    void initAttachments(QOpenGLContext *ctx, QOpenGLFramebufferObject::Attachment attachment);

    bool checkFramebufferStatus(QOpenGLContext *ctx) const;
    QOpenGLSharedResourceGuard *fbo_guard;
    QOpenGLSharedResourceGuard *texture_guard;
    QOpenGLSharedResourceGuard *depth_buffer_guard;
    QOpenGLSharedResourceGuard *stencil_buffer_guard;
    QOpenGLSharedResourceGuard *color_buffer_guard;
    GLenum target;
    QSize size;
    QOpenGLFramebufferObjectFormat format;
    int requestedSamples;
    uint valid : 1;
    QOpenGLFramebufferObject::Attachment fbo_attachment;
    QOpenGLExtensions funcs;

    inline GLuint fbo() const { return fbo_guard ? fbo_guard->id() : 0; }
};


QT_END_NAMESPACE

#endif // QOPENGLFRAMEBUFFEROBJECT_P_H
