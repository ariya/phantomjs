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

#include "qpdf_p.h"

#ifndef QT_NO_PDF

#include "qplatformdefs.h"
#include <qdebug.h>
#include <qfile.h>
#include <qtemporaryfile.h>
#include <private/qmath_p.h>
#include <private/qpainter_p.h>
#include <qnumeric.h>
#include "private/qfont_p.h"
#include <qimagewriter.h>
#include "qbuffer.h"
#include "QtCore/qdatetime.h"

#ifndef QT_NO_COMPRESS
#include <zlib.h>
#endif

#ifdef QT_NO_COMPRESS
static const bool do_compress = false;
#else
static const bool do_compress = true;
#endif

// might be helpful for smooth transforms of images
// Can't use it though, as gs generates completely wrong images if this is true.
static const bool interpolateImages = false;

QT_BEGIN_NAMESPACE

inline QPaintEngine::PaintEngineFeatures qt_pdf_decide_features()
{
    QPaintEngine::PaintEngineFeatures f = QPaintEngine::AllFeatures;
    f &= ~(QPaintEngine::PorterDuff | QPaintEngine::PerspectiveTransform
           | QPaintEngine::ObjectBoundingModeGradients
#ifndef USE_NATIVE_GRADIENTS
           | QPaintEngine::LinearGradientFill
#endif
           | QPaintEngine::RadialGradientFill
           | QPaintEngine::ConicalGradientFill);
    return f;
}



/* also adds a space at the end of the number */
const char *qt_real_to_string(qreal val, char *buf) {
    const char *ret = buf;

    if (qIsNaN(val)) {
        *(buf++) = '0';
        *(buf++) = ' ';
        *buf = 0;
        return ret;
    }

    if (val < 0) {
        *(buf++) = '-';
        val = -val;
    }
    unsigned int ival = (unsigned int) val;
    qreal frac = val - (qreal)ival;

    int ifrac = (int)(frac * 1000000000);
    if (ifrac == 1000000000) {
        ++ival;
        ifrac = 0;
    }
    char output[256];
    int i = 0;
    while (ival) {
        output[i] = '0' + (ival % 10);
        ++i;
        ival /= 10;
    }
    int fact = 100000000;
    if (i == 0) {
        *(buf++) = '0';
    } else {
        while (i) {
            *(buf++) = output[--i];
            fact /= 10;
            ifrac /= 10;
        }
    }

    if (ifrac) {
        *(buf++) =  '.';
        while (fact) {
            *(buf++) = '0' + ((ifrac/fact) % 10);
            fact /= 10;
        }
    }
    *(buf++) = ' ';
    *buf = 0;
    return ret;
}

const char *qt_int_to_string(int val, char *buf) {
    const char *ret = buf;
    if (val < 0) {
        *(buf++) = '-';
        val = -val;
    }
    char output[256];
    int i = 0;
    while (val) {
        output[i] = '0' + (val % 10);
        ++i;
        val /= 10;
    }
    if (i == 0) {
        *(buf++) = '0';
    } else {
        while (i)
            *(buf++) = output[--i];
    }
    *(buf++) = ' ';
    *buf = 0;
    return ret;
}


namespace QPdf {
    ByteStream::ByteStream(QByteArray *byteArray, bool fileBacking)
            : dev(new QBuffer(byteArray)),
            fileBackingEnabled(fileBacking),
            fileBackingActive(false),
            handleDirty(false)
    {
        dev->open(QIODevice::ReadWrite | QIODevice::Append);
    }

    ByteStream::ByteStream(bool fileBacking)
            : dev(new QBuffer(&ba)),
            fileBackingEnabled(fileBacking),
            fileBackingActive(false),
            handleDirty(false)
    {
        dev->open(QIODevice::ReadWrite);
    }

    ByteStream::~ByteStream()
    {
        delete dev;
    }

    ByteStream &ByteStream::operator <<(char chr)
    {
        if (handleDirty) prepareBuffer();
        dev->write(&chr, 1);
        return *this;
    }

    ByteStream &ByteStream::operator <<(const char *str)
    {
        if (handleDirty) prepareBuffer();
        dev->write(str, strlen(str));
        return *this;
    }

    ByteStream &ByteStream::operator <<(const QByteArray &str)
    {
        if (handleDirty) prepareBuffer();
        dev->write(str);
        return *this;
    }

    ByteStream &ByteStream::operator <<(const ByteStream &src)
    {
        Q_ASSERT(!src.dev->isSequential());
        if (handleDirty) prepareBuffer();
        // We do play nice here, even though it looks ugly.
        // We save the position and restore it afterwards.
        ByteStream &s = const_cast<ByteStream&>(src);
        qint64 pos = s.dev->pos();
        s.dev->reset();
        while (!s.dev->atEnd()) {
            QByteArray buf = s.dev->read(chunkSize());
            dev->write(buf);
        }
        s.dev->seek(pos);
        return *this;
    }

    ByteStream &ByteStream::operator <<(qreal val) {
        char buf[256];
        qt_real_to_string(val, buf);
        *this << buf;
        return *this;
    }

    ByteStream &ByteStream::operator <<(int val) {
        char buf[256];
        qt_int_to_string(val, buf);
        *this << buf;
        return *this;
    }

    ByteStream &ByteStream::operator <<(const QPointF &p) {
        char buf[256];
        qt_real_to_string(p.x(), buf);
        *this << buf;
        qt_real_to_string(p.y(), buf);
        *this << buf;
        return *this;
    }

    QIODevice *ByteStream::stream()
    {
        dev->reset();
        handleDirty = true;
        return dev;
    }

    void ByteStream::clear()
    {
        dev->open(QIODevice::ReadWrite | QIODevice::Truncate);
    }

    void ByteStream::constructor_helper(QByteArray *ba)
    {
        delete dev;
        dev = new QBuffer(ba);
        dev->open(QIODevice::ReadWrite);
    }

    void ByteStream::prepareBuffer()
    {
        Q_ASSERT(!dev->isSequential());
        qint64 size = dev->size();
        if (fileBackingEnabled && !fileBackingActive
                && size > maxMemorySize()) {
            // Switch to file backing.
            QTemporaryFile *newFile = new QTemporaryFile;
            newFile->open();
            dev->reset();
            while (!dev->atEnd()) {
                QByteArray buf = dev->read(chunkSize());
                newFile->write(buf);
            }
            delete dev;
            dev = newFile;
            ba.clear();
            fileBackingActive = true;
        }
        if (dev->pos() != size) {
            dev->seek(size);
            handleDirty = false;
        }
    }
}

#define QT_PATH_ELEMENT(elm)

QByteArray QPdf::generatePath(const QPainterPath &path, const QTransform &matrix, PathFlags flags)
{
    QByteArray result;
    if (!path.elementCount())
        return result;

    ByteStream s(&result);

    int start = -1;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &elm = path.elementAt(i);
        switch (elm.type) {
        case QPainterPath::MoveToElement:
            if (start >= 0
                && path.elementAt(start).x == path.elementAt(i-1).x
                && path.elementAt(start).y == path.elementAt(i-1).y)
                s << "h\n";
            s << matrix.map(QPointF(elm.x, elm.y)) << "m\n";
            start = i;
                break;
        case QPainterPath::LineToElement:
            s << matrix.map(QPointF(elm.x, elm.y)) << "l\n";
            break;
        case QPainterPath::CurveToElement:
            Q_ASSERT(path.elementAt(i+1).type == QPainterPath::CurveToDataElement);
            Q_ASSERT(path.elementAt(i+2).type == QPainterPath::CurveToDataElement);
            s << matrix.map(QPointF(elm.x, elm.y))
              << matrix.map(QPointF(path.elementAt(i+1).x, path.elementAt(i+1).y))
              << matrix.map(QPointF(path.elementAt(i+2).x, path.elementAt(i+2).y))
              << "c\n";
            i += 2;
            break;
        default:
            qFatal("QPdf::generatePath(), unhandled type: %d", elm.type);
        }
    }
    if (start >= 0
        && path.elementAt(start).x == path.elementAt(path.elementCount()-1).x
        && path.elementAt(start).y == path.elementAt(path.elementCount()-1).y)
        s << "h\n";

    Qt::FillRule fillRule = path.fillRule();

    const char *op = "";
    switch (flags) {
    case ClipPath:
        op = (fillRule == Qt::WindingFill) ? "W n\n" : "W* n\n";
        break;
    case FillPath:
        op = (fillRule == Qt::WindingFill) ? "f\n" : "f*\n";
        break;
    case StrokePath:
        op = "S\n";
        break;
    case FillAndStrokePath:
        op = (fillRule == Qt::WindingFill) ? "B\n" : "B*\n";
        break;
    }
    s << op;
    return result;
}

QByteArray QPdf::generateMatrix(const QTransform &matrix)
{
    QByteArray result;
    ByteStream s(&result);
    s << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy()
      << "cm\n";
    return result;
}

QByteArray QPdf::generateDashes(const QPen &pen)
{
    QByteArray result;
    ByteStream s(&result);
    s << '[';

    QVector<qreal> dasharray = pen.dashPattern();
    qreal w = pen.widthF();
    if (w < 0.001)
        w = 1;
    for (int i = 0; i < dasharray.size(); ++i) {
        qreal dw = dasharray.at(i)*w;
        if (dw < 0.0001) dw = 0.0001;
        s << dw;
    }
    s << ']';
    //qDebug() << "dasharray: pen has" << dasharray;
    //qDebug() << "  => " << result;
    return result;
}



static const char* pattern_for_brush[] = {
    0, // NoBrush
    0, // SolidPattern
    "0 J\n"
    "6 w\n"
    "[] 0 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // Dense1Pattern

    "0 J\n"
    "2 w\n"
    "[6 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[] 0 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[6 2] -3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense2Pattern

    "0 J\n"
    "2 w\n"
    "[6 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] -1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[6 2] -3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense3Pattern

    "0 J\n"
    "2 w\n"
    "[2 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] -1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[2 2] 1 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense4Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] 1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[2 6] 3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense5Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 6] 3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense6Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n", // Dense7Pattern

    "1 w\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // HorPattern

    "1 w\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // VerPattern

    "1 w\n"
    "4 0 m\n"
    "4 8 l\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // CrossPattern

    "1 w\n"
    "-1 5 m\n"
    "5 -1 l\n"
    "3 9 m\n"
    "9 3 l\n"
    "S\n", // BDiagPattern

    "1 w\n"
    "-1 3 m\n"
    "5 9 l\n"
    "3 -1 m\n"
    "9 5 l\n"
    "S\n", // FDiagPattern

    "1 w\n"
    "-1 3 m\n"
    "5 9 l\n"
    "3 -1 m\n"
    "9 5 l\n"
    "-1 5 m\n"
    "5 -1 l\n"
    "3 9 m\n"
    "9 3 l\n"
    "S\n", // DiagCrossPattern
};

QByteArray QPdf::patternForBrush(const QBrush &b)
{
    int style = b.style();
    if (style > Qt::DiagCrossPattern)
        return QByteArray();
    return pattern_for_brush[style];
}

#ifdef USE_NATIVE_GRADIENTS
static void writeTriangleLine(uchar *&data, int xpos, int ypos, int xoff, int yoff, uint rgb, uchar flag, bool alpha)
{
    data[0] =  flag;
    data[1] = (uchar)(xpos >> 16);
    data[2] = (uchar)(xpos >> 8);
    data[3] = (uchar)(xpos >> 0);
    data[4] = (uchar)(ypos >> 16);
    data[5] = (uchar)(ypos >> 8);
    data[6] = (uchar)(ypos >> 0);
    data += 7;
    if (alpha) {
        *data++ = (uchar)qAlpha(rgb);
    } else {
        *data++ = (uchar)qRed(rgb);
        *data++ = (uchar)qGreen(rgb);
        *data++ = (uchar)qBlue(rgb);
    }
    xpos += xoff;
    ypos += yoff;
    data[0] =  flag;
    data[1] = (uchar)(xpos >> 16);
    data[2] = (uchar)(xpos >> 8);
    data[3] = (uchar)(xpos >> 0);
    data[4] = (uchar)(ypos >> 16);
    data[5] = (uchar)(ypos >> 8);
    data[6] = (uchar)(ypos >> 0);
    data += 7;
    if (alpha) {
        *data++ = (uchar)qAlpha(rgb);
    } else {
        *data++ = (uchar)qRed(rgb);
        *data++ = (uchar)qGreen(rgb);
        *data++ = (uchar)qBlue(rgb);
    }
}


QByteArray QPdf::generateLinearGradientShader(const QLinearGradient *gradient, const QPointF *page_rect, bool alpha)
{
    // generate list of triangles with colors
    QPointF start = gradient->start();
    QPointF stop = gradient->finalStop();
    QGradientStops stops = gradient->stops();
    QPointF offset = stop - start;
    QGradient::Spread spread = gradient->spread();

    if (gradient->spread() == QGradient::ReflectSpread) {
        offset *= 2;
        for (int i = stops.size() - 2; i >= 0; --i) {
            QGradientStop stop = stops.at(i);
            stop.first = 2. - stop.first;
            stops.append(stop);
        }
        for (int i = 0 ; i < stops.size(); ++i)
            stops[i].first /= 2.;
    }

    QPointF orthogonal(offset.y(), -offset.x());
    qreal length = offset.x()*offset.x() + offset.y()*offset.y();

    // find the max and min values in offset and orth direction that are needed to cover
    // the whole page
    int off_min = INT_MAX;
    int off_max = INT_MIN;
    qreal ort_min = INT_MAX;
    qreal ort_max = INT_MIN;
    for (int i = 0; i < 4; ++i) {
        qreal off = ((page_rect[i].x() - start.x()) * offset.x() + (page_rect[i].y() - start.y()) * offset.y())/length;
        qreal ort = ((page_rect[i].x() - start.x()) * orthogonal.x() + (page_rect[i].y() - start.y()) * orthogonal.y())/length;
        off_min = qMin(off_min, qFloor(off));
        off_max = qMax(off_max, qCeil(off));
        ort_min = qMin(ort_min, ort);
        ort_max = qMax(ort_max, ort);
    }
    ort_min -= 1;
    ort_max += 1;

    start += off_min * offset + ort_min * orthogonal;
    orthogonal *= (ort_max - ort_min);
    int num = off_max - off_min;

    QPointF gradient_rect[4] = { start,
                                 start + orthogonal,
                                 start + num*offset,
                                 start + num*offset + orthogonal };
    qreal xmin = gradient_rect[0].x();
    qreal xmax = gradient_rect[0].x();
    qreal ymin = gradient_rect[0].y();
    qreal ymax = gradient_rect[0].y();
    for (int i = 1; i < 4; ++i) {
        xmin = qMin(xmin, gradient_rect[i].x());
        xmax = qMax(xmax, gradient_rect[i].x());
        ymin = qMin(ymin, gradient_rect[i].y());
        ymax = qMax(ymax, gradient_rect[i].y());
    }
    xmin -= 1000;
    xmax += 1000;
    ymin -= 1000;
    ymax += 1000;
    start -= QPointF(xmin, ymin);
    qreal factor_x = qreal(1<<24)/(xmax - xmin);
    qreal factor_y = qreal(1<<24)/(ymax - ymin);
    int xoff = (int)(orthogonal.x()*factor_x);
    int yoff = (int)(orthogonal.y()*factor_y);

    QByteArray triangles;
    triangles.resize(spread == QGradient::PadSpread ? 20*(stops.size()+2) : 20*num*stops.size());
    uchar *data = (uchar *) triangles.data();
    if (spread == QGradient::PadSpread) {
        if (off_min > 0 || off_max < 1) {
            // linear gradient outside of page
            const QGradientStop &current_stop = off_min > 0 ? stops.at(stops.size()-1) : stops.at(0);
            uint rgb = current_stop.second.rgba();
            int xpos = (int)(start.x()*factor_x);
            int ypos = (int)(start.y()*factor_y);
            writeTriangleLine(data, xpos, ypos, xoff, yoff, rgb, 0, alpha);
            start += num*offset;
            xpos = (int)(start.x()*factor_x);
            ypos = (int)(start.y()*factor_y);
            writeTriangleLine(data, xpos, ypos, xoff, yoff, rgb, 1, alpha);
        } else {
            int flag = 0;
            if (off_min < 0) {
                uint rgb = stops.at(0).second.rgba();
                int xpos = (int)(start.x()*factor_x);
                int ypos = (int)(start.y()*factor_y);
                writeTriangleLine(data, xpos, ypos, xoff, yoff, rgb, flag, alpha);
                start -= off_min*offset;
                flag = 1;
            }
            for (int s = 0; s < stops.size(); ++s) {
                const QGradientStop &current_stop = stops.at(s);
                uint rgb = current_stop.second.rgba();
                int xpos = (int)(start.x()*factor_x);
                int ypos = (int)(start.y()*factor_y);
                writeTriangleLine(data, xpos, ypos, xoff, yoff, rgb, flag, alpha);
                if (s < stops.size()-1)
                    start += offset*(stops.at(s+1).first - stops.at(s).first);
                flag = 1;
            }
            if (off_max > 1) {
                start += (off_max - 1)*offset;
                uint rgb = stops.at(stops.size()-1).second.rgba();
                int xpos = (int)(start.x()*factor_x);
                int ypos = (int)(start.y()*factor_y);
                writeTriangleLine(data, xpos, ypos, xoff, yoff, rgb, flag, alpha);
            }
        }
    } else {
        for (int i = 0; i < num; ++i) {
            uchar flag = 0;
            for (int s = 0; s < stops.size(); ++s) {
                uint rgb = stops.at(s).second.rgba();
                int xpos = (int)(start.x()*factor_x);
                int ypos = (int)(start.y()*factor_y);
                writeTriangleLine(data, xpos, ypos, xoff, yoff, rgb, flag, alpha);
                if (s < stops.size()-1)
                    start += offset*(stops.at(s+1).first - stops.at(s).first);
                flag = 1;
            }
        }
    }
    triangles.resize((char *)data - triangles.constData());

    QByteArray shader;
    QPdf::ByteStream s(&shader);
    s << "<<\n"
        "/ShadingType 4\n"
        "/ColorSpace " << (alpha ? "/DeviceGray\n" : "/DeviceRGB\n") <<
        "/AntiAlias true\n"
        "/BitsPerCoordinate 24\n"
        "/BitsPerComponent 8\n"
        "/BitsPerFlag 8\n"
        "/Decode [" << xmin << xmax << ymin << ymax << (alpha ? "0 1]\n" : "0 1 0 1 0 1]\n") <<
        "/AntiAlias true\n"
        "/Length " << triangles.length() << "\n"
        ">>\n"
        "stream\n" << triangles << "endstream\n"
        "endobj\n";
    return shader;
}
#endif

static void moveToHook(qfixed x, qfixed y, void *data)
{
    QPdf::Stroker *t = (QPdf::Stroker *)data;
    if (!t->first)
        *t->stream << "h\n";
    if (!t->cosmeticPen)
        t->matrix.map(x, y, &x, &y);
    *t->stream << x << y << "m\n";
    t->first = false;
}

static void lineToHook(qfixed x, qfixed y, void *data)
{
    QPdf::Stroker *t = (QPdf::Stroker *)data;
    if (!t->cosmeticPen)
        t->matrix.map(x, y, &x, &y);
    *t->stream << x << y << "l\n";
}

static void cubicToHook(qfixed c1x, qfixed c1y,
                        qfixed c2x, qfixed c2y,
                        qfixed ex, qfixed ey,
                        void *data)
{
    QPdf::Stroker *t = (QPdf::Stroker *)data;
    if (!t->cosmeticPen) {
        t->matrix.map(c1x, c1y, &c1x, &c1y);
        t->matrix.map(c2x, c2y, &c2x, &c2y);
        t->matrix.map(ex, ey, &ex, &ey);
    }
    *t->stream << c1x << c1y
               << c2x << c2y
               << ex << ey
               << "c\n";
}

QPdf::Stroker::Stroker()
    : stream(0),
    first(true),
    dashStroker(&basicStroker)
{
    stroker = &basicStroker;
    basicStroker.setMoveToHook(moveToHook);
    basicStroker.setLineToHook(lineToHook);
    basicStroker.setCubicToHook(cubicToHook);
    cosmeticPen = true;
    basicStroker.setStrokeWidth(.1);
}

void QPdf::Stroker::setPen(const QPen &pen, QPainter::RenderHints hints)
{
    if (pen.style() == Qt::NoPen) {
        stroker = 0;
        return;
    }
    qreal w = pen.widthF();
    bool zeroWidth = w < 0.0001;
    cosmeticPen = qt_pen_is_cosmetic(pen, hints);
    if (zeroWidth)
        w = .1;

    basicStroker.setStrokeWidth(w);
    basicStroker.setCapStyle(pen.capStyle());
    basicStroker.setJoinStyle(pen.joinStyle());
    basicStroker.setMiterLimit(pen.miterLimit());

    QVector<qreal> dashpattern = pen.dashPattern();
    if (zeroWidth) {
        for (int i = 0; i < dashpattern.size(); ++i)
            dashpattern[i] *= 10.;
    }
    if (!dashpattern.isEmpty()) {
        dashStroker.setDashPattern(dashpattern);
        dashStroker.setDashOffset(pen.dashOffset());
        stroker = &dashStroker;
    } else {
        stroker = &basicStroker;
    }
}

void QPdf::Stroker::strokePath(const QPainterPath &path)
{
    if (!stroker)
        return;
    first = true;

    stroker->strokePath(path, this, cosmeticPen ? matrix : QTransform());
    *stream << "h f\n";
}

QByteArray QPdf::ascii85Encode(const QByteArray &input)
{
    int isize = input.size()/4*4;
    QByteArray output;
    output.resize(input.size()*5/4+7);
    char *out = output.data();
    const uchar *in = (const uchar *)input.constData();
    for (int i = 0; i < isize; i += 4) {
        uint val = (((uint)in[i])<<24) + (((uint)in[i+1])<<16) + (((uint)in[i+2])<<8) + (uint)in[i+3];
        if (val == 0) {
            *out = 'z';
            ++out;
        } else {
            char base[5];
            base[4] = val % 85;
            val /= 85;
            base[3] = val % 85;
            val /= 85;
            base[2] = val % 85;
            val /= 85;
            base[1] = val % 85;
            val /= 85;
            base[0] = val % 85;
            *(out++) = base[0] + '!';
            *(out++) = base[1] + '!';
            *(out++) = base[2] + '!';
            *(out++) = base[3] + '!';
            *(out++) = base[4] + '!';
        }
    }
    //write the last few bytes
    int remaining = input.size() - isize;
    if (remaining) {
        uint val = 0;
        for (int i = isize; i < input.size(); ++i)
            val = (val << 8) + in[i];
        val <<= 8*(4-remaining);
        char base[5];
        base[4] = val % 85;
        val /= 85;
        base[3] = val % 85;
        val /= 85;
        base[2] = val % 85;
        val /= 85;
        base[1] = val % 85;
        val /= 85;
        base[0] = val % 85;
        for (int i = 0; i < remaining+1; ++i)
            *(out++) = base[i] + '!';
    }
    *(out++) = '~';
    *(out++) = '>';
    output.resize(out-output.data());
    return output;
}

const char *QPdf::toHex(ushort u, char *buffer)
{
    int i = 3;
    while (i >= 0) {
        ushort hex = (u & 0x000f);
        if (hex < 0x0a)
            buffer[i] = '0'+hex;
        else
            buffer[i] = 'A'+(hex-0x0a);
        u = u >> 4;
        i--;
    }
    buffer[4] = '\0';
    return buffer;
}

const char *QPdf::toHex(uchar u, char *buffer)
{
    int i = 1;
    while (i >= 0) {
        ushort hex = (u & 0x000f);
        if (hex < 0x0a)
            buffer[i] = '0'+hex;
        else
            buffer[i] = 'A'+(hex-0x0a);
        u = u >> 4;
        i--;
    }
    buffer[2] = '\0';
    return buffer;
}


QPdfPage::QPdfPage()
    : QPdf::ByteStream(true) // Enable file backing
{
}

void QPdfPage::streamImage(int w, int h, int object)
{
    *this << w << "0 0 " << -h << "0 " << h << "cm /Im" << object << " Do\n";
    if (!images.contains(object))
        images.append(object);
}


QPdfEngine::QPdfEngine(QPdfEnginePrivate &dd)
    : QPaintEngine(dd, qt_pdf_decide_features())
{
}

QPdfEngine::QPdfEngine()
    : QPaintEngine(*new QPdfEnginePrivate(), qt_pdf_decide_features())
{
}

void QPdfEngine::setOutputFilename(const QString &filename)
{
    Q_D(QPdfEngine);
    d->outputFileName = filename;
}


void QPdfEngine::drawPoints (const QPointF *points, int pointCount)
{
    if (!points)
        return;

    Q_D(QPdfEngine);
    QPainterPath p;
    for (int i=0; i!=pointCount;++i) {
        p.moveTo(points[i]);
        p.lineTo(points[i] + QPointF(0, 0.001));
    }

    bool hadBrush = d->hasBrush;
    d->hasBrush = false;
    drawPath(p);
    d->hasBrush = hadBrush;
}

void QPdfEngine::drawLines (const QLineF *lines, int lineCount)
{
    if (!lines)
        return;

    Q_D(QPdfEngine);
    QPainterPath p;
    for (int i=0; i!=lineCount;++i) {
        p.moveTo(lines[i].p1());
        p.lineTo(lines[i].p2());
    }
    bool hadBrush = d->hasBrush;
    d->hasBrush = false;
    drawPath(p);
    d->hasBrush = hadBrush;
}

void QPdfEngine::drawRects (const QRectF *rects, int rectCount)
{
    if (!rects)
        return;

    Q_D(QPdfEngine);

    if (d->clipEnabled && d->allClipped)
        return;
    if (!d->hasPen && !d->hasBrush)
        return;

    QBrush penBrush = d->pen.brush();
    if (d->simplePen || !d->hasPen) {
        // draw strokes natively in this case for better output
        if(!d->simplePen && !d->stroker.matrix.isIdentity())
            *d->currentPage << "q\n" << QPdf::generateMatrix(d->stroker.matrix);
        for (int i = 0; i < rectCount; ++i)
            *d->currentPage << rects[i].x() << rects[i].y() << rects[i].width() << rects[i].height() << "re\n";
        *d->currentPage << (d->hasPen ? (d->hasBrush ? "B\n" : "S\n") : "f\n");
        if(!d->simplePen && !d->stroker.matrix.isIdentity())
            *d->currentPage << "Q\n";
    } else {
        QPainterPath p;
        for (int i=0; i!=rectCount; ++i)
            p.addRect(rects[i]);
        drawPath(p);
    }
}

void QPdfEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QPdfEngine);

    if (!points || !pointCount)
        return;

    bool hb = d->hasBrush;
    QPainterPath p;

    switch(mode) {
        case OddEvenMode:
            p.setFillRule(Qt::OddEvenFill);
            break;
        case ConvexMode:
        case WindingMode:
            p.setFillRule(Qt::WindingFill);
            break;
        case PolylineMode:
            d->hasBrush = false;
            break;
        default:
            break;
    }

    p.moveTo(points[0]);
    for (int i = 1; i < pointCount; ++i)
        p.lineTo(points[i]);

    if (mode != PolylineMode)
        p.closeSubpath();
    drawPath(p);

    d->hasBrush = hb;
}

void QPdfEngine::drawPath (const QPainterPath &p)
{
    Q_D(QPdfEngine);

    if (d->clipEnabled && d->allClipped)
        return;
    if (!d->hasPen && !d->hasBrush)
        return;

    if (d->simplePen && d->opacity == 1.0) {
        // draw strokes natively in this case for better output
        *d->currentPage << QPdf::generatePath(p, QTransform(), d->hasBrush ? QPdf::FillAndStrokePath : QPdf::StrokePath);
    } else {
        if (d->hasBrush)
            *d->currentPage << QPdf::generatePath(p, d->stroker.matrix, QPdf::FillPath);
        if (d->hasPen) {
            *d->currentPage << "q\n";
            QBrush b = d->brush;
            d->brush = d->pen.brush();
            setBrush();
            d->stroker.strokePath(p);
            *d->currentPage << "Q\n";
            d->brush = b;
        }
    }
}

void QPdfEngine::drawPixmap (const QRectF &rectangle, const QPixmap &pixmap, const QRectF &sr)
{
    if (sr.isEmpty() || rectangle.isEmpty() || pixmap.isNull())
        return;
    Q_D(QPdfEngine);

    QBrush b = d->brush;

    QRect sourceRect = sr.toRect();
    QPixmap pm = sourceRect != pixmap.rect() ? pixmap.copy(sourceRect) : pixmap;
    QImage image = pm.toImage();
    bool bitmap = true;
    const int object = d->addImage(image, &bitmap, pm.cacheKey());
    if (object < 0)
        return;

    *d->currentPage << "q\n/GSa gs\n";
    *d->currentPage
        << QPdf::generateMatrix(QTransform(rectangle.width() / sr.width(), 0, 0, rectangle.height() / sr.height(),
                                           rectangle.x(), rectangle.y()) * (d->simplePen ? QTransform() : d->stroker.matrix));
    if (bitmap) {
        // set current pen as d->brush
        d->brush = d->pen.brush();
    }
    setBrush();
    d->currentPage->streamImage(image.width(), image.height(), object);
    *d->currentPage << "Q\n";

    d->brush = b;
}

void QPdfEngine::drawImage(const QRectF &rectangle, const QImage &image, const QRectF &sr, Qt::ImageConversionFlags)
{
    if (sr.isEmpty() || rectangle.isEmpty() || image.isNull())
        return;
    Q_D(QPdfEngine);

    QRect sourceRect = sr.toRect();
    QImage im = sourceRect != image.rect() ? image.copy(sourceRect) : image;
    bool bitmap = true;
    const int object = d->addImage(im, &bitmap, im.cacheKey());
    if (object < 0)
        return;

    *d->currentPage << "q\n/GSa gs\n";
    *d->currentPage
        << QPdf::generateMatrix(QTransform(rectangle.width() / sr.width(), 0, 0, rectangle.height() / sr.height(),
                                           rectangle.x(), rectangle.y()) * (d->simplePen ? QTransform() : d->stroker.matrix));
    setBrush();
    d->currentPage->streamImage(im.width(), im.height(), object);
    *d->currentPage << "Q\n";
}

void QPdfEngine::drawTiledPixmap (const QRectF &rectangle, const QPixmap &pixmap, const QPointF &point)
{
    Q_D(QPdfEngine);

    bool bitmap = (pixmap.depth() == 1);
    QBrush b = d->brush;
    QPointF bo = d->brushOrigin;
    bool hp = d->hasPen;
    d->hasPen = false;
    bool hb = d->hasBrush;
    d->hasBrush = true;

    d->brush = QBrush(pixmap);
    if (bitmap)
        // #### fix bitmap case where we have a brush pen
        d->brush.setColor(d->pen.color());

    d->brushOrigin = -point;
    *d->currentPage << "q\n";
    setBrush();

    drawRects(&rectangle, 1);
    *d->currentPage << "Q\n";

    d->hasPen = hp;
    d->hasBrush = hb;
    d->brush = b;
    d->brushOrigin = bo;
}

void QPdfEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QPdfEngine);

    if (!d->hasPen || (d->clipEnabled && d->allClipped))
        return;

    if (d->stroker.matrix.type() >= QTransform::TxProject) {
        QPaintEngine::drawTextItem(p, textItem);
        return;
    }

    *d->currentPage << "q\n";
    if(!d->simplePen)
        *d->currentPage << QPdf::generateMatrix(d->stroker.matrix);

    bool hp = d->hasPen;
    d->hasPen = false;
    QBrush b = d->brush;
    d->brush = d->pen.brush();
    setBrush();

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);
    Q_ASSERT(ti.fontEngine->type() != QFontEngine::Multi);
    d->drawTextItem(p, ti);
    d->hasPen = hp;
    d->brush = b;
    *d->currentPage << "Q\n";
}


void QPdfEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QPdfEngine);

    QPaintEngine::DirtyFlags flags = state.state();

    if (flags & DirtyTransform)
        d->stroker.matrix = state.transform();

    if (flags & DirtyPen) {
        d->pen = state.pen();
        d->hasPen = d->pen.style() != Qt::NoPen;
        d->stroker.setPen(d->pen, state.renderHints());
        QBrush penBrush = d->pen.brush();
        bool oldSimple = d->simplePen;
        d->simplePen = (d->hasPen && (penBrush.style() == Qt::SolidPattern) && penBrush.isOpaque());
        if (oldSimple != d->simplePen)
            flags |= DirtyTransform;
    } else if (flags & DirtyHints) {
        d->stroker.setPen(d->pen, state.renderHints());
    }
    if (flags & DirtyBrush) {
        d->brush = state.brush();
        if (d->brush.color().alpha() == 0 && d->brush.style() == Qt::SolidPattern)
            d->brush.setStyle(Qt::NoBrush);
        d->hasBrush = d->brush.style() != Qt::NoBrush;
    }
    if (flags & DirtyBrushOrigin) {
        d->brushOrigin = state.brushOrigin();
        flags |= DirtyBrush;
    }
    if (flags & DirtyOpacity)
        d->opacity = state.opacity();

    bool ce = d->clipEnabled;
    if (flags & DirtyClipPath) {
        d->clipEnabled = true;
        updateClipPath(state.clipPath(), state.clipOperation());
    } else if (flags & DirtyClipRegion) {
        d->clipEnabled = true;
        QPainterPath path;
        QVector<QRect> rects = state.clipRegion().rects();
        for (int i = 0; i < rects.size(); ++i)
            path.addRect(rects.at(i));
        updateClipPath(path, state.clipOperation());
        flags |= DirtyClipPath;
    } else if (flags & DirtyClipEnabled) {
        d->clipEnabled = state.isClipEnabled();
    }

    if (ce != d->clipEnabled)
        flags |= DirtyClipPath;
    else if (!d->clipEnabled)
        flags &= ~DirtyClipPath;

    setupGraphicsState(flags);
}

void QPdfEngine::setupGraphicsState(QPaintEngine::DirtyFlags flags)
{
    Q_D(QPdfEngine);
    if (flags & DirtyClipPath)
        flags |= DirtyTransform|DirtyPen|DirtyBrush;

    if (flags & DirtyTransform) {
        *d->currentPage << "Q\n";
        flags |= DirtyPen|DirtyBrush;
    }

    if (flags & DirtyClipPath) {
        *d->currentPage << "Q q\n";

        d->allClipped = false;
        if (d->clipEnabled && !d->clips.isEmpty()) {
            for (int i = 0; i < d->clips.size(); ++i) {
                if (d->clips.at(i).isEmpty()) {
                    d->allClipped = true;
                    break;
                }
            }
            if (!d->allClipped) {
                for (int i = 0; i < d->clips.size(); ++i) {
                    *d->currentPage << QPdf::generatePath(d->clips.at(i), QTransform(), QPdf::ClipPath);
                }
            }
        }
    }

    if (flags & DirtyTransform) {
        *d->currentPage << "q\n";
        if (d->simplePen && !d->stroker.matrix.isIdentity())
            *d->currentPage << QPdf::generateMatrix(d->stroker.matrix);
    }
    if (flags & DirtyBrush)
        setBrush();
    if (d->simplePen && (flags & DirtyPen))
        setPen();
}

extern QPainterPath qt_regionToPath(const QRegion &region);

void QPdfEngine::updateClipPath(const QPainterPath &p, Qt::ClipOperation op)
{
    Q_D(QPdfEngine);
    QPainterPath path = d->stroker.matrix.map(p);
    //qDebug() << "updateClipPath: " << d->stroker.matrix << p.boundingRect() << path.boundingRect() << op;

    if (op == Qt::NoClip) {
        d->clipEnabled = false;
        d->clips.clear();
    } else if (op == Qt::ReplaceClip) {
        d->clips.clear();
        d->clips.append(path);
    } else if (op == Qt::IntersectClip) {
        d->clips.append(path);
    } else { // UniteClip
        // ask the painter for the current clipping path. that's the easiest solution
        path = painter()->clipPath();
        path = d->stroker.matrix.map(path);
        d->clips.clear();
        d->clips.append(path);
    }
}

void QPdfEngine::setPen()
{
    Q_D(QPdfEngine);
    if (d->pen.style() == Qt::NoPen)
        return;
    QBrush b = d->pen.brush();
    Q_ASSERT(b.style() == Qt::SolidPattern && b.isOpaque());

    QColor rgba = b.color();
    if (d->grayscale) {
        qreal gray = qGray(rgba.rgba())/255.;
        *d->currentPage << gray << gray << gray;
    } else {
        *d->currentPage << rgba.redF()
                        << rgba.greenF()
                        << rgba.blueF();
    }
    *d->currentPage << "SCN\n";

    *d->currentPage << d->pen.widthF() << "w ";

    int pdfCapStyle = 0;
    switch(d->pen.capStyle()) {
    case Qt::FlatCap:
        pdfCapStyle = 0;
        break;
    case Qt::SquareCap:
        pdfCapStyle = 2;
        break;
    case Qt::RoundCap:
        pdfCapStyle = 1;
        break;
    default:
        break;
    }
    *d->currentPage << pdfCapStyle << "J ";

    int pdfJoinStyle = 0;
    switch(d->pen.joinStyle()) {
    case Qt::MiterJoin:
        pdfJoinStyle = 0;
        break;
    case Qt::BevelJoin:
        pdfJoinStyle = 2;
        break;
    case Qt::RoundJoin:
        pdfJoinStyle = 1;
        break;
    default:
        break;
    }
    *d->currentPage << pdfJoinStyle << "j ";

    *d->currentPage << QPdf::generateDashes(d->pen) << " 0 d\n";
}


void QPdfEngine::setBrush()
{
    Q_D(QPdfEngine);
    Qt::BrushStyle style = d->brush.style();
    if (style == Qt::NoBrush)
        return;

    bool specifyColor;
    int gStateObject = 0;
    int patternObject = d->addBrushPattern(d->stroker.matrix, &specifyColor, &gStateObject);

    *d->currentPage << (patternObject ? "/PCSp cs " : "/CSp cs ");
    if (specifyColor) {
        QColor rgba = d->brush.color();
        if (d->grayscale) {
            qreal gray = qGray(rgba.rgba())/255.;
            *d->currentPage << gray << gray << gray;
        } else {
            *d->currentPage << rgba.redF()
                            << rgba.greenF()
                            << rgba.blueF();
        }
    }
    if (patternObject)
        *d->currentPage << "/Pat" << patternObject;
    *d->currentPage << "scn\n";

    if (gStateObject)
        *d->currentPage << "/GState" << gStateObject << "gs\n";
    else
        *d->currentPage << "/GSa gs\n";
}


bool QPdfEngine::newPage()
{
    Q_D(QPdfEngine);
    if (!isActive())
        return false;
    d->newPage();

    setupGraphicsState(DirtyBrush|DirtyPen|DirtyClipPath);
    QFile *outfile = qobject_cast<QFile*> (d->outDevice);
    if (outfile && outfile->error() != QFile::NoError)
        return false;
    return true;
}

QPaintEngine::Type QPdfEngine::type() const
{
    return QPaintEngine::Pdf;
}

void QPdfEngine::setResolution(int resolution)
{
    Q_D(QPdfEngine);
    d->resolution = resolution;
}

int QPdfEngine::resolution() const
{
    Q_D(const QPdfEngine);
    return d->resolution;
}

void QPdfEngine::setPageLayout(const QPageLayout &pageLayout)
{
    Q_D(QPdfEngine);
    d->m_pageLayout = pageLayout;
}

void QPdfEngine::setPageSize(const QPageSize &pageSize)
{
    Q_D(QPdfEngine);
    d->m_pageLayout.setPageSize(pageSize);
}

void QPdfEngine::setPageOrientation(QPageLayout::Orientation orientation)
{
    Q_D(QPdfEngine);
    d->m_pageLayout.setOrientation(orientation);
}

void QPdfEngine::setPageMargins(const QMarginsF &margins, QPageLayout::Unit units)
{
    Q_D(QPdfEngine);
    d->m_pageLayout.setUnits(units);
    d->m_pageLayout.setMargins(margins);
}

QPageLayout QPdfEngine::pageLayout() const
{
    Q_D(const QPdfEngine);
    return d->m_pageLayout;
}

// Metrics are in Device Pixels
int QPdfEngine::metric(QPaintDevice::PaintDeviceMetric metricType) const
{
    Q_D(const QPdfEngine);
    int val;
    switch (metricType) {
    case QPaintDevice::PdmWidth:
        val = d->m_pageLayout.paintRectPixels(d->resolution).width();
        break;
    case QPaintDevice::PdmHeight:
        val = d->m_pageLayout.paintRectPixels(d->resolution).height();
        break;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmDpiY:
        val = d->resolution;
        break;
    case QPaintDevice::PdmPhysicalDpiX:
    case QPaintDevice::PdmPhysicalDpiY:
        val = 1200;
        break;
    case QPaintDevice::PdmWidthMM:
        val = qRound(d->m_pageLayout.paintRect(QPageLayout::Millimeter).width());
        break;
    case QPaintDevice::PdmHeightMM:
        val = qRound(d->m_pageLayout.paintRect(QPageLayout::Millimeter).height());
        break;
    case QPaintDevice::PdmNumColors:
        val = INT_MAX;
        break;
    case QPaintDevice::PdmDepth:
        val = 32;
        break;
    case QPaintDevice::PdmDevicePixelRatio:
        val = 1;
        break;
    default:
        qWarning("QPdfWriter::metric: Invalid metric command");
        return 0;
    }
    return val;
}

QPdfEnginePrivate::QPdfEnginePrivate()
    : clipEnabled(false), allClipped(false), hasPen(true), hasBrush(false), simplePen(false),
      outDevice(0), ownsDevice(false),
      embedFonts(true),
      grayscale(false),
      m_pageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(10, 10, 10, 10))
{
    resolution = 1200;
    currentObject = 1;
    currentPage = 0;
    stroker.stream = 0;

    streampos = 0;

    stream = new QDataStream;
}

bool QPdfEngine::begin(QPaintDevice *pdev)
{
    Q_D(QPdfEngine);
    d->pdev = pdev;

    if (!d->outDevice) {
        if (!d->outputFileName.isEmpty()) {
            QFile *file = new QFile(d->outputFileName);
            if (!file->open(QFile::WriteOnly|QFile::Truncate)) {
                delete file;
                return false;
            }
            d->outDevice = file;
        } else {
            return false;
        }
        d->ownsDevice = true;
    }

    d->currentObject = 1;

    d->currentPage = new QPdfPage;
    d->stroker.stream = d->currentPage;
    d->opacity = 1.0;

    d->stream->setDevice(d->outDevice);

    d->streampos = 0;
    d->hasPen = true;
    d->hasBrush = false;
    d->clipEnabled = false;
    d->allClipped = false;

    d->xrefPositions.clear();
    d->pageRoot = 0;
    d->catalog = 0;
    d->info = 0;
    d->graphicsState = 0;
    d->patternColorSpace = 0;
    d->simplePen = false;

    d->pages.clear();
    d->imageCache.clear();
    d->alphaCache.clear();

    setActive(true);
    d->writeHeader();
    newPage();

    return true;
}

bool QPdfEngine::end()
{
    Q_D(QPdfEngine);
    d->writeTail();

    d->stream->unsetDevice();

    qDeleteAll(d->fonts);
    d->fonts.clear();
    delete d->currentPage;
    d->currentPage = 0;

    if (d->outDevice && d->ownsDevice) {
        d->outDevice->close();
        delete d->outDevice;
        d->outDevice = 0;
    }

    setActive(false);
    return true;
}

QPdfEnginePrivate::~QPdfEnginePrivate()
{
    qDeleteAll(fonts);
    delete currentPage;
    delete stream;
}

void QPdfEnginePrivate::writeHeader()
{
    addXrefEntry(0,false);

    xprintf("%%PDF-1.4\n");

    writeInfo();

    catalog = addXrefEntry(-1);
    pageRoot = requestObject();
    xprintf("<<\n"
            "/Type /Catalog\n"
            "/Pages %d 0 R\n"
            ">>\n"
            "endobj\n", pageRoot);

    // graphics state
    graphicsState = addXrefEntry(-1);
    xprintf("<<\n"
            "/Type /ExtGState\n"
            "/SA true\n"
            "/SM 0.02\n"
            "/ca 1.0\n"
            "/CA 1.0\n"
            "/AIS false\n"
            "/SMask /None"
            ">>\n"
            "endobj\n");

    // color space for pattern
    patternColorSpace = addXrefEntry(-1);
    xprintf("[/Pattern /DeviceRGB]\n"
            "endobj\n");
}

void QPdfEnginePrivate::writeInfo()
{
    info = addXrefEntry(-1);
    xprintf("<<\n/Title ");
    printString(title);
    xprintf("\n/Creator ");
    printString(creator);
    xprintf("\n/Producer ");
    printString(QString::fromLatin1("Qt " QT_VERSION_STR));
    QDateTime now = QDateTime::currentDateTime().toUTC();
    QTime t = now.time();
    QDate d = now.date();
    xprintf("\n/CreationDate (D:%d%02d%02d%02d%02d%02d)\n",
            d.year(),
            d.month(),
            d.day(),
            t.hour(),
            t.minute(),
            t.second());
    xprintf(">>\n"
            "endobj\n");
}

void QPdfEnginePrivate::writePageRoot()
{
    addXrefEntry(pageRoot);

    xprintf("<<\n"
            "/Type /Pages\n"
            "/Kids \n"
            "[\n");
    int size = pages.size();
    for (int i = 0; i < size; ++i)
        xprintf("%d 0 R\n", pages[i]);
    xprintf("]\n");

    //xprintf("/Group <</S /Transparency /I true /K false>>\n");
    xprintf("/Count %d\n", pages.size());

    xprintf("/ProcSet [/PDF /Text /ImageB /ImageC]\n"
            ">>\n"
            "endobj\n");
}


void QPdfEnginePrivate::embedFont(QFontSubset *font)
{
    //qDebug() << "embedFont" << font->object_id;
    int fontObject = font->object_id;
    QByteArray fontData = font->toTruetype();
#ifdef FONT_DUMP
    static int i = 0;
    QString fileName("font%1.ttf");
    fileName = fileName.arg(i++);
    QFile ff(fileName);
    ff.open(QFile::WriteOnly);
    ff.write(fontData);
    ff.close();
#endif

    int fontDescriptor = requestObject();
    int fontstream = requestObject();
    int cidfont = requestObject();
    int toUnicode = requestObject();

    QFontEngine::Properties properties = font->fontEngine->properties();

    {
        qreal scale = 1000/properties.emSquare.toReal();
        addXrefEntry(fontDescriptor);
        QByteArray descriptor;
        QPdf::ByteStream s(&descriptor);
        s << "<< /Type /FontDescriptor\n"
            "/FontName /Q";
        int tag = fontDescriptor;
        for (int i = 0; i < 5; ++i) {
            s << (char)('A' + (tag % 26));
            tag /= 26;
        }
        s <<  '+' << properties.postscriptName << "\n"
            "/Flags " << 4 << "\n"
            "/FontBBox ["
          << properties.boundingBox.x()*scale
          << -(properties.boundingBox.y() + properties.boundingBox.height())*scale
          << (properties.boundingBox.x() + properties.boundingBox.width())*scale
          << -properties.boundingBox.y()*scale  << "]\n"
            "/ItalicAngle " << properties.italicAngle.toReal() << "\n"
            "/Ascent " << properties.ascent.toReal()*scale << "\n"
            "/Descent " << -properties.descent.toReal()*scale << "\n"
            "/CapHeight " << properties.capHeight.toReal()*scale << "\n"
            "/StemV " << properties.lineWidth.toReal()*scale << "\n"
            "/FontFile2 " << fontstream << "0 R\n"
            ">> endobj\n";
        write(descriptor);
    }
    {
        addXrefEntry(fontstream);
        QByteArray header;
        QPdf::ByteStream s(&header);

        int length_object = requestObject();
        s << "<<\n"
            "/Length1 " << fontData.size() << "\n"
            "/Length " << length_object << "0 R\n";
        if (do_compress)
            s << "/Filter /FlateDecode\n";
        s << ">>\n"
            "stream\n";
        write(header);
        int len = writeCompressed(fontData);
        write("endstream\n"
              "endobj\n");
        addXrefEntry(length_object);
        xprintf("%d\n"
                "endobj\n", len);
    }
    {
        addXrefEntry(cidfont);
        QByteArray cid;
        QPdf::ByteStream s(&cid);
        s << "<< /Type /Font\n"
            "/Subtype /CIDFontType2\n"
            "/BaseFont /" << properties.postscriptName << "\n"
            "/CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >>\n"
            "/FontDescriptor " << fontDescriptor << "0 R\n"
            "/CIDToGIDMap /Identity\n"
          << font->widthArray() <<
            ">>\n"
            "endobj\n";
        write(cid);
    }
    {
        addXrefEntry(toUnicode);
        QByteArray touc = font->createToUnicodeMap();
        xprintf("<< /Length %d >>\n"
                "stream\n", touc.length());
        write(touc);
        write("endstream\n"
              "endobj\n");
    }
    {
        addXrefEntry(fontObject);
        QByteArray font;
        QPdf::ByteStream s(&font);
        s << "<< /Type /Font\n"
            "/Subtype /Type0\n"
            "/BaseFont /" << properties.postscriptName << "\n"
            "/Encoding /Identity-H\n"
            "/DescendantFonts [" << cidfont << "0 R]\n"
            "/ToUnicode " << toUnicode << "0 R"
            ">>\n"
            "endobj\n";
        write(font);
    }
}


void QPdfEnginePrivate::writeFonts()
{
    for (QHash<QFontEngine::FaceId, QFontSubset *>::iterator it = fonts.begin(); it != fonts.end(); ++it) {
        embedFont(*it);
        delete *it;
    }
    fonts.clear();
}

void QPdfEnginePrivate::writePage()
{
    if (pages.empty())
        return;

    *currentPage << "Q Q\n";

    uint pageStream = requestObject();
    uint pageStreamLength = requestObject();
    uint resources = requestObject();
    uint annots = requestObject();

    addXrefEntry(pages.last());
    xprintf("<<\n"
            "/Type /Page\n"
            "/Parent %d 0 R\n"
            "/Contents %d 0 R\n"
            "/Resources %d 0 R\n"
            "/Annots %d 0 R\n"
            "/MediaBox [0 0 %d %d]\n"
            ">>\n"
            "endobj\n",
            pageRoot, pageStream, resources, annots,
            // make sure we use the pagesize from when we started the page, since the user may have changed it
            currentPage->pageSize.width(), currentPage->pageSize.height());

    addXrefEntry(resources);
    xprintf("<<\n"
            "/ColorSpace <<\n"
            "/PCSp %d 0 R\n"
            "/CSp /DeviceRGB\n"
            "/CSpg /DeviceGray\n"
            ">>\n"
            "/ExtGState <<\n"
            "/GSa %d 0 R\n",
            patternColorSpace, graphicsState);

    for (int i = 0; i < currentPage->graphicStates.size(); ++i)
        xprintf("/GState%d %d 0 R\n", currentPage->graphicStates.at(i), currentPage->graphicStates.at(i));
    xprintf(">>\n");

    xprintf("/Pattern <<\n");
    for (int i = 0; i < currentPage->patterns.size(); ++i)
        xprintf("/Pat%d %d 0 R\n", currentPage->patterns.at(i), currentPage->patterns.at(i));
    xprintf(">>\n");

    xprintf("/Font <<\n");
    for (int i = 0; i < currentPage->fonts.size();++i)
        xprintf("/F%d %d 0 R\n", currentPage->fonts[i], currentPage->fonts[i]);
    xprintf(">>\n");

    xprintf("/XObject <<\n");
    for (int i = 0; i<currentPage->images.size(); ++i) {
        xprintf("/Im%d %d 0 R\n", currentPage->images.at(i), currentPage->images.at(i));
    }
    xprintf(">>\n");

    xprintf(">>\n"
            "endobj\n");

    addXrefEntry(annots);
    xprintf("[ ");
    for (int i = 0; i<currentPage->annotations.size(); ++i) {
        xprintf("%d 0 R ", currentPage->annotations.at(i));
    }
    xprintf("]\nendobj\n");

    addXrefEntry(pageStream);
    xprintf("<<\n"
            "/Length %d 0 R\n", pageStreamLength); // object number for stream length object
    if (do_compress)
        xprintf("/Filter /FlateDecode\n");

    xprintf(">>\n");
    xprintf("stream\n");
    QIODevice *content = currentPage->stream();
    int len = writeCompressed(content);
    xprintf("endstream\n"
            "endobj\n");

    addXrefEntry(pageStreamLength);
    xprintf("%d\nendobj\n",len);
}

void QPdfEnginePrivate::writeTail()
{
    writePage();
    writeFonts();
    writePageRoot();
    addXrefEntry(xrefPositions.size(),false);
    xprintf("xref\n"
            "0 %d\n"
            "%010d 65535 f \n", xrefPositions.size()-1, xrefPositions[0]);

    for (int i = 1; i < xrefPositions.size()-1; ++i)
        xprintf("%010d 00000 n \n", xrefPositions[i]);

    xprintf("trailer\n"
            "<<\n"
            "/Size %d\n"
            "/Info %d 0 R\n"
            "/Root %d 0 R\n"
            ">>\n"
            "startxref\n%d\n"
            "%%%%EOF\n",
            xrefPositions.size()-1, info, catalog, xrefPositions.last());
}

int QPdfEnginePrivate::addXrefEntry(int object, bool printostr)
{
    if (object < 0)
        object = requestObject();

    if (object>=xrefPositions.size())
        xrefPositions.resize(object+1);

    xrefPositions[object] = streampos;
    if (printostr)
        xprintf("%d 0 obj\n",object);

    return object;
}

void QPdfEnginePrivate::printString(const QString &string) {
    // The 'text string' type in PDF is encoded either as PDFDocEncoding, or
    // Unicode UTF-16 with a Unicode byte order mark as the first character
    // (0xfeff), with the high-order byte first.
    QByteArray array("(\xfe\xff");
    const ushort *utf16 = string.utf16();

    for (int i=0; i < string.size(); ++i) {
        char part[2] = {char((*(utf16 + i)) >> 8), char((*(utf16 + i)) & 0xff)};
        for(int j=0; j < 2; ++j) {
            if (part[j] == '(' || part[j] == ')' || part[j] == '\\')
                array.append('\\');
            array.append(part[j]);
        }
    }
    array.append(")");
    write(array);
}


// For strings up to 10000 bytes only !
void QPdfEnginePrivate::xprintf(const char* fmt, ...)
{
    if (!stream)
        return;

    const int msize = 10000;
    char buf[msize];

    va_list args;
    va_start(args, fmt);
    int bufsize = qvsnprintf(buf, msize, fmt, args);

    Q_ASSERT(bufsize<msize);

    va_end(args);

    stream->writeRawData(buf, bufsize);
    streampos += bufsize;
}

int QPdfEnginePrivate::writeCompressed(QIODevice *dev)
{
#ifndef QT_NO_COMPRESS
    if (do_compress) {
        int size = QPdfPage::chunkSize();
        int sum = 0;
        ::z_stream zStruct;
        zStruct.zalloc = Z_NULL;
        zStruct.zfree = Z_NULL;
        zStruct.opaque = Z_NULL;
        if (::deflateInit(&zStruct, Z_DEFAULT_COMPRESSION) != Z_OK) {
            qWarning("QPdfStream::writeCompressed: Error in deflateInit()");
            return sum;
        }
        zStruct.avail_in = 0;
        QByteArray in, out;
        out.resize(size);
        while (!dev->atEnd() || zStruct.avail_in != 0) {
            if (zStruct.avail_in == 0) {
                in = dev->read(size);
                zStruct.avail_in = in.size();
                zStruct.next_in = reinterpret_cast<unsigned char*>(in.data());
                if (in.size() <= 0) {
                    qWarning("QPdfStream::writeCompressed: Error in read()");
                    ::deflateEnd(&zStruct);
                    return sum;
                }
            }
            zStruct.next_out = reinterpret_cast<unsigned char*>(out.data());
            zStruct.avail_out = out.size();
            if (::deflate(&zStruct, 0) != Z_OK) {
                qWarning("QPdfStream::writeCompressed: Error in deflate()");
                ::deflateEnd(&zStruct);
                return sum;
            }
            int written = out.size() - zStruct.avail_out;
            stream->writeRawData(out.constData(), written);
            streampos += written;
            sum += written;
        }
        int ret;
        do {
            zStruct.next_out = reinterpret_cast<unsigned char*>(out.data());
            zStruct.avail_out = out.size();
            ret = ::deflate(&zStruct, Z_FINISH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                qWarning("QPdfStream::writeCompressed: Error in deflate()");
                ::deflateEnd(&zStruct);
                return sum;
            }
            int written = out.size() - zStruct.avail_out;
            stream->writeRawData(out.constData(), written);
            streampos += written;
            sum += written;
        } while (ret == Z_OK);

        ::deflateEnd(&zStruct);

        return sum;
    } else
#endif
    {
        QByteArray arr;
        int sum = 0;
        while (!dev->atEnd()) {
            arr = dev->read(QPdfPage::chunkSize());
            stream->writeRawData(arr.constData(), arr.size());
            streampos += arr.size();
            sum += arr.size();
        }
        return sum;
    }
}

int QPdfEnginePrivate::writeCompressed(const char *src, int len)
{
#ifndef QT_NO_COMPRESS
    if(do_compress) {
        uLongf destLen = len + len/100 + 13; // zlib requirement
        Bytef* dest = new Bytef[destLen];
        if (Z_OK == ::compress(dest, &destLen, (const Bytef*) src, (uLongf)len)) {
            stream->writeRawData((const char*)dest, destLen);
        } else {
            qWarning("QPdfStream::writeCompressed: Error in compress()");
            destLen = 0;
        }
        delete [] dest;
        len = destLen;
    } else
#endif
    {
        stream->writeRawData(src,len);
    }
    streampos += len;
    return len;
}

int QPdfEnginePrivate::writeImage(const QByteArray &data, int width, int height, int depth,
                                  int maskObject, int softMaskObject, bool dct)
{
    int image = addXrefEntry(-1);
    xprintf("<<\n"
            "/Type /XObject\n"
            "/Subtype /Image\n"
            "/Width %d\n"
            "/Height %d\n", width, height);

    if (depth == 1) {
        xprintf("/ImageMask true\n"
                "/Decode [1 0]\n");
    } else {
        xprintf("/BitsPerComponent 8\n"
                "/ColorSpace %s\n", (depth == 32) ? "/DeviceRGB" : "/DeviceGray");
    }
    if (maskObject > 0)
        xprintf("/Mask %d 0 R\n", maskObject);
    if (softMaskObject > 0)
        xprintf("/SMask %d 0 R\n", softMaskObject);

    int lenobj = requestObject();
    xprintf("/Length %d 0 R\n", lenobj);
    if (interpolateImages)
        xprintf("/Interpolate true\n");
    int len = 0;
    if (dct) {
        //qDebug() << "DCT";
        xprintf("/Filter /DCTDecode\n>>\nstream\n");
        write(data);
        len = data.length();
    } else {
        if (do_compress)
            xprintf("/Filter /FlateDecode\n>>\nstream\n");
        else
            xprintf(">>\nstream\n");
        len = writeCompressed(data);
    }
    xprintf("endstream\n"
            "endobj\n");
    addXrefEntry(lenobj);
    xprintf("%d\n"
            "endobj\n", len);
    return image;
}

#ifdef USE_NATIVE_GRADIENTS
int QPdfEnginePrivate::gradientBrush(const QBrush &b, const QMatrix &matrix, int *gStateObject)
{
    const QGradient *gradient = b.gradient();
    if (!gradient)
        return 0;

    QTransform inv = matrix.inverted();
    QPointF page_rect[4] = { inv.map(QPointF(0, 0)),
                             inv.map(QPointF(width_, 0)),
                             inv.map(QPointF(0, height_)),
                             inv.map(QPointF(width_, height_)) };

    bool opaque = b.isOpaque();

    QByteArray shader;
    QByteArray alphaShader;
    if (gradient->type() == QGradient::LinearGradient) {
        const QLinearGradient *lg = static_cast<const QLinearGradient *>(gradient);
        shader = QPdf::generateLinearGradientShader(lg, page_rect);
        if (!opaque)
            alphaShader = QPdf::generateLinearGradientShader(lg, page_rect, true);
    } else {
        // #############
        return 0;
    }
    int shaderObject = addXrefEntry(-1);
    write(shader);

    QByteArray str;
    QPdf::ByteStream s(&str);
    s << "<<\n"
        "/Type /Pattern\n"
        "/PatternType 2\n"
        "/Shading " << shaderObject << "0 R\n"
        "/Matrix ["
      << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy() << "]\n";
    s << ">>\n"
        "endobj\n";

    int patternObj = addXrefEntry(-1);
    write(str);
    currentPage->patterns.append(patternObj);

    if (!opaque) {
        bool ca = true;
        QGradientStops stops = gradient->stops();
        int a = stops.at(0).second.alpha();
        for (int i = 1; i < stops.size(); ++i) {
            if (stops.at(i).second.alpha() != a) {
                ca = false;
                break;
            }
        }
        if (ca) {
            *gStateObject = addConstantAlphaObject(stops.at(0).second.alpha());
        } else {
            int alphaShaderObject = addXrefEntry(-1);
            write(alphaShader);

            QByteArray content;
            QPdf::ByteStream c(&content);
            c << "/Shader" << alphaShaderObject << "sh\n";

            QByteArray form;
            QPdf::ByteStream f(&form);
            f << "<<\n"
                "/Type /XObject\n"
                "/Subtype /Form\n"
                "/BBox [0 0 " << width_ << height_ << "]\n"
                "/Group <</S /Transparency >>\n"
                "/Resources <<\n"
                "/Shading << /Shader" << alphaShaderObject << alphaShaderObject << "0 R >>\n"
                ">>\n";

            f << "/Length " << content.length() << "\n"
                ">>\n"
                "stream\n"
              << content
              << "endstream\n"
                "endobj\n";

            int softMaskFormObject = addXrefEntry(-1);
            write(form);
            *gStateObject = addXrefEntry(-1);
            xprintf("<< /SMask << /S /Alpha /G %d 0 R >> >>\n"
                    "endobj\n", softMaskFormObject);
            currentPage->graphicStates.append(*gStateObject);
        }
    }

    return patternObj;
}
#endif

int QPdfEnginePrivate::addConstantAlphaObject(int brushAlpha, int penAlpha)
{
    if (brushAlpha == 255 && penAlpha == 255)
        return 0;
    int object = alphaCache.value(QPair<uint, uint>(brushAlpha, penAlpha), 0);
    if (!object) {
        object = addXrefEntry(-1);
        QByteArray alphaDef;
        QPdf::ByteStream s(&alphaDef);
        s << "<<\n/ca " << (brushAlpha/qreal(255.)) << '\n';
        s << "/CA " << (penAlpha/qreal(255.)) << "\n>>";
        xprintf("%s\nendobj\n", alphaDef.constData());
        alphaCache.insert(QPair<uint, uint>(brushAlpha, penAlpha), object);
    }
    if (currentPage->graphicStates.indexOf(object) < 0)
        currentPage->graphicStates.append(object);

    return object;
}

int QPdfEnginePrivate::addBrushPattern(const QTransform &m, bool *specifyColor, int *gStateObject)
{
    int paintType = 2; // Uncolored tiling
    int w = 8;
    int h = 8;

    *specifyColor = true;
    *gStateObject = 0;

    QTransform matrix = m;
    matrix.translate(brushOrigin.x(), brushOrigin.y());
    matrix = matrix * pageMatrix();
    //qDebug() << brushOrigin << matrix;

    Qt::BrushStyle style = brush.style();
    if (style == Qt::LinearGradientPattern) {// && style <= Qt::ConicalGradientPattern) {
#ifdef USE_NATIVE_GRADIENTS
        *specifyColor = false;
        return gradientBrush(b, matrix, gStateObject);
#else
        return 0;
#endif
    }

    if ((!brush.isOpaque() && brush.style() < Qt::LinearGradientPattern) || opacity != 1.0)
        *gStateObject = addConstantAlphaObject(qRound(brush.color().alpha() * opacity),
                                               qRound(pen.color().alpha() * opacity));

    int imageObject = -1;
    QByteArray pattern = QPdf::patternForBrush(brush);
    if (pattern.isEmpty()) {
        if (brush.style() != Qt::TexturePattern)
            return 0;
        QImage image = brush.texture().toImage();
        bool bitmap = true;
        imageObject = addImage(image, &bitmap, brush.texture().cacheKey());
        if (imageObject != -1) {
            QImage::Format f = image.format();
            if (f != QImage::Format_MonoLSB && f != QImage::Format_Mono) {
                paintType = 1; // Colored tiling
                *specifyColor = false;
            }
            w = image.width();
            h = image.height();
            QTransform m(w, 0, 0, -h, 0, h);
            QPdf::ByteStream s(&pattern);
            s << QPdf::generateMatrix(m);
            s << "/Im" << imageObject << " Do\n";
        }
    }

    QByteArray str;
    QPdf::ByteStream s(&str);
    s << "<<\n"
        "/Type /Pattern\n"
        "/PatternType 1\n"
        "/PaintType " << paintType << "\n"
        "/TilingType 1\n"
        "/BBox [0 0 " << w << h << "]\n"
        "/XStep " << w << "\n"
        "/YStep " << h << "\n"
        "/Matrix ["
      << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy() << "]\n"
        "/Resources \n<< "; // open resource tree
    if (imageObject > 0) {
        s << "/XObject << /Im" << imageObject << ' ' << imageObject << "0 R >> ";
    }
    s << ">>\n"
        "/Length " << pattern.length() << "\n"
        ">>\n"
        "stream\n"
      << pattern
      << "endstream\n"
        "endobj\n";

    int patternObj = addXrefEntry(-1);
    write(str);
    currentPage->patterns.append(patternObj);
    return patternObj;
}

/*!
 * Adds an image to the pdf and return the pdf-object id. Returns -1 if adding the image failed.
 */
int QPdfEnginePrivate::addImage(const QImage &img, bool *bitmap, qint64 serial_no)
{
    if (img.isNull())
        return -1;

    int object = imageCache.value(serial_no);
    if(object)
        return object;

    QImage image = img;
    QImage::Format format = image.format();
    if (image.depth() == 1 && *bitmap && img.colorTable().size() == 2
        && img.colorTable().at(0) == QColor(Qt::black).rgba()
        && img.colorTable().at(1) == QColor(Qt::white).rgba())
    {
        if (format == QImage::Format_MonoLSB)
            image = image.convertToFormat(QImage::Format_Mono);
        format = QImage::Format_Mono;
    } else {
        *bitmap = false;
        if (format != QImage::Format_RGB32 && format != QImage::Format_ARGB32) {
            image = image.convertToFormat(QImage::Format_ARGB32);
            format = QImage::Format_ARGB32;
        }
    }

    int w = image.width();
    int h = image.height();
    int d = image.depth();

    if (format == QImage::Format_Mono) {
        int bytesPerLine = (w + 7) >> 3;
        QByteArray data;
        data.resize(bytesPerLine * h);
        char *rawdata = data.data();
        for (int y = 0; y < h; ++y) {
            memcpy(rawdata, image.scanLine(y), bytesPerLine);
            rawdata += bytesPerLine;
        }
        object = writeImage(data, w, h, d, 0, 0);
    } else {
        QByteArray softMaskData;
        bool dct = false;
        QByteArray imageData;
        bool hasAlpha = false;
        bool hasMask = false;

        if (QImageWriter::supportedImageFormats().contains("jpeg") && !grayscale) {
            QBuffer buffer(&imageData);
            QImageWriter writer(&buffer, "jpeg");
            writer.setQuality(94);
            writer.write(image);
            dct = true;

            if (format != QImage::Format_RGB32) {
                softMaskData.resize(w * h);
                uchar *sdata = (uchar *)softMaskData.data();
                for (int y = 0; y < h; ++y) {
                    const QRgb *rgb = (const QRgb *)image.scanLine(y);
                    for (int x = 0; x < w; ++x) {
                        uchar alpha = qAlpha(*rgb);
                        *sdata++ = alpha;
                        hasMask |= (alpha < 255);
                        hasAlpha |= (alpha != 0 && alpha != 255);
                        ++rgb;
                    }
                }
            }
        } else {
            imageData.resize(grayscale ? w * h : 3 * w * h);
            uchar *data = (uchar *)imageData.data();
            softMaskData.resize(w * h);
            uchar *sdata = (uchar *)softMaskData.data();
            for (int y = 0; y < h; ++y) {
                const QRgb *rgb = (const QRgb *)image.scanLine(y);
                if (grayscale) {
                    for (int x = 0; x < w; ++x) {
                        *(data++) = qGray(*rgb);
                        uchar alpha = qAlpha(*rgb);
                        *sdata++ = alpha;
                        hasMask |= (alpha < 255);
                        hasAlpha |= (alpha != 0 && alpha != 255);
                        ++rgb;
                    }
                } else {
                    for (int x = 0; x < w; ++x) {
                        *(data++) = qRed(*rgb);
                        *(data++) = qGreen(*rgb);
                        *(data++) = qBlue(*rgb);
                        uchar alpha = qAlpha(*rgb);
                        *sdata++ = alpha;
                        hasMask |= (alpha < 255);
                        hasAlpha |= (alpha != 0 && alpha != 255);
                        ++rgb;
                    }
                }
            }
            if (format == QImage::Format_RGB32)
                hasAlpha = hasMask = false;
        }
        int maskObject = 0;
        int softMaskObject = 0;
        if (hasAlpha) {
            softMaskObject = writeImage(softMaskData, w, h, 8, 0, 0);
        } else if (hasMask) {
            // dither the soft mask to 1bit and add it. This also helps PDF viewers
            // without transparency support
            int bytesPerLine = (w + 7) >> 3;
            QByteArray mask(bytesPerLine * h, 0);
            uchar *mdata = (uchar *)mask.data();
            const uchar *sdata = (const uchar *)softMaskData.constData();
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (*sdata)
                        mdata[x>>3] |= (0x80 >> (x&7));
                    ++sdata;
                }
                mdata += bytesPerLine;
            }
            maskObject = writeImage(mask, w, h, 1, 0, 0);
        }
        object = writeImage(imageData, w, h, grayscale ? 8 : 32,
                            maskObject, softMaskObject, dct);
    }
    imageCache.insert(serial_no, object);
    return object;
}

void QPdfEnginePrivate::drawTextItem(const QPointF &p, const QTextItemInt &ti)
{
    Q_Q(QPdfEngine);

    if (ti.charFormat.isAnchor()) {
        qreal size = ti.fontEngine->fontDef.pixelSize;
        int synthesized = ti.fontEngine->synthesized();
        qreal stretch = synthesized & QFontEngine::SynthesizedStretch ? ti.fontEngine->fontDef.stretch/100. : 1.;

        QTransform trans;
        // Build text rendering matrix (Trm). We need it to map the text area to user
        // space units on the PDF page.
        trans = QTransform(size*stretch, 0, 0, size, 0, 0);
        // Apply text matrix (Tm).
        trans *= QTransform(1,0,0,-1,p.x(),p.y());
        // Apply page displacement (Identity for first page).
        trans *= stroker.matrix;
        // Apply Current Transformation Matrix (CTM)
        trans *= pageMatrix();
        qreal x1, y1, x2, y2;
        trans.map(0, 0, &x1, &y1);
        trans.map(ti.width.toReal()/size, (ti.ascent.toReal()-ti.descent.toReal())/size, &x2, &y2);

        uint annot = addXrefEntry(-1);
        QByteArray x1s, y1s, x2s, y2s;
        x1s.setNum(static_cast<double>(x1), 'f');
        y1s.setNum(static_cast<double>(y1), 'f');
        x2s.setNum(static_cast<double>(x2), 'f');
        y2s.setNum(static_cast<double>(y2), 'f');
        QByteArray rectData = x1s + ' ' + y1s + ' ' + x2s + ' ' + y2s;
        xprintf("<<\n/Type /Annot\n/Subtype /Link\n/Rect [");
        xprintf(rectData.constData());
#ifdef Q_DEBUG_PDF_LINKS
        xprintf("]\n/Border [16 16 1]\n/A <<\n");
#else
        xprintf("]\n/Border [0 0 0]\n/A <<\n");
#endif
        xprintf("/Type /Action\n/S /URI\n/URI (%s)\n",
                ti.charFormat.anchorHref().toLatin1().constData());
        xprintf(">>\n>>\n");
        xprintf("endobj\n");

        if (!currentPage->annotations.contains(annot)) {
            currentPage->annotations.append(annot);
        }
    }

    QFontEngine *fe = ti.fontEngine;

    QFontEngine::FaceId face_id = fe->faceId();
    bool noEmbed = false;
    if (face_id.filename.isEmpty()
        || fe->fsType & 0x200 /* bitmap embedding only */
        || fe->fsType == 2 /* no embedding allowed */) {
        *currentPage << "Q\n";
        q->QPaintEngine::drawTextItem(p, ti);
        *currentPage << "q\n";
        if (face_id.filename.isEmpty())
            return;
        noEmbed = true;
    }

    QFontSubset *font = fonts.value(face_id, 0);
    if (!font) {
        font = new QFontSubset(fe, requestObject());
        font->noEmbed = noEmbed;
    }
    fonts.insert(face_id, font);

    if (!currentPage->fonts.contains(font->object_id))
        currentPage->fonts.append(font->object_id);

    qreal size = ti.fontEngine->fontDef.pixelSize;

#if defined(Q_OS_WIN)
    size = (ti.fontEngine->ascent() + ti.fontEngine->descent()).toReal();
#endif

    QVarLengthArray<glyph_t> glyphs;
    QVarLengthArray<QFixedPoint> positions;
    QTransform m = QTransform::fromTranslate(p.x(), p.y());
    ti.fontEngine->getGlyphPositions(ti.glyphs, m, ti.flags,
                                     glyphs, positions);
    if (glyphs.size() == 0)
        return;
    int synthesized = ti.fontEngine->synthesized();
    qreal stretch = synthesized & QFontEngine::SynthesizedStretch ? ti.fontEngine->fontDef.stretch/100. : 1.;

    *currentPage << "BT\n"
                 << "/F" << font->object_id << size << "Tf "
                 << stretch << (synthesized & QFontEngine::SynthesizedItalic
                                ? "0 .3 -1 0 0 Tm\n"
                                : "0 0 -1 0 0 Tm\n");


#if 0
    // #### implement actual text for complex languages
    const unsigned short *logClusters = ti.logClusters;
    int pos = 0;
    do {
        int end = pos + 1;
        while (end < ti.num_chars && logClusters[end] == logClusters[pos])
            ++end;
        *currentPage << "/Span << /ActualText <FEFF";
        for (int i = pos; i < end; ++i) {
            s << toHex((ushort)ti.chars[i].unicode(), buf);
        }
        *currentPage << "> >>\n"
            "BDC\n"
            "<";
        int ge = end == ti.num_chars ? ti.num_glyphs : logClusters[end];
        for (int gs = logClusters[pos]; gs < ge; ++gs)
            *currentPage << toHex((ushort)ti.glyphs[gs].glyph, buf);
        *currentPage << "> Tj\n"
            "EMC\n";
        pos = end;
    } while (pos < ti.num_chars);
#else
    qreal last_x = 0.;
    qreal last_y = 0.;
    for (int i = 0; i < glyphs.size(); ++i) {
        qreal x = positions[i].x.toReal();
        qreal y = positions[i].y.toReal();
        if (synthesized & QFontEngine::SynthesizedItalic)
            x += .3*y;
        x /= stretch;
        char buf[5];
        int g = font->addGlyph(glyphs[i]);
        *currentPage << x - last_x << last_y - y << "Td <"
                     << QPdf::toHex((ushort)g, buf) << "> Tj\n";
        last_x = x;
        last_y = y;
    }
    if (synthesized & QFontEngine::SynthesizedBold) {
        *currentPage << stretch << (synthesized & QFontEngine::SynthesizedItalic
                            ? "0 .3 -1 0 0 Tm\n"
                            : "0 0 -1 0 0 Tm\n");
        *currentPage << "/Span << /ActualText <> >> BDC\n";
        last_x = 0.5*fe->lineThickness().toReal();
        last_y = 0.;
        for (int i = 0; i < glyphs.size(); ++i) {
            qreal x = positions[i].x.toReal();
            qreal y = positions[i].y.toReal();
            if (synthesized & QFontEngine::SynthesizedItalic)
                x += .3*y;
            x /= stretch;
            char buf[5];
            int g = font->addGlyph(glyphs[i]);
            *currentPage << x - last_x << last_y - y << "Td <"
                        << QPdf::toHex((ushort)g, buf) << "> Tj\n";
            last_x = x;
            last_y = y;
        }
        *currentPage << "EMC\n";
    }
#endif

    *currentPage << "ET\n";
}

QTransform QPdfEnginePrivate::pageMatrix() const
{
    qreal scale = 72./resolution;
    QTransform tmp(scale, 0.0, 0.0, -scale, 0.0, m_pageLayout.fullRectPoints().height());
    if (m_pageLayout.mode() != QPageLayout::FullPageMode) {
        QRect r = m_pageLayout.paintRectPixels(resolution);
        tmp.translate(r.left(), r.top());
    }
    return tmp;
}

void QPdfEnginePrivate::newPage()
{
    if (currentPage && currentPage->pageSize.isEmpty())
        currentPage->pageSize = m_pageLayout.fullRectPoints().size();
    writePage();

    delete currentPage;
    currentPage = new QPdfPage;
    currentPage->pageSize = m_pageLayout.fullRectPoints().size();
    stroker.stream = currentPage;
    pages.append(requestObject());

    *currentPage << "/GSa gs /CSp cs /CSp CS\n"
                 << QPdf::generateMatrix(pageMatrix())
                 << "q q\n";
}

QT_END_NAMESPACE

#endif // QT_NO_PDF
