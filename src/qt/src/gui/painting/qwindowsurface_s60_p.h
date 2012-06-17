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

#ifndef QWINDOWSURFACE_S60_P_H
#define QWINDOWSURFACE_S60_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qglobal.h>
#include "private/qwindowsurface_p.h"

class CFbsBitmap;

QT_BEGIN_NAMESPACE

struct QS60WindowSurfacePrivate;

class QS60WindowSurface : public QWindowSurface
{
public:
    QS60WindowSurface(QWidget *widget);
    ~QS60WindowSurface();

    QPaintDevice *paintDevice();
    void flush(QWidget *widget, const QRegion &region, const QPoint &offset);
    bool scroll(const QRegion &area, int dx, int dy);

    void beginPaint(const QRegion &);
    void endPaint(const QRegion &);

    QImage* buffer(const QWidget *widget);

    void setGeometry(const QRect &rect);

    WindowSurfaceFeatures features() const;

    CFbsBitmap *symbianBitmap() const;

private:
    QS60WindowSurfacePrivate* d_ptr;

};

QT_END_NAMESPACE

#endif // QWINDOWSURFACE_S60_P_H
