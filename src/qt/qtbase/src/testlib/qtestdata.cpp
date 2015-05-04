/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qmetaobject.h>

#include <QtTest/qtestassert.h>
#include <QtTest/qtestdata.h>
#include <QtTest/private/qtesttable_p.h>

#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

class QTestDataPrivate
{
public:
    QTestDataPrivate() : tag(0), parent(0), data(0), dataCount(0) {}

    char *tag;
    QTestTable *parent;
    void **data;
    int dataCount;
};

QTestData::QTestData(const char *tag, QTestTable *parent)
{
    QTEST_ASSERT(tag);
    QTEST_ASSERT(parent);
    d = new QTestDataPrivate;
    d->tag = qstrdup(tag);
    d->parent = parent;
    d->data = new void *[parent->elementCount()];
    memset(d->data, 0, parent->elementCount() * sizeof(void*));
}

QTestData::~QTestData()
{
    for (int i = 0; i < d->dataCount; ++i) {
        if (d->data[i])
            QMetaType::destroy(d->parent->elementTypeId(i), d->data[i]);
    }
    delete [] d->data;
    delete [] d->tag;
    delete d;
}

void QTestData::append(int type, const void *data)
{
    QTEST_ASSERT(d->dataCount < d->parent->elementCount());
    if (d->parent->elementTypeId(d->dataCount) != type) {
        qDebug("expected data of type '%s', got '%s' for element %d of data with tag '%s'",
                QMetaType::typeName(d->parent->elementTypeId(d->dataCount)),
                QMetaType::typeName(type),
                d->dataCount, d->tag);
        QTEST_ASSERT(false);
    }
    d->data[d->dataCount] = QMetaType::create(type, data);
    ++d->dataCount;
}

void *QTestData::data(int index) const
{
    QTEST_ASSERT(index >= 0);
    QTEST_ASSERT(index < d->parent->elementCount());
    return d->data[index];
}

QTestTable *QTestData::parent() const
{
    return d->parent;
}

const char *QTestData::dataTag() const
{
    return d->tag;
}

int QTestData::dataCount() const
{
    return d->dataCount;
}

QT_END_NAMESPACE
