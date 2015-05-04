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

#ifndef QOPENGLTEXTURECACHE_P_H
#define QOPENGLTEXTURECACHE_P_H

#include <QHash>
#include <QObject>
#include <QCache>
#include <private/qopenglcontext_p.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

class QOpenGLCachedTexture
{
public:
    QOpenGLCachedTexture(GLuint id, int options, QOpenGLContext *context);
    ~QOpenGLCachedTexture() { m_resource->free(); }

    GLuint id() const { return m_resource->id(); }
    int options() const { return m_options; }

private:
    QOpenGLSharedResourceGuard *m_resource;
    int m_options;
};

class Q_GUI_EXPORT QOpenGLTextureCache : public QOpenGLSharedResource
{
public:
    static QOpenGLTextureCache *cacheForContext(QOpenGLContext *context);

    QOpenGLTextureCache(QOpenGLContext *);
    ~QOpenGLTextureCache();

    enum BindOption {
        NoBindOption                            = 0x0000,
        PremultipliedAlphaBindOption            = 0x0001,
    };
    Q_DECLARE_FLAGS(BindOptions, BindOption)

    GLuint bindTexture(QOpenGLContext *context, const QPixmap &pixmap, QOpenGLTextureCache::BindOptions options = PremultipliedAlphaBindOption);
    GLuint bindTexture(QOpenGLContext *context, const QImage &image, QOpenGLTextureCache::BindOptions options = PremultipliedAlphaBindOption);

    void invalidate(qint64 key);

    void invalidateResource();
    void freeResource(QOpenGLContext *ctx);

private:
    GLuint bindTexture(QOpenGLContext *context, qint64 key, const QImage &image, QOpenGLTextureCache::BindOptions options);

    QMutex m_mutex;
    QCache<quint64, QOpenGLCachedTexture> m_cache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLTextureCache::BindOptions)

QT_END_NAMESPACE

#endif

