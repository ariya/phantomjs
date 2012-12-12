/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <private/qt_mac_p.h>
#include "qcoreapplication.h"
#include <qlibrary.h>

QT_BEGIN_NAMESPACE

QRegion::QRegionData QRegion::shared_empty = { Q_BASIC_ATOMIC_INITIALIZER(1), 0 };

#if defined(Q_WS_MAC32) && !defined(QT_MAC_USE_COCOA)
#define RGN_CACHE_SIZE 200
#ifdef RGN_CACHE_SIZE
static bool rgncache_init = false;
static int rgncache_used;
static RgnHandle rgncache[RGN_CACHE_SIZE];
static void qt_mac_cleanup_rgncache()
{
    rgncache_init = false;
    for(int i = 0; i < RGN_CACHE_SIZE; ++i) {
        if(rgncache[i]) {
            --rgncache_used;
            DisposeRgn(rgncache[i]);
            rgncache[i] = 0;
        }
    }
}
#endif

Q_GUI_EXPORT RgnHandle qt_mac_get_rgn()
{
#ifdef RGN_CACHE_SIZE
    if(!rgncache_init) {
        rgncache_used = 0;
        rgncache_init = true;
        for(int i = 0; i < RGN_CACHE_SIZE; ++i)
            rgncache[i] = 0;
        qAddPostRoutine(qt_mac_cleanup_rgncache);
    } else if(rgncache_used) {
        for(int i = 0; i < RGN_CACHE_SIZE; ++i) {
            if(rgncache[i]) {
                RgnHandle ret = rgncache[i];
                SetEmptyRgn(ret);
                rgncache[i] = 0;
                --rgncache_used;
                return ret;
            }
        }
    }
#endif
    return NewRgn();
}

Q_GUI_EXPORT void qt_mac_dispose_rgn(RgnHandle r)
{
#ifdef RGN_CACHE_SIZE
    if(rgncache_init && rgncache_used < RGN_CACHE_SIZE) {
        for(int i = 0; i < RGN_CACHE_SIZE; ++i) {
            if(!rgncache[i]) {
                ++rgncache_used;
                rgncache[i] = r;
                return;
            }
        }
    }
#endif
    DisposeRgn(r);
}

static OSStatus qt_mac_get_rgn_rect(UInt16 msg, RgnHandle, const Rect *rect, void *reg)
{
    if(msg == kQDRegionToRectsMsgParse) {
        QRect rct(rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top));
        if(!rct.isEmpty())
            *((QRegion *)reg) += rct;
    }
    return noErr;
}

Q_GUI_EXPORT QRegion qt_mac_convert_mac_region(RgnHandle rgn)
{
    return QRegion::fromQDRgn(rgn);
}

QRegion QRegion::fromQDRgn(RgnHandle rgn)
{
    QRegion ret;
    ret.detach();
    OSStatus oss = QDRegionToRects(rgn, kQDParseRegionFromTopLeft, qt_mac_get_rgn_rect, (void *)&ret);
    if(oss != noErr)
        return QRegion();
    return ret;
}

/*!
    \internal
     Create's a RegionHandle, it's the caller's responsibility to release.
*/
RgnHandle QRegion::toQDRgn() const
{
    RgnHandle rgnHandle = qt_mac_get_rgn();
    if(d->qt_rgn && d->qt_rgn->numRects) {
        RgnHandle tmp_rgn = qt_mac_get_rgn();
        int n = d->qt_rgn->numRects;
        const QRect *qt_r = (n == 1) ? &d->qt_rgn->extents : d->qt_rgn->rects.constData();
        while (n--) {
            SetRectRgn(tmp_rgn,
                       qMax(SHRT_MIN, qt_r->x()),
                       qMax(SHRT_MIN, qt_r->y()),
                       qMin(SHRT_MAX, qt_r->right() + 1),
                       qMin(SHRT_MAX, qt_r->bottom() + 1));
            UnionRgn(rgnHandle, tmp_rgn, rgnHandle);
            ++qt_r;
        }
        qt_mac_dispose_rgn(tmp_rgn);
    }
    return rgnHandle;
}

/*!
    \internal
     Create's a RegionHandle, it's the caller's responsibility to release.
     Returns 0 if the QRegion overflows.
*/
RgnHandle QRegion::toQDRgnForUpdate_sys() const
{
    RgnHandle rgnHandle = qt_mac_get_rgn();
    if(d->qt_rgn && d->qt_rgn->numRects) {
        RgnHandle tmp_rgn = qt_mac_get_rgn();
        int n = d->qt_rgn->numRects;
        const QRect *qt_r = (n == 1) ? &d->qt_rgn->extents : d->qt_rgn->rects.constData();
        while (n--) {

            // detect overflow. Tested for use with HIViewSetNeedsDisplayInRegion
            // in QWidgetPrivate::update_sys().
            enum { HIViewSetNeedsDisplayInRegionOverflow = 10000 }; // empirically determined conservative value
            if (qt_r->right() > HIViewSetNeedsDisplayInRegionOverflow || qt_r->bottom() > HIViewSetNeedsDisplayInRegionOverflow) {
                qt_mac_dispose_rgn(tmp_rgn);
                qt_mac_dispose_rgn(rgnHandle);
                return 0;
            }

            SetRectRgn(tmp_rgn,
                       qMax(SHRT_MIN, qt_r->x()),
                       qMax(SHRT_MIN, qt_r->y()),
                       qMin(SHRT_MAX, qt_r->right() + 1),
                       qMin(SHRT_MAX, qt_r->bottom() + 1));
            UnionRgn(rgnHandle, tmp_rgn, rgnHandle);
            ++qt_r;
        }
        qt_mac_dispose_rgn(tmp_rgn);
    }
    return rgnHandle;
}

#endif

#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
OSStatus QRegion::shape2QRegionHelper(int inMessage, HIShapeRef,
                                      const CGRect *inRect, void *inRefcon)
{
    QRegion *region = static_cast<QRegion *>(inRefcon);
    if (!region)
        return paramErr;

    switch (inMessage) {
    case kHIShapeEnumerateRect:
        *region += QRect(inRect->origin.x, inRect->origin.y,
                         inRect->size.width, inRect->size.height);
        break;
    case kHIShapeEnumerateInit:
        // Assume the region is already setup correctly
    case kHIShapeEnumerateTerminate:
    default:
        break;
    }
    return noErr;
}
#endif

/*!
    \internal
     Create's a mutable shape, it's the caller's responsibility to release.
     WARNING: this function clamps the coordinates to SHRT_MIN/MAX on 10.4 and below.
*/
HIMutableShapeRef QRegion::toHIMutableShape() const
{
    HIMutableShapeRef shape = HIShapeCreateMutable();
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        if (d->qt_rgn && d->qt_rgn->numRects) {
            int n = d->qt_rgn->numRects;
            const QRect *qt_r = (n == 1) ? &d->qt_rgn->extents : d->qt_rgn->rects.constData();
            while (n--) {
                CGRect cgRect = CGRectMake(qt_r->x(), qt_r->y(), qt_r->width(), qt_r->height());
                HIShapeUnionWithRect(shape, &cgRect);
                ++qt_r;
            }
        }
    } else
#endif
    {
#ifndef QT_MAC_USE_COCOA
        QCFType<HIShapeRef> qdShape = HIShapeCreateWithQDRgn(QMacSmartQuickDrawRegion(toQDRgn()));
        HIShapeUnion(qdShape, shape, shape);
#endif
    }
    return shape;
}

#if !defined(Q_WS_MAC64) && !defined(QT_MAC_USE_COCOA)
typedef OSStatus (*PtrHIShapeGetAsQDRgn)(HIShapeRef, RgnHandle);
static PtrHIShapeGetAsQDRgn ptrHIShapeGetAsQDRgn = 0;
#endif


QRegion QRegion::fromHIShapeRef(HIShapeRef shape)
{
    QRegion returnRegion;
    returnRegion.detach();
    // Begin gratuitous #if-defery
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
# ifndef Q_WS_MAC64
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
# endif
        HIShapeEnumerate(shape, kHIShapeParseFromTopLeft, shape2QRegionHelper, &returnRegion);
# ifndef Q_WS_MAC64
    } else
# endif
#endif
    {
#if !defined(Q_WS_MAC64) && !defined(QT_MAC_USE_COCOA)
        if (ptrHIShapeGetAsQDRgn == 0) {
            QLibrary library(QLatin1String("/System/Library/Frameworks/Carbon.framework/Carbon"));
            library.setLoadHints(QLibrary::ExportExternalSymbolsHint);
                    ptrHIShapeGetAsQDRgn = reinterpret_cast<PtrHIShapeGetAsQDRgn>(library.resolve("HIShapeGetAsQDRgn"));
        }
        RgnHandle rgn = qt_mac_get_rgn();
        ptrHIShapeGetAsQDRgn(shape, rgn);
        returnRegion = QRegion::fromQDRgn(rgn);
        qt_mac_dispose_rgn(rgn);
#endif
    }
    return returnRegion;
}

QT_END_NAMESPACE
