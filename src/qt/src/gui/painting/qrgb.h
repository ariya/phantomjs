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

#ifndef QRGB_H
#define QRGB_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

typedef unsigned int QRgb;                        // RGB triplet

const QRgb  RGB_MASK    = 0x00ffffff;                // masks RGB values

Q_GUI_EXPORT_INLINE int qRed(QRgb rgb)                // get red part of RGB
{ return ((rgb >> 16) & 0xff); }

Q_GUI_EXPORT_INLINE int qGreen(QRgb rgb)                // get green part of RGB
{ return ((rgb >> 8) & 0xff); }

Q_GUI_EXPORT_INLINE int qBlue(QRgb rgb)                // get blue part of RGB
{ return (rgb & 0xff); }

Q_GUI_EXPORT_INLINE int qAlpha(QRgb rgb)                // get alpha part of RGBA
{ return rgb >> 24; }

Q_GUI_EXPORT_INLINE QRgb qRgb(int r, int g, int b)// set RGB value
{ return (0xffu << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

Q_GUI_EXPORT_INLINE QRgb qRgba(int r, int g, int b, int a)// set RGBA value
{ return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

Q_GUI_EXPORT_INLINE int qGray(int r, int g, int b)// convert R,G,B to gray 0..255
{ return (r*11+g*16+b*5)/32; }

Q_GUI_EXPORT_INLINE int qGray(QRgb rgb)                // convert RGB to gray 0..255
{ return qGray(qRed(rgb), qGreen(rgb), qBlue(rgb)); }

Q_GUI_EXPORT_INLINE bool qIsGray(QRgb rgb)
{ return qRed(rgb) == qGreen(rgb) && qRed(rgb) == qBlue(rgb); }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QRGB_H
