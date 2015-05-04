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

#ifndef QREGION_H
#define QREGION_H

#include <QtCore/qatomic.h>
#include <QtCore/qrect.h>
#include <QtGui/qwindowdefs.h>

#ifndef QT_NO_DATASTREAM
#include <QtCore/qdatastream.h>
#endif

QT_BEGIN_NAMESPACE


template <class T> class QVector;
class QVariant;

struct QRegionPrivate;

class QBitmap;

class Q_GUI_EXPORT QRegion
{
public:
    enum RegionType { Rectangle, Ellipse };

    QRegion();
    QRegion(int x, int y, int w, int h, RegionType t = Rectangle);
    QRegion(const QRect &r, RegionType t = Rectangle);
    QRegion(const QPolygon &pa, Qt::FillRule fillRule = Qt::OddEvenFill);
    QRegion(const QRegion &region);
    QRegion(const QBitmap &bitmap);
    ~QRegion();
    QRegion &operator=(const QRegion &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QRegion &operator=(QRegion &&other)
    { qSwap(d, other.d); return *this; }
#endif
    inline void swap(QRegion &other) { qSwap(d, other.d); }
    bool isEmpty() const;
    bool isNull() const;

    bool contains(const QPoint &p) const;
    bool contains(const QRect &r) const;

    void translate(int dx, int dy);
    inline void translate(const QPoint &p) { translate(p.x(), p.y()); }
    QRegion translated(int dx, int dy) const Q_REQUIRED_RESULT;
    inline QRegion translated(const QPoint &p) const Q_REQUIRED_RESULT { return translated(p.x(), p.y()); }

    QRegion united(const QRegion &r) const Q_REQUIRED_RESULT;
    QRegion united(const QRect &r) const Q_REQUIRED_RESULT;
    QRegion intersected(const QRegion &r) const Q_REQUIRED_RESULT;
    QRegion intersected(const QRect &r) const Q_REQUIRED_RESULT;
    QRegion subtracted(const QRegion &r) const Q_REQUIRED_RESULT;
    QRegion xored(const QRegion &r) const Q_REQUIRED_RESULT;

#if QT_DEPRECATED_SINCE(5, 0)
    inline QT_DEPRECATED QRegion unite(const QRegion &r) const Q_REQUIRED_RESULT { return united(r); }
    inline QT_DEPRECATED QRegion unite(const QRect &r) const Q_REQUIRED_RESULT { return united(r); }
    inline QT_DEPRECATED QRegion intersect(const QRegion &r) const Q_REQUIRED_RESULT { return intersected(r); }
    inline QT_DEPRECATED QRegion intersect(const QRect &r) const Q_REQUIRED_RESULT { return intersected(r); }
    inline QT_DEPRECATED QRegion subtract(const QRegion &r) const Q_REQUIRED_RESULT { return subtracted(r); }
    inline QT_DEPRECATED QRegion eor(const QRegion &r) const Q_REQUIRED_RESULT { return xored(r); }
#endif

    bool intersects(const QRegion &r) const;
    bool intersects(const QRect &r) const;

    QRect boundingRect() const;
    QVector<QRect> rects() const;
    void setRects(const QRect *rect, int num);
    int rectCount() const;
#ifdef Q_COMPILER_MANGLES_RETURN_TYPE
    // ### Qt 6: remove these, they're kept for MSVC compat
    const QRegion operator|(const QRegion &r) const;
    const QRegion operator+(const QRegion &r) const;
    const QRegion operator+(const QRect &r) const;
    const QRegion operator&(const QRegion &r) const;
    const QRegion operator&(const QRect &r) const;
    const QRegion operator-(const QRegion &r) const;
    const QRegion operator^(const QRegion &r) const;
#else
    QRegion operator|(const QRegion &r) const;
    QRegion operator+(const QRegion &r) const;
    QRegion operator+(const QRect &r) const;
    QRegion operator&(const QRegion &r) const;
    QRegion operator&(const QRect &r) const;
    QRegion operator-(const QRegion &r) const;
    QRegion operator^(const QRegion &r) const;
#endif // Q_COMPILER_MANGLES_RETURN_TYPE
    QRegion& operator|=(const QRegion &r);
    QRegion& operator+=(const QRegion &r);
    QRegion& operator+=(const QRect &r);
    QRegion& operator&=(const QRegion &r);
    QRegion& operator&=(const QRect &r);
    QRegion& operator-=(const QRegion &r);
    QRegion& operator^=(const QRegion &r);

    bool operator==(const QRegion &r) const;
    inline bool operator!=(const QRegion &r) const { return !(operator==(r)); }
    operator QVariant() const;

#ifndef QT_NO_DATASTREAM
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QRegion &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QRegion &);
#endif
private:
    QRegion copy() const;   // helper of detach.
    void detach();
Q_GUI_EXPORT
    friend bool qt_region_strictContains(const QRegion &region,
                                         const QRect &rect);
    friend struct QRegionPrivate;

#ifndef QT_NO_DATASTREAM
    void exec(const QByteArray &ba, int ver = 0, QDataStream::ByteOrder byteOrder = QDataStream::BigEndian);
#endif
    struct QRegionData {
        QtPrivate::RefCount ref;
        QRegionPrivate *qt_rgn;
    };
    struct QRegionData *d;
    static const struct QRegionData shared_empty;
    static void cleanUp(QRegionData *x);
};

/*****************************************************************************
  QRegion stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QRegion &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QRegion &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QRegion &);
#endif

QT_END_NAMESPACE

#endif // QREGION_H
