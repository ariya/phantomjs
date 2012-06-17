/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qmath.h>
#include <private/qpainterpath_p.h>
#include <private/qpaintbuffer_p.h>
//#include <private/qtextengine_p.h>
#include <private/qfontengine_p.h>
#include <private/qemulationpaintengine_p.h>
#include <private/qimage_p.h>
#include <qstatictext.h>
#include <private/qstatictext_p.h>
#include <private/qrawfont_p.h>

#include <QDebug>

// #define QPAINTBUFFER_DEBUG_DRAW

QT_BEGIN_NAMESPACE

extern void qt_format_text(const QFont &font,
                           const QRectF &_r, int tf, const QTextOption *option, const QString& str, QRectF *brect,
                           int tabstops, int* tabarray, int tabarraylen,
                           QPainter *painter);

QTextItemIntCopy::QTextItemIntCopy(const QTextItem &item)
    : m_item(static_cast<const QTextItemInt &>(item))
{
    QChar *chars = new QChar[m_item.num_chars];
    unsigned short *logClusters = new unsigned short[m_item.num_chars];
    memcpy(chars, m_item.chars, m_item.num_chars * sizeof(QChar));
    memcpy(logClusters, m_item.logClusters, m_item.num_chars * sizeof(unsigned short));
    m_item.chars = chars;
    m_item.logClusters = logClusters;

    const int size = QGlyphLayout::spaceNeededForGlyphLayout(m_item.glyphs.numGlyphs);
    char *glyphLayoutData = new char[size];
    QGlyphLayout glyphs(glyphLayoutData, m_item.glyphs.numGlyphs);
    memcpy(glyphs.offsets, m_item.glyphs.offsets, m_item.glyphs.numGlyphs * sizeof(QFixedPoint));
    memcpy(glyphs.glyphs, m_item.glyphs.glyphs, m_item.glyphs.numGlyphs * sizeof(HB_Glyph));
    memcpy(glyphs.advances_x, m_item.glyphs.advances_x, m_item.glyphs.numGlyphs * sizeof(QFixed));
    memcpy(glyphs.advances_y, m_item.glyphs.advances_y, m_item.glyphs.numGlyphs * sizeof(QFixed));
    memcpy(glyphs.justifications, m_item.glyphs.justifications, m_item.glyphs.numGlyphs * sizeof(QGlyphJustification));
    memcpy(glyphs.attributes, m_item.glyphs.attributes, m_item.glyphs.numGlyphs * sizeof(HB_GlyphAttributes));
    m_item.glyphs = glyphs;

    m_font = *m_item.f;
    m_item.f = &m_font;

    m_item.fontEngine->ref.ref(); // Increment reference count.
}

QTextItemIntCopy::~QTextItemIntCopy()
{
    delete m_item.chars;
    delete m_item.logClusters;
    delete m_item.glyphs.data();
    if (!m_item.fontEngine->ref.deref())
        delete m_item.fontEngine;
}

/************************************************************************
 *
 * QPaintBufferSignalProxy
 *
 ************************************************************************/

Q_GLOBAL_STATIC(QPaintBufferSignalProxy, theSignalProxy)

QPaintBufferSignalProxy *QPaintBufferSignalProxy::instance()
{
    return theSignalProxy();
}

/************************************************************************
 *
 * QPaintBufferPrivate
 *
 ************************************************************************/

QPaintBufferPrivate::QPaintBufferPrivate()
    : ref(1), engine(0), penWidthAdjustment(0)
    , calculateBoundingRect(true)
    , cache(0)
{
}

QPaintBufferPrivate::~QPaintBufferPrivate()
{
    QPaintBufferSignalProxy::instance()->emitAboutToDestroy(this);

    for (int i = 0; i < commands.size(); ++i) {
        const QPaintBufferCommand &cmd = commands.at(i);
        if (cmd.id == QPaintBufferPrivate::Cmd_DrawTextItem)
            delete reinterpret_cast<QTextItemIntCopy *>(qvariant_cast<void *>(variants.at(cmd.offset)));
    }
}


inline void QPaintBufferPrivate::updateBoundingRect(const QRectF &br)
{
    // transform to device coords and adjust for pen width
    Q_ASSERT(engine && engine->painter());
    QPainter *painter = engine->painter();
    const QTransform transform = painter->transform();
    QRectF devRect = transform.mapRect(br);
    if (penWidthAdjustment > 0) {
        devRect = devRect.adjusted(-penWidthAdjustment, -penWidthAdjustment,
                                   penWidthAdjustment, penWidthAdjustment);
    }

    if (boundingRect.isEmpty()) {
        boundingRect = devRect;
    } else {
        qreal min_x = qMin(devRect.left(), boundingRect.left());
        qreal min_y = qMin(devRect.top(), boundingRect.top());
        qreal max_x = qMax(devRect.right(), boundingRect.right());
        qreal max_y = qMax(devRect.bottom(), boundingRect.bottom());
        boundingRect = QRectF(min_x, min_y, max_x - min_x, max_y - min_y);
    }
    if (painter->hasClipping())
        boundingRect &= transform.mapRect(painter->clipRegion().boundingRect());
}


/************************************************************************
 *
 * QPaintBuffer
 *
 ************************************************************************/



QPaintBuffer::QPaintBuffer()
    : d_ptr(new QPaintBufferPrivate)
{
}

QPaintBuffer::~QPaintBuffer()
{
    if (!d_ptr->ref.deref())
        delete d_ptr;
}

QPaintBuffer::QPaintBuffer(const QPaintBuffer &other)
    : QPaintDevice(), d_ptr(other.d_ptr)
{
    d_ptr->ref.ref();
}

QPaintEngine *QPaintBuffer::paintEngine() const
{
    QPaintBufferPrivate *d = const_cast<QPaintBuffer *>(this)->d_ptr;
    if (!d->engine)
        d->engine = new QPaintBufferEngine(d);
    return d->engine;
}


int QPaintBuffer::metric(PaintDeviceMetric metric) const
{
    int val = 0;
    switch (metric) {
    case PdmWidth:
        val = qCeil(d_ptr->boundingRect.width());
        break;
    case PdmHeight:
        val = qCeil(d_ptr->boundingRect.height());
        break;
    case PdmDpiX:
    case PdmPhysicalDpiX:
        val = qt_defaultDpiX();
        break;
    case PdmDpiY:
    case PdmPhysicalDpiY:
        val = qt_defaultDpiY();
        break;
    default:
        val = QPaintDevice::metric(metric);
    }

    return val;
}

int QPaintBuffer::devType() const
{
    return QInternal::PaintBuffer;
}

QPaintBuffer &QPaintBuffer::operator=(const QPaintBuffer &other)
{
    if (other.d_ptr != d_ptr) {
        QPaintBufferPrivate *data = other.d_ptr;
        data->ref.ref();
        if (d_ptr->ref.deref())
            delete d_ptr;
        d_ptr = data;
    }
    return *this;
}

bool QPaintBuffer::isEmpty() const
{
    return d_ptr->commands.isEmpty();
}



void QPaintBuffer::draw(QPainter *painter, int frame) const
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBuffer::draw() --------------------------------";

    Q_D(const QPaintBuffer);
    printf("Float buffer:");
    for (int i=0; i<d->floats.size(); i++) {
        if ((i % 10) == 0) {
            printf("\n%4d-%4d: ", i, i+9);
        }
        printf("%4.2f  ", d->floats[i]);
    }
    printf("\n");

    printf("Int Buffer:");
    for (int i=0; i<d->ints.size(); i++) {
        if ((i % 10) == 0) {
            printf("\n%4d-%4d: ", i, i+10);
        }
        printf("%5d", d->ints[i]);
    }
    printf("\n");
#endif

    processCommands(painter, frameStartIndex(frame), frameEndIndex(frame));

#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBuffer::draw() -------------------------------- DONE!";
#endif
}

int QPaintBuffer::frameStartIndex(int frame) const
{
    return (frame == 0) ? 0 : d_ptr->frames.at(frame - 1);
}

int QPaintBuffer::frameEndIndex(int frame) const
{
    return (frame == d_ptr->frames.size()) ? d_ptr->commands.size() : d_ptr->frames.at(frame);
}

int QPaintBuffer::processCommands(QPainter *painter, int begin, int end) const
{
    if (!painter || !painter->isActive())
        return 0;

    QPaintEngineEx *xengine = painter->paintEngine()->isExtended()
                              ? (QPaintEngineEx *) painter->paintEngine() : 0;
    if (xengine) {
        QPaintEngineExReplayer player;
        player.processCommands(*this, painter, begin, end);
    } else {
        QPainterReplayer player;
        player.processCommands(*this, painter, begin, end);
    }

    int depth = 0;
    for (int i = begin; i < end; ++i) {
        const QPaintBufferCommand &cmd = d_ptr->commands.at(i);
        if (cmd.id == QPaintBufferPrivate::Cmd_Save)
            ++depth;
        else if (cmd.id == QPaintBufferPrivate::Cmd_Restore)
            --depth;
    }
    return depth;
}

#ifndef QT_NO_DEBUG_STREAM
QString QPaintBuffer::commandDescription(int command) const
{
    QString desc;
    QDebug debug(&desc);

    const QPaintBufferCommand &cmd = d_ptr->commands.at(command);

    switch (cmd.id) {
    case QPaintBufferPrivate::Cmd_Save: {
        debug << "Cmd_Save";
        break; }

    case QPaintBufferPrivate::Cmd_Restore: {
        debug << "Cmd_Restore";
        break; }

    case QPaintBufferPrivate::Cmd_SetBrush: {
        QBrush brush = qvariant_cast<QBrush>(d_ptr->variants.at(cmd.offset));
        debug << "Cmd_SetBrush: " << brush;
        break; }

    case QPaintBufferPrivate::Cmd_SetBrushOrigin: {
        debug << "Cmd_SetBrushOrigin: " << d_ptr->variants.at(cmd.offset).toPointF();
        break; }

    case QPaintBufferPrivate::Cmd_SetCompositionMode: {
        QPainter::CompositionMode mode = (QPainter::CompositionMode) cmd.extra;
        debug << "ExCmd_SetCompositionMode, mode: " << mode;
        break; }

    case QPaintBufferPrivate::Cmd_SetOpacity: {
        debug << "ExCmd_SetOpacity: " << d_ptr->variants.at(cmd.offset).toDouble();
        break; }

    case QPaintBufferPrivate::Cmd_DrawVectorPath: {
        debug << "ExCmd_DrawVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2;
        break; }

    case QPaintBufferPrivate::Cmd_StrokeVectorPath: {
        QPen pen = qvariant_cast<QPen>(d_ptr->variants.at(cmd.extra));
        debug << "ExCmd_StrokeVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2 << pen;
        break; }

    case QPaintBufferPrivate::Cmd_FillVectorPath: {
        QBrush brush = qvariant_cast<QBrush>(d_ptr->variants.at(cmd.extra));
        debug << "ExCmd_FillVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2 << brush;
        break; }

    case QPaintBufferPrivate::Cmd_FillRectBrush: {
        QBrush brush = qvariant_cast<QBrush>(d_ptr->variants.at(cmd.extra));
        QRectF *rect = (QRectF *)(d_ptr->floats.constData() + cmd.offset);
        debug << "ExCmd_FillRectBrush, offset: " << cmd.offset << " rect: " << *rect << " brush: " << brush;
        break; }

    case QPaintBufferPrivate::Cmd_FillRectColor: {
        QColor color = qvariant_cast<QColor>(d_ptr->variants.at(cmd.extra));
        QRectF *rect = (QRectF *)(d_ptr->floats.constData() + cmd.offset);
        debug << "ExCmd_FillRectBrush, offset: " << cmd.offset << " rect: " << *rect << " color: " << color;
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolygonF: {
        debug << "ExCmd_DrawPolygonF, offset: " << cmd.offset << " size: " << cmd.size
                 << " mode: " << cmd.extra
                 << d_ptr->floats.at(cmd.offset)
                 << d_ptr->floats.at(cmd.offset+1);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolygonI: {
        debug << "ExCmd_DrawPolygonI, offset: " << cmd.offset << " size: " << cmd.size
                 << " mode: " << cmd.extra
                 << d_ptr->ints.at(cmd.offset)
                 << d_ptr->ints.at(cmd.offset+1);
        break; }

    case QPaintBufferPrivate::Cmd_DrawEllipseF: {
        debug << "ExCmd_DrawEllipseF, offset: " << cmd.offset;
        break; }

    case QPaintBufferPrivate::Cmd_DrawLineF: {
        debug << "ExCmd_DrawLineF, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawLineI: {
        debug << "ExCmd_DrawLineI, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawPointsF: {
        debug << "ExCmd_DrawPointsF, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawPointsI: {
        debug << "ExCmd_DrawPointsI, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolylineF: {
        debug << "ExCmd_DrawPolylineF, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolylineI: {
        debug << "ExCmd_DrawPolylineI, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawRectF: {
        debug << "ExCmd_DrawRectF, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawRectI: {
        debug << "ExCmd_DrawRectI, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_SetClipEnabled: {
        bool clipEnabled = d_ptr->variants.at(cmd.offset).toBool();
        debug << "ExCmd_SetClipEnabled:" << clipEnabled;
        break; }

    case QPaintBufferPrivate::Cmd_ClipVectorPath: {
        QVectorPathCmd path(d_ptr, cmd);
        debug << "ExCmd_ClipVectorPath:" << path().elementCount();
        break; }

    case QPaintBufferPrivate::Cmd_ClipRect: {
        QRect rect(QPoint(d_ptr->ints.at(cmd.offset), d_ptr->ints.at(cmd.offset + 1)),
                   QPoint(d_ptr->ints.at(cmd.offset + 2), d_ptr->ints.at(cmd.offset + 3)));
        debug << "ExCmd_ClipRect:" << rect << cmd.extra;
        break; }

    case QPaintBufferPrivate::Cmd_ClipRegion: {
        QRegion region(d_ptr->variants.at(cmd.offset).value<QRegion>());
        debug << "ExCmd_ClipRegion:" << region.boundingRect() << cmd.extra;
        break; }

    case QPaintBufferPrivate::Cmd_SetPen: {
        QPen pen = qvariant_cast<QPen>(d_ptr->variants.at(cmd.offset));
        debug << "Cmd_SetPen: " << pen;
        break; }

    case QPaintBufferPrivate::Cmd_SetTransform: {
        QTransform xform = qvariant_cast<QTransform>(d_ptr->variants.at(cmd.offset));
        debug << "Cmd_SetTransform, offset: " << cmd.offset << xform;
        break; }

    case QPaintBufferPrivate::Cmd_SetRenderHints: {
        debug << "Cmd_SetRenderHints, hints: " << cmd.extra;
        break; }

    case QPaintBufferPrivate::Cmd_SetBackgroundMode: {
        debug << "Cmd_SetBackgroundMode: " << cmd.extra;
        break; }

    case QPaintBufferPrivate::Cmd_DrawConvexPolygonF: {
        debug << "Cmd_DrawConvexPolygonF, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawConvexPolygonI: {
        debug << "Cmd_DrawConvexPolygonI, offset: " << cmd.offset << " size: " << cmd.size;
        break; }

    case QPaintBufferPrivate::Cmd_DrawEllipseI: {
        debug << "Cmd_DrawEllipseI, offset: " << cmd.offset;
        break; }

    case QPaintBufferPrivate::Cmd_DrawPixmapRect: {
        QPixmap pm(d_ptr->variants.at(cmd.offset).value<QPixmap>());
        QRectF r(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1),
                 d_ptr->floats.at(cmd.extra+2), d_ptr->floats.at(cmd.extra+3));

        QRectF sr(d_ptr->floats.at(cmd.extra+4), d_ptr->floats.at(cmd.extra+5),
                  d_ptr->floats.at(cmd.extra+6), d_ptr->floats.at(cmd.extra+7));
        debug << "Cmd_DrawPixmapRect:" << r << sr << pm.size();
        break; }

    case QPaintBufferPrivate::Cmd_DrawPixmapPos: {
        QPixmap pm(d_ptr->variants.at(cmd.offset).value<QPixmap>());
        QPointF pos(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1));
        debug << "Cmd_DrawPixmapPos:" << pos << pm.size();
        break; }

    case QPaintBufferPrivate::Cmd_DrawTiledPixmap: {
        QPixmap pm(d_ptr->variants.at(cmd.offset).value<QPixmap>());
        QRectF r(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1),
                 d_ptr->floats.at(cmd.extra+2), d_ptr->floats.at(cmd.extra+3));

        QPointF offset(d_ptr->floats.at(cmd.extra+4), d_ptr->floats.at(cmd.extra+5));
        debug << "Cmd_DrawTiledPixmap:" << r << offset << pm.size();
        break; }

    case QPaintBufferPrivate::Cmd_DrawImageRect: {
        QImage image(d_ptr->variants.at(cmd.offset).value<QImage>());
        QRectF r(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1),
                 d_ptr->floats.at(cmd.extra+2), d_ptr->floats.at(cmd.extra+3));
        QRectF sr(d_ptr->floats.at(cmd.extra+4), d_ptr->floats.at(cmd.extra+5),
                  d_ptr->floats.at(cmd.extra+6), d_ptr->floats.at(cmd.extra+7));
        debug << "Cmd_DrawImageRect:" << r << sr << image.size();
        break; }

    case QPaintBufferPrivate::Cmd_DrawImagePos: {
        QImage image(d_ptr->variants.at(cmd.offset).value<QImage>());
        QPointF pos(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1));
        debug << "Cmd_DrawImagePos:" << pos << image.size();
        break; }

    case QPaintBufferPrivate::Cmd_DrawText: {
        QPointF pos(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1));
        QList<QVariant> variants(d_ptr->variants.at(cmd.offset).value<QList<QVariant> >());

        QFont font(variants.at(0).value<QFont>());
        QString text(variants.at(1).value<QString>());

        debug << "Cmd_DrawText:" << pos << text << font.family();
        break; }

    case QPaintBufferPrivate::Cmd_DrawTextItem: {
        QPointF pos(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1));
        QTextItemIntCopy *tiCopy = reinterpret_cast<QTextItemIntCopy *>(qvariant_cast<void *>(d_ptr->variants.at(cmd.offset)));
        QTextItemInt &ti = (*tiCopy)();
        QString text(ti.text());

        debug << "Cmd_DrawTextItem:" << pos << " " << text;
        break; }
    case QPaintBufferPrivate::Cmd_SystemStateChanged: {
        QRegion systemClip(d_ptr->variants.at(cmd.offset).value<QRegion>());

        debug << "Cmd_SystemStateChanged:" << systemClip;
        break; }
    case QPaintBufferPrivate::Cmd_Translate: {
        QPointF delta(d_ptr->floats.at(cmd.extra), d_ptr->floats.at(cmd.extra+1));
        debug << "Cmd_Translate:" << delta;
        break; }
    case QPaintBufferPrivate::Cmd_DrawStaticText: {
        debug << "Cmd_DrawStaticText";
        break; }
    }

    return desc;
}
#endif

QRectF QPaintBuffer::boundingRect() const
{
    return d_ptr->boundingRect;
}

void QPaintBuffer::setBoundingRect(const QRectF &rect)
{
    d_ptr->boundingRect = rect;
    d_ptr->calculateBoundingRect = false;
}


class QPaintBufferEnginePrivate : public QPaintEngineExPrivate
{
    Q_DECLARE_PUBLIC(QPaintBufferEngine)
public:
    void systemStateChanged() {
        Q_Q(QPaintBufferEngine);
        q->buffer->addCommand(QPaintBufferPrivate::Cmd_SystemStateChanged, QVariant(systemClip));
    }

    QTransform last;
};


/************************************************************************
 *
 * QPaintBufferEngine
 *
 ************************************************************************/

QPaintBufferEngine::QPaintBufferEngine(QPaintBufferPrivate *b)
    : QPaintEngineEx(*(new QPaintBufferEnginePrivate))
    , buffer(b)
    , m_begin_detected(false)
    , m_save_detected(false)
    , m_stream_raw_text_items(false)
{
}

bool QPaintBufferEngine::begin(QPaintDevice *)
{
    Q_D(QPaintBufferEngine);
    painter()->save();
    d->systemStateChanged();
    return true;
}

bool QPaintBufferEngine::end()
{
    painter()->restore();
    m_created_state = 0;
    return true;
}

QPainterState *QPaintBufferEngine::createState(QPainterState *orig) const
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: createState, orig=" << orig << ", current=" << state();
#endif

    Q_ASSERT(!m_begin_detected);
    Q_ASSERT(!m_save_detected);

    if (orig == 0) {
        m_begin_detected = true;
        return new QPainterState();
    } else {
        m_save_detected = true;
        return new QPainterState(orig);
    }
}

void QPaintBufferEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: clip vpath:" << path.elementCount() << "op:" << op;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_ClipVectorPath, path);
    cmd->extra = op;
}

void QPaintBufferEngine::clip(const QRect &rect, Qt::ClipOperation op)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: clip rect:" << rect << "op:" << op;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_ClipRect, (int *) &rect, 4, 1);
    cmd->extra = op;
}

void QPaintBufferEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: clip region br:" << region.boundingRect() << "op:" << op;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_ClipRegion, QVariant(region));
    cmd->extra = op;
}

void QPaintBufferEngine::clip(const QPainterPath &path, Qt::ClipOperation op)
{
    // ### TODO
//     QPaintBufferCommand *cmd =
//         buffer->addCommand(QPaintBufferPrivate::Cmd_ClipPath, QVariant(path));
//     cmd->extra = op;
    QPaintEngineEx::clip(path, op);
}

void QPaintBufferEngine::clipEnabledChanged()
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: clip enable change" << state()->clipEnabled;
#endif

    buffer->addCommand(QPaintBufferPrivate::Cmd_SetClipEnabled, state()->clipEnabled);
}

void QPaintBufferEngine::penChanged()
{
    const QPen &pen = state()->pen;

    if (!buffer->commands.isEmpty()
        && buffer->commands.last().id == QPaintBufferPrivate::Cmd_SetPen) {
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: penChanged (compressed)" << state()->pen;
#endif
        buffer->variants[buffer->commands.last().offset] = pen;
        return;
    }

    if (buffer->calculateBoundingRect) {
        if (pen.style() == Qt::NoPen) {
            buffer->penWidthAdjustment = 0;
        } else {
            qreal penWidth = (pen.widthF() == 0) ? 1 : pen.widthF();
            QPointF transformedWidth(penWidth, penWidth);
            if (!pen.isCosmetic())
                transformedWidth = painter()->transform().map(transformedWidth);
            buffer->penWidthAdjustment = transformedWidth.x() / 2.0;
        }
    }
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: penChanged" << state()->pen;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_SetPen, pen);
}

void QPaintBufferEngine::brushChanged()
{
    const QBrush &brush = state()->brush;

    if (!buffer->commands.isEmpty()
        && buffer->commands.last().id == QPaintBufferPrivate::Cmd_SetBrush) {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << "QPaintBufferEngine: brushChanged (compressed)" << state()->brush;
#endif
        buffer->variants[buffer->commands.last().offset] = brush;
        return;
    }

#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: brushChanged" << state()->brush;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_SetBrush, brush);
}

void QPaintBufferEngine::brushOriginChanged()
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: brush origin changed" << state()->brushOrigin;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_SetBrushOrigin, state()->brushOrigin);
}

void QPaintBufferEngine::opacityChanged()
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: opacity changed" << state()->opacity;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_SetOpacity, state()->opacity);
}

void QPaintBufferEngine::compositionModeChanged()
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: composition mode" << state()->composition_mode;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_SetCompositionMode);
    cmd->extra = state()->composition_mode;
}

void QPaintBufferEngine::renderHintsChanged()
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: render hints changed" << state()->renderHints;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_SetRenderHints);
    cmd->extra = state()->renderHints;
}

void QPaintBufferEngine::transformChanged()
{
    Q_D(QPaintBufferEngine);
    const QTransform &transform = state()->matrix;

    QTransform delta;

    bool invertible = false;
    if (transform.type() <= QTransform::TxScale && transform.type() == d->last.type())
        delta = transform * d->last.inverted(&invertible);

    d->last = transform;

    if (invertible && delta.type() == QTransform::TxNone)
        return;

    if (invertible && delta.type() == QTransform::TxTranslate) {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << "QPaintBufferEngine: transformChanged (translate only) " << state()->matrix;
#endif
        QPaintBufferCommand *cmd =
            buffer->addCommand(QPaintBufferPrivate::Cmd_Translate);

        qreal data[] = { delta.dx(), delta.dy() };
        cmd->extra = buffer->addData((qreal *) data, 2);
        return;
    }

    // ### accumulate, like in QBrush case...
    if (!buffer->commands.isEmpty()
        && buffer->commands.last().id == QPaintBufferPrivate::Cmd_SetTransform) {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << "QPaintBufferEngine: transformChanged (compressing) " << state()->matrix;
#endif
        buffer->variants[buffer->commands.last().offset] = state()->matrix;
        return;
    }

#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << "QPaintBufferEngine: transformChanged:" << state()->matrix;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_SetTransform, state()->matrix);
}

void QPaintBufferEngine::backgroundModeChanged()
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintEngineBuffer: background mode changed" << state()->bgMode;
#endif
    QPaintBufferCommand *cmd = buffer->addCommand(QPaintBufferPrivate::Cmd_SetBackgroundMode);
    cmd->extra = state()->bgMode;
}

void QPaintBufferEngine::draw(const QVectorPath &path)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: draw vpath:" << path.elementCount();
#endif

    bool hasBrush = qbrush_style(state()->brush) != Qt::NoBrush;
    bool hasPen = qpen_style(state()->pen) != Qt::NoPen
                  && qbrush_style(qpen_brush(state()->pen)) != Qt::NoBrush;

    if (hasPen || hasBrush)
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawVectorPath, path);
#ifdef QPAINTBUFFER_DEBUG_DRAW
    else
        qDebug() << " - no pen or brush active, discarded...\n";
#endif

//     if (buffer->calculateBoundingRect) {
//         QRealRect r = path.controlPointRect();
//         buffer->updateBoundingRect(QRectF(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1));
//     }
}

void QPaintBufferEngine::fill(const QVectorPath &path, const QBrush &brush)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: fill vpath:" << path.elementCount() << brush;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_FillVectorPath, path);
    cmd->extra = buffer->addData(QVariant(brush));
//     if (buffer->calculateBoundingRect) {
//         QRealRect r = path.controlPointRect();
//         buffer->updateBoundingRect(QRectF(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1));
//     }
}

void QPaintBufferEngine::stroke(const QVectorPath &path, const QPen &pen)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: stroke vpath:" << path.elementCount() << pen;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_StrokeVectorPath, path);
    cmd->extra = buffer->addData(QVariant(pen));
//     if (buffer->calculateBoundingRect) {
//         QRealRect r = path.controlPointRect();
//         buffer->updateBoundingRect(QRectF(r.x1, r.y1, r.x2 - r.x1, r.y2 - r.y1));
//     }
}

void QPaintBufferEngine::fillRect(const QRectF &rect, const QBrush &brush)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: fillRect brush:" << rect << brush;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_FillRectBrush, (qreal *) &rect, 4, 1);
    cmd->extra = buffer->addData(brush);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(rect);
}

void QPaintBufferEngine::fillRect(const QRectF &rect, const QColor &color)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: fillRect color:" << rect << color;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_FillRectColor, (qreal *) &rect, 4, 1);
    cmd->extra = buffer->addData(color);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(rect);
}

void QPaintBufferEngine::drawRects(const QRect *rects, int rectCount)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawRectsI:" << rectCount;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawRectI, (int *) rects, 4 * rectCount, rectCount);
    cmd->extra = rectCount;

    if (buffer->calculateBoundingRect) {
        if (rectCount == 1) {
            buffer->updateBoundingRect(rects[0]);
        } else {
            int min_x = rects[0].left();
            int min_y = rects[0].top();
            int max_x = rects[0].left() + rects[0].width();
            int max_y = rects[0].top() + rects[0].height();
            for (int i=1; i< rectCount; ++i) {
                if (rects[i].left() < min_x)
                    min_x = rects[i].left();
                if (rects[i].top() < min_y)
                    min_y = rects[i].top();
                if (rects[i].right() > max_x)
                    max_x = rects[i].left() + rects[i].width();
                if (rects[i].bottom() > max_y)
                    max_y = rects[i].top() + rects[i].height();

            }
            buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
        }
    }
}

void QPaintBufferEngine::drawRects(const QRectF *rects, int rectCount)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawRectsF:" << rectCount;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawRectF, (qreal *) rects, 4 * rectCount, rectCount);
    cmd->extra = rectCount;

    if (buffer->calculateBoundingRect) {
        if (rectCount == 1) {
            buffer->updateBoundingRect(rects[0]);
        } else {
            qreal min_x = rects[0].left();
            qreal min_y = rects[0].top();
            qreal max_x = rects[0].right();
            qreal max_y = rects[0].bottom();
            for (int i=1; i< rectCount; ++i) {
                if (rects[i].left() < min_x)
                    min_x = rects[i].left();
                if (rects[i].top() < min_y)
                    min_y = rects[i].top();
                if (rects[i].right() > max_x)
                    max_x = rects[i].right();
                if (rects[i].bottom() > max_y)
                    max_y = rects[i].bottom();

            }
            buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
        }
    }
}

void QPaintBufferEngine::drawLines(const QLine *lines, int lineCount)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawLinesI:" << lineCount;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawLineI, (int *) lines, 4 * lineCount, lineCount);
    cmd->extra = lineCount;

    if (buffer->calculateBoundingRect) {
        int min_x = lines[0].p1().x();
        int min_y = lines[0].p1().y();
        int max_x = lines[0].p2().x();
        int max_y = lines[0].p2().y();
        if (min_x > max_x)
            qSwap(min_x, max_x);
        if (min_y > max_y)
            qSwap(min_y, max_y);
        for (int i=1; i < lineCount; ++i) {
            int p1_x = lines[i].p1().x();
            int p1_y = lines[i].p1().y();
            int p2_x = lines[i].p2().x();
            int p2_y = lines[i].p2().y();
            if (p1_x > p2_x) {
                min_x = qMin(p2_x, min_x);
                max_x = qMax(p1_x, max_x);
            } else {
                min_x = qMin(p1_x, min_x);
                max_x = qMax(p2_x, max_x);
            }
            if (p1_y > p2_y) {
                min_y = qMin(p2_y, min_y);
                max_y = qMax(p1_y, max_y);
            } else {
                min_y = qMin(p1_y, min_y);
                max_y = qMax(p2_y, max_y);
            }
        }
        buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
    }
}

void QPaintBufferEngine::drawLines(const QLineF *lines, int lineCount)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawLinesF:" << lineCount;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawLineF, (qreal *) lines, 4 * lineCount, lineCount);
    cmd->extra = lineCount;

    if (buffer->calculateBoundingRect) {
        qreal min_x = lines[0].p1().x();
        qreal min_y = lines[0].p1().y();
        qreal max_x = lines[0].p2().x();
        qreal max_y = lines[0].p2().y();
        if (min_x > max_x)
            qSwap(min_x, max_x);
        if (min_y > max_y)
            qSwap(min_y, max_y);
        for (int i=1; i < lineCount; ++i) {
            qreal p1_x = lines[i].p1().x();
            qreal p1_y = lines[i].p1().y();
            qreal p2_x = lines[i].p2().x();
            qreal p2_y = lines[i].p2().y();
            if (p1_x > p2_x) {
                min_x = qMin(p2_x, min_x);
                max_x = qMax(p1_x, max_x);
            } else {
                min_x = qMin(p1_x, min_x);
                max_x = qMax(p2_x, max_x);
            }
            if (p1_y > p2_y) {
                min_y = qMin(p2_y, min_y);
                max_y = qMax(p1_y, max_y);
            } else {
                min_y = qMin(p1_y, min_y);
                max_y = qMax(p2_y, max_y);
            }
        }
        buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
    }
}

void QPaintBufferEngine::drawEllipse(const QRectF &r)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawEllipseF:" << r;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_DrawEllipseF, (qreal *) &r, 4, 1);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(r);
}

void QPaintBufferEngine::drawEllipse(const QRect &r)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawEllipseI:" << r;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_DrawEllipseI, (int *) &r, 4, 1);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(r);
}

void QPaintBufferEngine::drawPath(const QPainterPath &path)
{
// #ifdef QPAINTBUFFER_DEBUG_DRAW
//     qDebug() << "QPaintBufferEngine: drawPath: element count:" << path.elementCount();
// #endif
//     // ### Path -> QVariant
//     // buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPath, QVariant(path));
    QPaintEngineEx::drawPath(path);

//     if (buffer->calculateBoundingRect)
//         buffer->updateBoundingRect(path.boundingRect());
}

void QPaintBufferEngine::drawPoints(const QPoint *points, int pointCount)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawPointsI: " << pointCount;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPointsI, (int *) points, 2 * pointCount, pointCount);

    if (buffer->calculateBoundingRect) {
        int min_x = points[0].x();
        int min_y = points[0].y();
        int max_x = points[0].x()+1;
        int max_y = points[0].y()+1;
        for (int i=1; i<pointCount; ++i) {
            int x = points[i].x();
            int y = points[i].y();
            min_x = qMin(min_x, x);
            min_y = qMin(min_y, y);
            max_x = qMax(max_x, x+1);
            max_y = qMax(max_y, y+1);
        }
        buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
    }
}

void QPaintBufferEngine::drawPoints(const QPointF *points, int pointCount)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawPointsF: " << pointCount;
#endif
    buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPointsF, (qreal *) points, 2 * pointCount, pointCount);

    if (buffer->calculateBoundingRect) {
        qreal min_x = points[0].x();
        qreal min_y = points[0].y();
        qreal max_x = points[0].x()+1;
        qreal max_y = points[0].y()+1;
        for (int i=1; i<pointCount; ++i) {
            qreal x = points[i].x();
            qreal y = points[i].y();
            min_x = qMin(min_x, x);
            min_y = qMin(min_y, y);
            max_x = qMax(max_x, x+1);
            max_y = qMax(max_y, y+1);
        }
        buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
    }
}

void QPaintBufferEngine::drawPolygon(const QPoint *pts, int count, PolygonDrawMode mode)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawPolygonI: size:" << count << ", mode:" << mode;
#endif
    if (mode == QPaintEngine::OddEvenMode || mode == QPaintEngine::WindingMode) {
        QPaintBufferCommand *cmd = buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPolygonI,
                                                      (int *) pts, 2 * count, count);
        cmd->extra = mode;
    } else if (mode == QPaintEngine::PolylineMode) {
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPolylineI, (int *) pts, 2 * count, count);
    } else {
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawConvexPolygonI, (int *) pts, 2 * count, count);
    }

    if (buffer->calculateBoundingRect) {
        int min_x = pts[0].x();
        int min_y = pts[0].y();
        int max_x = pts[0].x();
        int max_y = pts[0].y();
        for (int i=1; i<count; ++i) {
            int x = pts[i].x();
            int y = pts[i].y();
            min_x = qMin(min_x, x);
            min_y = qMin(min_y, y);
            max_x = qMax(max_x, x);
            max_y = qMax(max_y, y);
        }
        buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
    }
}

void QPaintBufferEngine::drawPolygon(const QPointF *pts, int count, PolygonDrawMode mode)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawPolygonF: size:" << count << ", mode:" << mode;
#endif
    if (mode == QPaintEngine::OddEvenMode || mode == QPaintEngine::WindingMode) {
        QPaintBufferCommand *cmd = buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPolygonF,
                                                      (qreal *) pts, 2 * count, count);
        cmd->extra = mode;
    } else if (mode == QPaintEngine::PolylineMode) {
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPolylineF, (qreal *) pts, 2 * count, count);
    } else {
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawConvexPolygonF, (qreal *) pts, 2 * count, count);
    }

    if (buffer->calculateBoundingRect) {
        qreal min_x = pts[0].x();
        qreal min_y = pts[0].y();
        qreal max_x = pts[0].x();
        qreal max_y = pts[0].y();
        for (int i=1; i<count; ++i) {
            qreal x = pts[i].x();
            qreal y = pts[i].y();
            min_x = qMin(min_x, x);
            min_y = qMin(min_y, y);
            max_x = qMax(max_x, x);
            max_y = qMax(max_y, y);
        }
        buffer->updateBoundingRect(QRectF(min_x, min_y, max_x - min_x, max_y - min_y));
    }
}

void QPaintBufferEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawPixmap: src/dest rects " << r << sr;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPixmapRect, QVariant(pm));
    cmd->extra = buffer->addData((qreal *) &r, 4);
    buffer->addData((qreal *) &sr, 4);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(r);
}

void QPaintBufferEngine::drawPixmap(const QPointF &pos, const QPixmap &pm)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawPixmap: pos:" << pos;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawPixmapPos, QVariant(pm));
    cmd->extra = buffer->addData((qreal *) &pos, 2);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(QRectF(pos, pm.size()));
}

static inline QImage qpaintbuffer_storable_image(const QImage &src)
{
    QImageData *d = const_cast<QImage &>(src).data_ptr();
    return d->own_data ? src : src.copy();
}

void QPaintBufferEngine::drawImage(const QRectF &r, const QImage &image, const QRectF &sr,
                                   Qt::ImageConversionFlags /*flags */)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawImage: src/dest rects " << r << sr;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawImageRect,
                           QVariant(qpaintbuffer_storable_image(image)));
    cmd->extra = buffer->addData((qreal *) &r, 4);
    buffer->addData((qreal *) &sr, 4);
    // ### flags...
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(r);
}

void QPaintBufferEngine::drawImage(const QPointF &pos, const QImage &image)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawImage: pos:" << pos;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawImagePos,
                           QVariant(qpaintbuffer_storable_image(image)));
    cmd->extra = buffer->addData((qreal *) &pos, 2);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(QRectF(pos, image.size()));
}

void QPaintBufferEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &s)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawTiledPixmap: src rect/offset:" << r << s;
#endif
    QPaintBufferCommand *cmd =
        buffer->addCommand(QPaintBufferPrivate::Cmd_DrawTiledPixmap, QVariant(pm));
    cmd->extra = buffer->addData((qreal *) &r, 4);
    buffer->addData((qreal *) &s, 2);
    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(r);
}

void QPaintBufferEngine::drawStaticTextItem(QStaticTextItem *staticTextItem)
{
    QVariantList variants;

    variants << QVariant(staticTextItem->font);
    for (int i=0; i<staticTextItem->numGlyphs; ++i) {
        variants.append(staticTextItem->glyphs[i]);
        variants.append(staticTextItem->glyphPositions[i].toPointF());
    }

    buffer->addCommand(QPaintBufferPrivate::Cmd_DrawStaticText, QVariant(variants));
}

void QPaintBufferEngine::drawTextItem(const QPointF &pos, const QTextItem &ti)
{
#ifdef QPAINTBUFFER_DEBUG_DRAW
    qDebug() << "QPaintBufferEngine: drawTextItem: pos:" << pos << ti.text();
#endif
    if (m_stream_raw_text_items) {
        QPaintBufferCommand *cmd = buffer->addCommand(QPaintBufferPrivate::Cmd_DrawTextItem, QVariant::fromValue<void *>(new QTextItemIntCopy(ti)));

        QFont font(ti.font());
        font.setUnderline(false);
        font.setStrikeOut(false);
        font.setOverline(false);

        const QTextItemInt &si = static_cast<const QTextItemInt &>(ti);
        qreal justificationWidth = 0;
        if (si.justified)
            justificationWidth = si.width.toReal();
        int renderFlags = ti.renderFlags();
        qreal scaleFactor = font.d->dpi/qreal(qt_defaultDpiY());

        buffer->addData(QVariant(font));
        cmd->extra = buffer->addData((qreal *) &pos, 2);
        buffer->addData((qreal *) &justificationWidth, 1);
        buffer->addData((qreal *) &scaleFactor, 1);
        cmd->offset2 = buffer->addData((int *) &renderFlags, 1);
    } else {
        QList<QVariant> variants;
        variants << QVariant(ti.font()) << QVariant(ti.text());
        QPaintBufferCommand *cmd = buffer->addCommand(QPaintBufferPrivate::Cmd_DrawText, QVariant(variants));
        cmd->extra = buffer->addData((qreal *) &pos, 2);
    }

    if (buffer->calculateBoundingRect)
        buffer->updateBoundingRect(QRectF(pos, QSize(ti.width(), ti.ascent() + ti.descent() + 1)));
}


void QPaintBufferEngine::setState(QPainterState *s)
{
    Q_D(QPaintBufferEngine);
    if (m_begin_detected) {
#ifdef QPAINTBUFFER_DEBUG_DRAW
            qDebug() << "QPaintBufferEngine: setState: begin, ignoring.";
#endif
        m_begin_detected = false;
    } else if (m_save_detected) {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << "QPaintBufferEngine: setState: save.";
#endif
        m_save_detected = false;
        buffer->addCommand(QPaintBufferPrivate::Cmd_Save);
    } else {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << "QPaintBufferEngine: setState: restore.";
#endif
        buffer->addCommand(QPaintBufferPrivate::Cmd_Restore);
    }

    d->last = s->matrix;

    QPaintEngineEx::setState(s);
}


/***********************************************************************
 *
 * class QPaintBufferPlayback_Painter
 *
 */

// QFakeDevice is used to create fonts with a custom DPI
//
class QFakeDevice : public QPaintDevice
{
public:
    QFakeDevice() { dpi_x = qt_defaultDpiX(); dpi_y = qt_defaultDpiY(); }
    void setDpiX(int dpi) { dpi_x = dpi; }
    void setDpiY(int dpi) { dpi_y = dpi; }
    QPaintEngine *paintEngine() const { return 0; }
    int metric(PaintDeviceMetric m) const
    {
        switch(m) {
            case PdmPhysicalDpiX:
            case PdmDpiX:
                return dpi_x;
            case PdmPhysicalDpiY:
            case PdmDpiY:
                return dpi_y;
            default:
                return QPaintDevice::metric(m);
        }
    }

private:
    int dpi_x;
    int dpi_y;
};


void QPainterReplayer::setupTransform(QPainter *_painter)
{
    painter = _painter;
    m_world_matrix = painter->transform();
    m_world_matrix.scale(qreal(painter->device()->logicalDpiX()) / qreal(qt_defaultDpiX()),
                         qreal(painter->device()->logicalDpiY()) / qreal(qt_defaultDpiY()));
    painter->setTransform(m_world_matrix);
}

void QPainterReplayer::processCommands(const QPaintBuffer &buffer, QPainter *p, int begin, int end)
{
    d = buffer.d_ptr;
    painter = p;

    for (int cmdIndex = begin; cmdIndex < end; ++cmdIndex) {
        const QPaintBufferCommand &cmd = d->commands.at(cmdIndex);
        process(cmd);
    }
}

void QPaintBuffer::beginNewFrame()
{
    if (!d_ptr->commands.isEmpty())
        d_ptr->frames << d_ptr->commands.size();
}

int QPaintBuffer::numFrames() const
{
    return d_ptr->frames.size() + 1;
}

void QPainterReplayer::process(const QPaintBufferCommand &cmd)
{
    switch (cmd.id) {
    case QPaintBufferPrivate::Cmd_Save: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_Save";
#endif
        painter->save();
        break; }

    case QPaintBufferPrivate::Cmd_Restore: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_Restore";
#endif
        painter->restore();
        break; }

    case QPaintBufferPrivate::Cmd_SetPen: {
        QPen pen = qvariant_cast<QPen>(d->variants.at(cmd.offset));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetPen: " << pen;
#endif
        painter->setPen(pen);
        break; }

    case QPaintBufferPrivate::Cmd_SetBrush: {
        QBrush brush = qvariant_cast<QBrush>(d->variants.at(cmd.offset));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetBrush: " << brush;
#endif
        painter->setBrush(brush);
        break; }

    case QPaintBufferPrivate::Cmd_SetBrushOrigin: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetBrushOrigin: " << d->variants.at(cmd.offset).toPointF();
#endif
        painter->setBrushOrigin(d->variants.at(cmd.offset).toPointF());
        break; }

    case QPaintBufferPrivate::Cmd_SetTransform: {
        QTransform xform = qvariant_cast<QTransform>(d->variants.at(cmd.offset));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetTransform, offset: " << cmd.offset << xform;
#endif
        painter->setTransform(xform * m_world_matrix);
        break; }

    case QPaintBufferPrivate::Cmd_Translate: {
        QPointF delta(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_Translate, offset: " << cmd.offset << delta;
#endif
        painter->translate(delta.x(), delta.y());
        return;
    }

    case QPaintBufferPrivate::Cmd_SetCompositionMode: {
        QPainter::CompositionMode mode = (QPainter::CompositionMode) cmd.extra;
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetCompositionMode, mode: " << mode;
#endif
        painter->setCompositionMode(mode);
        break; }

    case QPaintBufferPrivate::Cmd_SetRenderHints: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetRenderHints, hints: " << cmd.extra;
#endif
        QPainter::RenderHints ph = painter->renderHints();
        QPainter::RenderHints nh = (QPainter::RenderHints) cmd.extra;
        QPainter::RenderHints xored = ph ^ nh;
        if (xored & QPainter::Antialiasing)
            painter->setRenderHint(QPainter::Antialiasing, nh & QPainter::Antialiasing);
        if (xored & QPainter::HighQualityAntialiasing)
            painter->setRenderHint(QPainter::HighQualityAntialiasing, nh & QPainter::HighQualityAntialiasing);
        if (xored & QPainter::TextAntialiasing)
            painter->setRenderHint(QPainter::TextAntialiasing, nh & QPainter::TextAntialiasing);
        if (xored & QPainter::SmoothPixmapTransform)
            painter->setRenderHint(QPainter::SmoothPixmapTransform, nh & QPainter::SmoothPixmapTransform);
        if (xored & QPainter::NonCosmeticDefaultPen)
            painter->setRenderHint(QPainter::NonCosmeticDefaultPen, nh & QPainter::NonCosmeticDefaultPen);
        break; }

    case QPaintBufferPrivate::Cmd_SetOpacity: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetOpacity: " << d->variants.at(cmd.offset).toDouble();
#endif
        painter->setOpacity(d->variants.at(cmd.offset).toDouble());
        break; }

    case QPaintBufferPrivate::Cmd_SetBackgroundMode: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetBackgroundMode: " << cmd.extra;
#endif
        painter->setBackgroundMode((Qt::BGMode)cmd.extra);
        break; }

    case QPaintBufferPrivate::Cmd_DrawVectorPath: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2;
#endif
        QVectorPathCmd path(d, cmd);
        painter->drawPath(path().convertToPainterPath());
        break; }

    case QPaintBufferPrivate::Cmd_StrokeVectorPath: {
        QPen pen = qvariant_cast<QPen>(d->variants.at(cmd.extra));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_StrokeVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2;
#endif
        QVectorPathCmd path(d, cmd);
        painter->strokePath(path().convertToPainterPath(), pen);
        break; }

    case QPaintBufferPrivate::Cmd_FillVectorPath: {
        QBrush brush = qvariant_cast<QBrush>(d->variants.at(cmd.extra));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_FillVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2 << brush;
#endif
        QVectorPathCmd path(d, cmd);
        painter->fillPath(path().convertToPainterPath(), brush);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolygonF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPolygonF, offset: " << cmd.offset << " size: " << cmd.size
                 << " mode: " << cmd.extra
                 << d->floats.at(cmd.offset)
                 << d->floats.at(cmd.offset+1);
#endif
        Qt::FillRule fill = (QPaintEngine::PolygonDrawMode) cmd.extra == QPaintEngine::OddEvenMode
                            ? Qt::OddEvenFill : Qt::WindingFill;
        painter->drawPolygon((QPointF *) (d->floats.constData() + cmd.offset), cmd.size, fill);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolygonI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPolygonI, offset: " << cmd.offset << " size: " << cmd.size
                 << " mode: " << cmd.extra
                 << d->ints.at(cmd.offset)
                 << d->ints.at(cmd.offset+1);
#endif
        Qt::FillRule fill = (QPaintEngine::PolygonDrawMode) cmd.extra == QPaintEngine::OddEvenMode
                            ? Qt::OddEvenFill : Qt::WindingFill;
        painter->drawPolygon((QPoint *) (d->ints.constData() + cmd.offset), cmd.size, fill);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolylineF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPolylineF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawPolyline((QPointF *) (d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolylineI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPolylineI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawPolyline((QPoint *) (d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawConvexPolygonF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawConvexPolygonF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawConvexPolygon((QPointF *) (d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawConvexPolygonI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawConvexPolygonI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawConvexPolygon((QPoint *) (d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawEllipseF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawEllipseF, offset: " << cmd.offset;
#endif
        painter->drawEllipse(*(QRectF *)(d->floats.constData() + cmd.offset));
        break; }

    case QPaintBufferPrivate::Cmd_DrawEllipseI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawEllipseI, offset: " << cmd.offset;
#endif
        painter->drawEllipse(*(QRect *)(d->ints.constData() + cmd.offset));
        break; }

    case QPaintBufferPrivate::Cmd_DrawLineF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawLineF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawLines((QLineF *)(d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawLineI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawLineI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawLines((QLine *)(d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPointsF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPointsF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawPoints((QPointF *)(d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPointsI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPointsI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        painter->drawPoints((QPoint *)(d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPixmapRect: {
        QPixmap pm(d->variants.at(cmd.offset).value<QPixmap>());
        QRectF r(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1),
                 d->floats.at(cmd.extra+2), d->floats.at(cmd.extra+3));

        QRectF sr(d->floats.at(cmd.extra+4), d->floats.at(cmd.extra+5),
                  d->floats.at(cmd.extra+6), d->floats.at(cmd.extra+7));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPixmapRect:" << r << sr;
#endif
        painter->drawPixmap(r, pm, sr);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPixmapPos: {
        QPixmap pm(d->variants.at(cmd.offset).value<QPixmap>());
        QPointF pos(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawPixmapPos:" << pos;
#endif
        painter->drawPixmap(pos, pm);
        break; }

    case QPaintBufferPrivate::Cmd_DrawTiledPixmap: {
        QPixmap pm(d->variants.at(cmd.offset).value<QPixmap>());
        QRectF r(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1),
                 d->floats.at(cmd.extra+2), d->floats.at(cmd.extra+3));

        QPointF offset(d->floats.at(cmd.extra+4), d->floats.at(cmd.extra+5));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawTiledPixmap:" << r << offset;
#endif
        painter->drawTiledPixmap(r, pm, offset);
        break; }

    case QPaintBufferPrivate::Cmd_DrawImageRect: {
        QImage image(d->variants.at(cmd.offset).value<QImage>());
        QRectF r(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1),
                 d->floats.at(cmd.extra+2), d->floats.at(cmd.extra+3));
        QRectF sr(d->floats.at(cmd.extra+4), d->floats.at(cmd.extra+5),
                  d->floats.at(cmd.extra+6), d->floats.at(cmd.extra+7));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawImageRect:" << r << sr;
#endif
        painter->drawImage(r, image, sr);
        break; }

    case QPaintBufferPrivate::Cmd_DrawImagePos: {
        QImage image(d->variants.at(cmd.offset).value<QImage>());
        QPointF pos(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawImagePos:" << pos;
#endif
        painter->drawImage(pos, image);
        break; }

    case QPaintBufferPrivate::Cmd_DrawRectF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawRectF, offset: " << cmd.offset;
#endif
        painter->drawRects((QRectF *)(d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawRectI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawRectI, offset: " << cmd.offset;
#endif
        painter->drawRects((QRect *)(d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_FillRectBrush: {
        QBrush brush = qvariant_cast<QBrush>(d->variants.at(cmd.extra));
        QRectF *rect = (QRectF *)(d->floats.constData() + cmd.offset);
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_FillRectBrush, offset: " << cmd.offset << " rect: " << *rect << " brush: " << brush;
#endif
        painter->fillRect(*rect, brush);
        break; }

    case QPaintBufferPrivate::Cmd_FillRectColor: {
        QColor color = qvariant_cast<QColor>(d->variants.at(cmd.extra));
        QRectF *rect = (QRectF *)(d->floats.constData() + cmd.offset);
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_FillRectBrush, offset: " << cmd.offset << " rect: " << *rect << " color: " << color;
#endif
        painter->fillRect(*rect, color);
        break; }

    case QPaintBufferPrivate::Cmd_SetClipEnabled: {
        bool clipEnabled = d->variants.at(cmd.offset).toBool();
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SetClipEnabled:" << clipEnabled;
#endif
        painter->setClipping(clipEnabled);
        break; }

    case QPaintBufferPrivate::Cmd_ClipVectorPath: {
        QVectorPathCmd path(d, cmd);
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_ClipVectorPath:" << path().elementCount();
#endif
        painter->setClipPath(path().convertToPainterPath(), Qt::ClipOperation(cmd.extra));
        break; }


    case QPaintBufferPrivate::Cmd_ClipRect: {
        QRect rect(QPoint(d->ints.at(cmd.offset), d->ints.at(cmd.offset + 1)),
                   QPoint(d->ints.at(cmd.offset + 2), d->ints.at(cmd.offset + 3)));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_ClipRect:" << rect << cmd.extra;
#endif
        painter->setClipRect(rect, Qt::ClipOperation(cmd.extra));
        break; }

    case QPaintBufferPrivate::Cmd_ClipRegion: {
        QRegion region(d->variants.at(cmd.offset).value<QRegion>());
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_ClipRegion:" << region.boundingRect() << cmd.extra;
#endif
        painter->setClipRegion(region, Qt::ClipOperation(cmd.extra));
        break; }
        
#if !defined(QT_NO_RAWFONT)
    case QPaintBufferPrivate::Cmd_DrawStaticText: {
            
            QVariantList variants(d->variants.at(cmd.offset).value<QVariantList>());
            
            QFont font = variants.at(0).value<QFont>();

            QVector<quint32> glyphIndexes;
            QVector<QPointF> positions;

            for (int i=0; i<(variants.size() - 1) / 2; ++i) {
                glyphIndexes.append(variants.at(i*2 + 1).toUInt());
                positions.append(variants.at(i*2 + 2).toPointF());
            }

            painter->setFont(font);

            QRawFont rawFont;
            QRawFontPrivate *rawFontD = QRawFontPrivate::get(rawFont);
            QFontPrivate *fontD = QFontPrivate::get(font);
            rawFontD->fontEngine = fontD->engineForScript(QUnicodeTables::Common);
            rawFontD->fontEngine->ref.ref();

            QGlyphRun glyphs;
            glyphs.setRawFont(rawFont);
            glyphs.setGlyphIndexes(glyphIndexes);
            glyphs.setPositions(positions);

            painter->drawGlyphRun(QPointF(), glyphs);
            break;
    }
#endif

    case QPaintBufferPrivate::Cmd_DrawText: {
        QPointF pos(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1));
        QList<QVariant> variants(d->variants.at(cmd.offset).value<QList<QVariant> >());

        QFont font(variants.at(0).value<QFont>());
        QString text(variants.at(1).value<QString>());

        painter->setFont(font);
        painter->drawText(pos, text);
        break; }

    case QPaintBufferPrivate::Cmd_DrawTextItem: {
        QPointF pos(d->floats.at(cmd.extra), d->floats.at(cmd.extra+1));
        QTextItemIntCopy *tiCopy = reinterpret_cast<QTextItemIntCopy *>(qvariant_cast<void *>(d->variants.at(cmd.offset)));
        QTextItemInt &ti = (*tiCopy)();
        QString text(ti.text());

        QFont font(ti.font());
        font.setUnderline(false);
        font.setStrikeOut(false);
        font.setOverline(false);

        const QTextItemInt &si = static_cast<const QTextItemInt &>(ti);
        qreal justificationWidth = 0;
        if (si.justified)
            justificationWidth = si.width.toReal();
        qreal scaleFactor = font.d->dpi/qreal(qt_defaultDpiY());

#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_DrawTextItem:" << pos << " " << text << " " << scaleFactor;
#endif

        if (scaleFactor != 1.0) {
            QFont fnt(font);
            QFakeDevice fake;
            fake.setDpiX(qRound(scaleFactor*qt_defaultDpiX()));
            fake.setDpiY(qRound(scaleFactor*qt_defaultDpiY()));
            font = QFont(fnt, &fake);
        }

        int flags = Qt::TextSingleLine | Qt::TextDontClip | Qt::TextForceLeftToRight;
        QSizeF size(1, 1);
        if (justificationWidth > 0) {
            size.setWidth(justificationWidth);
            flags |= Qt::TextJustificationForced;
            flags |= Qt::AlignJustify;
        }

        QFontMetrics fm(font);
        QPointF pt(pos.x(), pos.y() - fm.ascent());
        qt_format_text(font, QRectF(pt, size), flags, /*opt*/0,
                       text, /*brect=*/0, /*tabstops=*/0, /*...*/0, /*tabarraylen=*/0, painter);
        break; }
    case QPaintBufferPrivate::Cmd_SystemStateChanged: {
        QRegion systemClip(d->variants.at(cmd.offset).value<QRegion>());

#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> Cmd_SystemStateChanged:" << systemClip;
#endif

        painter->paintEngine()->setSystemClip(systemClip);
        painter->paintEngine()->d_ptr->systemStateChanged();
        break; }
    }
}

void QPaintEngineExReplayer::process(const QPaintBufferCommand &cmd)
{
    Q_ASSERT(painter->paintEngine()->isExtended());
    QPaintEngineEx *xengine = static_cast<QPaintEngineEx *>(painter->paintEngine());

    switch (cmd.id) {
    case QPaintBufferPrivate::Cmd_SetBrushOrigin: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_SetBrushOrigin: " << d->variants.at(cmd.offset).toPointF();
#endif
        xengine->state()->brushOrigin = d->variants.at(cmd.offset).toPointF();
        xengine->brushOriginChanged();
        break; }

    case QPaintBufferPrivate::Cmd_SetCompositionMode: {
        QPainter::CompositionMode mode = (QPainter::CompositionMode) cmd.extra;
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_SetCompositionMode, mode: " << mode;
#endif
        xengine->state()->composition_mode = mode;
        xengine->compositionModeChanged();
        break; }

    case QPaintBufferPrivate::Cmd_SetOpacity: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_SetOpacity: " << d->variants.at(cmd.offset).toDouble();
#endif
        xengine->state()->opacity = d->variants.at(cmd.offset).toDouble();
        xengine->opacityChanged();
        break; }

    case QPaintBufferPrivate::Cmd_DrawVectorPath: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2;
#endif
        QVectorPathCmd path(d, cmd);
        xengine->draw(path());
        break; }

    case QPaintBufferPrivate::Cmd_StrokeVectorPath: {
        QPen pen = qvariant_cast<QPen>(d->variants.at(cmd.extra));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_StrokeVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2;
#endif
        QVectorPathCmd path(d, cmd);
        xengine->stroke(path(), pen);
        break; }

    case QPaintBufferPrivate::Cmd_FillVectorPath: {
        QBrush brush = qvariant_cast<QBrush>(d->variants.at(cmd.extra));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_FillVectorPath: size: " << cmd.size
//                 << ", hints:" << d->ints[cmd.offset2+cmd.size]
                 << "pts/elms:" << cmd.offset << cmd.offset2 << brush;
#endif
        QVectorPathCmd path(d, cmd);
        xengine->fill(path(), brush);
        break; }

    case QPaintBufferPrivate::Cmd_FillRectBrush: {
        QBrush brush = qvariant_cast<QBrush>(d->variants.at(cmd.extra));
        QRectF *rect = (QRectF *)(d->floats.constData() + cmd.offset);
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_FillRectBrush, offset: " << cmd.offset << " rect: " << *rect << " brush: " << brush;
#endif
        xengine->fillRect(*rect, brush);
        break; }

    case QPaintBufferPrivate::Cmd_FillRectColor: {
        QColor color = qvariant_cast<QColor>(d->variants.at(cmd.extra));
        QRectF *rect = (QRectF *)(d->floats.constData() + cmd.offset);
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_FillRectBrush, offset: " << cmd.offset << " rect: " << *rect << " color: " << color;
#endif
        xengine->fillRect(*rect, color);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolygonF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawPolygonF, offset: " << cmd.offset << " size: " << cmd.size
                 << " mode: " << cmd.extra
                 << d->floats.at(cmd.offset)
                 << d->floats.at(cmd.offset+1);
#endif
        xengine->drawPolygon((QPointF *) (d->floats.constData() + cmd.offset), cmd.size,
                             (QPaintEngine::PolygonDrawMode) cmd.extra);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolygonI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawPolygonI, offset: " << cmd.offset << " size: " << cmd.size
                 << " mode: " << cmd.extra
                 << d->ints.at(cmd.offset)
                 << d->ints.at(cmd.offset+1);
#endif
        xengine->drawPolygon((QPoint *) (d->ints.constData() + cmd.offset), cmd.size,
                             (QPaintEngine::PolygonDrawMode) cmd.extra);
        break; }

    case QPaintBufferPrivate::Cmd_DrawEllipseF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawEllipseF, offset: " << cmd.offset;
#endif
        xengine->drawEllipse(*(QRectF *)(d->floats.constData() + cmd.offset));
        break; }

    case QPaintBufferPrivate::Cmd_DrawEllipseI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawEllipseI, offset: " << cmd.offset;
#endif
        xengine->drawEllipse(*(QRect *)(d->ints.constData() + cmd.offset));
        break; }

    case QPaintBufferPrivate::Cmd_DrawLineF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawLineF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawLines((QLineF *)(d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawLineI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawLineI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawLines((QLine *)(d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPointsF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawPointsF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawPoints((QPointF *)(d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPointsI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawPointsI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawPoints((QPoint *)(d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolylineF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawPolylineF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawPolygon((QPointF *) (d->floats.constData() + cmd.offset), cmd.size, QPaintEngine::PolylineMode);
        break; }

    case QPaintBufferPrivate::Cmd_DrawPolylineI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawPolylineI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawPolygon((QPoint *) (d->ints.constData() + cmd.offset), cmd.size, QPaintEngine::PolylineMode);
        break; }

    case QPaintBufferPrivate::Cmd_DrawRectF: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawRectF, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawRects((QRectF *) (d->floats.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_DrawRectI: {
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_DrawRectI, offset: " << cmd.offset << " size: " << cmd.size;
#endif
        xengine->drawRects((QRect *) (d->ints.constData() + cmd.offset), cmd.size);
        break; }

    case QPaintBufferPrivate::Cmd_SetClipEnabled: {
        bool clipEnabled = d->variants.at(cmd.offset).toBool();
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_SetClipEnabled:" << clipEnabled;
#endif
        xengine->state()->clipEnabled = clipEnabled;
        xengine->clipEnabledChanged();
        break; }

    case QPaintBufferPrivate::Cmd_ClipVectorPath: {
        QVectorPathCmd path(d, cmd);
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_ClipVectorPath:" << path().elementCount();
#endif
        xengine->clip(path(), Qt::ClipOperation(cmd.extra));
        break; }


    case QPaintBufferPrivate::Cmd_ClipRect: {
        QRect rect(QPoint(d->ints.at(cmd.offset), d->ints.at(cmd.offset + 1)),
                   QPoint(d->ints.at(cmd.offset + 2), d->ints.at(cmd.offset + 3)));
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_ClipRect:" << rect << cmd.extra;
#endif
        xengine->clip(rect, Qt::ClipOperation(cmd.extra));
        break; }

    case QPaintBufferPrivate::Cmd_ClipRegion: {
        QRegion region(d->variants.at(cmd.offset).value<QRegion>());
#ifdef QPAINTBUFFER_DEBUG_DRAW
        qDebug() << " -> ExCmd_ClipRegion:" << region.boundingRect() << cmd.extra;
#endif
        xengine->clip(region, Qt::ClipOperation(cmd.extra));
        break; }

    default:
        QPainterReplayer::process(cmd);
        break;
    }
}

QPaintBufferResource::QPaintBufferResource(FreeFunc f, QObject *parent) : QObject(parent), free(f)
{
    connect(QPaintBufferSignalProxy::instance(), SIGNAL(aboutToDestroy(const QPaintBufferPrivate*)), this, SLOT(remove(const QPaintBufferPrivate*)));
}

QPaintBufferResource::~QPaintBufferResource()
{
    for (Cache::iterator it = m_cache.begin(); it != m_cache.end(); ++it)
        free(it.value());
}

void QPaintBufferResource::insert(const QPaintBufferPrivate *key, void *value)
{
    Cache::iterator it = m_cache.find(key);
    if (it != m_cache.end()) {
        free(it.value());
        it.value() = value;
    } else {
        m_cache.insert(key, value);
    }
}

void *QPaintBufferResource::value(const QPaintBufferPrivate *key)
{
    Cache::iterator it = m_cache.find(key);
    if (it != m_cache.end())
        return it.value();
    return 0;
}

void QPaintBufferResource::remove(const QPaintBufferPrivate *key)
{
    Cache::iterator it = m_cache.find(key);
    if (it != m_cache.end()) {
        free(it.value());
        m_cache.erase(it);
    }
}

QDataStream &operator<<(QDataStream &stream, const QPaintBufferCommand &command)
{
    quint32 id = command.id;
    quint32 size = command.size;
    stream << id << size;
    stream << command.offset << command.offset2 << command.extra;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QPaintBufferCommand &command)
{
    quint32 id;
    quint32 size;
    stream >> id >> size;
    stream >> command.offset >> command.offset2 >> command.extra;
    command.id = id;
    command.size = size;
    return stream;
}

struct QPaintBufferCacheEntry
{
    QVariant::Type type;
    quint64 cacheKey;
};

struct QPaintBufferCacheEntryV2
{
    enum Type {
        ImageKey,
        PixmapKey
    };

    struct Flags {
        uint type : 8;
        uint key : 24;
    };

    union {
        Flags flags;
        uint bits;
    };
};

QT_END_NAMESPACE
Q_DECLARE_METATYPE(QPaintBufferCacheEntry)
Q_DECLARE_METATYPE(QPaintBufferCacheEntryV2)
QT_BEGIN_NAMESPACE

QDataStream &operator<<(QDataStream &stream, const QPaintBufferCacheEntry &entry)
{
    return stream << entry.type << entry.cacheKey;
}

QDataStream &operator>>(QDataStream &stream, QPaintBufferCacheEntry &entry)
{
    return stream >> entry.type >> entry.cacheKey;
}

QDataStream &operator<<(QDataStream &stream, const QPaintBufferCacheEntryV2 &entry)
{
    return stream << entry.bits;
}

QDataStream &operator>>(QDataStream &stream, QPaintBufferCacheEntryV2 &entry)
{
    return stream >> entry.bits;
}

static int qRegisterPaintBufferMetaTypes()
{
    qRegisterMetaType<QPaintBufferCacheEntry>();
    qRegisterMetaTypeStreamOperators<QPaintBufferCacheEntry>("QPaintBufferCacheEntry");
    qRegisterMetaType<QPaintBufferCacheEntryV2>();
    qRegisterMetaTypeStreamOperators<QPaintBufferCacheEntryV2>("QPaintBufferCacheEntryV2");

    return 0; // something
}

Q_CONSTRUCTOR_FUNCTION(qRegisterPaintBufferMetaTypes)

QDataStream &operator<<(QDataStream &stream, const QPaintBuffer &buffer)
{
    QHash<qint64, uint> pixmapKeys;
    QHash<qint64, uint> imageKeys;

    QHash<qint64, QPixmap> pixmaps;
    QHash<qint64, QImage> images;

    QVector<QVariant> variants = buffer.d_ptr->variants;
    for (int i = 0; i < variants.size(); ++i) {
        const QVariant &v = variants.at(i);
        if (v.type() == QVariant::Image) {
            const QImage image(v.value<QImage>());

            QPaintBufferCacheEntryV2 entry;
            entry.flags.type = QPaintBufferCacheEntryV2::ImageKey;

            QHash<qint64, uint>::iterator it = imageKeys.find(image.cacheKey());
            if (it != imageKeys.end()) {
                entry.flags.key = *it;
            } else {
                imageKeys[image.cacheKey()] = entry.flags.key = images.size();
                images[images.size()] = image;
            }

            variants[i] = QVariant::fromValue(entry);
        } else if (v.type() == QVariant::Pixmap) {
            const QPixmap pixmap(v.value<QPixmap>());

            QPaintBufferCacheEntryV2 entry;
            entry.flags.type = QPaintBufferCacheEntryV2::PixmapKey;

            QHash<qint64, uint>::iterator it = pixmapKeys.find(pixmap.cacheKey());
            if (it != pixmapKeys.end()) {
                entry.flags.key = *it;
            } else {
                pixmapKeys[pixmap.cacheKey()] = entry.flags.key = pixmaps.size();
                pixmaps[pixmaps.size()] = pixmap;
            }

            variants[i] = QVariant::fromValue(entry);
        }
    }

    stream << pixmaps;
    stream << images;

    stream << buffer.d_ptr->ints;
    stream << buffer.d_ptr->floats;
    stream << variants;
    stream << buffer.d_ptr->commands;
    stream << buffer.d_ptr->boundingRect;
    stream << buffer.d_ptr->frames;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, QPaintBuffer &buffer)
{
    QHash<qint64, QPixmap> pixmaps;
    QHash<qint64, QImage> images;

    stream >> pixmaps;
    stream >> images;

    stream >> buffer.d_ptr->ints;
    stream >> buffer.d_ptr->floats;
    stream >> buffer.d_ptr->variants;
    stream >> buffer.d_ptr->commands;
    stream >> buffer.d_ptr->boundingRect;
    stream >> buffer.d_ptr->frames;

    QVector<QVariant> &variants = buffer.d_ptr->variants;
    for (int i = 0; i < variants.size(); ++i) {
        const QVariant &v = variants.at(i);
        if (v.canConvert<QPaintBufferCacheEntry>()) {
            QPaintBufferCacheEntry entry = v.value<QPaintBufferCacheEntry>();
            if (entry.type == QVariant::Image)
                variants[i] = QVariant(images.value(entry.cacheKey));
            else
                variants[i] = QVariant(pixmaps.value(entry.cacheKey));
        } else if (v.canConvert<QPaintBufferCacheEntryV2>()) {
            QPaintBufferCacheEntryV2 entry = v.value<QPaintBufferCacheEntryV2>();

            if (entry.flags.type == QPaintBufferCacheEntryV2::ImageKey)
                variants[i] = QVariant(images.value(entry.flags.key));
            else if (entry.flags.type == QPaintBufferCacheEntryV2::PixmapKey)
                variants[i] = QVariant(pixmaps.value(entry.flags.key));
            else
                qWarning() << "operator<<(QDataStream &stream, QPaintBuffer &buffer): unrecognized cache entry type:" << entry.flags.type;
        }
    }

    return stream;
}

QT_END_NAMESPACE
