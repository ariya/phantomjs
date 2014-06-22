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
typedef void (*_qt_pixmap_cleanup_hook_pmd)(QPlatformPixmap*);


class QImagePixmapCleanupHooks;

class Q_GUI_EXPORT QImagePixmapCleanupHooks
{
public:
    static QImagePixmapCleanupHooks *instance();

    static void enableCleanupHooks(const QImage &image);
    static void enableCleanupHooks(const QPixmap &pixmap);
    static void enableCleanupHooks(QPlatformPixmap *handle);

    static bool isImageCached(const QImage &image);
    static bool isPixmapCached(const QPixmap &pixmap);

    // Gets called when a pixmap data is about to be modified:
    void addPlatformPixmapModificationHook(_qt_pixmap_cleanup_hook_pmd);

    // Gets called when a pixmap data is about to be destroyed:
    void addPlatformPixmapDestructionHook(_qt_pixmap_cleanup_hook_pmd);

    // Gets called when an image is about to be modified or destroyed:
    void addImageHook(_qt_image_cleanup_hook_64);

    void removePlatformPixmapModificationHook(_qt_pixmap_cleanup_hook_pmd);
    void removePlatformPixmapDestructionHook(_qt_pixmap_cleanup_hook_pmd);
    void removeImageHook(_qt_image_cleanup_hook_64);

    static void executePlatformPixmapModificationHooks(QPlatformPixmap*);
    static void executePlatformPixmapDestructionHooks(QPlatformPixmap*);
    static void executeImageHooks(qint64 key);

private:
    QList<_qt_image_cleanup_hook_64> imageHooks;
    QList<_qt_pixmap_cleanup_hook_pmd> pixmapModificationHooks;
    QList<_qt_pixmap_cleanup_hook_pmd> pixmapDestructionHooks;
};

QT_END_NAMESPACE

#endif // QIMAGEPIXMAP_CLEANUPHOOKS_P_H
