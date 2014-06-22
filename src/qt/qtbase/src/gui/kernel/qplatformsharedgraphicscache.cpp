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

#include "qplatformsharedgraphicscache.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformSharedGraphicsCache
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformSharedGraphicsCache is an abstraction of a cross-process graphics cache.

    If supported, it is possible to retrieve a QPlatformSharedGraphicsCache object from the
    platform integration. This is typically used to store graphical items which should be shared
    between several processes.

    Items are requested from the cache by calling requestItems(). If the cache contains the
    requested items in the requested cache, the itemsAvailable() signal is emitted with the ID of
    the graphical buffer and each item's coordinates inside the buffer. Before requesting items
    from a cache, the user must call ensureCacheInitialized() to set the correct parameters for
    the cache.

    If the cache does not yet contain the requested items, it will emit a similar itemsMissing()
    signal. The client can then call updateItems() with rasterizations of the items and they will be
    entered into the shared cache. As the items are rendered into the cache, itemsAvailable() signals
    will be emitted for each of the items which have previously been requested and which have not
    yet been reported as ready.

    Using beginRequestBatch() and endRequestBatch(), it's possible to batch glyph requests, which
    could improve performance in cases where you have a sequence of requests pending, and you
    do not need the results during this sequence.
*/

/*!
    \enum QPlatformSharedGraphicsCache::BufferType

    Defines how the type of buffer required to contain a cache.

    \value OpenGLTexture The buffer will be allocated in graphics memory, and an OpenGL texture
                         for a buffer belonging to the cache can be requested using
                         textureIdForBuffer().
*/

/*!
    \enum QPlatformSharedGraphicsCache::PixelFormat

    Defines the pixel format of a cache.

    \value Alpha8 The cache will use 8 bits to represent the alpha value of each pixel. If an
                  OpenGL texture is created for a buffer belong to the cache, it will have the
                  pixel format GL_ALPHA.
*/

/*!
   \fn void QPlatformSharedGraphicsCache::ensureCacheInitialized(const QByteArray &cacheId, BufferType bufferType, PixelFormat pixelFormat)

   Initializes a cache named \a cacheId if it has not yet been initialized. The \a bufferType and
   \a pixelFormat gives the format of the buffers that will be used to contain the items in the
   cache. If a cache with the same \a cacheId has previously been initialized, the call will be
   ignored. The cache will keep its previously set buffer type and pixel format.
*/

/*!
    \fn void QPlatformSharedGraphicsCache::requestItems(const QByteArray &cacheId, const QVector<quint32> &itemIds)

    Requests all the items in \a itemIds from the cache with the name \a cacheId.

    If any or all of the items are available in the cache, one or more itemsAvailable() signals will be
    emitted corresponding to the items. If the cache does not contain all of the items in question,
    then an itemsMissing() signal will be emitted corresponding to the missing items. The user
    is at this point expected to call insertItems() to insert the missing items into the cache. If
    the inserted items have previously been requested by the user, at which point an itemsAvailable()
    signal will be emitted corresponding to the items.

    Before requesting items from a cache, the user must call ensureCacheInitialized() with the
    correct parameters for the cache.
*/

/*!
    \fn void QPlatformSharedGraphicsCache::insertItems(const QByteArray &cacheId, const QVector<quint32> &itemIds, const QVector<QImage> &items)

    Inserts the items in \a itemIds into the cache named \a cacheId. The appearance of
    each item is stored in \a items. The format of the QImage objects is expected to match the
    pixel format of the cache as it was initialized in ensureCacheInitialized().

    When the items have been successfully entered into the cache, one or more itemsAvailable() signals
    will be emitted for the items.

    If the cache already contains the items, the behavior is implementation-specific. The
    implementation may choose to ignore the items or it may overwrite the existing instances in
    the cache. Either way, itemsAvailable() signals corresponding to the inserted items will be
    emitted.
*/

/*!
    \fn void QPlatformSharedGraphicsCache::releaseItems(const QByteArray &cacheId, const QVector<quint32> &itemIds)

    Releases the reference to the items in \a itemIds from the cache named \a cacheId. This should
    only be called when all references to the items have been released by the user, and they are no
    longer needed.
*/

/*!
    \fn void QPlatformSharedGraphicsCache::itemsMissing(const QByteArray &cacheId, const QVector<quint32> &itemIds)

    This signal is emitted when requestItems() has been called for one or more items in the
    cache named \a cacheId which are not yet available in the cache. The user is then expected to
    call insertItems() to update the cache with the respective items, at which point they will
    become available to all clients of the shared cache.

    The vector \a itemIds contains the IDs of the items that need to be inserted into the cache.

    \sa itemsAvailable(), insertItems(), requestItems()
*/

/*!
    \fn void QPlatformSharedGraphicsCache::itemsAvailable(const QByteArray &cacheId, void *bufferId, const QVector<quint32> &itemIds, const QVector<QPoint> &positionsInBuffer)

    This signal can be emitted at any time when either requestItems() or insertItems() has been
    called by the application for one or more items in the cache named \a cacheId, as long as
    releaseItems() has not subsequently been called for the same items. It instructs the application
    on where to find the items that have been entered into the cache. When the application receives
    a buffer, it is expected to reference it using referenceBuffer() on it if it keeps a reference
    to the buffer.

    The \a bufferId is an ID for the buffer that contains the items. The \a bufferId can be
    converted to a format usable by the application depending on which format it was given at
    initialization. If it is a OpenGLTexture, its texture ID can be requested using the
    textureIdForBuffer() function. The dimensions of the buffer are given by \a bufferSize.

    The items provided by the cache are identified in the \a itemIds vector. The
    \a positionsInBuffer vector contains the locations inside the buffer of each item. Each entry in
    \a positionsInBuffer corresponds to an item in \a itemIds.

    The buffer and the items' locations within the buffer can be considered valid until an
    itemsInvalidated() signal has been emitted for the items, or until releaseItems() is called
    for the items.

    \sa itemsMissing(), requestItems(), bufferType()
*/

/*!
    \fn void QPlatformSharedGraphicsCache::itemsUpdated(const QByteArray &cacheId, void *bufferId, const QVector<quint32> &itemIds, const QVector<QPoint> &positionsInBuffer)

    This signal is similar in usage to the itemsAvailable() signal, but will be emitted when
    the location of a previously requested or inserted item has been updated. The application
    must update its data for the respective items and release any references to old buffers held
    by the items.

    If the application no longer holds any references to previously referenced items in a given
    cache, it should call releaseItems() for these items, at which point it will no longer receive
    any itemsUpdated() signal for these items.

    \sa requestItems(), insertItems(), itemsAvailable()
*/

/*!
    \fn void QPlatformSharedGraphicsCache::itemsInvalidated(const QByteArray &cacheId, const QVector<quint32> &itemIds)

    This signal is emitted when the items given by \a itemIds in the cache named \a cacheId have
    been removed from the cache and the previously reported information about them is considered
    invalid. It will only be emitted for items for which a buffer has previously been identified
    through the itemsAvailable() signal (either as response to a requestItems() call or an
    insertItems() call.)

    The application is expected to throw away information about the items in the \a itemIds array
    and drop any references it might have to the memory held by the buffer. If the items are still
    required by the application, it can re-commit them to the cache using the insertItems() function.

    If the application no longer holds any references to previously referenced items in a given
    cache, it should call releaseItems() for these items, at which point it will no longer receive
    any itemsInvalidated() signal for these items.
*/

/*!
    \fn void QPlatformSharedGraphicsCache::beginRequestBatch()

    This is a hint to the cache that a burst of requests is pending. In some implementations, this
    will improve performance, as the cache can focus on handling the requests and wait with the
    results until it is done. It should typically be called prior to a sequence of calls to
    requestItems() and releaseItems().

    Any call to beginRequestBatch() must be followed at some point by a call to endRequestBatch().
    Failing to do this may lead to the results of requests never being emitted.

    \note beginRequestBatch() and endRequestBatch() have no stacking logic. Calling
    beginRequestBatch() twice in a row has no effect, and the single existing batch will be ended
    by the earliest call to endRequestBatch().

    \sa endRequestBatch(), requestBatchStarted()
*/

/*!
    \fn void QPlatformSharedGraphicsCache::endRequestBatch()

    Signals to the cache that the request sequence which has previously been commenced using
    beginRequestBatch() has now finished.

    \sa beginRequestBatch(), requestBatchStarted()
*/

/*!
   \fn bool QPlatformSharedGraphicsCache::requestBatchStarted() const

   Returns \c true if a request batch has previously been started using beginRequestBatch()
   and not yet stopped using endRequestBatch().

   \sa beginRequestBatch(), endRequestBatch()
*/

/*!
    \fn uint QPlatformSharedGraphicsCache::textureIdForBuffer(void *bufferId)

    Returns an OpenGL texture ID corresponding to the buffer \a bufferId, which has previously
    been passed through signals itemsAvailable() or itemsUpdated(). The relevant OpenGL context
    should be current when calling this function.

    \sa eglImageForBuffer(), sizeOfBuffer()
*/

/*!
    \fn void *QPlatformSharedGraphicsCache::eglImageForBuffer(void *bufferId)

    Returns an EGLImageKHR image corresponding to the buffer \a bufferId.

    \sa textureIdForBuffer(), sizeOfBuffer()
*/

/*!
    \fn void QPlatformSharedGraphicsCache::referenceBuffer(void *bufferId)

    Registers a reference to the buffer \a bufferId.

    \sa dereferenceBuffer()
*/

/*!
    \fn bool QPlatformSharedGraphicsCache::dereferenceBuffer(void *bufferId)

    Removed a previously registered reference to the buffer \a bufferId. Returns \c true if there
    are still more references to the buffer in question, or false if this was the last reference
    (in which case the buffer may have been deleted in the cache.)

    \sa dereferenceBuffer()
*/

/*!
    \fn QSize QPlatformSharedGraphicsCache::sizeOfBuffer(void *bufferId)

    Returns the size of the buffer \a bufferId.

    \sa textureIdForBuffer(), eglImageForBuffer()
*/

QT_END_NAMESPACE
