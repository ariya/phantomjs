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

#include "qimagepixmapcleanuphooks_p.h"
#include "private/qpixmapdata_p.h"
#include "private/qimage_p.h"


QT_BEGIN_NAMESPACE

// Legacy, single instance hooks: ### Qt 5: remove
typedef void (*_qt_pixmap_cleanup_hook)(int);
typedef void (*_qt_pixmap_cleanup_hook_64)(qint64);
typedef void (*_qt_image_cleanup_hook)(int);
Q_GUI_EXPORT _qt_pixmap_cleanup_hook qt_pixmap_cleanup_hook = 0;
Q_GUI_EXPORT _qt_pixmap_cleanup_hook_64 qt_pixmap_cleanup_hook_64 = 0;
Q_GUI_EXPORT _qt_image_cleanup_hook qt_image_cleanup_hook = 0;
Q_GUI_EXPORT _qt_image_cleanup_hook_64 qt_image_cleanup_hook_64 = 0;

Q_GLOBAL_STATIC(QImagePixmapCleanupHooks, qt_image_and_pixmap_cleanup_hooks)

QImagePixmapCleanupHooks *QImagePixmapCleanupHooks::instance()
{
    return qt_image_and_pixmap_cleanup_hooks();
}

void QImagePixmapCleanupHooks::addPixmapDataModificationHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapModificationHooks.append(hook);
}

void QImagePixmapCleanupHooks::addPixmapDataDestructionHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapDestructionHooks.append(hook);
}


void QImagePixmapCleanupHooks::addImageHook(_qt_image_cleanup_hook_64 hook)
{
    imageHooks.append(hook);
}

void QImagePixmapCleanupHooks::removePixmapDataModificationHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapModificationHooks.removeAll(hook);
}

void QImagePixmapCleanupHooks::removePixmapDataDestructionHook(_qt_pixmap_cleanup_hook_pmd hook)
{
    pixmapDestructionHooks.removeAll(hook);
}

void QImagePixmapCleanupHooks::removeImageHook(_qt_image_cleanup_hook_64 hook)
{
    imageHooks.removeAll(hook);
}

void QImagePixmapCleanupHooks::executePixmapDataModificationHooks(QPixmapData* pmd)
{
    QImagePixmapCleanupHooks *h = qt_image_and_pixmap_cleanup_hooks();
    // the global destructor for the pixmap and image hooks might have
    // been called already if the app is "leaking" global
    // pixmaps/images
    if (!h)
        return;
    for (int i = 0; i < h->pixmapModificationHooks.count(); ++i)
        h->pixmapModificationHooks[i](pmd);

    if (qt_pixmap_cleanup_hook_64)
        qt_pixmap_cleanup_hook_64(pmd->cacheKey());
}

void QImagePixmapCleanupHooks::executePixmapDataDestructionHooks(QPixmapData* pmd)
{
    QImagePixmapCleanupHooks *h = qt_image_and_pixmap_cleanup_hooks();
    // the global destructor for the pixmap and image hooks might have
    // been called already if the app is "leaking" global
    // pixmaps/images
    if (!h)
        return;
    for (int i = 0; i < h->pixmapDestructionHooks.count(); ++i)
        h->pixmapDestructionHooks[i](pmd);

    if (qt_pixmap_cleanup_hook_64)
        qt_pixmap_cleanup_hook_64(pmd->cacheKey());
}

void QImagePixmapCleanupHooks::executeImageHooks(qint64 key)
{
    for (int i = 0; i < qt_image_and_pixmap_cleanup_hooks()->imageHooks.count(); ++i)
        qt_image_and_pixmap_cleanup_hooks()->imageHooks[i](key);

    if (qt_image_cleanup_hook_64)
        qt_image_cleanup_hook_64(key);
}


void QImagePixmapCleanupHooks::enableCleanupHooks(QPixmapData *pixmapData)
{
    pixmapData->is_cached = true;
}

void QImagePixmapCleanupHooks::enableCleanupHooks(const QPixmap &pixmap)
{
    enableCleanupHooks(const_cast<QPixmap &>(pixmap).data_ptr().data());
}

void QImagePixmapCleanupHooks::enableCleanupHooks(const QImage &image)
{
    const_cast<QImage &>(image).data_ptr()->is_cached = true;
}

bool QImagePixmapCleanupHooks::isImageCached(const QImage &image)
{
    return const_cast<QImage &>(image).data_ptr()->is_cached;
}

bool QImagePixmapCleanupHooks::isPixmapCached(const QPixmap &pixmap)
{
    return const_cast<QPixmap&>(pixmap).data_ptr().data()->is_cached;
}



QT_END_NAMESPACE
