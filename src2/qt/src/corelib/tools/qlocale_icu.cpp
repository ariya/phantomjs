/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "unicode/uversion.h"
#include "unicode/ucol.h"

QT_BEGIN_NAMESPACE

typedef UCollator *(*Ptr_ucol_open)(const char *loc, UErrorCode *status);
typedef void (*Ptr_ucol_close)(UCollator *coll);
typedef UCollationResult (*Ptr_ucol_strcoll)(const UCollator *coll, const UChar *source, int32_t sourceLength, const UChar *target, int32_t targetLength);
typedef int32_t (*Ptr_u_strToCase)(UChar *dest, int32_t destCapacity, const UChar *src, int32_t srcLength, const char *locale, UErrorCode *pErrorCode);

static Ptr_ucol_open ptr_ucol_open = 0;
static Ptr_ucol_strcoll ptr_ucol_strcoll = 0;
static Ptr_ucol_close ptr_ucol_close = 0;
static Ptr_u_strToCase ptr_u_strToUpper = 0;
static Ptr_u_strToCase ptr_u_strToLower = 0;

enum LibLoadStatus
{
    ErrorLoading = -1,
    NotLoaded = 0,
    Loaded = 1
};

static LibLoadStatus status = NotLoaded;

static UCollator *icuCollator = 0;

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

bool qt_initIcu(const QString &localeString)
{
    if (status == ErrorLoading)
        return false;

    if (status == NotLoaded) {

        // resolve libicui18n
        QLibrary lib(QLatin1String("icui18n"), QLatin1String(U_ICU_VERSION_SHORT));
        lib.setLoadHints(QLibrary::ImprovedSearchHeuristics);
        if (!lib.load()) {
            qWarning() << "Unable to load library icui18n" << lib.errorString();
            status = ErrorLoading;
            return false;
        }

        ptr_ucol_open = (Ptr_ucol_open)lib.resolve("ucol_open");
        ptr_ucol_close = (Ptr_ucol_close)lib.resolve("ucol_close");
        ptr_ucol_strcoll = (Ptr_ucol_strcoll)lib.resolve("ucol_strcoll");

        if (!ptr_ucol_open || !ptr_ucol_close || !ptr_ucol_strcoll) {
            // try again with decorated symbol names
            ptr_ucol_open = (Ptr_ucol_open)lib.resolve("ucol_open" STRINGIFY(U_ICU_VERSION_SUFFIX));
            ptr_ucol_close = (Ptr_ucol_close)lib.resolve("ucol_close" STRINGIFY(U_ICU_VERSION_SUFFIX));
            ptr_ucol_strcoll = (Ptr_ucol_strcoll)lib.resolve("ucol_strcoll" STRINGIFY(U_ICU_VERSION_SUFFIX));
        }

        if (!ptr_ucol_open || !ptr_ucol_close || !ptr_ucol_strcoll) {
            ptr_ucol_open = 0;
            ptr_ucol_close = 0;
            ptr_ucol_strcoll = 0;

            qWarning("Unable to find symbols in icui18n");
            status = ErrorLoading;
            return false;
        }

        // resolve libicuuc
        QLibrary ucLib(QLatin1String("icuuc"), QLatin1String(U_ICU_VERSION_SHORT));
        ucLib.setLoadHints(QLibrary::ImprovedSearchHeuristics);
        if (!ucLib.load()) {
            qWarning() << "Unable to load library icuuc" << ucLib.errorString();
            status = ErrorLoading;
            return false;
        }

        ptr_u_strToUpper = (Ptr_u_strToCase)ucLib.resolve("u_strToUpper");
        ptr_u_strToLower = (Ptr_u_strToCase)ucLib.resolve("u_strToLower");

        if (!ptr_u_strToUpper || !ptr_u_strToLower) {
            ptr_u_strToUpper = (Ptr_u_strToCase)ucLib.resolve("u_strToUpper" STRINGIFY(U_ICU_VERSION_SUFFIX));
            ptr_u_strToLower = (Ptr_u_strToCase)ucLib.resolve("u_strToLower" STRINGIFY(U_ICU_VERSION_SUFFIX));
        }

        if (!ptr_u_strToUpper || !ptr_u_strToLower) {
            ptr_u_strToUpper = 0;
            ptr_u_strToLower = 0;

            qWarning("Unable to find symbols in icuuc");
            status = ErrorLoading;
            return false;
        }

        // success :)
        status = Loaded;
    }

    if (icuCollator) {
        ptr_ucol_close(icuCollator);
        icuCollator = 0;
    }

    UErrorCode icuStatus = U_ZERO_ERROR;
    icuCollator = ptr_ucol_open(localeString.toLatin1().constData(), &icuStatus);

    if (!icuCollator) {
        qWarning("Unable to open locale %s in ICU, error code %d", qPrintable(localeString), icuStatus);
        return false;
    }

    return true;
}

bool qt_ucol_strcoll(const QChar *source, int sourceLength, const QChar *target, int targetLength, int *result)
{
    Q_ASSERT(result);
    Q_ASSERT(source);
    Q_ASSERT(target);

    if (!icuCollator)
        return false;

    *result = ptr_ucol_strcoll(icuCollator, reinterpret_cast<const UChar *>(source), int32_t(sourceLength),
                               reinterpret_cast<const UChar *>(target), int32_t(targetLength));

    return true;
}

// caseFunc can either be u_strToUpper or u_strToLower
static bool qt_u_strToCase(const QString &str, QString *out, const QLocale &locale, Ptr_u_strToCase caseFunc)
{
    Q_ASSERT(out);

    if (!icuCollator)
        return false;

    QString result(str.size(), Qt::Uninitialized);

    UErrorCode status = U_ZERO_ERROR;

    int32_t size = caseFunc(reinterpret_cast<UChar *>(result.data()), result.size(),
            reinterpret_cast<const UChar *>(str.constData()), str.size(),
            locale.bcp47Name().toLatin1().constData(), &status);

    if (U_FAILURE(status))
        return false;

    if (size < result.size()) {
        result.resize(size);
    } else if (size > result.size()) {
        // the resulting string is larger than our source string
        result.resize(size);

        status = U_ZERO_ERROR;
        size = caseFunc(reinterpret_cast<UChar *>(result.data()), result.size(),
            reinterpret_cast<const UChar *>(str.constData()), str.size(),
            locale.bcp47Name().toLatin1().constData(), &status);

        if (U_FAILURE(status))
            return false;

        // if the sizes don't match now, we give up.
        if (size != result.size())
            return false;
    }

    *out = result;
    return true;
}

bool qt_u_strToUpper(const QString &str, QString *out, const QLocale &locale)
{
    return qt_u_strToCase(str, out, locale, ptr_u_strToUpper);
}

bool qt_u_strToLower(const QString &str, QString *out, const QLocale &locale)
{
    return qt_u_strToCase(str, out, locale, ptr_u_strToLower);
}

QT_END_NAMESPACE
