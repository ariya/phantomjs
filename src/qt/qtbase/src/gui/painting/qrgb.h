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

#ifndef QRGB_H
#define QRGB_H

#include <QtCore/qglobal.h>
#include <QtCore/qprocessordetection.h>

QT_BEGIN_NAMESPACE


typedef unsigned int QRgb;                        // RGB triplet

// non-namespaced Qt global variable
const Q_DECL_UNUSED QRgb  RGB_MASK    = 0x00ffffff;     // masks RGB values

inline Q_DECL_CONSTEXPR int qRed(QRgb rgb)                // get red part of RGB
{ return ((rgb >> 16) & 0xff); }

inline Q_DECL_CONSTEXPR int qGreen(QRgb rgb)                // get green part of RGB
{ return ((rgb >> 8) & 0xff); }

inline Q_DECL_CONSTEXPR int qBlue(QRgb rgb)                // get blue part of RGB
{ return (rgb & 0xff); }

inline Q_DECL_CONSTEXPR int qAlpha(QRgb rgb)                // get alpha part of RGBA
{ return rgb >> 24; }

inline Q_DECL_CONSTEXPR QRgb qRgb(int r, int g, int b)// set RGB value
{ return (0xffu << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

inline Q_DECL_CONSTEXPR QRgb qRgba(int r, int g, int b, int a)// set RGBA value
{ return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

inline Q_DECL_CONSTEXPR int qGray(int r, int g, int b)// convert R,G,B to gray 0..255
{ return (r*11+g*16+b*5)/32; }

inline Q_DECL_CONSTEXPR int qGray(QRgb rgb)                // convert RGB to gray 0..255
{ return qGray(qRed(rgb), qGreen(rgb), qBlue(rgb)); }

inline Q_DECL_CONSTEXPR bool qIsGray(QRgb rgb)
{ return qRed(rgb) == qGreen(rgb) && qRed(rgb) == qBlue(rgb); }


#if Q_PROCESSOR_WORDSIZE == 8 // 64-bit version
inline QRgb qPremultiply(QRgb x)
{
    const uint a = qAlpha(x);
    quint64 t = (((quint64(x)) | ((quint64(x)) << 24)) & 0x00ff00ff00ff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff00ff00ff) + 0x80008000800080) >> 8;
    t &= 0x000000ff00ff00ff;
    return (uint(t)) | (uint(t >> 24)) | (a << 24);
}
#else // 32-bit version
inline QRgb qPremultiply(QRgb x)
{
    const uint a = qAlpha(x);
    uint t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t &= 0xff00ff;

    x = ((x >> 8) & 0xff) * a;
    x = (x + ((x >> 8) & 0xff) + 0x80);
    x &= 0xff00;
    x |= t | (a << 24);
    return x;
}
#endif

Q_GUI_EXPORT extern const uint qt_inv_premul_factor[];

inline QRgb qUnpremultiply(QRgb p)
{
    const uint alpha = qAlpha(p);
    // Alpha 255 and 0 are the two most common values, which makes them beneficial to short-cut.
    if (alpha == 255)
        return p;
    if (alpha == 0)
        return 0;
    // (p*(0x00ff00ff/alpha)) >> 16 == (p*255)/alpha for all p and alpha <= 256.
    const uint invAlpha = qt_inv_premul_factor[alpha];
    // We add 0x8000 to get even rounding. The rounding also ensures that qPremultiply(qUnpremultiply(p)) == p for all p.
    return qRgba((qRed(p)*invAlpha + 0x8000)>>16, (qGreen(p)*invAlpha + 0x8000)>>16, (qBlue(p)*invAlpha + 0x8000)>>16, alpha);
}

QT_END_NAMESPACE

#endif // QRGB_H
