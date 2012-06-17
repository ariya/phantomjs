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

#ifndef QFONTENGINEGLYPHCACHE_P_H
#define QFONTENGINEGLYPHCACHE_P_H

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
#include "QtCore/qatomic.h"
#include <QtCore/qvarlengtharray.h>
#include "private/qfont_p.h"

#ifdef Q_WS_WIN
#   include "QtCore/qt_windows.h"
#endif

#ifdef Q_WS_MAC
#   include "private/qt_mac_p.h"
#   include "QtCore/qmap.h"
#   include "QtCore/qcache.h"
#   include "private/qcore_mac_p.h"
#endif

QT_BEGIN_NAMESPACE

class QFontEngineGlyphCache: public QSharedData
{
public:
    enum Type {
        Raster_RGBMask,
        Raster_A8,
        Raster_Mono
    };

    QFontEngineGlyphCache(const QTransform &matrix, Type type) : m_transform(matrix), m_type(type) { }

    virtual ~QFontEngineGlyphCache() { }

    Type cacheType() const { return m_type; }

    QTransform m_transform;
    QFontEngineGlyphCache::Type m_type;
};
typedef QHash<void *, QList<QFontEngineGlyphCache *> > GlyphPointerHash;
typedef QHash<int, QList<QFontEngineGlyphCache *> > GlyphIntHash;

QT_END_NAMESPACE

#endif
