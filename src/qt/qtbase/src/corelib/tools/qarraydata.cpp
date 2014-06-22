/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include <QtCore/qarraydata.h>
#include <QtCore/private/qtools_p.h>

#include <stdlib.h>

QT_BEGIN_NAMESPACE

#if defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__  >= 406) && !defined(Q_CC_INTEL)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

const QArrayData QArrayData::shared_null[2] = {
    { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, sizeof(QArrayData) }, // shared null
    /* zero initialized terminator */};

static const QArrayData qt_array[3] = {
    { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, sizeof(QArrayData) }, // shared empty
    { { Q_BASIC_ATOMIC_INITIALIZER(0) }, 0, 0, 0, sizeof(QArrayData) }, // unsharable empty
    /* zero initialized terminator */};

#if defined (__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__  >= 406) && !defined(Q_CC_INTEL)
  #pragma GCC diagnostic pop
#endif

static const QArrayData &qt_array_empty = qt_array[0];
static const QArrayData &qt_array_unsharable_empty = qt_array[1];

QArrayData *QArrayData::allocate(size_t objectSize, size_t alignment,
        size_t capacity, AllocationOptions options)
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= Q_ALIGNOF(QArrayData)
            && !(alignment & (alignment - 1)));

    // Don't allocate empty headers
    if (!(options & RawData) && !capacity) {
#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
        if (options & Unsharable)
            return const_cast<QArrayData *>(&qt_array_unsharable_empty);
#endif
        return const_cast<QArrayData *>(&qt_array_empty);
    }

    size_t headerSize = sizeof(QArrayData);

    // Allocate extra (alignment - Q_ALIGNOF(QArrayData)) padding bytes so we
    // can properly align the data array. This assumes malloc is able to
    // provide appropriate alignment for the header -- as it should!
    // Padding is skipped when allocating a header for RawData.
    if (!(options & RawData))
        headerSize += (alignment - Q_ALIGNOF(QArrayData));

    // Allocate additional space if array is growing
    if (options & Grow)
        capacity = qAllocMore(int(objectSize * capacity), int(headerSize)) / int(objectSize);

    size_t allocSize = headerSize + objectSize * capacity;

    QArrayData *header = static_cast<QArrayData *>(::malloc(allocSize));
    if (header) {
        quintptr data = (quintptr(header) + sizeof(QArrayData) + alignment - 1)
                & ~(alignment - 1);

        header->ref.atomic.store(bool(!(options & Unsharable)));
        header->size = 0;
        header->alloc = capacity;
        header->capacityReserved = bool(options & CapacityReserved);
        header->offset = data - quintptr(header);
    }

    return header;
}

void QArrayData::deallocate(QArrayData *data, size_t objectSize,
        size_t alignment)
{
    // Alignment is a power of two
    Q_ASSERT(alignment >= Q_ALIGNOF(QArrayData)
            && !(alignment & (alignment - 1)));
    Q_UNUSED(objectSize) Q_UNUSED(alignment)

#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
    if (data == &qt_array_unsharable_empty)
        return;
#endif

    Q_ASSERT_X(!data->ref.isStatic(), "QArrayData::deallocate", "Static data can not be deleted");
    ::free(data);
}

QT_END_NAMESPACE
