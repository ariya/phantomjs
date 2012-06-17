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

#include "qcolormap.h"
#include "qcolor.h"

QT_BEGIN_NAMESPACE

class QColormapPrivate
{
public:
    inline QColormapPrivate()
        : ref(1)
    { }

    QAtomicInt ref;
};

void QColormap::initialize()
{
}

void QColormap::cleanup()
{
}

QColormap QColormap::instance(int)
{
    return QColormap();
}

QColormap::QColormap() : d(new QColormapPrivate)
{}

QColormap::QColormap(const QColormap &colormap) :d (colormap.d)
{ d->ref.ref(); }

QColormap::~QColormap()
{
    if (!d->ref.deref())
        delete d;
}

QColormap::Mode QColormap::mode() const
{ return QColormap::Direct; }

int QColormap::depth() const
{
    return 32;
}

int QColormap::size() const
{
    return -1;
}

uint QColormap::pixel(const QColor &color) const
{ return color.rgba(); }

const QColor QColormap::colorAt(uint pixel) const
{ return QColor(pixel); }

const QVector<QColor> QColormap::colormap() const
{ return QVector<QColor>(); }

QColormap &QColormap::operator=(const QColormap &colormap)
{ qAtomicAssign(d, colormap.d); return *this; }

QT_END_NAMESPACE
