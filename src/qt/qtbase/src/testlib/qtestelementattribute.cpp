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

#include <QtTest/private/qtestelementattribute_p.h>
#include <QtCore/qbytearray.h>
#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*! \enum QTest::AttributeIndex
  This enum numbers the different tests.

  \value AI_Undefined

  \value AI_Name

  \value AI_Result

  \value AI_Tests

  \value AI_Failures

  \value AI_Errors

  \value AI_Type

  \value AI_Description

  \value AI_PropertyValue

  \value AI_QTestVersion

  \value AI_QtVersion

  \value AI_File

  \value AI_Line

  \value AI_Metric

  \value AI_Tag

  \value AI_Value

  \value AI_Iterations
*/

/*! \enum QTest::LogElementType
  The enum specifies the kinds of test log messages.

  \value LET_Undefined

  \value LET_Property

  \value LET_Properties

  \value LET_Failure

  \value LET_Error

  \value LET_TestCase

  \value LET_TestSuite

  \value LET_Benchmark

  \value LET_SystemError
*/

QTestElementAttribute::QTestElementAttribute()
    :attributeValue(0),
    attributeIndex(QTest::AI_Undefined)
{
}

QTestElementAttribute::~QTestElementAttribute()
{
    delete[] attributeValue;
}

const char *QTestElementAttribute::value() const
{
    return attributeValue;
}

const char *QTestElementAttribute::name() const
{
    const char *AttributeNames[] =
    {
        "name",
        "result",
        "tests",
        "failures",
        "errors",
        "type",
        "description",
        "value",
        "qtestversion",
        "qtversion",
        "file",
        "line",
        "metric",
        "tag",
        "value",
        "iterations"
    };

    if (attributeIndex != QTest::AI_Undefined)
        return AttributeNames[attributeIndex];

    return 0;
}

QTest::AttributeIndex QTestElementAttribute::index() const
{
    return attributeIndex;
}

bool QTestElementAttribute::isNull() const
{
    return attributeIndex == QTest::AI_Undefined;
}

bool QTestElementAttribute::setPair(QTest::AttributeIndex index, const char *value)
{
    if (!value)
        return false;

    delete[] attributeValue;

    attributeIndex = index;
    attributeValue = qstrdup(value);

    return attributeValue != 0;
}

QT_END_NAMESPACE

