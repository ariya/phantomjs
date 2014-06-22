/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qopengltextureblitter_p.h"

#include <QtGui/QOpenGLBuffer>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

QT_BEGIN_NAMESPACE

static const char vertex_shader150[] =
    "#version 150 core\n"
    "in vec3 vertexCoord;"
    "in vec2 textureCoord;"
    "out vec2 uv;"
    "uniform mat4 vertexTransform;"
    "uniform mat3 textureTransform;"
    "void main() {"
    "   uv = (textureTransform * vec3(textureCoord,1.0)).xy;"
    "   gl_Position = vertexTransform * vec4(vertexCoord,1.0);"
    "}";

static const char fragment_shader150[] =
    "#version 150 core\n"
    "in vec2 uv;"
    "out vec4 fragcolor;"
    "uniform sampler2D textureSampler;"
    "uniform bool swizzle;"
    "void main() {"
    "   vec4 tmpFragColor = texture(textureSampler, uv);"
    "   fragcolor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const char vertex_shader[] =
    "attribute highp vec3 vertexCoord;"
    "attribute highp vec2 textureCoord;"
    "varying highp vec2 uv;"
    "uniform highp mat4 vertexTransform;"
    "uniform highp mat3 textureTransform;"
    "void main() {"
    "   uv = (textureTransform * vec3(textureCoord,1.0)).xy;"
    "   gl_Position = vertexTransform * vec4(vertexCoord,1.0);"
    "}";

static const char fragment_shader[] =
    "varying highp vec2 uv;"
    "uniform sampler2D textureSampler;"
    "uniform bool swizzle;"
    "void main() {"
    "   highp vec4 tmpFragColor = texture2D(textureSampler,uv);"
    "   gl_FragColor = swizzle ? tmpFragColor.bgra : tmpFragColor;"
    "}";

static const GLfloat vertex_buffer_data[] = {
        -1,-1, 0,
        -1, 1, 0,
         1,-1, 0,
        -1, 1, 0,
         1,-1, 0,
         1, 1, 0
};

static const GLfloat texture_buffer_data[] = {
        0, 0,
        0, 1,
        1, 0,
        0, 1,
        1, 0,
        1, 1
};

class TextureBinder
{
public:
    TextureBinder(GLuint textureId)
    {
        QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, textureId);
    }
    ~TextureBinder()
    {
        QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, 0);
    }
};

class QOpenGLTextureBlitterPrivate
{
public:
    enum TextureMatrixUniform {
        User,
        Identity,
        IdentityFlipped
    };

    QOpenGLTextureBlitterPrivate()
        : program(0)
        , vertexCoordAttribPos(0)
        , vertexTransformUniformPos(0)
        , textureCoordAttribPos(0)
        , textureTransformUniformPos(0)
        , swizzle(false)
        , swizzleOld(false)
        , textureMatrixUniformState(User)
        , vao(new QOpenGLVertexArrayObject())
    { }

    void blit(GLuint texture, const QMatrix4x4 &vertexTransform, const QMatrix3x3 &textureTransform);
    void blit(GLuint texture, const QMatrix4x4 &vertexTransform, QOpenGLTextureBlitter::Origin origin);

    void prepareProgram(const QMatrix4x4 &vertexTransform)
    {
        vertexBuffer.bind();
        program->setAttributeBuffer(vertexCoordAttribPos, GL_FLOAT, 0, 3, 0);
        program->enableAttributeArray(vertexCoordAttribPos);
        vertexBuffer.release();

        program->setUniformValue(vertexTransformUniformPos, vertexTransform);

        textureBuffer.bind();
        program->setAttributeBuffer(textureCoordAttribPos, GL_FLOAT, 0, 2, 0);
        program->enableAttributeArray(textureCoordAttribPos);
        textureBuffer.release();

        if (swizzle != swizzleOld) {
            program->setUniformValue(swizzleUniformPos, swizzle);
            swizzleOld = swizzle;
        }
    }

    QOpenGLBuffer vertexBuffer;
    QOpenGLBuffer textureBuffer;
    QScopedPointer<QOpenGLShaderProgram> program;
    GLuint vertexCoordAttribPos;
    GLuint vertexTransformUniformPos;
    GLuint textureCoordAttribPos;
    GLuint textureTransformUniformPos;
    GLuint swizzleUniformPos;
    bool swizzle;
    bool swizzleOld;
    TextureMatrixUniform textureMatrixUniformState;
    QScopedPointer<QOpenGLVertexArrayObject> vao;
};

void QOpenGLTextureBlitterPrivate::blit(GLuint texture,
                                        const QMatrix4x4 &vertexTransform,
                                        const QMatrix3x3 &textureTransform)
{
    TextureBinder binder(texture);
    prepareProgram(vertexTransform);

    program->setUniformValue(textureTransformUniformPos, textureTransform);
    textureMatrixUniformState = User;

    QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLES, 0, 6);
}

void QOpenGLTextureBlitterPrivate::blit(GLuint texture,
                                        const QMatrix4x4 &vertexTransform,
                                        QOpenGLTextureBlitter::Origin origin)
{
    TextureBinder binder(texture);
    prepareProgram(vertexTransform);

    if (origin == QOpenGLTextureBlitter::OriginTopLeft) {
        if (textureMatrixUniformState != IdentityFlipped) {
            QMatrix3x3 flipped;
            flipped(1,1) = -1;
            flipped(1,2) = 1;
            program->setUniformValue(textureTransformUniformPos, flipped);
            textureMatrixUniformState = IdentityFlipped;
        }
    } else if (textureMatrixUniformState != Identity) {
        program->setUniformValue(textureTransformUniformPos, QMatrix3x3());
        textureMatrixUniformState = Identity;
    }

    QOpenGLContext::currentContext()->functions()->glDrawArrays(GL_TRIANGLES, 0, 6);
}

QOpenGLTextureBlitter::QOpenGLTextureBlitter()
    : d_ptr(new QOpenGLTextureBlitterPrivate)
{
}

QOpenGLTextureBlitter::~QOpenGLTextureBlitter()
{
}

bool QOpenGLTextureBlitter::create()
{
    QOpenGLContext *currentContext = QOpenGLContext::currentContext();
    if (!currentContext)
        return false;

    Q_D(QOpenGLTextureBlitter);

    d->vao->create();
    d->vao->bind();

    if (d->program)
        return true;

    d->program.reset(new QOpenGLShaderProgram());

    QSurfaceFormat format = currentContext->format();

    if (format.profile() == QSurfaceFormat::CoreProfile && format.version() >= qMakePair(3,2)) {
        d->program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader150);
        d->program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader150);
    } else {
        d->program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader);
        d->program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader);
    }
    d->program->link();
    if (!d->program->isLinked()) {
        qWarning() << Q_FUNC_INFO << "Could not link shader program:\n" << d->program->log();
        return false;
    }

    d->program->bind();

    d->vertexBuffer.create();
    d->vertexBuffer.bind();
    d->vertexBuffer.allocate(vertex_buffer_data, sizeof(vertex_buffer_data) * sizeof(vertex_buffer_data[0]));
    d->vertexBuffer.release();

    d->textureBuffer.create();
    d->textureBuffer.bind();
    d->textureBuffer.allocate(texture_buffer_data, sizeof(texture_buffer_data) * sizeof(texture_buffer_data[0]));
    d->textureBuffer.release();

    d->vertexCoordAttribPos = d->program->attributeLocation("vertexCoord");
    d->vertexTransformUniformPos = d->program->uniformLocation("vertexTransform");
    d->textureCoordAttribPos = d->program->attributeLocation("textureCoord");
    d->textureTransformUniformPos = d->program->uniformLocation("textureTransform");
    d->swizzleUniformPos = d->program->uniformLocation("swizzle");

    d->program->setUniformValue(d->swizzleUniformPos,false);

    d->vao->release();

    return true;
}

bool QOpenGLTextureBlitter::isCreated() const
{
    Q_D(const QOpenGLTextureBlitter);
    return d->program;
}

void QOpenGLTextureBlitter::destroy()
{
    Q_D(QOpenGLTextureBlitter);
    d->program.reset();
    d->vertexBuffer.destroy();
    d->textureBuffer.destroy();
    d->vao.reset();
}

void QOpenGLTextureBlitter::bind()
{
    Q_D(QOpenGLTextureBlitter);

    d->vao->bind();

    d->program->bind();

    d->vertexBuffer.bind();
    d->program->setAttributeBuffer(d->vertexCoordAttribPos, GL_FLOAT, 0, 3, 0);
    d->program->enableAttributeArray(d->vertexCoordAttribPos);
    d->vertexBuffer.release();

    d->textureBuffer.bind();
    d->program->setAttributeBuffer(d->textureCoordAttribPos, GL_FLOAT, 0, 2, 0);
    d->program->enableAttributeArray(d->textureCoordAttribPos);
    d->textureBuffer.release();
}

void QOpenGLTextureBlitter::release()
{
    Q_D(QOpenGLTextureBlitter);
    d->program->release();
    d->vao->release();
}

void QOpenGLTextureBlitter::setSwizzleRB(bool swizzle)
{
    Q_D(QOpenGLTextureBlitter);
    d->swizzle = swizzle;
}

void QOpenGLTextureBlitter::blit(GLuint texture,
                                 const QMatrix4x4 &targetTransform,
                                 Origin sourceOrigin)
{
    Q_D(QOpenGLTextureBlitter);
    d->blit(texture,targetTransform, sourceOrigin);
}

void QOpenGLTextureBlitter::blit(GLuint texture,
                                 const QMatrix4x4 &targetTransform,
                                 const QMatrix3x3 &sourceTransform)
{
    Q_D(QOpenGLTextureBlitter);
    d->blit(texture, targetTransform, sourceTransform);
}

QMatrix4x4 QOpenGLTextureBlitter::targetTransform(const QRectF &target,
                                                  const QRect &viewport)
{
    qreal x_scale = target.width() / viewport.width();
    qreal y_scale = target.height() / viewport.height();

    const QPointF relative_to_viewport = target.topLeft() - viewport.topLeft();
    qreal x_translate = x_scale - 1 + ((relative_to_viewport.x() / viewport.width()) * 2);
    qreal y_translate = -y_scale + 1 - ((relative_to_viewport.y() / viewport.height()) * 2);

    QMatrix4x4 matrix;
    matrix(0,3) = x_translate;
    matrix(1,3) = y_translate;

    matrix(0,0) = x_scale;
    matrix(1,1) = y_scale;

    return matrix;
}

QMatrix3x3 QOpenGLTextureBlitter::sourceTransform(const QRectF &subTexture,
                                                  const QSize &textureSize,
                                                  Origin origin)
{
    qreal x_scale = subTexture.width() / textureSize.width();
    qreal y_scale = subTexture.height() / textureSize.height();

    const QPointF topLeft = subTexture.topLeft();
    qreal x_translate = topLeft.x() / textureSize.width();
    qreal y_translate = topLeft.y() / textureSize.height();

    if (origin == OriginTopLeft) {
        y_scale = -y_scale;
        y_translate = 1 - y_translate;
    }

    QMatrix3x3 matrix;
    matrix(0,2) = x_translate;
    matrix(1,2) = y_translate;

    matrix(0,0) = x_scale;
    matrix(1,1) = y_scale;

    return matrix;
}

QT_END_NAMESPACE
