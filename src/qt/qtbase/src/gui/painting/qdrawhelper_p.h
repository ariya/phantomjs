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

#ifndef QDRAWHELPER_P_H
#define QDRAWHELPER_P_H

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

#include "QtCore/qglobal.h"
#include "QtGui/qcolor.h"
#include "QtGui/qpainter.h"
#include "QtGui/qimage.h"
#ifndef QT_FT_BEGIN_HEADER
#define QT_FT_BEGIN_HEADER
#define QT_FT_END_HEADER
#endif
#include "private/qrasterdefs_p.h"
#include <private/qsimd_p.h>
#include <private/qmath_p.h>

QT_BEGIN_NAMESPACE

#if defined(Q_CC_RVCT)
// RVCT doesn't like static template functions
#  define Q_STATIC_TEMPLATE_FUNCTION
#  define Q_ALWAYS_INLINE __forceinline
#  define Q_DECL_RESTRICT
#elif defined(Q_CC_GNU)
#  define Q_STATIC_TEMPLATE_FUNCTION static
#  define Q_ALWAYS_INLINE inline __attribute__((always_inline))
#  define Q_DECL_RESTRICT __restrict__
#elif defined(Q_CC_MSVC)
#  define Q_STATIC_TEMPLATE_FUNCTION static
#  define Q_ALWAYS_INLINE __forceinline
#  define Q_DECL_RESTRICT __restrict
#else
#  define Q_STATIC_TEMPLATE_FUNCTION static
#  define Q_ALWAYS_INLINE inline
#  define Q_DECL_RESTRICT
#endif

static const uint AMASK = 0xff000000;
static const uint RMASK = 0x00ff0000;
static const uint GMASK = 0x0000ff00;
static const uint BMASK = 0x000000ff;

/*******************************************************************************
 * QSpan
 *
 * duplicate definition of FT_Span
 */
typedef QT_FT_Span QSpan;

struct QSolidData;
struct QTextureData;
struct QGradientData;
struct QLinearGradientData;
struct QRadialGradientData;
struct QConicalGradientData;
struct QSpanData;
class QGradient;
class QRasterBuffer;
class QClipData;
class QRasterPaintEngineState;

typedef QT_FT_SpanFunc ProcessSpans;
typedef void (*BitmapBlitFunc)(QRasterBuffer *rasterBuffer,
                               int x, int y, quint32 color,
                               const uchar *bitmap,
                               int mapWidth, int mapHeight, int mapStride);

typedef void (*AlphamapBlitFunc)(QRasterBuffer *rasterBuffer,
                                 int x, int y, quint32 color,
                                 const uchar *bitmap,
                                 int mapWidth, int mapHeight, int mapStride,
                                 const QClipData *clip);

typedef void (*AlphaRGBBlitFunc)(QRasterBuffer *rasterBuffer,
                                 int x, int y, quint32 color,
                                 const uint *rgbmask,
                                 int mapWidth, int mapHeight, int mapStride,
                                 const QClipData *clip);

typedef void (*RectFillFunc)(QRasterBuffer *rasterBuffer,
                             int x, int y, int width, int height,
                             quint32 color);

typedef void (*SrcOverBlendFunc)(uchar *destPixels, int dbpl,
                                 const uchar *src, int spbl,
                                 int w, int h,
                                 int const_alpha);

typedef void (*SrcOverScaleFunc)(uchar *destPixels, int dbpl,
                                 const uchar *src, int spbl, int srch,
                                 const QRectF &targetRect,
                                 const QRectF &sourceRect,
                                 const QRect &clipRect,
                                 int const_alpha);

typedef void (*SrcOverTransformFunc)(uchar *destPixels, int dbpl,
                                     const uchar *src, int spbl,
                                     const QRectF &targetRect,
                                     const QRectF &sourceRect,
                                     const QRect &clipRect,
                                     const QTransform &targetRectTransform,
                                     int const_alpha);

typedef void (*MemRotateFunc)(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl);

struct DrawHelper {
    ProcessSpans blendColor;
    ProcessSpans blendGradient;
    BitmapBlitFunc bitmapBlit;
    AlphamapBlitFunc alphamapBlit;
    AlphaRGBBlitFunc alphaRGBBlit;
    RectFillFunc fillRect;
};

extern SrcOverBlendFunc qBlendFunctions[QImage::NImageFormats][QImage::NImageFormats];
extern SrcOverScaleFunc qScaleFunctions[QImage::NImageFormats][QImage::NImageFormats];
extern SrcOverTransformFunc qTransformFunctions[QImage::NImageFormats][QImage::NImageFormats];
extern MemRotateFunc qMemRotateFunctions[QImage::NImageFormats][3];

extern DrawHelper qDrawHelper[QImage::NImageFormats];

void qBlendTexture(int count, const QSpan *spans, void *userData);
extern void qt_memfill32(quint32 *dest, quint32 value, int count);
extern void qt_memfill16(quint16 *dest, quint16 value, int count);

typedef void (QT_FASTCALL *CompositionFunction)(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha);
typedef void (QT_FASTCALL *CompositionFunctionSolid)(uint *dest, int length, uint color, uint const_alpha);

struct LinearGradientValues
{
    qreal dx;
    qreal dy;
    qreal l;
    qreal off;
};

struct RadialGradientValues
{
    qreal dx;
    qreal dy;
    qreal dr;
    qreal sqrfr;
    qreal a;
    qreal inv2a;
    bool extended;
};

struct Operator;
typedef uint* (QT_FASTCALL *DestFetchProc)(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length);
typedef void (QT_FASTCALL *DestStoreProc)(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length);
typedef const uint* (QT_FASTCALL *SourceFetchProc)(uint *buffer, const Operator *o, const QSpanData *data, int y, int x, int length);

struct Operator
{
    QPainter::CompositionMode mode;
    DestFetchProc dest_fetch;
    DestStoreProc dest_store;
    SourceFetchProc src_fetch;
    CompositionFunctionSolid funcSolid;
    CompositionFunction func;
    union {
        LinearGradientValues linear;
        RadialGradientValues radial;
    };
};

void qInitDrawhelperAsm();

class QRasterPaintEngine;

struct QSolidData
{
    uint color;
};

struct QLinearGradientData
{
    struct {
        qreal x;
        qreal y;
    } origin;
    struct {
        qreal x;
        qreal y;
    } end;
};

struct QRadialGradientData
{
    struct {
        qreal x;
        qreal y;
        qreal radius;
    } center;
    struct {
        qreal x;
        qreal y;
        qreal radius;
    } focal;
};

struct QConicalGradientData
{
    struct {
        qreal x;
        qreal y;
    } center;
    qreal angle;
};

struct QGradientData
{
    QGradient::Spread spread;

    union {
        QLinearGradientData linear;
        QRadialGradientData radial;
        QConicalGradientData conical;
    };

#define GRADIENT_STOPTABLE_SIZE 1024
#define GRADIENT_STOPTABLE_SIZE_SHIFT 10

    uint* colorTable; //[GRADIENT_STOPTABLE_SIZE];

    uint alphaColor : 1;
};

struct QTextureData
{
    const uchar *imageData;
    const uchar *scanLine(int y) const { return imageData + y*bytesPerLine; }

    int width;
    int height;
    // clip rect
    int x1;
    int y1;
    int x2;
    int y2;
    int bytesPerLine;
    QImage::Format format;
    const QVector<QRgb> *colorTable;
    bool hasAlpha;
    enum Type {
        Plain,
        Tiled
    };
    Type type;
    int const_alpha;
};

struct QSpanData
{
    QSpanData() : tempImage(0) {}
    ~QSpanData() { delete tempImage; }

    QRasterBuffer *rasterBuffer;
    ProcessSpans blend;
    ProcessSpans unclipped_blend;
    BitmapBlitFunc bitmapBlit;
    AlphamapBlitFunc alphamapBlit;
    AlphaRGBBlitFunc alphaRGBBlit;
    RectFillFunc fillRect;
    qreal m11, m12, m13, m21, m22, m23, m33, dx, dy;   // inverse xform matrix
    const QClipData *clip;
    enum Type {
        None,
        Solid,
        LinearGradient,
        RadialGradient,
        ConicalGradient,
        Texture
    } type : 8;
    int txop : 8;
    int fast_matrix : 1;
    bool bilinear;
    QImage *tempImage;
    union {
        QSolidData solid;
        QGradientData gradient;
        QTextureData texture;
    };

    void init(QRasterBuffer *rb, const QRasterPaintEngine *pe);
    void setup(const QBrush &brush, int alpha, QPainter::CompositionMode compositionMode);
    void setupMatrix(const QTransform &matrix, int bilinear);
    void initTexture(const QImage *image, int alpha, QTextureData::Type = QTextureData::Plain, const QRect &sourceRect = QRect());
    void adjustSpanMethods();
};

struct QDrawHelperGammaTables
{
    explicit QDrawHelperGammaTables(qreal smoothing);

    void refresh(qreal smoothing);

    uchar qt_pow_rgb_gamma[256];
    uchar qt_pow_rgb_invgamma[256];
    uint qt_pow_gamma[256];
    uchar qt_pow_invgamma[2048];
};

static inline uint qt_gradient_clamp(const QGradientData *data, int ipos)
{
    if (ipos < 0 || ipos >= GRADIENT_STOPTABLE_SIZE) {
        if (data->spread == QGradient::RepeatSpread) {
            ipos = ipos % GRADIENT_STOPTABLE_SIZE;
            ipos = ipos < 0 ? GRADIENT_STOPTABLE_SIZE + ipos : ipos;
        } else if (data->spread == QGradient::ReflectSpread) {
            const int limit = GRADIENT_STOPTABLE_SIZE * 2;
            ipos = ipos % limit;
            ipos = ipos < 0 ? limit + ipos : ipos;
            ipos = ipos >= GRADIENT_STOPTABLE_SIZE ? limit - 1 - ipos : ipos;
        } else {
            if (ipos < 0)
                ipos = 0;
            else if (ipos >= GRADIENT_STOPTABLE_SIZE)
                ipos = GRADIENT_STOPTABLE_SIZE-1;
        }
    }

    Q_ASSERT(ipos >= 0);
    Q_ASSERT(ipos < GRADIENT_STOPTABLE_SIZE);

    return ipos;
}

static inline uint qt_gradient_pixel(const QGradientData *data, qreal pos)
{
    int ipos = int(pos * (GRADIENT_STOPTABLE_SIZE - 1) + qreal(0.5));
    return data->colorTable[qt_gradient_clamp(data, ipos)];
}

static inline qreal qRadialDeterminant(qreal a, qreal b, qreal c)
{
    return (b * b) - (4 * a * c);
}

template <class RadialFetchFunc> Q_STATIC_TEMPLATE_FUNCTION
const uint * QT_FASTCALL qt_fetch_radial_gradient_template(uint *buffer, const Operator *op, const QSpanData *data,
                                                           int y, int x, int length)
{
    // avoid division by zero
    if (qFuzzyIsNull(op->radial.a)) {
        qt_memfill32(buffer, 0, length);
        return buffer;
    }

    const uint *b = buffer;
    qreal rx = data->m21 * (y + qreal(0.5))
               + data->dx + data->m11 * (x + qreal(0.5));
    qreal ry = data->m22 * (y + qreal(0.5))
               + data->dy + data->m12 * (x + qreal(0.5));
    bool affine = !data->m13 && !data->m23;

    uint *end = buffer + length;
    if (affine) {
        rx -= data->gradient.radial.focal.x;
        ry -= data->gradient.radial.focal.y;

        qreal inv_a = 1 / qreal(2 * op->radial.a);

        const qreal delta_rx = data->m11;
        const qreal delta_ry = data->m12;

        qreal b = 2*(op->radial.dr*data->gradient.radial.focal.radius + rx * op->radial.dx + ry * op->radial.dy);
        qreal delta_b = 2*(delta_rx * op->radial.dx + delta_ry * op->radial.dy);
        const qreal b_delta_b = 2 * b * delta_b;
        const qreal delta_b_delta_b = 2 * delta_b * delta_b;

        const qreal bb = b * b;
        const qreal delta_bb = delta_b * delta_b;

        b *= inv_a;
        delta_b *= inv_a;

        const qreal rxrxryry = rx * rx + ry * ry;
        const qreal delta_rxrxryry = delta_rx * delta_rx + delta_ry * delta_ry;
        const qreal rx_plus_ry = 2*(rx * delta_rx + ry * delta_ry);
        const qreal delta_rx_plus_ry = 2 * delta_rxrxryry;

        inv_a *= inv_a;

        qreal det = (bb - 4 * op->radial.a * (op->radial.sqrfr - rxrxryry)) * inv_a;
        qreal delta_det = (b_delta_b + delta_bb + 4 * op->radial.a * (rx_plus_ry + delta_rxrxryry)) * inv_a;
        const qreal delta_delta_det = (delta_b_delta_b + 4 * op->radial.a * delta_rx_plus_ry) * inv_a;

        RadialFetchFunc::fetch(buffer, end, op, data, det, delta_det, delta_delta_det, b, delta_b);
    } else {
        qreal rw = data->m23 * (y + qreal(0.5))
                   + data->m33 + data->m13 * (x + qreal(0.5));

        while (buffer < end) {
            if (rw == 0) {
                *buffer = 0;
            } else {
                qreal invRw = 1 / rw;
                qreal gx = rx * invRw - data->gradient.radial.focal.x;
                qreal gy = ry * invRw - data->gradient.radial.focal.y;
                qreal b  = 2*(op->radial.dr*data->gradient.radial.focal.radius + gx*op->radial.dx + gy*op->radial.dy);
                qreal det = qRadialDeterminant(op->radial.a, b, op->radial.sqrfr - (gx*gx + gy*gy));

                quint32 result = 0;
                if (det >= 0) {
                    qreal detSqrt = qSqrt(det);

                    qreal s0 = (-b - detSqrt) * op->radial.inv2a;
                    qreal s1 = (-b + detSqrt) * op->radial.inv2a;

                    qreal s = qMax(s0, s1);

                    if (data->gradient.radial.focal.radius + op->radial.dr * s >= 0)
                        result = qt_gradient_pixel(&data->gradient, s);
                }

                *buffer = result;
            }

            rx += data->m11;
            ry += data->m12;
            rw += data->m13;

            ++buffer;
        }
    }

    return b;
}

template <class Simd>
class QRadialFetchSimd
{
public:
    static void fetch(uint *buffer, uint *end, const Operator *op, const QSpanData *data, qreal det,
                      qreal delta_det, qreal delta_delta_det, qreal b, qreal delta_b)
    {
        typename Simd::Vect_buffer_f det_vec;
        typename Simd::Vect_buffer_f delta_det4_vec;
        typename Simd::Vect_buffer_f b_vec;

        for (int i = 0; i < 4; ++i) {
            det_vec.f[i] = det;
            delta_det4_vec.f[i] = 4 * delta_det;
            b_vec.f[i] = b;

            det += delta_det;
            delta_det += delta_delta_det;
            b += delta_b;
        }

        const typename Simd::Float32x4 v_delta_delta_det16 = Simd::v_dup(16 * delta_delta_det);
        const typename Simd::Float32x4 v_delta_delta_det6 = Simd::v_dup(6 * delta_delta_det);
        const typename Simd::Float32x4 v_delta_b4 = Simd::v_dup(4 * delta_b);

        const typename Simd::Float32x4 v_r0 = Simd::v_dup(data->gradient.radial.focal.radius);
        const typename Simd::Float32x4 v_dr = Simd::v_dup(op->radial.dr);

        const typename Simd::Float32x4 v_min = Simd::v_dup(0.0f);
        const typename Simd::Float32x4 v_max = Simd::v_dup(float(GRADIENT_STOPTABLE_SIZE-1));
        const typename Simd::Float32x4 v_half = Simd::v_dup(0.5f);

        const typename Simd::Int32x4 v_repeat_mask = Simd::v_dup(~(uint(0xffffff) << GRADIENT_STOPTABLE_SIZE_SHIFT));
        const typename Simd::Int32x4 v_reflect_mask = Simd::v_dup(~(uint(0xffffff) << (GRADIENT_STOPTABLE_SIZE_SHIFT+1)));

        const typename Simd::Int32x4 v_reflect_limit = Simd::v_dup(2 * GRADIENT_STOPTABLE_SIZE - 1);

        const int extended_mask = op->radial.extended ? 0x0 : ~0x0;

#define FETCH_RADIAL_LOOP_PROLOGUE \
        while (buffer < end) { \
            typename Simd::Vect_buffer_i v_buffer_mask; \
            v_buffer_mask.v = Simd::v_greaterOrEqual(det_vec.v, v_min); \
            const typename Simd::Float32x4 v_index_local = Simd::v_sub(Simd::v_sqrt(Simd::v_max(v_min, det_vec.v)), b_vec.v); \
            const typename Simd::Float32x4 v_index = Simd::v_add(Simd::v_mul(v_index_local, v_max), v_half); \
            v_buffer_mask.v = Simd::v_and(v_buffer_mask.v, Simd::v_greaterOrEqual(Simd::v_add(v_r0, Simd::v_mul(v_dr, v_index_local)), v_min)); \
            typename Simd::Vect_buffer_i index_vec;
#define FETCH_RADIAL_LOOP_CLAMP_REPEAT \
            index_vec.v = Simd::v_and(v_repeat_mask, Simd::v_toInt(v_index));
#define FETCH_RADIAL_LOOP_CLAMP_REFLECT \
            const typename Simd::Int32x4 v_index_i = Simd::v_and(v_reflect_mask, Simd::v_toInt(v_index)); \
            const typename Simd::Int32x4 v_index_i_inv = Simd::v_sub(v_reflect_limit, v_index_i); \
            index_vec.v = Simd::v_min_16(v_index_i, v_index_i_inv);
#define FETCH_RADIAL_LOOP_CLAMP_PAD \
            index_vec.v = Simd::v_toInt(Simd::v_min(v_max, Simd::v_max(v_min, v_index)));
#define FETCH_RADIAL_LOOP_EPILOGUE \
            det_vec.v = Simd::v_add(Simd::v_add(det_vec.v, delta_det4_vec.v), v_delta_delta_det6); \
            delta_det4_vec.v = Simd::v_add(delta_det4_vec.v, v_delta_delta_det16); \
            b_vec.v = Simd::v_add(b_vec.v, v_delta_b4); \
            for (int i = 0; i < 4; ++i) \
                *buffer++ = (extended_mask | v_buffer_mask.i[i]) & data->gradient.colorTable[index_vec.i[i]]; \
        }

#define FETCH_RADIAL_LOOP(FETCH_RADIAL_LOOP_CLAMP) \
        FETCH_RADIAL_LOOP_PROLOGUE \
        FETCH_RADIAL_LOOP_CLAMP \
        FETCH_RADIAL_LOOP_EPILOGUE

        switch (data->gradient.spread) {
        case QGradient::RepeatSpread:
            FETCH_RADIAL_LOOP(FETCH_RADIAL_LOOP_CLAMP_REPEAT)
            break;
        case QGradient::ReflectSpread:
            FETCH_RADIAL_LOOP(FETCH_RADIAL_LOOP_CLAMP_REFLECT)
            break;
        case QGradient::PadSpread:
            FETCH_RADIAL_LOOP(FETCH_RADIAL_LOOP_CLAMP_PAD)
            break;
        default:
            Q_ASSERT(false);
        }
    }
};

#if defined(Q_CC_RVCT)
#  pragma push
#  pragma arm
#endif
static Q_ALWAYS_INLINE uint INTERPOLATE_PIXEL_255(uint x, uint a, uint y, uint b) {
    uint t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}
#if defined(Q_CC_RVCT)
#  pragma pop
#endif

#if QT_POINTER_SIZE == 8 // 64-bit versions

static Q_ALWAYS_INLINE uint INTERPOLATE_PIXEL_256(uint x, uint a, uint y, uint b) {
    quint64 t = (((quint64(x)) | ((quint64(x)) << 24)) & 0x00ff00ff00ff00ff) * a;
    t += (((quint64(y)) | ((quint64(y)) << 24)) & 0x00ff00ff00ff00ff) * b;
    t >>= 8;
    t &= 0x00ff00ff00ff00ff;
    return (uint(t)) | (uint(t >> 24));
}

static Q_ALWAYS_INLINE uint BYTE_MUL(uint x, uint a) {
    quint64 t = (((quint64(x)) | ((quint64(x)) << 24)) & 0x00ff00ff00ff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff00ff00ff) + 0x80008000800080) >> 8;
    t &= 0x00ff00ff00ff00ff;
    return (uint(t)) | (uint(t >> 24));
}

#else // 32-bit versions

static Q_ALWAYS_INLINE uint INTERPOLATE_PIXEL_256(uint x, uint a, uint y, uint b) {
    uint t = (x & 0xff00ff) * a + (y & 0xff00ff) * b;
    t >>= 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a + ((y >> 8) & 0xff00ff) * b;
    x &= 0xff00ff00;
    x |= t;
    return x;
}

#if defined(Q_CC_RVCT)
#  pragma push
#  pragma arm
#endif
static Q_ALWAYS_INLINE uint BYTE_MUL(uint x, uint a) {
    uint t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff00ff) * a;
    x = (x + ((x >> 8) & 0xff00ff) + 0x800080);
    x &= 0xff00ff00;
    x |= t;
    return x;
}
#if defined(Q_CC_RVCT)
#  pragma pop
#endif

#endif


#if Q_BYTE_ORDER == Q_BIG_ENDIAN
static Q_ALWAYS_INLINE quint32 RGBA2ARGB(quint32 x) {
    quint32 rgb = x >> 8;
    quint32 a = x << 24;
    return a | rgb;
}

static Q_ALWAYS_INLINE quint32 ARGB2RGBA(quint32 x) {
    quint32 rgb = x << 8;
    quint32 a = x >> 24;
    return a | rgb;
}
#else
static Q_ALWAYS_INLINE quint32 RGBA2ARGB(quint32 x) {
    // RGBA8888 is ABGR32 on little endian.
    quint32 ag = x & 0xff00ff00;
    quint32 rg = x & 0x00ff00ff;
    return ag | (rg  << 16) | (rg >> 16);
}

static Q_ALWAYS_INLINE quint32 ARGB2RGBA(quint32 x) {
    return RGBA2ARGB(x);
}
#endif

static Q_ALWAYS_INLINE uint BYTE_MUL_RGB16(uint x, uint a) {
    a += 1;
    uint t = (((x & 0x07e0)*a) >> 8) & 0x07e0;
    t |= (((x & 0xf81f)*(a>>2)) >> 6) & 0xf81f;
    return t;
}

static Q_ALWAYS_INLINE uint BYTE_MUL_RGB16_32(uint x, uint a) {
    uint t = (((x & 0xf81f07e0) >> 5)*a) & 0xf81f07e0;
    t |= (((x & 0x07e0f81f)*a) >> 5) & 0x07e0f81f;
    return t;
}

// FIXME: Remove when all Qt modules have stopped using PREMUL and INV_PREMUL
#define PREMUL(x) qPremultiply(x)
#define INV_PREMUL(p) qUnpremultiply(p)

struct quint24 {
    quint24(uint value);
    operator uint() const;
    uchar data[3];
};

inline quint24::quint24(uint value)
{
    data[0] = uchar(value >> 16);
    data[1] = uchar(value >> 8);
    data[2] = uchar(value);
}

inline quint24::operator uint() const
{
    return data[2] | (data[1] << 8) | (data[0] << 16);
}

template <class T> Q_STATIC_TEMPLATE_FUNCTION
void qt_memfill(T *dest, T value, int count);

template<> inline void qt_memfill(quint32 *dest, quint32 color, int count)
{
    qt_memfill32(dest, color, count);
}

template<> inline void qt_memfill(quint16 *dest, quint16 color, int count)
{
    qt_memfill16(dest, color, count);
}

template<> inline void qt_memfill(quint8 *dest, quint8 color, int count)
{
    memset(dest, color, count);
}

template <class T>
inline void qt_memfill(T *dest, T value, int count)
{
    if (!count)
        return;

    int n = (count + 7) / 8;
    switch (count & 0x07)
    {
    case 0: do { *dest++ = value;
    case 7:      *dest++ = value;
    case 6:      *dest++ = value;
    case 5:      *dest++ = value;
    case 4:      *dest++ = value;
    case 3:      *dest++ = value;
    case 2:      *dest++ = value;
    case 1:      *dest++ = value;
    } while (--n > 0);
    }
}

template <class T> Q_STATIC_TEMPLATE_FUNCTION
inline void qt_rectfill(T *dest, T value,
                        int x, int y, int width, int height, int stride)
{
    char *d = reinterpret_cast<char*>(dest + x) + y * stride;
    if (uint(stride) == (width * sizeof(T))) {
        qt_memfill(reinterpret_cast<T*>(d), value, width * height);
    } else {
        for (int j = 0; j < height; ++j) {
            dest = reinterpret_cast<T*>(d);
            qt_memfill(dest, value, width);
            d += stride;
        }
    }
}

#define QT_MEMFILL_UINT(dest, length, color)            \
    qt_memfill<quint32>(dest, color, length);

#define QT_MEMFILL_USHORT(dest, length, color) \
    qt_memfill<quint16>(dest, color, length);

#define QT_MEMCPY_REV_UINT(dest, src, length) \
do {                                          \
    /* Duff's device */                       \
    uint *_d = (uint*)(dest) + length;         \
    const uint *_s = (uint*)(src) + length;    \
    int n = ((length) + 7) / 8;               \
    switch ((length) & 0x07)                  \
    {                                         \
    case 0: do { *--_d = *--_s;                 \
    case 7:      *--_d = *--_s;                 \
    case 6:      *--_d = *--_s;                 \
    case 5:      *--_d = *--_s;                 \
    case 4:      *--_d = *--_s;                 \
    case 3:      *--_d = *--_s;                 \
    case 2:      *--_d = *--_s;                 \
    case 1:      *--_d = *--_s;                 \
    } while (--n > 0);                        \
    }                                         \
} while (0)

#define QT_MEMCPY_USHORT(dest, src, length) \
do {                                          \
    /* Duff's device */                       \
    ushort *_d = (ushort*)(dest);         \
    const ushort *_s = (ushort*)(src);    \
    int n = ((length) + 7) / 8;               \
    switch ((length) & 0x07)                  \
    {                                         \
    case 0: do { *_d++ = *_s++;                 \
    case 7:      *_d++ = *_s++;                 \
    case 6:      *_d++ = *_s++;                 \
    case 5:      *_d++ = *_s++;                 \
    case 4:      *_d++ = *_s++;                 \
    case 3:      *_d++ = *_s++;                 \
    case 2:      *_d++ = *_s++;                 \
    case 1:      *_d++ = *_s++;                 \
    } while (--n > 0);                        \
    }                                         \
} while (0)

#if defined(Q_CC_RVCT)
#  pragma push
#  pragma arm
#endif
static Q_ALWAYS_INLINE int qt_div_255(int x) { return (x + (x>>8) + 0x80) >> 8; }
#if defined(Q_CC_RVCT)
#  pragma pop
#endif

inline ushort qConvertRgb32To16(uint c)
{
   return (((c) >> 3) & 0x001f)
       | (((c) >> 5) & 0x07e0)
       | (((c) >> 8) & 0xf800);
}

inline QRgb qConvertRgb16To32(uint c)
{
    return 0xff000000
        | ((((c) << 3) & 0xf8) | (((c) >> 2) & 0x7))
        | ((((c) << 5) & 0xfc00) | (((c) >> 1) & 0x300))
        | ((((c) << 8) & 0xf80000) | (((c) << 3) & 0x70000));
}

inline int qRed565(quint16 rgb) {
    const int r = (rgb & 0xf800);
    return (r >> 8) | (r >> 13);
}

inline int qGreen565(quint16 rgb) {
    const int g = (rgb & 0x07e0);
    return (g >> 3) | (g >> 9);
}

inline int qBlue565(quint16 rgb) {
    const int b = (rgb & 0x001f);
    return (b << 3) | (b >> 2);
}

const uint qt_bayer_matrix[16][16] = {
    { 0x1, 0xc0, 0x30, 0xf0, 0xc, 0xcc, 0x3c, 0xfc,
      0x3, 0xc3, 0x33, 0xf3, 0xf, 0xcf, 0x3f, 0xff},
    { 0x80, 0x40, 0xb0, 0x70, 0x8c, 0x4c, 0xbc, 0x7c,
      0x83, 0x43, 0xb3, 0x73, 0x8f, 0x4f, 0xbf, 0x7f},
    { 0x20, 0xe0, 0x10, 0xd0, 0x2c, 0xec, 0x1c, 0xdc,
      0x23, 0xe3, 0x13, 0xd3, 0x2f, 0xef, 0x1f, 0xdf},
    { 0xa0, 0x60, 0x90, 0x50, 0xac, 0x6c, 0x9c, 0x5c,
      0xa3, 0x63, 0x93, 0x53, 0xaf, 0x6f, 0x9f, 0x5f},
    { 0x8, 0xc8, 0x38, 0xf8, 0x4, 0xc4, 0x34, 0xf4,
      0xb, 0xcb, 0x3b, 0xfb, 0x7, 0xc7, 0x37, 0xf7},
    { 0x88, 0x48, 0xb8, 0x78, 0x84, 0x44, 0xb4, 0x74,
      0x8b, 0x4b, 0xbb, 0x7b, 0x87, 0x47, 0xb7, 0x77},
    { 0x28, 0xe8, 0x18, 0xd8, 0x24, 0xe4, 0x14, 0xd4,
      0x2b, 0xeb, 0x1b, 0xdb, 0x27, 0xe7, 0x17, 0xd7},
    { 0xa8, 0x68, 0x98, 0x58, 0xa4, 0x64, 0x94, 0x54,
      0xab, 0x6b, 0x9b, 0x5b, 0xa7, 0x67, 0x97, 0x57},
    { 0x2, 0xc2, 0x32, 0xf2, 0xe, 0xce, 0x3e, 0xfe,
      0x1, 0xc1, 0x31, 0xf1, 0xd, 0xcd, 0x3d, 0xfd},
    { 0x82, 0x42, 0xb2, 0x72, 0x8e, 0x4e, 0xbe, 0x7e,
      0x81, 0x41, 0xb1, 0x71, 0x8d, 0x4d, 0xbd, 0x7d},
    { 0x22, 0xe2, 0x12, 0xd2, 0x2e, 0xee, 0x1e, 0xde,
      0x21, 0xe1, 0x11, 0xd1, 0x2d, 0xed, 0x1d, 0xdd},
    { 0xa2, 0x62, 0x92, 0x52, 0xae, 0x6e, 0x9e, 0x5e,
      0xa1, 0x61, 0x91, 0x51, 0xad, 0x6d, 0x9d, 0x5d},
    { 0xa, 0xca, 0x3a, 0xfa, 0x6, 0xc6, 0x36, 0xf6,
      0x9, 0xc9, 0x39, 0xf9, 0x5, 0xc5, 0x35, 0xf5},
    { 0x8a, 0x4a, 0xba, 0x7a, 0x86, 0x46, 0xb6, 0x76,
      0x89, 0x49, 0xb9, 0x79, 0x85, 0x45, 0xb5, 0x75},
    { 0x2a, 0xea, 0x1a, 0xda, 0x26, 0xe6, 0x16, 0xd6,
      0x29, 0xe9, 0x19, 0xd9, 0x25, 0xe5, 0x15, 0xd5},
    { 0xaa, 0x6a, 0x9a, 0x5a, 0xa6, 0x66, 0x96, 0x56,
      0xa9, 0x69, 0x99, 0x59, 0xa5, 0x65, 0x95, 0x55}
};

#define ARGB_COMBINE_ALPHA(argb, alpha) \
    ((((argb >> 24) * alpha) >> 8) << 24) | (argb & 0x00ffffff)


#if QT_POINTER_SIZE == 8 // 64-bit versions
#define AMIX(mask) (qMin(((qint64(s)&mask) + (qint64(d)&mask)), qint64(mask)))
#define MIX(mask) (qMin(((qint64(s)&mask) + (qint64(d)&mask)), qint64(mask)))
#else // 32 bits
// The mask for alpha can overflow over 32 bits
#define AMIX(mask) quint32(qMin(((qint64(s)&mask) + (qint64(d)&mask)), qint64(mask)))
#define MIX(mask) (qMin(((quint32(s)&mask) + (quint32(d)&mask)), quint32(mask)))
#endif

inline int comp_func_Plus_one_pixel_const_alpha(uint d, const uint s, const uint const_alpha, const uint one_minus_const_alpha)
{
    const int result = (AMIX(AMASK) | MIX(RMASK) | MIX(GMASK) | MIX(BMASK));
    return INTERPOLATE_PIXEL_255(result, const_alpha, d, one_minus_const_alpha);
}

inline int comp_func_Plus_one_pixel(uint d, const uint s)
{
    const int result = (AMIX(AMASK) | MIX(RMASK) | MIX(GMASK) | MIX(BMASK));
    return result;
}

#undef MIX
#undef AMIX

// prototypes of all the composition functions
void QT_FASTCALL comp_func_SourceOver(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_DestinationOver(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha);
void QT_FASTCALL comp_func_Source(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Destination(uint *, const uint *, int, uint);
void QT_FASTCALL comp_func_SourceIn(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_DestinationIn(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_SourceOut(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_DestinationOut(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_SourceAtop(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_DestinationAtop(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_XOR(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Plus(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Multiply(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Screen(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Overlay(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Darken(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Lighten(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_ColorDodge(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_ColorBurn(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_HardLight(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_SoftLight(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Difference(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL comp_func_Exclusion(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_SourceOrDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_SourceAndDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_SourceXorDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotSourceAndNotDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotSourceOrNotDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotSourceXorDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotSource(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotSourceAndDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_SourceAndNotDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotSourceOrDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_SourceOrNotDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_ClearDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_SetDestination(uint *dest, const uint *src, int length, uint const_alpha);
void QT_FASTCALL rasterop_NotDestination(uint *dest, const uint *src, int length, uint const_alpha);
// prototypes of all the solid composition functions
void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Destination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Plus(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Multiply(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Screen(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Overlay(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Darken(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Lighten(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_ColorDodge(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_ColorBurn(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_HardLight(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_SoftLight(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Difference(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_solid_Exclusion(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_SourceOrDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_SourceAndDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_SourceXorDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotSourceAndNotDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotSourceOrNotDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotSourceXorDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotSource(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotSourceAndDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_SourceAndNotDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotSourceOrDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_SourceOrNotDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_ClearDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_SetDestination(uint *dest, int length, uint color, uint const_alpha);
void QT_FASTCALL rasterop_solid_NotDestination(uint *dest, int length, uint color, uint const_alpha);


struct QPixelLayout;
typedef const uint *(QT_FASTCALL *ConvertFunc)(uint *buffer, const uint *src, int count,
                                               const QPixelLayout *layout, const QRgb *clut);

struct QPixelLayout
{
    // Bits per pixel
    enum BPP {
        BPPNone,
        BPP1MSB,
        BPP1LSB,
        BPP8,
        BPP16,
        BPP24,
        BPP32,
        BPPCount
    };

    // All numbers in bits.
    uchar redWidth;
    uchar redShift;
    uchar greenWidth;
    uchar greenShift;
    uchar blueWidth;
    uchar blueShift;
    uchar alphaWidth;
    uchar alphaShift;
    bool premultiplied;
    BPP bpp;
    ConvertFunc convertToARGB32PM;
    ConvertFunc convertFromARGB32PM;
    ConvertFunc convertFromRGB32;
};

typedef const uint *(QT_FASTCALL *FetchPixelsFunc)(uint *buffer, const uchar *src, int index, int count);
typedef void (QT_FASTCALL *StorePixelsFunc)(uchar *dest, const uint *src, int index, int count);

extern QPixelLayout qPixelLayouts[QImage::NImageFormats];
extern FetchPixelsFunc qFetchPixels[QPixelLayout::BPPCount];
extern StorePixelsFunc qStorePixels[QPixelLayout::BPPCount];



QT_END_NAMESPACE

#endif // QDRAWHELPER_P_H
