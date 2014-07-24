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

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtDebug>

#include <QtPlatformSupport/private/qdevicediscovery_p.h>

#include "qeglplatformcursor_p.h"
#include "qeglplatformintegration_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformCursor
    \brief Mouse cursor implementation using OpenGL.
    \since 5.2
    \internal
    \ingroup qpa
 */

QEGLPlatformCursor::QEGLPlatformCursor(QPlatformScreen *screen)
    : m_visible(true),
      m_screen(screen),
      m_program(0),
      m_vertexCoordEntry(0),
      m_textureCoordEntry(0),
      m_textureEntry(0),
      m_deviceListener(0),
      m_updater(screen)
{
    QByteArray hideCursorVal = qgetenv("QT_QPA_EGLFS_HIDECURSOR");
    if (!hideCursorVal.isEmpty())
        m_visible = hideCursorVal.toInt() == 0;
    if (!m_visible)
        return;

    // Try to load the cursor atlas. If this fails, m_visible is set to false and
    // paintOnScreen() and setCurrentCursor() become no-ops.
    initCursorAtlas();

    // initialize the cursor
#ifndef QT_NO_CURSOR
    QCursor cursor(Qt::ArrowCursor);
    setCurrentCursor(&cursor);
#endif
}

QEGLPlatformCursor::~QEGLPlatformCursor()
{
    resetResources();
    delete m_deviceListener;
}

void QEGLPlatformCursor::setMouseDeviceDiscovery(QDeviceDiscovery *dd)
{
    if (m_visible && dd) {
        m_deviceListener = new QEGLPlatformCursorDeviceListener(dd, this);
        updateMouseStatus();
    }
}

void QEGLPlatformCursor::updateMouseStatus()
{
    m_visible = m_deviceListener->hasMouse();
}

QEGLPlatformCursorDeviceListener::QEGLPlatformCursorDeviceListener(QDeviceDiscovery *dd, QEGLPlatformCursor *cursor)
    : m_cursor(cursor)
{
    m_mouseCount = dd->scanConnectedDevices().count();
    connect(dd, SIGNAL(deviceDetected(QString)), SLOT(onDeviceAdded()));
    connect(dd, SIGNAL(deviceRemoved(QString)), SLOT(onDeviceRemoved()));
}

bool QEGLPlatformCursorDeviceListener::hasMouse() const
{
    return m_mouseCount > 0;
}

void QEGLPlatformCursorDeviceListener::onDeviceAdded()
{
    ++m_mouseCount;
    m_cursor->updateMouseStatus();
}

void QEGLPlatformCursorDeviceListener::onDeviceRemoved()
{
    --m_mouseCount;
    m_cursor->updateMouseStatus();
}

void QEGLPlatformCursor::resetResources()
{
    if (QOpenGLContext::currentContext()) {
        delete m_program;
        glDeleteTextures(1, &m_cursor.customCursorTexture);
        glDeleteTextures(1, &m_cursorAtlas.texture);
    }
    m_program = 0;
    m_cursor.customCursorTexture = 0;
    m_cursor.customCursorPending = !m_cursor.customCursorImage.isNull();
    m_cursorAtlas.texture = 0;
}

void QEGLPlatformCursor::createShaderPrograms()
{
    static const char *textureVertexProgram =
        "attribute highp vec2 vertexCoordEntry;\n"
        "attribute highp vec2 textureCoordEntry;\n"
        "varying highp vec2 textureCoord;\n"
        "void main() {\n"
        "   textureCoord = textureCoordEntry;\n"
        "   gl_Position = vec4(vertexCoordEntry, 1.0, 1.0);\n"
        "}\n";

    static const char *textureFragmentProgram =
        "uniform sampler2D texture;\n"
        "varying highp vec2 textureCoord;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(texture, textureCoord).bgra;\n"
        "}\n";

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
    m_program->link();

    m_vertexCoordEntry = m_program->attributeLocation("vertexCoordEntry");
    m_textureCoordEntry = m_program->attributeLocation("textureCoordEntry");
    m_textureEntry = m_program->attributeLocation("texture");
}

void QEGLPlatformCursor::createCursorTexture(uint *texture, const QImage &image)
{
    if (!*texture)
        glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA, image.width(), image.height(), 0 /* border */,
                 GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
}

void QEGLPlatformCursor::initCursorAtlas()
{
    static QByteArray json = qgetenv("QT_QPA_EGLFS_CURSOR");
    if (json.isEmpty())
        json = ":/cursor.json";

    QFile file(QString::fromUtf8(json));
    if (!file.open(QFile::ReadOnly)) {
        m_visible = false;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject object = doc.object();

    QString atlas = object.value(QLatin1String("image")).toString();
    Q_ASSERT(!atlas.isEmpty());

    const int cursorsPerRow = object.value(QLatin1String("cursorsPerRow")).toDouble();
    Q_ASSERT(cursorsPerRow);
    m_cursorAtlas.cursorsPerRow = cursorsPerRow;

    const QJsonArray hotSpots = object.value(QLatin1String("hotSpots")).toArray();
    Q_ASSERT(hotSpots.count() == Qt::LastCursor + 1);
    for (int i = 0; i < hotSpots.count(); i++) {
        QPoint hotSpot(hotSpots[i].toArray()[0].toDouble(), hotSpots[i].toArray()[1].toDouble());
        m_cursorAtlas.hotSpots << hotSpot;
    }

    QImage image = QImage(atlas).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_cursorAtlas.cursorWidth = image.width() / m_cursorAtlas.cursorsPerRow;
    m_cursorAtlas.cursorHeight = image.height() / ((Qt::LastCursor + cursorsPerRow) / cursorsPerRow);
    m_cursorAtlas.width = image.width();
    m_cursorAtlas.height = image.height();
    m_cursorAtlas.image = image;
}

#ifndef QT_NO_CURSOR
void QEGLPlatformCursor::changeCursor(QCursor *cursor, QWindow *window)
{
    Q_UNUSED(window);
    const QRect oldCursorRect = cursorRect();
    if (setCurrentCursor(cursor))
        update(oldCursorRect | cursorRect());
}

bool QEGLPlatformCursor::setCurrentCursor(QCursor *cursor)
{
    if (!m_visible)
        return false;

    const Qt::CursorShape newShape = cursor ? cursor->shape() : Qt::ArrowCursor;
    if (m_cursor.shape == newShape && newShape != Qt::BitmapCursor)
        return false;

    if (m_cursor.shape == Qt::BitmapCursor) {
        m_cursor.customCursorImage = QImage();
        m_cursor.customCursorPending = false;
    }
    m_cursor.shape = newShape;
    if (newShape != Qt::BitmapCursor) { // standard cursor
        const float ws = (float)m_cursorAtlas.cursorWidth / m_cursorAtlas.width,
                    hs = (float)m_cursorAtlas.cursorHeight / m_cursorAtlas.height;
        m_cursor.textureRect = QRectF(ws * (m_cursor.shape % m_cursorAtlas.cursorsPerRow),
                                      hs * (m_cursor.shape / m_cursorAtlas.cursorsPerRow),
                                      ws, hs);
        m_cursor.hotSpot = m_cursorAtlas.hotSpots[m_cursor.shape];
        m_cursor.texture = m_cursorAtlas.texture;
        m_cursor.size = QSize(m_cursorAtlas.cursorWidth, m_cursorAtlas.cursorHeight);
    } else {
        QImage image = cursor->pixmap().toImage();
        m_cursor.textureRect = QRectF(0, 0, 1, 1);
        m_cursor.hotSpot = cursor->hotSpot();
        m_cursor.texture = 0; // will get updated in the next render()
        m_cursor.size = image.size();
        m_cursor.customCursorImage = image;
        m_cursor.customCursorPending = true;
    }

    return true;
}
#endif

void QEGLPlatformCursorUpdater::update(const QPoint &pos, const QRegion &rgn)
{
    m_active = false;
    QWindowSystemInterface::handleExposeEvent(m_screen->topLevelAt(pos), rgn);
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QEGLPlatformCursorUpdater::scheduleUpdate(const QPoint &pos, const QRegion &rgn)
{
    if (m_active)
        return;

    m_active = true;

    // Must not flush the window system events directly from here since we are likely to
    // be a called directly from QGuiApplication's processMouseEvents. Flushing events
    // could cause reentering by dispatching more queued mouse events.
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection,
                              Q_ARG(QPoint, pos), Q_ARG(QRegion, rgn));
}

void QEGLPlatformCursor::update(const QRegion &rgn)
{
    m_updater.scheduleUpdate(m_cursor.pos, rgn);
}

QRect QEGLPlatformCursor::cursorRect() const
{
    return QRect(m_cursor.pos - m_cursor.hotSpot, m_cursor.size);
}

QPoint QEGLPlatformCursor::pos() const
{
    return m_cursor.pos;
}

void QEGLPlatformCursor::setPos(const QPoint &pos)
{
    const QRect oldCursorRect = cursorRect();
    m_cursor.pos = pos;
    update(oldCursorRect | cursorRect());
}

void QEGLPlatformCursor::pointerEvent(const QMouseEvent &event)
{
    if (event.type() != QEvent::MouseMove)
        return;
    const QRect oldCursorRect = cursorRect();
    m_cursor.pos = event.screenPos().toPoint();
    update(oldCursorRect | cursorRect());
}

void QEGLPlatformCursor::paintOnScreen()
{
    if (!m_visible)
        return;

    const QRectF cr = cursorRect();
    const QRect screenRect(m_screen->geometry());
    const GLfloat x1 = 2 * (cr.left() / screenRect.width()) - 1;
    const GLfloat x2 = 2 * (cr.right() / screenRect.width()) - 1;
    const GLfloat y1 = 1 - (cr.top() / screenRect.height()) * 2;
    const GLfloat y2 = 1 - (cr.bottom() / screenRect.height()) * 2;
    QRectF r(QPointF(x1, y1), QPointF(x2, y2));

    draw(r);
}

void QEGLPlatformCursor::draw(const QRectF &r)
{
    if (!m_program) {
        // one time initialization
        createShaderPrograms();

        if (!m_cursorAtlas.texture) {
            createCursorTexture(&m_cursorAtlas.texture, m_cursorAtlas.image);

            if (m_cursor.shape != Qt::BitmapCursor)
                m_cursor.texture = m_cursorAtlas.texture;
        }
    }

    if (m_cursor.shape == Qt::BitmapCursor && m_cursor.customCursorPending) {
        // upload the custom cursor
        createCursorTexture(&m_cursor.customCursorTexture, m_cursor.customCursorImage);
        m_cursor.texture = m_cursor.customCursorTexture;
        m_cursor.customCursorPending = false;
    }

    Q_ASSERT(m_cursor.texture);

    m_program->bind();

    const GLfloat x1 = r.left();
    const GLfloat x2 = r.right();
    const GLfloat y1 = r.top();
    const GLfloat y2 = r.bottom();
    const GLfloat cursorCoordinates[] = {
        x1, y2,
        x2, y2,
        x1, y1,
        x2, y1
    };

    const GLfloat s1 = m_cursor.textureRect.left();
    const GLfloat s2 = m_cursor.textureRect.right();
    const GLfloat t1 = m_cursor.textureRect.top();
    const GLfloat t2 = m_cursor.textureRect.bottom();
    const GLfloat textureCoordinates[] = {
        s1, t2,
        s2, t2,
        s1, t1,
        s2, t1
    };

    glBindTexture(GL_TEXTURE_2D, m_cursor.texture);

    m_program->enableAttributeArray(m_vertexCoordEntry);
    m_program->enableAttributeArray(m_textureCoordEntry);

    m_program->setAttributeArray(m_vertexCoordEntry, cursorCoordinates, 2);
    m_program->setAttributeArray(m_textureCoordEntry, textureCoordinates, 2);

    m_program->setUniformValue(m_textureEntry, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // disable depth testing to make sure cursor is always on top
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, 0);
    m_program->disableAttributeArray(m_textureCoordEntry);
    m_program->disableAttributeArray(m_vertexCoordEntry);

    m_program->release();
}

QT_END_NAMESPACE
