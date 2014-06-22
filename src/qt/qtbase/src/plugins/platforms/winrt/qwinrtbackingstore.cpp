/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwinrtbackingstore.h"

#include "qwinrtscreen.h"
#include "qwinrtwindow.h"
#include "qwinrteglcontext.h"
#include <QtGui/QOpenGLContext>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// Generated shader headers
#include "blitps.h"
#include "blitvs.h"

namespace { // Utility namespace for writing out an ANGLE-compatible binary blob

// Must match packaged ANGLE
enum : quint32 {
    AngleMajorVersion = 1,
    AngleMinorVersion = 3
};

struct ShaderString
{
    ShaderString(const char *data = 0) : data(data) { }
    const char *data;
};

// ANGLE stream compatibility - when size_t is 32-bit, QDataStream::writeBytes() also works
QDataStream &operator<<(QDataStream &stream, const ShaderString &shaderString)
{
    if (!shaderString.data)
        return stream << size_t(0);

    size_t len = strlen(shaderString.data);
    stream << len;
    stream.writeRawData(shaderString.data, int(len));
    return stream;
}

struct Attribute
{
    Attribute(GLenum type = 0, const char *name = 0, quint32 index = 0)
        : type(type), name(name), index(index) { }
    GLenum type;
    ShaderString name;
    quint32 index;
};

struct Sampler
{
    enum TextureType { Texture2D, TextureCube };
    Sampler(bool active = false, GLint unit = 0, TextureType type = Texture2D)
        : active(active), unit(unit), type(type) { }
    bool active;
    GLint unit;
    TextureType type;
};

struct Uniform
{
    Uniform() { }
    Uniform(GLenum type, quint32 precision, const char *name, quint32 arraySize,
            quint32 psRegisterIndex, quint32 vsRegisterIndex, quint32 registerCount)
        : type(type), precision(precision), name(name), arraySize(arraySize)
        , psRegisterIndex(psRegisterIndex), vsRegisterIndex(vsRegisterIndex), registerCount(registerCount) { }
    GLenum type;
    quint32 precision;
    ShaderString name;
    quint32 arraySize;
    quint32 psRegisterIndex;
    quint32 vsRegisterIndex;
    quint32 registerCount;
};

struct UniformIndex
{
    UniformIndex(const char *name = 0, quint32 element = 0, quint32 index = 0)
        : name(name), element(element), index(index) { }
    ShaderString name;
    quint32 element;
    quint32 index;
};

static const QByteArray createAngleBinary(
        const QVector<Attribute> &attributes,
        const QVector<Sampler> &textureSamplers,
        const QVector<Sampler> &vertexSamplers,
        const QVector<Uniform> &uniforms,
        const QVector<UniformIndex> &uniformIndex,
        const QByteArray &pixelShader,
        const QByteArray &vertexShader,
        const QByteArray &geometryShader = QByteArray(),
        bool usesPointSize = false)
{
    QByteArray binary;

    QDataStream stream(&binary, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << quint32(GL_PROGRAM_BINARY_ANGLE)
           << qint32(AngleMajorVersion)
           << qint32(AngleMinorVersion);

    // Vertex attributes
    for (int i = 0; i < 16; ++i) {
        if (i < attributes.size())
            stream << quint32(attributes[i].type) << attributes[i].name << attributes[i].index;
        else
            stream << quint32(GL_NONE) << ShaderString() << qint32(-1);
    }

    // Texture units
    for (int i = 0; i < 16; ++i) {
        if (i < textureSamplers.size())
            stream << textureSamplers[i].active << textureSamplers[i].unit << qint32(textureSamplers[i].type);
        else
            stream << false << qint32(0) << qint32(Sampler::Texture2D);
    }

    // Vertex texture units
    for (int i = 0; i < 16; ++i) {
        if (i < vertexSamplers.size())
            stream << vertexSamplers[i].active << vertexSamplers[i].unit << qint32(vertexSamplers[i].type);
        else
            stream << false << qint32(0) << qint32(Sampler::Texture2D);
    }

    stream << vertexSamplers.size()
           << textureSamplers.size()
           << usesPointSize;

    stream << size_t(uniforms.size());
    foreach (const Uniform &uniform, uniforms) {
        stream << uniform.type << uniform.precision << uniform.name << uniform.arraySize
               << uniform.psRegisterIndex << uniform.vsRegisterIndex << uniform.registerCount;
    }

    stream << size_t(uniformIndex.size());
    foreach (const UniformIndex &index, uniformIndex)
        stream << index.name << index.element << index.index;

    stream << quint32(pixelShader.size())
           << quint32(vertexShader.size())
           << quint32(geometryShader.size());

    stream.writeRawData(pixelShader.constData(), pixelShader.size());
    stream.writeRawData(vertexShader.constData(), vertexShader.size());
    if (!geometryShader.isEmpty())
        stream.writeRawData(geometryShader.constData(), geometryShader.size());

    return binary;
}

} // namespace

QT_BEGIN_NAMESPACE

static const GLfloat normCoords[] = { -1, 1, 1, 1, 1, -1, -1, -1 };
static const GLfloat quadCoords[] = { 0, 0, 1, 0, 1, 1, 0, 1 };

QWinRTBackingStore::QWinRTBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , m_context(new QOpenGLContext)
    , m_shaderProgram(0)
    , m_fbo(0)
    , m_texture(0)
    , m_screen(static_cast<QWinRTScreen*>(window->screen()->handle()))
    , m_initialized(false)
{
    window->setSurfaceType(QSurface::OpenGLSurface); // Required for flipping, but could be done in the swap
}

bool QWinRTBackingStore::initialize()
{
    if (m_initialized)
        return true;

    m_context->setFormat(window()->requestedFormat());
    m_context->setScreen(window()->screen());
    if (!m_context->create())
        return false;

    if (!m_context->makeCurrent(window()))
        return false;

    glGenFramebuffers(1, &m_fbo);
    glGenRenderbuffers(1, &m_rbo);
    glGenTextures(1, &m_texture);
    m_shaderProgram = glCreateProgram();

#if 0 // Standard GLES passthrough shader program
    static const char *vertexShaderSource =
            "attribute vec4 pos0;\n"
            "attribute vec2 tex0;\n"
            "varying vec2 coord;\n"
            "void main() {\n"
            "   coord = tex0;\n"
            "   gl_Position = pos0;\n"
            "}\n";
    static const char *fragmentShaderSource =
            "uniform sampler2D texture;\n"
            "varying highp vec2 coord;\n"
            "void main() {\n"
            "   gl_FragColor = texture2D(texture, coord);\n"
            "}\n";
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
#else // Precompiled passthrough shader
    QVector<Attribute> attributes = QVector<Attribute>() << Attribute(GL_FLOAT_VEC4, "pos0", 0)
                                                         << Attribute(GL_FLOAT_VEC2, "tex0", 1);
    QVector<Sampler> textureSamplers = QVector<Sampler>() << Sampler(true, 0, Sampler::Texture2D);
    QVector<Sampler> vertexSamplers;
    QVector<Uniform> uniforms = QVector<Uniform>() << Uniform(GL_SAMPLER_2D, 0, "texture", 0, 0, -1, 1);
    QVector<UniformIndex> uniformsIndex = QVector<UniformIndex>() << UniformIndex("texture", 0, 0);
    QByteArray pixelShader(reinterpret_cast<const char *>(q_blitps), sizeof(q_blitps));
    QByteArray vertexShader(reinterpret_cast<const char *>(q_blitvs), sizeof(q_blitvs));
    QByteArray binary = createAngleBinary(attributes, textureSamplers, vertexSamplers,
                                          uniforms, uniformsIndex, pixelShader, vertexShader);
    glProgramBinaryOES(m_shaderProgram, GL_PROGRAM_BINARY_ANGLE, binary.constData(), binary.size());
#endif
    m_context->doneCurrent();
    m_initialized = true;
    return true;
}

QWinRTBackingStore::~QWinRTBackingStore()
{
    if (!m_initialized)
        return;
    glDeleteBuffers(1, &m_fbo);
    glDeleteRenderbuffers(1, &m_rbo);
    glDeleteTextures(1, &m_texture);
    glDeleteProgram(m_shaderProgram);
}

QPaintDevice *QWinRTBackingStore::paintDevice()
{
    return m_paintDevice.data();
}

void QWinRTBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(offset)
    if (m_size.isEmpty())
        return;

    const QImage *image = static_cast<QImage *>(m_paintDevice.data());

    m_context->makeCurrent(window);

    // Blitting the entire image width trades zero image copy/relayout for a larger texture upload.
    // Since we're blitting the whole width anyway, the boundingRect() is used in the assumption that
    // we don't repeat upload. This is of course dependent on the distance between update regions.
    // Ideally, we would use the GL_EXT_unpack_subimage extension, which should be possible to implement
    // since D3D11_MAPPED_SUBRESOURCE supports RowPitch (see below).
    // Note that single-line blits in a loop are *very* slow, so reducing calls to glTexSubImage2D
    // is probably a good idea anyway.
    glBindTexture(GL_TEXTURE_2D, m_texture);
    QRect bounds = region.boundingRect();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, bounds.y(), m_size.width(), bounds.height(),
                    GL_BGRA_EXT, GL_UNSIGNED_BYTE, image->scanLine(bounds.y()));
    // TODO: Implement GL_EXT_unpack_subimage in ANGLE for more minimal uploads
    //glPixelStorei(GL_UNPACK_ROW_LENGTH, image->bytesPerLine());
    //glTexSubImage2D(GL_TEXTURE_2D, 0, bounds.x(), bounds.y(), bounds.width(), bounds.height(),
    //                GL_BGRA_EXT, GL_UNSIGNED_BYTE, image->scanLine(bounds.y()) + bounds.x() * 4);

    // Bind render buffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);

    // Bind position
    glUseProgram(m_shaderProgram);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, normCoords);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quadCoords);

    // Render
    glViewport(0, 0, m_size.width(), m_size.height());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // Unbind
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // fast blit - TODO: perform the blit inside swap buffers instead
    glBindFramebuffer(GL_READ_FRAMEBUFFER_ANGLE, m_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER_ANGLE, 0);
    glBlitFramebufferANGLE(0, 0, m_size.width(), m_size.height(), // TODO: blit only the changed rectangle
                           0, 0, m_size.width(), m_size.height(),
                           GL_COLOR_BUFFER_BIT, GL_NEAREST);

    m_context->swapBuffers(window);
    m_context->doneCurrent();
}

void QWinRTBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    Q_UNUSED(staticContents)
    if (!initialize())
        return;

    if (m_size == size)
        return;

    m_size = size;
    if (m_size.isEmpty())
        return;

    m_paintDevice.reset(new QImage(m_size, QImage::Format_ARGB32_Premultiplied));

    m_context->makeCurrent(window());
    // Input texture
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, m_size.width(), m_size.height(),
                 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    // Render buffer
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_BGRA8_EXT, m_size.width(), m_size.height());
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    m_context->doneCurrent();
}

void QWinRTBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region)
    resize(window()->size(), QRegion());
}

void QWinRTBackingStore::endPaint()
{
}

QT_END_NAMESPACE
