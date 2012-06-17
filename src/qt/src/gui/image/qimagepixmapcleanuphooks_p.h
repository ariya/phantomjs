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

#ifndef QIMAGEPIXMAP_CLEANUPHOOKS_P_H
#define QIMAGEPIXMAP_CLEANUPHOOKS_P_H

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

#include <QtGui/qpixmap.h>

QT_BEGIN_NAMESPACE

typedef void (*_qt_image_cleanup_hook_64)(qint64);
typedef void (*_qt_pixmap_cleanup_hook_pmd)(QPixmapData*);


class QImagePixmapCleanupHooks;

class Q_GUI_EXPORT QImagePixmapCleanupHooks
{
public:
    static QImagePixmapCleanupHooks *instance();

    static void enableCleanupHooks(const QImage &image);
    static void enableCleanupHooks(const QPixmap &pixmap);
    static void enableCleanupHooks(QPixmapData *pixmapData);

    static bool isImageCached(const QImage &image);
    static bool isPixmapCached(const QPixmap &pixmap);

    // Gets called when a pixmap data is about to be modified:
    void addPixmapDataModificationHook(_qt_pixmap_cleanup_hook_pmd);

    // Gets called when a pixmap data is about to be destroyed:
    void addPixmapDataDestructionHook(_qt_pixmap_cleanup_hook_pmd);

    // Gets called when an image is about to be modified or destroyed:
    void addImageHook(_qt_image_cleanup_hook_64);

    void removePixmapDataModificationHook(_qt_pixmap_cleanup_hook_pmd);
    void removePixmapDataDestructionHook(_qt_pixmap_cleanup_hook_pmd);
    void removeImageHook(_qt_image_cleanup_hook_64);

    static void executePixmapDataModificationHooks(QPixmapData*);
    static void executePixmapDataDestructionHooks(QPixmapData*);
    static void executeImageHooks(qint64 key);

private:
    QList<_qt_image_cleanup_hook_64> imageHooks;
    QList<_qt_pixmap_cleanup_hook_pmd> pixmapModificationHooks;
    QList<_qt_pixmap_cleanup_hook_pmd> pixmapDestructionHooks;
};

QT_END_NAMESPACE

#endif // QIMAGEPIXMAP_CLEANUPHOOKS_P_H
