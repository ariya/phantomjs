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

#include "qglobal.h"
#include "qlibrary.h"
#include "qdebug.h"
#include "qlocale_p.h"
#include "qmutex.h"

#include "unicode/uloc.h"
#include "unicode/ustring.h"

QT_BEGIN_NAMESPACE

typedef int32_t (*Ptr_u_strToCase)(UChar *dest, int32_t destCapacity, const UChar *src, int32_t srcLength, const char *locale, UErrorCode *pErrorCode);

// caseFunc can either be u_strToUpper or u_strToLower
static bool qt_u_strToCase(const QString &str, QString *out, const char *localeID, Ptr_u_strToCase caseFunc)
{
    Q_ASSERT(out);

    int32_t size = str.size();
    size += size >> 2; // add 25% for possible expansions
    QString result(size, Qt::Uninitialized);

    UErrorCode status = U_ZERO_ERROR;

    size = caseFunc(reinterpret_cast<UChar *>(result.data()), result.size(),
            reinterpret_cast<const UChar *>(str.constData()), str.size(),
            localeID, &status);

    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        return false;

    if (size < result.size()) {
        result.resize(size);
    } else if (size > result.size()) {
        // the resulting string is larger than our source string
        result.resize(size);

        status = U_ZERO_ERROR;
        size = caseFunc(reinterpret_cast<UChar *>(result.data()), result.size(),
            reinterpret_cast<const UChar *>(str.constData()), str.size(),
            localeID, &status);

        if (U_FAILURE(status))
            return false;

        // if the sizes don't match now, we give up.
        if (size != result.size())
            return false;
    }

    *out = result;
    return true;
}

QString QIcu::toUpper(const QByteArray &localeID, const QString &str, bool *ok)
{
    QString out;
    bool err = qt_u_strToCase(str, &out, localeID, u_strToUpper);
    if (ok)
        *ok = err;
    return out;
}

QString QIcu::toLower(const QByteArray &localeID, const QString &str, bool *ok)
{
    QString out;
    bool err = qt_u_strToCase(str, &out, localeID, u_strToLower);
    if (ok)
        *ok = err;
    return out;
}

QT_END_NAMESPACE
