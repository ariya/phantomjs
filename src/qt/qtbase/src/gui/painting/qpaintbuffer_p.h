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

#ifndef QPAINTBUFFER_P_H
#define QPAINTBUFFER_P_H

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

#include <qpaintdevice.h>

#include <private/qpaintengineex_p.h>
#include <private/qtextengine_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QPaintBufferPrivate;
class QPaintBufferPlayback;

class Q_GUI_EXPORT QPaintBuffer : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QPaintBuffer)
public:
    QPaintBuffer();
    QPaintBuffer(const QPaintBuffer &other);
    ~QPaintBuffer();

    bool isEmpty() const;

    void beginNewFrame();
    int numFrames() const;

    void draw(QPainter *painter, int frame = 0) const;

    int frameStartIndex(int frame) const;
    int frameEndIndex(int frame) const;
    int processCommands(QPainter *painter, int begin, int end) const;
#ifndef QT_NO_DEBUG_STREAM
    QString commandDescription(int command) const;
#endif

    void setBoundingRect(const QRectF &rect);
    QRectF boundingRect() const;

    virtual QPaintEngine *paintEngine() const;
    virtual int metric(PaintDeviceMetric m) const;
    virtual int devType() const;

    QPaintBuffer &operator=(const QPaintBuffer &other);

private:
    friend class QPainterReplayer;
    friend class QOpenGLReplayer;

    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &stream, const QPaintBuffer &buffer);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &stream, QPaintBuffer &buffer);

    QPaintBufferPrivate *d_ptr;
};

Q_GUI_EXPORT QDataStream &operator<<(QDataStream &stream, const QPaintBuffer &buffer);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &stream, QPaintBuffer &buffer);

class QPaintBufferEngine;

class QTextItemIntCopy
{
public:
    QTextItemIntCopy(const QTextItem &item);
    ~QTextItemIntCopy();
    QTextItemInt &operator () () {return m_item;}
private:
    QTextItemInt m_item;
    QFont m_font;
};

struct QPaintBufferCommand
{
    uint id : 8;
    uint size : 24;

    int offset;
    int offset2;
    int extra;
};

QDataStream &operator<<(QDataStream &stream, const QPaintBufferCommand &command);
QDataStream &operator>>(QDataStream &stream, QPaintBufferCommand &command);

Q_DECLARE_TYPEINFO(QPaintBufferCommand, Q_MOVABLE_TYPE);

class QPaintBufferPrivate
{
public:
    enum Command {
        Cmd_Save,
        Cmd_Restore,

        Cmd_SetBrush,
        Cmd_SetBrushOrigin,
        Cmd_SetClipEnabled,
        Cmd_SetCompositionMode,
        Cmd_SetOpacity,
        Cmd_SetPen,
        Cmd_SetRenderHints,
        Cmd_SetTransform,
        Cmd_SetBackgroundMode,

        Cmd_ClipPath,
        Cmd_ClipRect,
        Cmd_ClipRegion,
        Cmd_ClipVectorPath,

        Cmd_DrawVectorPath,
        Cmd_FillVectorPath,
        Cmd_StrokeVectorPath,

        Cmd_DrawConvexPolygonF,
        Cmd_DrawConvexPolygonI,
        Cmd_DrawEllipseF,
        Cmd_DrawEllipseI,
        Cmd_DrawLineF,
        Cmd_DrawLineI,
        Cmd_DrawPath,
        Cmd_DrawPointsF,
        Cmd_DrawPointsI,
        Cmd_DrawPolygonF,
        Cmd_DrawPolygonI,
        Cmd_DrawPolylineF,
        Cmd_DrawPolylineI,
        Cmd_DrawRectF,
        Cmd_DrawRectI,

        Cmd_FillRectBrush,
        Cmd_FillRectColor,

        Cmd_DrawText,
        Cmd_DrawTextItem,

        Cmd_DrawImagePos,
        Cmd_DrawImageRect,
        Cmd_DrawPixmapPos,
        Cmd_DrawPixmapRect,
        Cmd_DrawTiledPixmap,

        Cmd_SystemStateChanged,
        Cmd_Translate,
        Cmd_DrawStaticText,

        // new commands must be added above this line

        Cmd_LastCommand
    };

    QPaintBufferPrivate();
    ~QPaintBufferPrivate();

    int addData(const int *data, int count) {
        if (count <= 0)
            return 0;
        int pos = ints.size();
        ints.resize(pos + count);
        memcpy(ints.data() + pos, data, count * sizeof(int));
        return pos;
    }

    int addData(const qreal *data, int count) {
        if (count <= 0)
            return 0;
        int pos = floats.size();
        floats.resize(pos + count);
        memcpy(floats.data() + pos, data, count * sizeof(qreal));
        return pos;
    }

    int addData(const QVariant &var) {
        variants << var;
        return variants.size() - 1;
    }

    QPaintBufferCommand *addCommand(Command command) {
        QPaintBufferCommand cmd;
        cmd.id = command;
        cmd.size = cmd.offset = cmd.offset2 = cmd.extra = 0;
        commands << cmd;
        return &commands.last();
    }

    QPaintBufferCommand *addCommand(Command command, const QVariant &var) {
        QPaintBufferCommand cmd;
        cmd.id = command;
        cmd.offset = addData(var);
        cmd.size = cmd.offset2 = cmd.extra = 0;
        commands << cmd;
        return &commands.last();
    }

    QPaintBufferCommand *addCommand(Command command, const QVectorPath &path) {
        QPaintBufferCommand cmd;
        cmd.id = command;
        cmd.offset = addData(path.points(), path.elementCount() * 2);
        cmd.offset2 = ints.size();
        ints << path.hints();
        // The absence of path elements is indicated by setting the highest bit in 'cmd.offset2'.
        if (path.elements())
            addData((const int *) path.elements(), path.elementCount());
        else
            cmd.offset2 |= 0x80000000;
        cmd.size = path.elementCount();
        cmd.extra = 0;
        commands << cmd;
        return &commands.last();
    }

    QPaintBufferCommand *addCommand(Command command , const qreal *pts, int arrayLength, int elementCount) {
        QPaintBufferCommand cmd;
        cmd.id = command;
        cmd.offset = addData(pts, arrayLength);
        cmd.size = elementCount;
        cmd.offset2 = cmd.extra = 0;
        commands << cmd;
        return &commands.last();
    }

    QPaintBufferCommand *addCommand(Command command , const int *pts, int arrayLength, int elementCount) {
        QPaintBufferCommand cmd;
        cmd.id = command;
        cmd.offset = addData(pts, arrayLength);
        cmd.size = elementCount;
        cmd.offset2 = cmd.extra = 0;
        commands << cmd;
        return &commands.last();
    }

    inline void updateBoundingRect(const QRectF &rect);

    QAtomicInt ref;

    QVector<int> ints;
    QVector<qreal> floats;
    QVector<QVariant> variants;

    QVector<QPaintBufferCommand> commands;
    QList<int> frames;

    QPaintBufferEngine *engine;
    QRectF boundingRect;
    qreal penWidthAdjustment;
    uint calculateBoundingRect : 1;

    void *cache;
};


struct QVectorPathCmd
{
    // The absence of path elements is indicated by setting the highest bit in 'cmd.offset2'.
    QVectorPathCmd(QPaintBufferPrivate *d, const QPaintBufferCommand &cmd)
        : vectorPath(d->floats.constData() + cmd.offset,
                     cmd.size,
                     cmd.offset2 & 0x80000000
                     ? 0
                     : (const QPainterPath::ElementType *) (d->ints.constData() + cmd.offset2 + 1),
                     *(d->ints.constData() + (cmd.offset2 & 0x7fffffff))) {}

    inline const QVectorPath &operator()() const { return vectorPath; }

    QVectorPath vectorPath;
};


class Q_GUI_EXPORT QPainterReplayer
{
public:
    QPainterReplayer() { }

    virtual ~QPainterReplayer() { }

    void setupTransform(QPainter *painter);
    virtual void process(const QPaintBufferCommand &cmd);
    void processCommands(const QPaintBuffer &buffer, QPainter *painter, int begin, int end);

protected:
    QPaintBufferPrivate *d;
    QTransform m_world_matrix;

    QPainter *painter;
};

class Q_GUI_EXPORT QPaintEngineExReplayer : public QPainterReplayer
{
public:
    QPaintEngineExReplayer() { }

    virtual void process(const QPaintBufferCommand &cmd);
};

class QPaintBufferEnginePrivate;

class QPaintBufferEngine : public QPaintEngineEx
{
    Q_DECLARE_PRIVATE(QPaintBufferEngine)
public:
    QPaintBufferEngine(QPaintBufferPrivate *buffer);

    virtual bool begin(QPaintDevice *device);
    virtual bool end();

    virtual Type type() const { return QPaintEngine::PaintBuffer; }

    virtual QPainterState *createState(QPainterState *orig) const;

    virtual void draw(const QVectorPath &path);
    virtual void fill(const QVectorPath &path, const QBrush &brush);
    virtual void stroke(const QVectorPath &path, const QPen &pen);

    virtual void clip(const QVectorPath &path, Qt::ClipOperation op);
    virtual void clip(const QRect &rect, Qt::ClipOperation op);
    virtual void clip(const QRegion &region, Qt::ClipOperation op);
    virtual void clip(const QPainterPath &path, Qt::ClipOperation op);

    virtual void clipEnabledChanged();
    virtual void penChanged();
    virtual void brushChanged();
    virtual void brushOriginChanged();
    virtual void opacityChanged();
    virtual void compositionModeChanged();
    virtual void renderHintsChanged();
    virtual void transformChanged();
    virtual void backgroundModeChanged();

    virtual void fillRect(const QRectF &rect, const QBrush &brush);
    virtual void fillRect(const QRectF &rect, const QColor &color);

    virtual void drawRects(const QRect *rects, int rectCount);
    virtual void drawRects(const QRectF *rects, int rectCount);

    virtual void drawLines(const QLine *lines, int lineCount);
    virtual void drawLines(const QLineF *lines, int lineCount);

    virtual void drawEllipse(const QRectF &r);
    virtual void drawEllipse(const QRect &r);

    virtual void drawPath(const QPainterPath &path);

    virtual void drawPoints(const QPointF *points, int pointCount);
    virtual void drawPoints(const QPoint *points, int pointCount);

    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode);
    virtual void drawPolygon(const QPoint *points, int pointCount, PolygonDrawMode mode);

    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr);
    virtual void drawPixmap(const QPointF &pos, const QPixmap &pm);

    virtual void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr,
                           Qt::ImageConversionFlags flags = Qt::AutoColor);
    virtual void drawImage(const QPointF &pos, const QImage &image);

    virtual void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s);

    virtual void drawTextItem(const QPointF &pos, const QTextItem &ti);
    virtual void drawStaticTextItem(QStaticTextItem *staticTextItem);

    virtual void setState(QPainterState *s);
    virtual uint flags() const {return QPaintEngineEx::DoNotEmulate;}

    QPaintBufferPrivate *buffer;

    mutable int m_begin_detected : 1;
    mutable int m_save_detected : 1;
    mutable int m_stream_raw_text_items : 1;
    mutable int m_unused : 29;

    mutable QPainterState *m_created_state;
};

QT_END_NAMESPACE

#endif // QPAINTBUFFER_P_H
