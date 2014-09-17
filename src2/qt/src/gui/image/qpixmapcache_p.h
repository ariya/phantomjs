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

#ifndef QPIXMAPCACHE_P_H
#define QPIXMAPCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include "qpixmapcache.h"
#include "qpaintengine.h"
#include <private/qimage_p.h>
#include <private/qpixmap_raster_p.h>
#include "qcache.h"

QT_BEGIN_NAMESPACE

uint qHash(const QPixmapCache::Key &k);

class QPixmapCache::KeyData
{
public:
    KeyData() : isValid(true), key(0), ref(1) {}
    KeyData(const KeyData &other)
     : isValid(other.isValid), key(other.key), ref(1) {}
    ~KeyData() {}

    bool isValid;
    int key;
    int ref;
};

// XXX: hw: is this a general concept we need to abstract?
class QPixmapCacheEntry : public QPixmap
{
public:
    QPixmapCacheEntry(const QPixmapCache::Key &key, const QPixmap &pix) : QPixmap(pix), key(key)
    {
        QPixmapData *pd = pixmapData();
        if (pd && pd->classId() == QPixmapData::RasterClass) {
            QRasterPixmapData *d = static_cast<QRasterPixmapData*>(pd);
            if (!d->image.isNull() && d->image.d->paintEngine
                && !d->image.d->paintEngine->isActive())
            {
                delete d->image.d->paintEngine;
                d->image.d->paintEngine = 0;
            }
        }
    }
    ~QPixmapCacheEntry();
    QPixmapCache::Key key;
};

inline bool qIsDetached(QPixmapCacheEntry &t) { return t.isDetached(); }

QT_END_NAMESPACE

#endif // QPIXMAPCACHE_P_H
