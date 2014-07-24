/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qopenglpixeltransferoptions.h"
#include <QSharedData>

QT_BEGIN_NAMESPACE

class QOpenGLPixelTransferOptionsData : public QSharedData
{
public:
    QOpenGLPixelTransferOptionsData()
        : alignment(4)
        , skipImages(0)
        , skipRows(0)
        , skipPixels(0)
        , imageHeight(0)
        , rowLength(0)
        , lsbFirst(false)
        , swapBytes(false)
    {}

    int alignment;
    int skipImages;
    int skipRows;
    int skipPixels;
    int imageHeight;
    int rowLength;
    bool lsbFirst;
    bool swapBytes;
};

QOpenGLPixelTransferOptions::QOpenGLPixelTransferOptions()
    : data(new QOpenGLPixelTransferOptionsData)
{
}

QOpenGLPixelTransferOptions::QOpenGLPixelTransferOptions(const QOpenGLPixelTransferOptions &rhs)
    : data(rhs.data)
{
}

QOpenGLPixelTransferOptions &QOpenGLPixelTransferOptions::operator=(const QOpenGLPixelTransferOptions &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QOpenGLPixelTransferOptions::~QOpenGLPixelTransferOptions()
{
}

void QOpenGLPixelTransferOptions::setAlignment(int alignment)
{
    data->alignment = alignment;
}

int QOpenGLPixelTransferOptions::alignment() const
{
    return data->alignment;
}

void QOpenGLPixelTransferOptions::setSkipImages(int skipImages)
{
    data->skipImages = skipImages;
}

int QOpenGLPixelTransferOptions::skipImages() const
{
    return data->skipImages;
}

void QOpenGLPixelTransferOptions::setSkipRows(int skipRows)
{
    data->skipRows = skipRows;
}

int QOpenGLPixelTransferOptions::skipRows() const
{
    return data->skipRows;
}

void QOpenGLPixelTransferOptions::setSkipPixels(int skipPixels)
{
    data->skipPixels = skipPixels;
}

int QOpenGLPixelTransferOptions::skipPixels() const
{
    return data->skipPixels;
}

void QOpenGLPixelTransferOptions::setImageHeight(int imageHeight)
{
    data->imageHeight = imageHeight;
}

int QOpenGLPixelTransferOptions::imageHeight() const
{
    return data->imageHeight;
}

void QOpenGLPixelTransferOptions::setRowLength(int rowLength)
{
    data->rowLength = rowLength;
}

int QOpenGLPixelTransferOptions::rowLength() const
{
    return data->rowLength;
}

void QOpenGLPixelTransferOptions::setLeastSignificantByteFirst(bool lsbFirst)
{
    data->lsbFirst = lsbFirst;
}

bool QOpenGLPixelTransferOptions::isLeastSignificantBitFirst() const
{
    return data->lsbFirst;
}

void QOpenGLPixelTransferOptions::setSwapBytesEnabled(bool swapBytes)
{
    data->swapBytes = swapBytes;
}

bool QOpenGLPixelTransferOptions::isSwapBytesEnabled() const
{
    return data->swapBytes;
}

QT_END_NAMESPACE
