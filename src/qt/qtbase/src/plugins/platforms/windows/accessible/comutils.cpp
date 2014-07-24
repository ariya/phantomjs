/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <qt_windows.h>

#include <ocidl.h>
#include <olectl.h>

#include "comutils.h"
#include <QtCore/qdatetime.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qfont.h>


#include <QtCore/qvariant.h>
#include <QtCore/qbytearray.h>
#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

static DATE QDateTimeToDATE(const QDateTime &dt)
{
    if (!dt.isValid() || dt.isNull())
        return 949998;  // Special value for no date (01/01/4501)

    SYSTEMTIME stime;
    memset(&stime, 0, sizeof(stime));
    QDate date = dt.date();
    QTime time = dt.time();
    if (date.isValid() && !date.isNull()) {
        stime.wDay = date.day();
        stime.wMonth = date.month();
        stime.wYear = date.year();
    }
    if (time.isValid() && !time.isNull()) {
        stime.wMilliseconds = time.msec();
        stime.wSecond = time.second();
        stime.wMinute = time.minute();
        stime.wHour = time.hour();
    }

    double vtime;
    SystemTimeToVariantTime(&stime, &vtime);

    return vtime;
}

inline uint QColorToOLEColor(const QColor &col)
{
    return qRgba(col.blue(), col.green(), col.red(), 0x00);
}

bool QVariant2VARIANT(const QVariant &var, VARIANT &arg, const QByteArray &typeName, bool out)
{
    QVariant qvar = var;
    // "type" is the expected type, so coerce if necessary
    QVariant::Type proptype = typeName.isEmpty() ? QVariant::Invalid : QVariant::nameToType(typeName);
    if (proptype == QVariant::UserType && !typeName.isEmpty()) {
        if (typeName == "short" || typeName == "char")
            proptype = QVariant::Int;
        else if (typeName == "float")
            proptype = QVariant::Double;
    }
    if (proptype != QVariant::Invalid && proptype != QVariant::UserType && proptype != qvar.type()) {
        if (qvar.canConvert(proptype))
            qvar.convert(proptype);
        else
            qvar = QVariant(proptype);
    }

    if (out && arg.vt == (VT_VARIANT|VT_BYREF) && arg.pvarVal) {
        return QVariant2VARIANT(var, *arg.pvarVal, typeName, false);
    }

    if (out && proptype == QVariant::UserType && typeName == "QVariant") {
        VARIANT *pVariant = new VARIANT;
        QVariant2VARIANT(var, *pVariant, QByteArray(), false);
        arg.vt = VT_VARIANT|VT_BYREF;
        arg.pvarVal = pVariant;
        return true;
    }

    switch ((int)qvar.type()) {
    case QVariant::String:
        if (out && arg.vt == (VT_BSTR|VT_BYREF)) {
            if (*arg.pbstrVal)
                SysFreeString(*arg.pbstrVal);
            *arg.pbstrVal = QStringToBSTR(qvar.toString());
            arg.vt = VT_BSTR|VT_BYREF;
        } else {
            arg.vt = VT_BSTR;
            arg.bstrVal = QStringToBSTR(qvar.toString());
            if (out) {
                arg.pbstrVal = new BSTR(arg.bstrVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::Int:
        if (out && arg.vt == (VT_I4|VT_BYREF)) {
            *arg.plVal = qvar.toInt();
        } else {
            arg.vt = VT_I4;
            arg.lVal = qvar.toInt();
            if (out) {
                if (typeName == "short") {
                    arg.vt = VT_I2;
                    arg.piVal = new short(arg.lVal);
                } else if (typeName == "char") {
                    arg.vt = VT_I1;
                    arg.pcVal= new char(arg.lVal);
                } else {
                    arg.plVal = new long(arg.lVal);
                }
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::UInt:
        if (out && (arg.vt == (VT_UINT|VT_BYREF) || arg.vt == (VT_I4|VT_BYREF))) {
            *arg.puintVal = qvar.toUInt();
        } else {
            arg.vt = VT_UINT;
            arg.uintVal = qvar.toUInt();
            if (out) {
                arg.puintVal = new uint(arg.uintVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::LongLong:
        if (out && arg.vt == (VT_CY|VT_BYREF)) {
            arg.pcyVal->int64 = qvar.toLongLong();
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
        } else if (out && arg.vt == (VT_I8|VT_BYREF)) {
            *arg.pllVal = qvar.toLongLong();
        } else {
            arg.vt = VT_I8;
            arg.llVal = qvar.toLongLong();
            if (out) {
                arg.pllVal = new LONGLONG(arg.llVal);
                arg.vt |= VT_BYREF;
            }
        }
#else
        } else {
            arg.vt = VT_CY;
            arg.cyVal.int64 = qvar.toLongLong();
            if (out) {
                arg.pcyVal = new CY(arg.cyVal);
                arg.vt |= VT_BYREF;
            }
        }
#endif
        break;

    case QVariant::ULongLong:
        if (out && arg.vt == (VT_CY|VT_BYREF)) {
            arg.pcyVal->int64 = qvar.toULongLong();
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
        } else if (out && arg.vt == (VT_UI8|VT_BYREF)) {
            *arg.pullVal = qvar.toULongLong();
        } else {
            arg.vt = VT_UI8;
            arg.ullVal = qvar.toULongLong();
            if (out) {
                arg.pullVal = new ULONGLONG(arg.ullVal);
                arg.vt |= VT_BYREF;
            }
        }
#else
        } else {
            arg.vt = VT_CY;
            arg.cyVal.int64 = qvar.toULongLong();
            if (out) {
                arg.pcyVal = new CY(arg.cyVal);
                arg.vt |= VT_BYREF;
            }
        }

#endif

        break;

    case QVariant::Bool:
        if (out && arg.vt == (VT_BOOL|VT_BYREF)) {
            *arg.pboolVal = qvar.toBool() ? VARIANT_TRUE : VARIANT_FALSE;
        } else {
            arg.vt = VT_BOOL;
            arg.boolVal = qvar.toBool() ? VARIANT_TRUE : VARIANT_FALSE;
            if (out) {
                arg.pboolVal = new short(arg.boolVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;
    case QVariant::Double:
        if (out && arg.vt == (VT_R8|VT_BYREF)) {
            *arg.pdblVal = qvar.toDouble();
        } else {
            arg.vt = VT_R8;
            arg.dblVal = qvar.toDouble();
            if (out) {
                if (typeName == "float") {
                    arg.vt = VT_R4;
                    arg.pfltVal = new float(arg.dblVal);
                } else {
                    arg.pdblVal = new double(arg.dblVal);
                }
                arg.vt |= VT_BYREF;
            }
        }
        break;
    case QVariant::Color:
        if (out && arg.vt == (VT_COLOR|VT_BYREF)) {

            *arg.plVal = QColorToOLEColor(qvariant_cast<QColor>(qvar));
        } else {
            arg.vt = VT_COLOR;
            arg.lVal = QColorToOLEColor(qvariant_cast<QColor>(qvar));
            if (out) {
                arg.plVal = new long(arg.lVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    case QVariant::Date:
    case QVariant::Time:
    case QVariant::DateTime:
        if (out && arg.vt == (VT_DATE|VT_BYREF)) {
            *arg.pdate = QDateTimeToDATE(qvar.toDateTime());
        } else {
            arg.vt = VT_DATE;
            arg.date = QDateTimeToDATE(qvar.toDateTime());
            if (out) {
                arg.pdate = new DATE(arg.date);
                arg.vt |= VT_BYREF;
            }
        }
        break;
#if 0   // not a value with min/max semantics
    case QVariant::Font:
        if (out && arg.vt == (VT_DISPATCH|VT_BYREF)) {
            if (*arg.ppdispVal)
                (*arg.ppdispVal)->Release();
            *arg.ppdispVal = QFontToIFont(qvariant_cast<QFont>(qvar));
        } else {
            arg.vt = VT_DISPATCH;
            arg.pdispVal = QFontToIFont(qvariant_cast<QFont>(qvar));
            if (out) {
                arg.ppdispVal = new IDispatch*(arg.pdispVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;
    case QVariant::Pixmap:
        if (out && arg.vt == (VT_DISPATCH|VT_BYREF)) {
            if (*arg.ppdispVal)
                (*arg.ppdispVal)->Release();
            *arg.ppdispVal = QPixmapToIPicture(qvariant_cast<QPixmap>(qvar));
        } else {
            arg.vt = VT_DISPATCH;
            arg.pdispVal = QPixmapToIPicture(qvariant_cast<QPixmap>(qvar));
            if (out) {
                arg.ppdispVal = new IDispatch*(arg.pdispVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;
    case QVariant::Cursor:
        {
#ifndef QT_NO_CURSOR
            int shape = qvariant_cast<QCursor>(qvar).shape();
            if (out && (arg.vt & VT_BYREF)) {
                switch (arg.vt & ~VT_BYREF) {
                case VT_I4:
                    *arg.plVal = shape;
                    break;
                case VT_I2:
                    *arg.piVal = shape;
                    break;
                case VT_UI4:
                    *arg.pulVal = shape;
                    break;
                case VT_UI2:
                    *arg.puiVal = shape;
                    break;
                case VT_INT:
                    *arg.pintVal = shape;
                    break;
                case VT_UINT:
                    *arg.puintVal = shape;
                    break;
                }
            } else {
                arg.vt = VT_I4;
                arg.lVal = shape;
                if (out) {
                    arg.plVal = new long(arg.lVal);
                    arg.vt |= VT_BYREF;
                }
            }
#endif
        }
        break;

    case QVariant::List:
        {
            const QList<QVariant> list = qvar.toList();
            const int count = list.count();
            VARTYPE vt = VT_VARIANT;
            QVariant::Type listType = QVariant::LastType; // == QVariant
            if (!typeName.isEmpty() && typeName.startsWith("QList<")) {
                const QByteArray listTypeName = typeName.mid(6, typeName.length() - 7); // QList<int> -> int
                listType = QVariant::nameToType(listTypeName);
            }

            VARIANT variant;
            void *pElement = &variant;
            switch (listType) {
            case QVariant::Int:
                vt = VT_I4;
                pElement = &variant.lVal;
                break;
            case QVariant::Double:
                vt = VT_R8;
                pElement = &variant.dblVal;
                break;
            case QVariant::DateTime:
                vt = VT_DATE;
                pElement = &variant.date;
                break;
            case QVariant::Bool:
                vt = VT_BOOL;
                pElement = &variant.boolVal;
                break;
            case QVariant::LongLong:
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
                vt = VT_I8;
                pElement = &variant.llVal;
#else
                vt = VT_CY;
                pElement = &variant.cyVal;
#endif
                break;
            default:
                break;
            }
            SAFEARRAY *array = 0;
            bool is2D = false;
            // If the first element in the array is a list the whole list is
            // treated as a 2D array. The column count is taken from the 1st element.
            if (count) {
                QVariantList col = list.at(0).toList();
                int maxColumns = col.count();
                if (maxColumns) {
                    is2D = true;
                    SAFEARRAYBOUND rgsabound[2] = { {0} };
                    rgsabound[0].cElements = count;
                    rgsabound[1].cElements = maxColumns;
                    array = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
                    LONG rgIndices[2];
                    for (LONG i = 0; i < count; ++i) {
                        rgIndices[0] = i;
                        QVariantList columns = list.at(i).toList();
                        int columnCount = qMin(maxColumns, columns.count());
                        for (LONG j = 0;  j < columnCount; ++j) {
                            QVariant elem = columns.at(j);
                            VariantInit(&variant);
                            QVariant2VARIANT(elem, variant, elem.typeName());
                            rgIndices[1] = j;
                            SafeArrayPutElement(array, rgIndices, pElement);
                            clearVARIANT(&variant);
                        }
                    }

                }
            }
            if (!is2D) {
                array = SafeArrayCreateVector(vt, 0, count);
                for (LONG index = 0; index < count; ++index) {
                    QVariant elem = list.at(index);
                    if (listType != QVariant::LastType)
                        elem.convert(listType);
                    VariantInit(&variant);
                    QVariant2VARIANT(elem, variant, elem.typeName());
                    SafeArrayPutElement(array, &index, pElement);
                    clearVARIANT(&variant);
                }
            }
            if (out && arg.vt == (VT_ARRAY|vt|VT_BYREF)) {
                if (*arg.pparray)
                    SafeArrayDestroy(*arg.pparray);
                *arg.pparray = array;
            } else {
                arg.vt = VT_ARRAY|vt;
                arg.parray = array;
                if (out) {
                    arg.pparray = new SAFEARRAY*(arg.parray);
                    arg.vt |= VT_BYREF;
                }
            }
        }
        break;

    case QVariant::StringList:
        {
            const QStringList list = qvar.toStringList();
            const int count = list.count();
            SAFEARRAY *array = SafeArrayCreateVector(VT_BSTR, 0, count);
            for (LONG index = 0; index < count; ++index) {
                QString elem = list.at(index);
                BSTR bstr = QStringToBSTR(elem);
                SafeArrayPutElement(array, &index, bstr);
                SysFreeString(bstr);
            }

            if (out && arg.vt == (VT_ARRAY|VT_BSTR|VT_BYREF)) {
                if (*arg.pparray)
                    SafeArrayDestroy(*arg.pparray);
                *arg.pparray = array;
            } else {
                arg.vt = VT_ARRAY|VT_BSTR;
                arg.parray = array;
                if (out) {
                    arg.pparray = new SAFEARRAY*(arg.parray);
                    arg.vt |= VT_BYREF;
                }
            }
        }
        break;

    case QVariant::ByteArray:
        {
            const QByteArray bytes = qvar.toByteArray();
            const uint count = bytes.count();
            SAFEARRAY *array = SafeArrayCreateVector(VT_UI1, 0, count);
            if (count) {
                const char *data = bytes.constData();
                char *dest;
                SafeArrayAccessData(array, (void **)&dest);
                memcpy(dest, data, count);
                SafeArrayUnaccessData(array);
            }

            if (out && arg.vt == (VT_ARRAY|VT_UI1|VT_BYREF)) {
                if (*arg.pparray)
                    SafeArrayDestroy(*arg.pparray);
                *arg.pparray = array;
            } else {
                arg.vt = VT_ARRAY|VT_UI1;
                arg.parray = array;
                if (out) {
                    arg.pparray = new SAFEARRAY*(arg.parray);
                    arg.vt |= VT_BYREF;
                }
            }
        }
        break;

#ifdef QAX_SERVER
    case QVariant::Rect:
    case QVariant::Size:
    case QVariant::Point:
        {
            typedef HRESULT(WINAPI* PGetRecordInfoFromTypeInfo)(ITypeInfo *, IRecordInfo **);
            static PGetRecordInfoFromTypeInfo pGetRecordInfoFromTypeInfo = 0;
            static bool resolved = false;
            if (!resolved) {
                QSystemLibrary oleaut32(QLatin1String("oleaut32"));
                pGetRecordInfoFromTypeInfo = (PGetRecordInfoFromTypeInfo)oleaut32.resolve("GetRecordInfoFromTypeInfo");
                resolved = true;
            }
            if (!pGetRecordInfoFromTypeInfo)
                break;

            ITypeInfo *typeInfo = 0;
            IRecordInfo *recordInfo = 0;
            CLSID clsid = qvar.type() == QVariant::Rect ? CLSID_QRect
                :qvar.type() == QVariant::Size ? CLSID_QSize
                :CLSID_QPoint;
            qAxTypeLibrary->GetTypeInfoOfGuid(clsid, &typeInfo);
            if (!typeInfo)
                break;
            pGetRecordInfoFromTypeInfo(typeInfo, &recordInfo);
            typeInfo->Release();
            if (!recordInfo)
                break;

            void *record = 0;
            switch (qvar.type()) {
            case QVariant::Rect:
                {
                    QRect qrect(qvar.toRect());
                    recordInfo->RecordCreateCopy(&qrect, &record);
                }
                break;
            case QVariant::Size:
                {
                    QSize qsize(qvar.toSize());
                    recordInfo->RecordCreateCopy(&qsize, &record);
                }
                break;
            case QVariant::Point:
                {
                    QPoint qpoint(qvar.toPoint());
                    recordInfo->RecordCreateCopy(&qpoint, &record);
                }
                break;
            }

            arg.vt = VT_RECORD;
            arg.pRecInfo = recordInfo,
            arg.pvRecord = record;
            if (out) {
                qWarning("QVariant2VARIANT: out-parameter not supported for records");
                return false;
           }
        }
        break;
#endif // QAX_SERVER
    case QVariant::UserType:
        {
            QByteArray subType = qvar.typeName();
#ifdef QAX_SERVER
            if (subType.endsWith('*'))
                subType.truncate(subType.length() - 1);
#endif
            if (!qstrcmp(qvar.typeName(), "IDispatch*")) {
                arg.vt = VT_DISPATCH;
                arg.pdispVal = *(IDispatch**)qvar.data();
                if (arg.pdispVal)
                    arg.pdispVal->AddRef();
                if (out) {
                    qWarning("QVariant2VARIANT: out-parameter not supported for IDispatch");
                    return false;
                }
            } else if (!qstrcmp(qvar.typeName(), "IDispatch**")) {
                arg.vt = VT_DISPATCH;
                arg.ppdispVal = *(IDispatch***)qvar.data();
                if (out)
                    arg.vt |= VT_BYREF;
            } else if (!qstrcmp(qvar.typeName(), "IUnknown*")) {
                arg.vt = VT_UNKNOWN;
                arg.punkVal = *(IUnknown**)qvar.data();
                if (arg.punkVal)
                    arg.punkVal->AddRef();
                if (out) {
                    qWarning("QVariant2VARIANT: out-parameter not supported for IUnknown");
                    return false;
                }
#ifdef QAX_SERVER
            } else if (qAxFactory()->metaObject(QString::fromLatin1(subType.constData()))) {
                arg.vt = VT_DISPATCH;
                void *user = *(void**)qvar.constData();
//                qVariantGet(qvar, user, qvar.typeName());
                if (!user) {
                    arg.pdispVal = 0;
                } else {
                    qAxFactory()->createObjectWrapper(static_cast<QObject*>(user), &arg.pdispVal);
                }
                if (out) {
                    qWarning("QVariant2VARIANT: out-parameter not supported for subtype");
                    return false;
                }
#else
            } else if (QMetaType::type(subType)) {
                QAxObject *object = *(QAxObject**)qvar.constData();
//                qVariantGet(qvar, object, subType);
                arg.vt = VT_DISPATCH;
                object->queryInterface(IID_IDispatch, (void**)&arg.pdispVal);
                if (out) {
                    qWarning("QVariant2VARIANT: out-parameter not supported for subtype");
                    return false;
                }
#endif
            } else {
                return false;
            }
        }
        break;
#endif

    case QVariant::Invalid: // default-parameters not set
        if (out && arg.vt == (VT_ERROR|VT_BYREF)) {
            *arg.plVal = DISP_E_PARAMNOTFOUND;
        } else {
            arg.vt = VT_ERROR;
            arg.lVal = DISP_E_PARAMNOTFOUND;
            if (out) {
                arg.plVal = new long(arg.lVal);
                arg.vt |= VT_BYREF;
            }
        }
        break;

    default:
        return false;
    }

    Q_ASSERT(!out || (arg.vt & VT_BYREF));
    return true;
}

QT_END_NAMESPACE

