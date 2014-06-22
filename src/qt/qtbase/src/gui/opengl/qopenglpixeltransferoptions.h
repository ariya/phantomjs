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

#ifndef QOPENGLPIXELUPLOADOPTIONS_H
#define QOPENGLPIXELUPLOADOPTIONS_H

#include <QtCore/qglobal.h>

#if !defined(QT_NO_OPENGL)

#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE

class QOpenGLPixelTransferOptionsData;

class Q_GUI_EXPORT QOpenGLPixelTransferOptions
{
public:
    QOpenGLPixelTransferOptions();
    QOpenGLPixelTransferOptions(const QOpenGLPixelTransferOptions &);
    QOpenGLPixelTransferOptions &operator=(const QOpenGLPixelTransferOptions &);
    ~QOpenGLPixelTransferOptions();

#ifdef Q_COMPILER_RVALUE_REFS
    QOpenGLPixelTransferOptions &operator=(QOpenGLPixelTransferOptions &&other)
    { swap(other); return *this; }
#endif

    void swap(QOpenGLPixelTransferOptions &other)
    { data.swap(other.data); }

    void setAlignment(int alignment);
    int alignment() const;

    void setSkipImages(int skipImages);
    int skipImages() const;

    void setSkipRows(int skipRows);
    int skipRows() const;

    void setSkipPixels(int skipPixels);
    int skipPixels() const;

    void setImageHeight(int imageHeight);
    int imageHeight() const;

    void setRowLength(int rowLength);
    int rowLength() const;

    void setLeastSignificantByteFirst(bool lsbFirst);
    bool isLeastSignificantBitFirst() const;

    void setSwapBytesEnabled(bool swapBytes);
    bool isSwapBytesEnabled() const;

private:
    QSharedDataPointer<QOpenGLPixelTransferOptionsData> data;
};

Q_DECLARE_SHARED(QOpenGLPixelTransferOptions)

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLPIXELUPLOADOPTIONS_H
