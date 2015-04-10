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

#include "qopenglcustomshaderstage_p.h"
#include "qopenglengineshadermanager_p.h"
#include "qopenglpaintengine_p.h"
#include <private/qpainter_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLCustomShaderStagePrivate
{
public:
    QOpenGLCustomShaderStagePrivate() :
        m_manager(0) {}

    QPointer<QOpenGLEngineShaderManager> m_manager;
    QByteArray              m_source;
};




QOpenGLCustomShaderStage::QOpenGLCustomShaderStage()
    : d_ptr(new QOpenGLCustomShaderStagePrivate)
{
}

QOpenGLCustomShaderStage::~QOpenGLCustomShaderStage()
{
    Q_D(QOpenGLCustomShaderStage);
    if (d->m_manager) {
        d->m_manager->removeCustomStage();
        d->m_manager->sharedShaders->cleanupCustomStage(this);
    }
}

void QOpenGLCustomShaderStage::setUniformsDirty()
{
    Q_D(QOpenGLCustomShaderStage);
    if (d->m_manager)
        d->m_manager->setDirty(); // ### Probably a bit overkill!
}

bool QOpenGLCustomShaderStage::setOnPainter(QPainter* p)
{
    Q_D(QOpenGLCustomShaderStage);
    if (p->paintEngine()->type() != QPaintEngine::OpenGL2) {
        qWarning("QOpenGLCustomShaderStage::setOnPainter() - paint engine not OpenGL2");
        return false;
    }
    if (d->m_manager)
        qWarning("Custom shader is already set on a painter");

    QOpenGL2PaintEngineEx *engine = static_cast<QOpenGL2PaintEngineEx*>(p->paintEngine());
    d->m_manager = QOpenGL2PaintEngineExPrivate::shaderManagerForEngine(engine);
    Q_ASSERT(d->m_manager);

    d->m_manager->setCustomStage(this);
    return true;
}

void QOpenGLCustomShaderStage::removeFromPainter(QPainter* p)
{
    Q_D(QOpenGLCustomShaderStage);
    if (p->paintEngine()->type() != QPaintEngine::OpenGL2)
        return;

    QOpenGL2PaintEngineEx *engine = static_cast<QOpenGL2PaintEngineEx*>(p->paintEngine());
    d->m_manager = QOpenGL2PaintEngineExPrivate::shaderManagerForEngine(engine);
    Q_ASSERT(d->m_manager);

    // Just set the stage to null, don't call removeCustomStage().
    // This should leave the program in a compiled/linked state
    // if the next custom shader stage is this one again.
    d->m_manager->setCustomStage(0);
    d->m_manager = 0;
}

QByteArray QOpenGLCustomShaderStage::source() const
{
    Q_D(const QOpenGLCustomShaderStage);
    return d->m_source;
}

// Called by the shader manager if another custom shader is attached or
// the manager is deleted
void QOpenGLCustomShaderStage::setInactive()
{
    Q_D(QOpenGLCustomShaderStage);
    d->m_manager = 0;
}

void QOpenGLCustomShaderStage::setSource(const QByteArray& s)
{
    Q_D(QOpenGLCustomShaderStage);
    d->m_source = s;
}

QT_END_NAMESPACE
