/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "qt_runtime.h"

#include "BooleanObject.h"
#include "DateInstance.h"
#include "DateMath.h"
#include "DatePrototype.h"
#include "DumpRenderTreeSupportQt.h"
#include "FunctionPrototype.h"
#include "Interpreter.h"
#include "JSArray.h"
#include "JSByteArray.h"
#include "JSDocument.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include <JSFunction.h>
#include "JSGlobalObject.h"
#include "JSHTMLElement.h"
#include "JSLock.h"
#include "JSObject.h"
#include "ObjectPrototype.h"
#include "PropertyNameArray.h"
#include "RegExpConstructor.h"
#include "RegExpObject.h"
#include "qdatetime.h"
#include "qdebug.h"
#include "qmetaobject.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qstringlist.h"
#include "qt_instance.h"
#include "qt_pixmapruntime.h"
#include "qvarlengtharray.h"
#include "qwebelement.h"
#include <limits.h>
#include <runtime/Error.h>
#include <runtime_array.h>
#include <runtime_object.h>

// QtScript has these
Q_DECLARE_METATYPE(QObjectList);
Q_DECLARE_METATYPE(QList<int>);
Q_DECLARE_METATYPE(QVariant);

using namespace WebCore;

namespace JSC {
namespace Bindings {

// Debugging
//#define QTWK_RUNTIME_CONVERSION_DEBUG
//#define QTWK_RUNTIME_MATCH_DEBUG

class QWKNoDebug
{
public:
    inline QWKNoDebug(){}
    inline ~QWKNoDebug(){}

    template<typename T>
    inline QWKNoDebug &operator<<(const T &) { return *this; }
};

#ifdef QTWK_RUNTIME_CONVERSION_DEBUG
#define qConvDebug() qDebug()
#else
#define qConvDebug() QWKNoDebug()
#endif

#ifdef QTWK_RUNTIME_MATCH_DEBUG
#define qMatchDebug() qDebug()
#else
#define qMatchDebug() QWKNoDebug()
#endif

typedef enum {
    Variant = 0,
    Number,
    Boolean,
    String,
    Date,
    RegExp,
    Array,
    QObj,
    Object,
    Null,
    RTArray,
    JSByteArray
} JSRealType;

#if defined(QTWK_RUNTIME_CONVERSION_DEBUG) || defined(QTWK_RUNTIME_MATCH_DEBUG)
QDebug operator<<(QDebug dbg, const JSRealType &c)
{
     const char *map[] = { "Variant", "Number", "Boolean", "String", "Date",
         "RegExp", "Array", "RTObject", "Object", "Null", "RTArray"};

     dbg.nospace() << "JSType(" << ((int)c) << ", " <<  map[c] << ")";

     return dbg.space();
}
#endif

// this is here as a proxy, so we'd have a class to friend in QWebElement,
// as getting/setting a WebCore in QWebElement is private
class QtWebElementRuntime {
public:
    static QWebElement create(Element* element)
    {
        return QWebElement(element);
    }

    static Element* get(const QWebElement& element)
    {
        return element.m_element;
    }
};

// this is here as a proxy, so we'd have a class to friend in QDRTNode,
// as getting/setting a WebCore in QDRTNode is private.
// We only need to pass WebCore Nodes for layout tests.
class QtDRTNodeRuntime {
public:
    static QDRTNode create(Node* node)
    {
        return QDRTNode(node);
    }

    static Node* get(const QDRTNode& node)
    {
        return node.m_node;
    }
};

static JSRealType valueRealType(ExecState* exec, JSValue val)
{
    if (val.isNumber())
        return Number;
    else if (val.isString())
        return String;
    else if (val.isBoolean())
        return Boolean;
    else if (val.isNull())
        return Null;
    else if (isJSByteArray(&exec->globalData(), val))
        return JSByteArray;
    else if (val.isObject()) {
        JSObject *object = val.toObject(exec);
        if (object->inherits(&RuntimeArray::s_info))  // RuntimeArray 'inherits' from Array, but not in C++
            return RTArray;
        else if (object->inherits(&JSArray::s_info))
            return Array;
        else if (object->inherits(&DateInstance::s_info))
            return Date;
        else if (object->inherits(&RegExpObject::s_info))
            return RegExp;
        else if (object->inherits(&RuntimeObject::s_info))
            return QObj;
        return Object;
    }

    return String; // I don't know.
}

QVariant convertValueToQVariant(ExecState*, JSValue, QMetaType::Type, int*, HashSet<JSObject*>*, int);

static QVariantMap convertValueToQVariantMap(ExecState* exec, JSObject* object, HashSet<JSObject*>* visitedObjects, int recursionLimit)
{
    Q_ASSERT(!exec->hadException());

    PropertyNameArray properties(exec);
    object->getPropertyNames(exec, properties);
    PropertyNameArray::const_iterator it = properties.begin();
    QVariantMap result;
    int objdist = 0;

    while (it != properties.end()) {
        if (object->propertyIsEnumerable(exec, *it)) {
            JSValue val = object->get(exec, *it);
            if (exec->hadException())
                exec->clearException();
            else {
                QVariant v = convertValueToQVariant(exec, val, QMetaType::Void, &objdist, visitedObjects, recursionLimit);
                if (objdist >= 0) {
                    UString ustring = (*it).ustring();
                    QString id = QString((const QChar*)ustring.impl()->characters(), ustring.length());
                    result.insert(id, v);
                }
            }
        }
        ++it;
    }
    return result;
}

QVariant convertValueToQVariant(ExecState* exec, JSValue value, QMetaType::Type hint, int *distance, HashSet<JSObject*>* visitedObjects, int recursionLimit)
{
    --recursionLimit;

    if (!value || !recursionLimit)
        return QVariant();

    JSObject* object = 0;
    if (value.isObject()) {
        object = value.toObject(exec);
        if (visitedObjects->contains(object))
            return QVariant();

        visitedObjects->add(object);
    }

    // check magic pointer values before dereferencing value
    if (value == jsNaN()
        || (value == jsUndefined()
            && hint != QMetaType::QString
            && hint != (QMetaType::Type) qMetaTypeId<QVariant>())) {
        if (distance)
            *distance = -1;
        return QVariant();
    }

    JSLock lock(SilenceAssertionsOnly);
    JSRealType type = valueRealType(exec, value);
    if (hint == QMetaType::Void) {
        switch(type) {
            case Number:
                hint = QMetaType::Double;
                break;
            case Boolean:
                hint = QMetaType::Bool;
                break;
            case String:
            default:
                hint = QMetaType::QString;
                break;
            case Date:
                hint = QMetaType::QDateTime;
                break;
            case RegExp:
                hint = QMetaType::QRegExp;
                break;
            case Object:
                if (object->inherits(&NumberObject::s_info))
                    hint = QMetaType::Double;
                else if (object->inherits(&BooleanObject::s_info))
                    hint = QMetaType::Bool;
                else
                    hint = QMetaType::QVariantMap;
                break;
            case QObj:
                hint = QMetaType::QObjectStar;
                break;
            case JSByteArray:
                hint = QMetaType::QByteArray;
                break;
            case Array:
            case RTArray:
                hint = QMetaType::QVariantList;
                break;
        }
    }

    qConvDebug() << "convertValueToQVariant: jstype is " << type << ", hint is" << hint;

    if (value == jsNull()
        && hint != QMetaType::QObjectStar
        && hint != QMetaType::VoidStar
        && hint != QMetaType::QString
        && hint != (QMetaType::Type) qMetaTypeId<QVariant>()) {
        if (distance)
            *distance = -1;
        return QVariant();
    }

    QVariant ret;
    int dist = -1;
    switch (hint) {
        case QMetaType::Bool:
            if (type == Object && object->inherits(&BooleanObject::s_info))
                ret = QVariant(asBooleanObject(value)->internalValue().toBoolean(exec));
            else
                ret = QVariant(value.toBoolean(exec));
            if (type == Boolean)
                dist = 0;
            else
                dist = 10;
            break;

        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        case QMetaType::ULong:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::Float:
        case QMetaType::Double:
            ret = QVariant(value.toNumber(exec));
            ret.convert((QVariant::Type)hint);
            if (type == Number) {
                switch (hint) {
                case QMetaType::Double:
                    dist = 0;
                    break;
                case QMetaType::Float:
                    dist = 1;
                    break;
                case QMetaType::LongLong:
                case QMetaType::ULongLong:
                    dist = 2;
                    break;
                case QMetaType::Long:
                case QMetaType::ULong:
                    dist = 3;
                    break;
                case QMetaType::Int:
                case QMetaType::UInt:
                    dist = 4;
                    break;
                case QMetaType::Short:
                case QMetaType::UShort:
                    dist = 5;
                    break;
                    break;
                default:
                    dist = 10;
                    break;
                }
            } else {
                dist = 10;
            }
            break;

        case QMetaType::QChar:
            if (type == Number || type == Boolean) {
                ret = QVariant(QChar((ushort)value.toNumber(exec)));
                if (type == Boolean)
                    dist = 3;
                else
                    dist = 6;
            } else {
                UString str = value.toString(exec);
                ret = QVariant(QChar(str.length() ? *(const ushort*)str.impl()->characters() : 0));
                if (type == String)
                    dist = 3;
                else
                    dist = 10;
            }
            break;

        case QMetaType::QString: {
            if (value.isUndefinedOrNull()) {
                if (distance)
                    *distance = 1;
                return QString();
            } else {
                UString ustring = value.toString(exec);
                ret = QVariant(QString((const QChar*)ustring.impl()->characters(), ustring.length()));
                if (type == String)
                    dist = 0;
                else
                    dist = 10;
            }
            break;
        }

        case QMetaType::QVariantMap:
            if (type == Object || type == Array || type == RTArray) {
                ret = QVariant(convertValueToQVariantMap(exec, object, visitedObjects, recursionLimit));
                // Those types can still have perfect matches, e.g. 'bool' if value is a Boolean Object.
                dist = 1;
            }
            break;

        case QMetaType::QVariantList:
            if (type == RTArray) {
                RuntimeArray* rtarray = static_cast<RuntimeArray*>(object);

                QVariantList result;
                int len = rtarray->getLength();
                int objdist = 0;
                qConvDebug() << "converting a " << len << " length Array";
                for (int i = 0; i < len; ++i) {
                    JSValue val = rtarray->getConcreteArray()->valueAt(exec, i);
                    result.append(convertValueToQVariant(exec, val, QMetaType::Void, &objdist, visitedObjects, recursionLimit));
                    if (objdist == -1) {
                        qConvDebug() << "Failed converting element at index " << i;
                        break; // Failed converting a list entry, so fail the array
                    }
                }
                if (objdist != -1) {
                    dist = 5;
                    ret = QVariant(result);
                }
            } else if (type == Array) {
                JSArray* array = static_cast<JSArray*>(object);

                QVariantList result;
                int len = array->length();
                int objdist = 0;
                qConvDebug() << "converting a " << len << " length Array";
                for (int i = 0; i < len; ++i) {
                    JSValue val = array->get(exec, i);
                    result.append(convertValueToQVariant(exec, val, QMetaType::Void, &objdist, visitedObjects, recursionLimit));
                    if (objdist == -1) {
                        qConvDebug() << "Failed converting element at index " << i;
                        break; // Failed converting a list entry, so fail the array
                    }
                }
                if (objdist != -1) {
                    dist = 5;
                    ret = QVariant(result);
                }
            } else {
                // Make a single length array
                int objdist;
                qConvDebug() << "making a single length variantlist";
                QVariant var = convertValueToQVariant(exec, value, QMetaType::Void, &objdist, visitedObjects, recursionLimit);
                if (objdist != -1) {
                    QVariantList result;
                    result << var;
                    ret = QVariant(result);
                    dist = 10;
                } else {
                    qConvDebug() << "failed making single length varlist";
                }
            }
            break;

        case QMetaType::QStringList: {
            if (type == RTArray) {
                RuntimeArray* rtarray = static_cast<RuntimeArray*>(object);

                QStringList result;
                int len = rtarray->getLength();
                for (int i = 0; i < len; ++i) {
                    JSValue val = rtarray->getConcreteArray()->valueAt(exec, i);
                    UString ustring = val.toString(exec);
                    QString qstring = QString((const QChar*)ustring.impl()->characters(), ustring.length());

                    result.append(qstring);
                }
                dist = 5;
                ret = QVariant(result);
            } else if (type == Array) {
                JSArray* array = static_cast<JSArray*>(object);

                QStringList result;
                int len = array->length();
                for (int i = 0; i < len; ++i) {
                    JSValue val = array->get(exec, i);
                    UString ustring = val.toString(exec);
                    QString qstring = QString((const QChar*)ustring.impl()->characters(), ustring.length());

                    result.append(qstring);
                }
                dist = 5;
                ret = QVariant(result);
            } else {
                // Make a single length array
                UString ustring = value.toString(exec);
                QString qstring = QString((const QChar*)ustring.impl()->characters(), ustring.length());
                QStringList result;
                result.append(qstring);
                ret = QVariant(result);
                dist = 10;
            }
            break;
        }

        case QMetaType::QByteArray: {
            if (type == JSByteArray) {
                WTF::ByteArray* arr = asByteArray(value)->storage();
                ret = QVariant(QByteArray(reinterpret_cast<const char*>(arr->data()), arr->length()));
                dist = 0;
            } else {
                UString ustring = value.toString(exec);
                ret = QVariant(QString((const QChar*)ustring.impl()->characters(), ustring.length()).toLatin1());
                if (type == String)
                    dist = 5;
                else
                    dist = 10;
            }
            break;
        }

        case QMetaType::QDateTime:
        case QMetaType::QDate:
        case QMetaType::QTime:
            if (type == Date) {
                DateInstance* date = static_cast<DateInstance*>(object);
                GregorianDateTime gdt;
                msToGregorianDateTime(exec, date->internalNumber(), true, gdt);
                if (hint == QMetaType::QDateTime) {
                    ret = QDateTime(QDate(gdt.year + 1900, gdt.month + 1, gdt.monthDay), QTime(gdt.hour, gdt.minute, gdt.second), Qt::UTC);
                    dist = 0;
                } else if (hint == QMetaType::QDate) {
                    ret = QDate(gdt.year + 1900, gdt.month + 1, gdt.monthDay);
                    dist = 1;
                } else {
                    ret = QTime(gdt.hour + 1900, gdt.minute, gdt.second);
                    dist = 2;
                }
            } else if (type == Number) {
                double b = value.toNumber(exec);
                GregorianDateTime gdt;
                msToGregorianDateTime(exec, b, true, gdt);
                if (hint == QMetaType::QDateTime) {
                    ret = QDateTime(QDate(gdt.year + 1900, gdt.month + 1, gdt.monthDay), QTime(gdt.hour, gdt.minute, gdt.second), Qt::UTC);
                    dist = 6;
                } else if (hint == QMetaType::QDate) {
                    ret = QDate(gdt.year + 1900, gdt.month + 1, gdt.monthDay);
                    dist = 8;
                } else {
                    ret = QTime(gdt.hour, gdt.minute, gdt.second);
                    dist = 10;
                }
#ifndef QT_NO_DATESTRING
            } else if (type == String) {
                UString ustring = value.toString(exec);
                QString qstring = QString((const QChar*)ustring.impl()->characters(), ustring.length());

                if (hint == QMetaType::QDateTime) {
                    QDateTime dt = QDateTime::fromString(qstring, Qt::ISODate);
                    if (!dt.isValid())
                        dt = QDateTime::fromString(qstring, Qt::TextDate);
                    if (!dt.isValid())
                        dt = QDateTime::fromString(qstring, Qt::SystemLocaleDate);
                    if (!dt.isValid())
                        dt = QDateTime::fromString(qstring, Qt::LocaleDate);
                    if (dt.isValid()) {
                        ret = dt;
                        dist = 2;
                    }
                } else if (hint == QMetaType::QDate) {
                    QDate dt = QDate::fromString(qstring, Qt::ISODate);
                    if (!dt.isValid())
                        dt = QDate::fromString(qstring, Qt::TextDate);
                    if (!dt.isValid())
                        dt = QDate::fromString(qstring, Qt::SystemLocaleDate);
                    if (!dt.isValid())
                        dt = QDate::fromString(qstring, Qt::LocaleDate);
                    if (dt.isValid()) {
                        ret = dt;
                        dist = 3;
                    }
                } else {
                    QTime dt = QTime::fromString(qstring, Qt::ISODate);
                    if (!dt.isValid())
                        dt = QTime::fromString(qstring, Qt::TextDate);
                    if (!dt.isValid())
                        dt = QTime::fromString(qstring, Qt::SystemLocaleDate);
                    if (!dt.isValid())
                        dt = QTime::fromString(qstring, Qt::LocaleDate);
                    if (dt.isValid()) {
                        ret = dt;
                        dist = 3;
                    }
                }
#endif // QT_NO_DATESTRING
            }
            break;

        case QMetaType::QRegExp:
            if (type == RegExp) {
/*
                RegExpObject *re = static_cast<RegExpObject*>(object);
*/
                // Attempt to convert.. a bit risky
                UString ustring = value.toString(exec);
                QString qstring = QString((const QChar*)ustring.impl()->characters(), ustring.length());

                // this is of the form '/xxxxxx/i'
                int firstSlash = qstring.indexOf(QLatin1Char('/'));
                int lastSlash = qstring.lastIndexOf(QLatin1Char('/'));
                if (firstSlash >=0 && lastSlash > firstSlash) {
                    QRegExp realRe;

                    realRe.setPattern(qstring.mid(firstSlash + 1, lastSlash - firstSlash - 1));

                    if (qstring.mid(lastSlash + 1).contains(QLatin1Char('i')))
                        realRe.setCaseSensitivity(Qt::CaseInsensitive);

                    ret = QVariant::fromValue(realRe);
                    dist = 0;
                } else {
                    qConvDebug() << "couldn't parse a JS regexp";
                }
            } else if (type == String) {
                UString ustring = value.toString(exec);
                QString qstring = QString((const QChar*)ustring.impl()->characters(), ustring.length());

                QRegExp re(qstring);
                if (re.isValid()) {
                    ret = QVariant::fromValue(re);
                    dist = 10;
                }
            }
            break;

        case QMetaType::QObjectStar:
            if (type == QObj) {
                QtInstance* qtinst = QtInstance::getInstance(object);
                if (qtinst) {
                    if (qtinst->getObject()) {
                        qConvDebug() << "found instance, with object:" << (void*) qtinst->getObject();
                        ret = QVariant::fromValue(qtinst->getObject());
                        qConvDebug() << ret;
                        dist = 0;
                    } else {
                        qConvDebug() << "can't convert deleted qobject";
                    }
                } else {
                    qConvDebug() << "wasn't a qtinstance";
                }
            } else if (type == Null) {
                QObject* nullobj = 0;
                ret = QVariant::fromValue(nullobj);
                dist = 0;
            } else {
                qConvDebug() << "previous type was not an object:" << type;
            }
            break;

        case QMetaType::VoidStar:
            if (type == QObj) {
                QtInstance* qtinst = QtInstance::getInstance(object);
                if (qtinst) {
                    if (qtinst->getObject()) {
                        qConvDebug() << "found instance, with object:" << (void*) qtinst->getObject();
                        ret = QVariant::fromValue((void *)qtinst->getObject());
                        qConvDebug() << ret;
                        dist = 0;
                    } else {
                        qConvDebug() << "can't convert deleted qobject";
                    }
                } else {
                    qConvDebug() << "wasn't a qtinstance";
                }
            } else if (type == Null) {
                ret = QVariant::fromValue((void*)0);
                dist = 0;
            } else if (type == Number) {
                // I don't think that converting a double to a pointer is a wise
                // move.  Except maybe 0.
                qConvDebug() << "got number for void * - not converting, seems unsafe:" << value.toNumber(exec);
            } else {
                qConvDebug() << "void* - unhandled type" << type;
            }
            break;

        default:
            // Non const type ids
            if (hint == (QMetaType::Type) qMetaTypeId<QObjectList>())
            {
                if (type == RTArray) {
                    RuntimeArray* rtarray = static_cast<RuntimeArray*>(object);

                    QObjectList result;
                    int len = rtarray->getLength();
                    for (int i = 0; i < len; ++i) {
                        JSValue val = rtarray->getConcreteArray()->valueAt(exec, i);
                        int itemdist = -1;
                        QVariant item = convertValueToQVariant(exec, val, QMetaType::QObjectStar, &itemdist, visitedObjects, recursionLimit);
                        if (itemdist >= 0)
                            result.append(item.value<QObject*>());
                        else
                            break;
                    }
                    // If we didn't fail conversion
                    if (result.count() == len) {
                        dist = 5;
                        ret = QVariant::fromValue(result);
                    }
                } else if (type == Array) {
                    JSObject* object = value.toObject(exec);
                    JSArray* array = static_cast<JSArray *>(object);
                    QObjectList result;
                    int len = array->length();
                    for (int i = 0; i < len; ++i) {
                        JSValue val = array->get(exec, i);
                        int itemdist = -1;
                        QVariant item = convertValueToQVariant(exec, val, QMetaType::QObjectStar, &itemdist, visitedObjects, recursionLimit);
                        if (itemdist >= 0)
                            result.append(item.value<QObject*>());
                        else
                            break;
                    }
                    // If we didn't fail conversion
                    if (result.count() == len) {
                        dist = 5;
                        ret = QVariant::fromValue(result);
                    }
                } else {
                    // Make a single length array
                    QObjectList result;
                    int itemdist = -1;
                    QVariant item = convertValueToQVariant(exec, value, QMetaType::QObjectStar, &itemdist, visitedObjects, recursionLimit);
                    if (itemdist >= 0) {
                        result.append(item.value<QObject*>());
                        dist = 10;
                        ret = QVariant::fromValue(result);
                    }
                }
                break;
            } else if (hint == (QMetaType::Type) qMetaTypeId<QList<int> >()) {
                if (type == RTArray) {
                    RuntimeArray* rtarray = static_cast<RuntimeArray*>(object);

                    QList<int> result;
                    int len = rtarray->getLength();
                    for (int i = 0; i < len; ++i) {
                        JSValue val = rtarray->getConcreteArray()->valueAt(exec, i);
                        int itemdist = -1;
                        QVariant item = convertValueToQVariant(exec, val, QMetaType::Int, &itemdist, visitedObjects, recursionLimit);
                        if (itemdist >= 0)
                            result.append(item.value<int>());
                        else
                            break;
                    }
                    // If we didn't fail conversion
                    if (result.count() == len) {
                        dist = 5;
                        ret = QVariant::fromValue(result);
                    }
                } else if (type == Array) {
                    JSArray* array = static_cast<JSArray *>(object);

                    QList<int> result;
                    int len = array->length();
                    for (int i = 0; i < len; ++i) {
                        JSValue val = array->get(exec, i);
                        int itemdist = -1;
                        QVariant item = convertValueToQVariant(exec, val, QMetaType::Int, &itemdist, visitedObjects, recursionLimit);
                        if (itemdist >= 0)
                            result.append(item.value<int>());
                        else
                            break;
                    }
                    // If we didn't fail conversion
                    if (result.count() == len) {
                        dist = 5;
                        ret = QVariant::fromValue(result);
                    }
                } else {
                    // Make a single length array
                    QList<int> result;
                    int itemdist = -1;
                    QVariant item = convertValueToQVariant(exec, value, QMetaType::Int, &itemdist, visitedObjects, recursionLimit);
                    if (itemdist >= 0) {
                        result.append(item.value<int>());
                        dist = 10;
                        ret = QVariant::fromValue(result);
                    }
                }
                break;
            } else if (QtPixmapInstance::canHandle(static_cast<QMetaType::Type>(hint))) {
                ret = QtPixmapInstance::variantFromObject(object, static_cast<QMetaType::Type>(hint));
            } else if (hint == (QMetaType::Type) qMetaTypeId<QWebElement>()) {
                if (object && object->inherits(&JSElement::s_info)) {
                    ret = QVariant::fromValue<QWebElement>(QtWebElementRuntime::create((static_cast<JSElement*>(object))->impl()));
                    dist = 0;
                    // Allow other objects to reach this one. This won't cause our algorithm to
                    // loop since when we find an Element we do not recurse.
                    visitedObjects->remove(object);
                    break;
                }
                if (object && object->inherits(&JSDocument::s_info)) {
                    // To support LayoutTestControllerQt::nodesFromRect(), used in DRT, we do an implicit
                    // conversion from 'document' to the QWebElement representing the 'document.documentElement'.
                    // We can't simply use a QVariantMap in nodesFromRect() because it currently times out
                    // when serializing DOMMimeType and DOMPlugin, even if we limit the recursion.
                    ret = QVariant::fromValue<QWebElement>(QtWebElementRuntime::create((static_cast<JSDocument*>(object))->impl()->documentElement()));
                } else
                    ret = QVariant::fromValue<QWebElement>(QWebElement());
            } else if (hint == (QMetaType::Type) qMetaTypeId<QDRTNode>()) {
                if (object && object->inherits(&JSNode::s_info))
                    ret = QVariant::fromValue<QDRTNode>(QtDRTNodeRuntime::create((static_cast<JSNode*>(object))->impl()));
            } else if (hint == (QMetaType::Type) qMetaTypeId<QVariant>()) {
                if (value.isUndefinedOrNull()) {
                    if (distance)
                        *distance = 1;
                    return QVariant();
                } else {
                    if (type == Object) {
                        // Since we haven't really visited this object yet, we remove it
                        visitedObjects->remove(object);
                    }

                    // And then recurse with the autodetect flag
                    ret = convertValueToQVariant(exec, value, QMetaType::Void, distance, visitedObjects, recursionLimit);
                    dist = 10;
                }
                break;
            }

            dist = 10;
            break;
    }

    if (!ret.isValid())
        dist = -1;
    if (distance)
        *distance = dist;

    return ret;
}

QVariant convertValueToQVariant(ExecState* exec, JSValue value, QMetaType::Type hint, int *distance)
{
    const int recursionLimit = 200;
    HashSet<JSObject*> visitedObjects;
    return convertValueToQVariant(exec, value, hint, distance, &visitedObjects, recursionLimit);
}

JSValue convertQVariantToValue(ExecState* exec, PassRefPtr<RootObject> root, const QVariant& variant)
{
    // Variants with QObject * can be isNull but not a null pointer
    // An empty QString variant is also null
    QMetaType::Type type = (QMetaType::Type) variant.userType();

    qConvDebug() << "convertQVariantToValue: metatype:" << type << ", isnull: " << variant.isNull();
    if (variant.isNull() &&
        type != QMetaType::QObjectStar &&
        type != QMetaType::VoidStar &&
        type != QMetaType::QWidgetStar &&
        type != QMetaType::QString) {
        return jsNull();
    }

    JSLock lock(SilenceAssertionsOnly);

    if (type == QMetaType::Bool)
        return jsBoolean(variant.toBool());

    if (type == QMetaType::Int ||
        type == QMetaType::UInt ||
        type == QMetaType::Long ||
        type == QMetaType::ULong ||
        type == QMetaType::LongLong ||
        type == QMetaType::ULongLong ||
        type == QMetaType::Short ||
        type == QMetaType::UShort ||
        type == QMetaType::Float ||
        type == QMetaType::Double)
        return jsNumber(variant.toDouble());

    if (type == QMetaType::QRegExp) {
        QRegExp re = variant.value<QRegExp>();

        if (re.isValid()) {
            UString pattern((UChar*)re.pattern().utf16(), re.pattern().length());
            RegExpFlags flags = (re.caseSensitivity() == Qt::CaseInsensitive) ? FlagIgnoreCase : NoFlags;

            RefPtr<JSC::RegExp> regExp = JSC::RegExp::create(&exec->globalData(), pattern, flags);
            if (regExp->isValid())
                return new (exec) RegExpObject(exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->regExpStructure(), regExp.release());
            return jsNull();
        }
    }

    if (type == QMetaType::QDateTime ||
        type == QMetaType::QDate ||
        type == QMetaType::QTime) {

        QDate date = QDate::currentDate();
        QTime time(0,0,0); // midnight

        if (type == QMetaType::QDate)
            date = variant.value<QDate>();
        else if (type == QMetaType::QTime)
            time = variant.value<QTime>();
        else {
            QDateTime dt = variant.value<QDateTime>().toLocalTime();
            date = dt.date();
            time = dt.time();
        }

        // Dates specified this way are in local time (we convert DateTimes above)
        GregorianDateTime dt;
        dt.year = date.year() - 1900;
        dt.month = date.month() - 1;
        dt.monthDay = date.day();
        dt.hour = time.hour();
        dt.minute = time.minute();
        dt.second = time.second();
        dt.isDST = -1;
        double ms = gregorianDateTimeToMS(exec, dt, time.msec(), /*inputIsUTC*/ false);

        return new (exec) DateInstance(exec, exec->lexicalGlobalObject()->dateStructure(), trunc(ms));
    }

    if (type == QMetaType::QByteArray) {
        QByteArray qtByteArray = variant.value<QByteArray>();
        WTF::RefPtr<WTF::ByteArray> wtfByteArray = WTF::ByteArray::create(qtByteArray.length());
        memcpy(wtfByteArray->data(), qtByteArray.constData(), qtByteArray.length());
        return new (exec) JSC::JSByteArray(exec, JSC::JSByteArray::createStructure(exec->globalData(), jsNull()), wtfByteArray.get());
    }

    if (type == QMetaType::QObjectStar || type == QMetaType::QWidgetStar) {
        QObject* obj = variant.value<QObject*>();
        if (!obj)
            return jsNull();
        return QtInstance::getQtInstance(obj, root, QScriptEngine::QtOwnership)->createRuntimeObject(exec);
    }

    if (QtPixmapInstance::canHandle(static_cast<QMetaType::Type>(variant.type())))
        return QtPixmapInstance::createPixmapRuntimeObject(exec, root, variant);

    if (type == qMetaTypeId<QWebElement>()) {
        if (!root->globalObject()->inherits(&JSDOMWindow::s_info))
            return jsUndefined();

        Document* document = (static_cast<JSDOMWindow*>(root->globalObject()))->impl()->document();
        if (!document)
            return jsUndefined();

        return toJS(exec, toJSDOMGlobalObject(document, exec), QtWebElementRuntime::get(variant.value<QWebElement>()));
    }

    if (type == qMetaTypeId<QDRTNode>()) {
        if (!root->globalObject()->inherits(&JSDOMWindow::s_info))
            return jsUndefined();

        Document* document = (static_cast<JSDOMWindow*>(root->globalObject()))->impl()->document();
        if (!document)
            return jsUndefined();

        return toJS(exec, toJSDOMGlobalObject(document, exec), QtDRTNodeRuntime::get(variant.value<QDRTNode>()));
    }

    if (type == QMetaType::QVariantMap) {
        // create a new object, and stuff properties into it
        JSObject* ret = constructEmptyObject(exec);
        QVariantMap map = variant.value<QVariantMap>();
        QVariantMap::const_iterator i = map.constBegin();
        while (i != map.constEnd()) {
            QString s = i.key();
            JSValue val = convertQVariantToValue(exec, root.get(), i.value());
            if (val) {
                PutPropertySlot slot;
                ret->put(exec, Identifier(exec, reinterpret_cast_ptr<const UChar *>(s.constData()), s.length()), val, slot);
                // ### error case?
            }
            ++i;
        }

        return ret;
    }

    // List types
    if (type == QMetaType::QVariantList) {
        QVariantList vl = variant.toList();
        qConvDebug() << "got a " << vl.count() << " length list:" << vl;
        return new (exec) RuntimeArray(exec, new QtArray<QVariant>(vl, QMetaType::Void, root));
    } else if (type == QMetaType::QStringList) {
        QStringList sl = variant.value<QStringList>();
        return new (exec) RuntimeArray(exec, new QtArray<QString>(sl, QMetaType::QString, root));
    } else if (type == (QMetaType::Type) qMetaTypeId<QObjectList>()) {
        QObjectList ol= variant.value<QObjectList>();
        return new (exec) RuntimeArray(exec, new QtArray<QObject*>(ol, QMetaType::QObjectStar, root));
    } else if (type == (QMetaType::Type)qMetaTypeId<QList<int> >()) {
        QList<int> il= variant.value<QList<int> >();
        return new (exec) RuntimeArray(exec, new QtArray<int>(il, QMetaType::Int, root));
    }

    if (type == (QMetaType::Type)qMetaTypeId<QVariant>()) {
        QVariant real = variant.value<QVariant>();
        qConvDebug() << "real variant is:" << real;
        return convertQVariantToValue(exec, root, real);
    }

    qConvDebug() << "fallback path for" << variant << variant.userType();

    QString string = variant.toString();
    UString ustring((UChar*)string.utf16(), string.length());
    return jsString(exec, ustring);
}

// ===============

// Qt-like macros
#define QW_D(Class) Class##Data* d = d_func()
#define QW_DS(Class,Instance) Class##Data* d = Instance->d_func()

const ClassInfo QtRuntimeMethod::s_info = { "QtRuntimeMethod", &InternalFunction::s_info, 0, 0 };

QtRuntimeMethod::QtRuntimeMethod(QtRuntimeMethodData* dd, ExecState* exec, const Identifier& ident, PassRefPtr<QtInstance> inst)
    : InternalFunction(&exec->globalData(), exec->lexicalGlobalObject(), deprecatedGetDOMStructure<QtRuntimeMethod>(exec), ident)
    , d_ptr(dd)
{
    QW_D(QtRuntimeMethod);
    d->m_instance = inst;
}

QtRuntimeMethod::~QtRuntimeMethod()
{
    QW_D(QtRuntimeMethod);
    d->m_instance->removeCachedMethod(this);
    delete d_ptr;
}

// ===============

QtRuntimeMethodData::~QtRuntimeMethodData()
{
}

QtRuntimeMetaMethodData::~QtRuntimeMetaMethodData()
{

}

QtRuntimeConnectionMethodData::~QtRuntimeConnectionMethodData()
{

}

// ===============

// Type conversion metadata (from QtScript originally)
class QtMethodMatchType
{
public:
    enum Kind {
        Invalid,
        Variant,
        MetaType,
        Unresolved,
        MetaEnum
    };


    QtMethodMatchType()
        : m_kind(Invalid) { }

    Kind kind() const
    { return m_kind; }

    QMetaType::Type typeId() const;

    bool isValid() const
    { return (m_kind != Invalid); }

    bool isVariant() const
    { return (m_kind == Variant); }

    bool isMetaType() const
    { return (m_kind == MetaType); }

    bool isUnresolved() const
    { return (m_kind == Unresolved); }

    bool isMetaEnum() const
    { return (m_kind == MetaEnum); }

    QByteArray name() const;

    int enumeratorIndex() const
    { Q_ASSERT(isMetaEnum()); return m_typeId; }

    static QtMethodMatchType variant()
    { return QtMethodMatchType(Variant); }

    static QtMethodMatchType metaType(int typeId, const QByteArray &name)
    { return QtMethodMatchType(MetaType, typeId, name); }

    static QtMethodMatchType metaEnum(int enumIndex, const QByteArray &name)
    { return QtMethodMatchType(MetaEnum, enumIndex, name); }

    static QtMethodMatchType unresolved(const QByteArray &name)
    { return QtMethodMatchType(Unresolved, /*typeId=*/0, name); }

private:
    QtMethodMatchType(Kind kind, int typeId = 0, const QByteArray &name = QByteArray())
        : m_kind(kind), m_typeId(typeId), m_name(name) { }

    Kind m_kind;
    int m_typeId;
    QByteArray m_name;
};

QMetaType::Type QtMethodMatchType::typeId() const
{
    if (isVariant())
        return (QMetaType::Type) QMetaType::type("QVariant");
    return (QMetaType::Type) (isMetaEnum() ? QMetaType::Int : m_typeId);
}

QByteArray QtMethodMatchType::name() const
{
    if (!m_name.isEmpty())
        return m_name;
    else if (m_kind == Variant)
        return "QVariant";
    return QByteArray();
}

struct QtMethodMatchData
{
    int matchDistance;
    int index;
    QVector<QtMethodMatchType> types;
    QVarLengthArray<QVariant, 10> args;

    QtMethodMatchData(int dist, int idx, QVector<QtMethodMatchType> typs,
                                const QVarLengthArray<QVariant, 10> &as)
        : matchDistance(dist), index(idx), types(typs), args(as) { }
    QtMethodMatchData()
        : index(-1) { }

    bool isValid() const
    { return (index != -1); }

    int firstUnresolvedIndex() const
    {
        for (int i=0; i < types.count(); i++) {
            if (types.at(i).isUnresolved())
                return i;
        }
        return -1;
    }
};

static int indexOfMetaEnum(const QMetaObject *meta, const QByteArray &str)
{
    QByteArray scope;
    QByteArray name;
    int scopeIdx = str.indexOf("::");
    if (scopeIdx != -1) {
        scope = str.left(scopeIdx);
        name = str.mid(scopeIdx + 2);
    } else {
        name = str;
    }
    for (int i = meta->enumeratorCount() - 1; i >= 0; --i) {
        QMetaEnum m = meta->enumerator(i);
        if ((m.name() == name)/* && (scope.isEmpty() || (m.scope() == scope))*/)
            return i;
    }
    return -1;
}

// Helper function for resolving methods
// Largely based on code in QtScript for compatibility reasons
static int findMethodIndex(ExecState* exec,
                           const QMetaObject* meta,
                           const QByteArray& signature,
                           bool allowPrivate,
                           QVarLengthArray<QVariant, 10> &vars,
                           void** vvars,
                           JSObject **pError)
{
    QList<int> matchingIndices;

    bool overloads = !signature.contains('(');

    int count = meta->methodCount();
    for (int i = count - 1; i >= 0; --i) {
        const QMetaMethod m = meta->method(i);

        // Don't choose private methods
        if (m.access() == QMetaMethod::Private && !allowPrivate)
            continue;

        // try and find all matching named methods
        if (m.signature() == signature)
            matchingIndices.append(i);
        else if (overloads) {
            QByteArray rawsignature = m.signature();
            rawsignature.truncate(rawsignature.indexOf('('));
            if (rawsignature == signature)
                matchingIndices.append(i);
        }
    }

    int chosenIndex = -1;
    *pError = 0;
    QVector<QtMethodMatchType> chosenTypes;

    QVarLengthArray<QVariant, 10> args;
    QVector<QtMethodMatchData> candidates;
    QVector<QtMethodMatchData> unresolved;
    QVector<int> tooFewArgs;
    QVector<int> conversionFailed;

    foreach(int index, matchingIndices) {
        QMetaMethod method = meta->method(index);

        QVector<QtMethodMatchType> types;
        bool unresolvedTypes = false;

        // resolve return type
        QByteArray returnTypeName = method.typeName();
        int rtype = QMetaType::type(returnTypeName);
        if ((rtype == 0) && !returnTypeName.isEmpty()) {
            if (returnTypeName == "QVariant") {
                types.append(QtMethodMatchType::variant());
            } else if (returnTypeName.endsWith('*')) {
                types.append(QtMethodMatchType::metaType(QMetaType::VoidStar, returnTypeName));
            } else {
                int enumIndex = indexOfMetaEnum(meta, returnTypeName);
                if (enumIndex != -1)
                    types.append(QtMethodMatchType::metaEnum(enumIndex, returnTypeName));
                else {
                    unresolvedTypes = true;
                    types.append(QtMethodMatchType::unresolved(returnTypeName));
                }
            }
        } else {
            if (returnTypeName == "QVariant")
                types.append(QtMethodMatchType::variant());
            else
                types.append(QtMethodMatchType::metaType(rtype, returnTypeName));
        }

        // resolve argument types
        QList<QByteArray> parameterTypeNames = method.parameterTypes();
        for (int i = 0; i < parameterTypeNames.count(); ++i) {
            QByteArray argTypeName = parameterTypeNames.at(i);
            int atype = QMetaType::type(argTypeName);
            if (atype == 0) {
                if (argTypeName == "QVariant") {
                    types.append(QtMethodMatchType::variant());
                } else {
                    int enumIndex = indexOfMetaEnum(meta, argTypeName);
                    if (enumIndex != -1)
                        types.append(QtMethodMatchType::metaEnum(enumIndex, argTypeName));
                    else {
                        unresolvedTypes = true;
                        types.append(QtMethodMatchType::unresolved(argTypeName));
                    }
                }
            } else {
                if (argTypeName == "QVariant")
                    types.append(QtMethodMatchType::variant());
                else
                    types.append(QtMethodMatchType::metaType(atype, argTypeName));
            }
        }

        // If the native method requires more arguments than what was passed from JavaScript
        if (exec->argumentCount() + 1 < static_cast<unsigned>(types.count())) {
            qMatchDebug() << "Match:too few args for" << method.signature();
            tooFewArgs.append(index);
            continue;
        }

        if (unresolvedTypes) {
            qMatchDebug() << "Match:unresolved arg types for" << method.signature();
            // remember it so we can give an error message later, if necessary
            unresolved.append(QtMethodMatchData(/*matchDistance=*/INT_MAX, index,
                                                   types, QVarLengthArray<QVariant, 10>()));
            continue;
        }

        // Now convert arguments
        if (args.count() != types.count())
            args.resize(types.count());

        QtMethodMatchType retType = types[0];
        args[0] = QVariant(retType.typeId(), (void *)0); // the return value

        bool converted = true;
        int matchDistance = 0;
        for (unsigned i = 0; converted && i + 1 < static_cast<unsigned>(types.count()); ++i) {
            JSValue arg = i < exec->argumentCount() ? exec->argument(i) : jsUndefined();

            int argdistance = -1;
            QVariant v = convertValueToQVariant(exec, arg, types.at(i+1).typeId(), &argdistance);
            if (argdistance >= 0) {
                matchDistance += argdistance;
                args[i+1] = v;
            } else {
                qMatchDebug() << "failed to convert argument " << i << "type" << types.at(i+1).typeId() << QMetaType::typeName(types.at(i+1).typeId());
                converted = false;
            }
        }

        qMatchDebug() << "Match: " << method.signature() << (converted ? "converted":"failed to convert") << "distance " << matchDistance;

        if (converted) {
            if ((exec->argumentCount() + 1 == static_cast<unsigned>(types.count()))
                && (matchDistance == 0)) {
                // perfect match, use this one
                chosenIndex = index;
                break;
            } else {
                QtMethodMatchData currentMatch(matchDistance, index, types, args);
                if (candidates.isEmpty()) {
                    candidates.append(currentMatch);
                } else {
                    QtMethodMatchData bestMatchSoFar = candidates.at(0);
                    if ((args.count() > bestMatchSoFar.args.count())
                        || ((args.count() == bestMatchSoFar.args.count())
                            && (matchDistance <= bestMatchSoFar.matchDistance))) {
                        candidates.prepend(currentMatch);
                    } else {
                        candidates.append(currentMatch);
                    }
                }
            }
        } else {
            conversionFailed.append(index);
        }

        if (!overloads)
            break;
    }

    if (chosenIndex == -1 && candidates.count() == 0) {
        // No valid functions at all - format an error message
        if (!conversionFailed.isEmpty()) {
            QString message = QString::fromLatin1("incompatible type of argument(s) in call to %0(); candidates were\n")
                              .arg(QLatin1String(signature));
            for (int i = 0; i < conversionFailed.size(); ++i) {
                if (i > 0)
                    message += QLatin1String("\n");
                QMetaMethod mtd = meta->method(conversionFailed.at(i));
                message += QString::fromLatin1("    %0").arg(QString::fromLatin1(mtd.signature()));
            }
            *pError = throwError(exec, createTypeError(exec, message.toLatin1().constData()));
        } else if (!unresolved.isEmpty()) {
            QtMethodMatchData argsInstance = unresolved.first();
            int unresolvedIndex = argsInstance.firstUnresolvedIndex();
            Q_ASSERT(unresolvedIndex != -1);
            QtMethodMatchType unresolvedType = argsInstance.types.at(unresolvedIndex);
            QString message = QString::fromLatin1("cannot call %0(): unknown type `%1'")
                .arg(QString::fromLatin1(signature))
                .arg(QLatin1String(unresolvedType.name()));
            *pError = throwError(exec, createTypeError(exec, message.toLatin1().constData()));
        } else {
            QString message = QString::fromLatin1("too few arguments in call to %0(); candidates are\n")
                              .arg(QLatin1String(signature));
            for (int i = 0; i < tooFewArgs.size(); ++i) {
                if (i > 0)
                    message += QLatin1String("\n");
                QMetaMethod mtd = meta->method(tooFewArgs.at(i));
                message += QString::fromLatin1("    %0").arg(QString::fromLatin1(mtd.signature()));
            }
            *pError = throwError(exec, createSyntaxError(exec, message.toLatin1().constData()));
        }
    }

    if (chosenIndex == -1 && candidates.count() > 0) {
        QtMethodMatchData bestMatch = candidates.at(0);
        if ((candidates.size() > 1)
            && (bestMatch.args.count() == candidates.at(1).args.count())
            && (bestMatch.matchDistance == candidates.at(1).matchDistance)) {
            // ambiguous call
            QString message = QString::fromLatin1("ambiguous call of overloaded function %0(); candidates were\n")
                                .arg(QLatin1String(signature));
            for (int i = 0; i < candidates.size(); ++i) {
                // Only candidate for overload if argument count and match distance is same as best match
                if (candidates.at(i).args.count() == bestMatch.args.count()
                    || candidates.at(i).matchDistance == bestMatch.matchDistance) {
                    if (i > 0)
                        message += QLatin1String("\n");
                    QMetaMethod mtd = meta->method(candidates.at(i).index);
                    message += QString::fromLatin1("    %0").arg(QString::fromLatin1(mtd.signature()));
                }
            }
            *pError = throwError(exec, createTypeError(exec, message.toLatin1().constData()));
        } else {
            chosenIndex = bestMatch.index;
            args = bestMatch.args;
        }
    }

    if (chosenIndex != -1) {
        /* Copy the stuff over */
        int i;
        vars.resize(args.count());
        for (i=0; i < args.count(); i++) {
            vars[i] = args[i];
            vvars[i] = vars[i].data();
        }
    }

    return chosenIndex;
}

// Signals are not fuzzy matched as much as methods
static int findSignalIndex(const QMetaObject* meta, int initialIndex, QByteArray signature)
{
    int index = initialIndex;
    QMetaMethod method = meta->method(index);
    bool overloads = !signature.contains('(');
    if (overloads && (method.attributes() & QMetaMethod::Cloned)) {
        // find the most general method
        do {
            method = meta->method(--index);
        } while (method.attributes() & QMetaMethod::Cloned);
    }
    return index;
}

QtRuntimeMetaMethod::QtRuntimeMetaMethod(ExecState* exec, const Identifier& ident, PassRefPtr<QtInstance> inst, int index, const QByteArray& signature, bool allowPrivate)
    : QtRuntimeMethod (new QtRuntimeMetaMethodData(), exec, ident, inst)
{
    QW_D(QtRuntimeMetaMethod);
    d->m_signature = signature;
    d->m_index = index;
    d->m_allowPrivate = allowPrivate;
}

void QtRuntimeMetaMethod::visitChildren(SlotVisitor& visitor)
{
    QtRuntimeMethod::visitChildren(visitor);
    QW_D(QtRuntimeMetaMethod);
    if (d->m_connect)
        visitor.append(&d->m_connect);
    if (d->m_disconnect)
        visitor.append(&d->m_disconnect);
}

EncodedJSValue QtRuntimeMetaMethod::call(ExecState* exec)
{
    QtRuntimeMetaMethodData* d = static_cast<QtRuntimeMetaMethod *>(exec->callee())->d_func();

    // We're limited to 10 args
    if (exec->argumentCount() > 10)
        return JSValue::encode(jsUndefined());

    // We have to pick a method that matches..
    JSLock lock(SilenceAssertionsOnly);

    QObject *obj = d->m_instance->getObject();
    if (obj) {
        QVarLengthArray<QVariant, 10> vargs;
        void *qargs[11];

        int methodIndex;
        JSObject* errorObj = 0;
        if ((methodIndex = findMethodIndex(exec, obj->metaObject(), d->m_signature, d->m_allowPrivate, vargs, (void **)qargs, &errorObj)) != -1) {
            if (QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, methodIndex, qargs) >= 0)
                return JSValue::encode(jsUndefined());

            if (vargs[0].isValid())
                return JSValue::encode(convertQVariantToValue(exec, d->m_instance->rootObject(), vargs[0]));
        }

        if (errorObj)
            return JSValue::encode(errorObj);
    } else {
        return throwVMError(exec, createError(exec, "cannot call function of deleted QObject"));
    }

    // void functions return undefined
    return JSValue::encode(jsUndefined());
}

CallType QtRuntimeMetaMethod::getCallData(CallData& callData)
{
    callData.native.function = call;
    return CallTypeHost;
}

bool QtRuntimeMetaMethod::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (propertyName == "connect") {
        slot.setCustom(this, connectGetter);
        return true;
    } else if (propertyName == "disconnect") {
        slot.setCustom(this, disconnectGetter);
        return true;
    } else if (propertyName == exec->propertyNames().length) {
        slot.setCustom(this, lengthGetter);
        return true;
    }

    return QtRuntimeMethod::getOwnPropertySlot(exec, propertyName, slot);
}

bool QtRuntimeMetaMethod::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == "connect") {
        PropertySlot slot;
        slot.setCustom(this, connectGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), DontDelete | ReadOnly | DontEnum);
        return true;
    }

    if (propertyName == "disconnect") {
        PropertySlot slot;
        slot.setCustom(this, disconnectGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), DontDelete | ReadOnly | DontEnum);
        return true;
    }

    if (propertyName == exec->propertyNames().length) {
        PropertySlot slot;
        slot.setCustom(this, lengthGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), DontDelete | ReadOnly | DontEnum);
        return true;
    }

    return QtRuntimeMethod::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void QtRuntimeMetaMethod::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    if (mode == IncludeDontEnumProperties) {
        propertyNames.add(Identifier(exec, "connect"));
        propertyNames.add(Identifier(exec, "disconnect"));
        propertyNames.add(exec->propertyNames().length);
    }

    QtRuntimeMethod::getOwnPropertyNames(exec, propertyNames, mode);
}

JSValue QtRuntimeMetaMethod::lengthGetter(ExecState*, JSValue, const Identifier&)
{
    // QtScript always returns 0
    return jsNumber(0);
}

JSValue QtRuntimeMetaMethod::connectGetter(ExecState* exec, JSValue slotBase, const Identifier& ident)
{
    QtRuntimeMetaMethod* thisObj = static_cast<QtRuntimeMetaMethod*>(asObject(slotBase));
    QW_DS(QtRuntimeMetaMethod, thisObj);

    if (!d->m_connect)
        d->m_connect.set(exec->globalData(), thisObj, new (exec) QtRuntimeConnectionMethod(exec, ident, true, d->m_instance, d->m_index, d->m_signature));
    return d->m_connect.get();
}

JSValue QtRuntimeMetaMethod::disconnectGetter(ExecState* exec, JSValue slotBase, const Identifier& ident)
{
    QtRuntimeMetaMethod* thisObj = static_cast<QtRuntimeMetaMethod*>(asObject(slotBase));
    QW_DS(QtRuntimeMetaMethod, thisObj);

    if (!d->m_disconnect)
        d->m_disconnect.set(exec->globalData(), thisObj, new (exec) QtRuntimeConnectionMethod(exec, ident, false, d->m_instance, d->m_index, d->m_signature));
    return d->m_disconnect.get();
}

// ===============

QMultiMap<QObject*, QtConnectionObject*> QtRuntimeConnectionMethod::connections;

QtRuntimeConnectionMethod::QtRuntimeConnectionMethod(ExecState* exec, const Identifier& ident, bool isConnect, PassRefPtr<QtInstance> inst, int index, const QByteArray& signature)
    : QtRuntimeMethod (new QtRuntimeConnectionMethodData(), exec, ident, inst)
{
    QW_D(QtRuntimeConnectionMethod);

    d->m_signature = signature;
    d->m_index = index;
    d->m_isConnect = isConnect;
}

EncodedJSValue QtRuntimeConnectionMethod::call(ExecState* exec)
{
    QtRuntimeConnectionMethodData* d = static_cast<QtRuntimeConnectionMethod *>(exec->callee())->d_func();

    JSLock lock(SilenceAssertionsOnly);

    QObject* sender = d->m_instance->getObject();

    if (sender) {

        JSObject* thisObject = exec->lexicalGlobalObject();
        JSObject* funcObject = 0;

        // QtScript checks signalness first, arguments second
        int signalIndex = -1;

        // Make sure the initial index is a signal
        QMetaMethod m = sender->metaObject()->method(d->m_index);
        if (m.methodType() == QMetaMethod::Signal)
            signalIndex = findSignalIndex(sender->metaObject(), d->m_index, d->m_signature);

        if (signalIndex != -1) {
            if (exec->argumentCount() == 1) {
                funcObject = exec->argument(0).toObject(exec);
                CallData callData;
                if (funcObject->getCallData(callData) == CallTypeNone) {
                    if (d->m_isConnect)
                        return throwVMError(exec, createTypeError(exec, "QtMetaMethod.connect: target is not a function"));
                    else
                        return throwVMError(exec, createTypeError(exec, "QtMetaMethod.disconnect: target is not a function"));
                }
            } else if (exec->argumentCount() >= 2) {
                if (exec->argument(0).isObject()) {
                    thisObject = exec->argument(0).toObject(exec);

                    // Get the actual function to call
                    JSObject *asObj = exec->argument(1).toObject(exec);
                    CallData callData;
                    if (asObj->getCallData(callData) != CallTypeNone) {
                        // Function version
                        funcObject = asObj;
                    } else {
                        // Convert it to a string
                        UString funcName = exec->argument(1).toString(exec);
                        Identifier funcIdent(exec, funcName);

                        // ### DropAllLocks
                        // This is resolved at this point in QtScript
                        JSValue val = thisObject->get(exec, funcIdent);
                        JSObject* asFuncObj = val.toObject(exec);

                        if (asFuncObj->getCallData(callData) != CallTypeNone) {
                            funcObject = asFuncObj;
                        } else {
                            if (d->m_isConnect)
                                return throwVMError(exec, createTypeError(exec, "QtMetaMethod.connect: target is not a function"));
                            else
                                return throwVMError(exec, createTypeError(exec, "QtMetaMethod.disconnect: target is not a function"));
                        }
                    }
                } else {
                    if (d->m_isConnect)
                        return throwVMError(exec, createTypeError(exec, "QtMetaMethod.connect: thisObject is not an object"));
                    else
                        return throwVMError(exec, createTypeError(exec, "QtMetaMethod.disconnect: thisObject is not an object"));
                }
            } else {
                if (d->m_isConnect)
                    return throwVMError(exec, createError(exec, "QtMetaMethod.connect: no arguments given"));
                else
                    return throwVMError(exec, createError(exec, "QtMetaMethod.disconnect: no arguments given"));
            }

            if (d->m_isConnect) {
                // to connect, we need:
                //  target object [from ctor]
                //  target signal index etc. [from ctor]
                //  receiver function [from arguments]
                //  receiver this object [from arguments]

                QtConnectionObject* conn = new QtConnectionObject(exec->globalData(), d->m_instance, signalIndex, thisObject, funcObject);
                bool ok = QMetaObject::connect(sender, signalIndex, conn, conn->metaObject()->methodOffset());
                if (!ok) {
                    delete conn;
                    QString msg = QString(QLatin1String("QtMetaMethod.connect: failed to connect to %1::%2()"))
                            .arg(QLatin1String(sender->metaObject()->className()))
                            .arg(QLatin1String(d->m_signature));
                    return throwVMError(exec, createError(exec, msg.toLatin1().constData()));
                }
                else {
                    // Store connection
                    connections.insert(sender, conn);
                }
            } else {
                // Now to find our previous connection object. Hmm.
                QList<QtConnectionObject*> conns = connections.values(sender);
                bool ret = false;

                foreach(QtConnectionObject* conn, conns) {
                    // Is this the right connection?
                    if (conn->match(sender, signalIndex, thisObject, funcObject)) {
                        // Yep, disconnect it
                        QMetaObject::disconnect(sender, signalIndex, conn, conn->metaObject()->methodOffset());
                        delete conn; // this will also remove it from the map
                        ret = true;
                        break;
                    }
                }

                if (!ret) {
                    QString msg = QString(QLatin1String("QtMetaMethod.disconnect: failed to disconnect from %1::%2()"))
                            .arg(QLatin1String(sender->metaObject()->className()))
                            .arg(QLatin1String(d->m_signature));
                    return throwVMError(exec, createError(exec, msg.toLatin1().constData()));
                }
            }
        } else {
            QString msg = QString(QLatin1String("QtMetaMethod.%1: %2::%3() is not a signal"))
                    .arg(QLatin1String(d->m_isConnect ? "connect": "disconnect"))
                    .arg(QLatin1String(sender->metaObject()->className()))
                    .arg(QLatin1String(d->m_signature));
            return throwVMError(exec, createTypeError(exec, msg.toLatin1().constData()));
        }
    } else {
        return throwVMError(exec, createError(exec, "cannot call function of deleted QObject"));
    }

    return JSValue::encode(jsUndefined());
}

CallType QtRuntimeConnectionMethod::getCallData(CallData& callData)
{
    callData.native.function = call;
    return CallTypeHost;
}

bool QtRuntimeConnectionMethod::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (propertyName == exec->propertyNames().length) {
        slot.setCustom(this, lengthGetter);
        return true;
    }

    return QtRuntimeMethod::getOwnPropertySlot(exec, propertyName, slot);
}

bool QtRuntimeConnectionMethod::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == exec->propertyNames().length) {
        PropertySlot slot;
        slot.setCustom(this, lengthGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), DontDelete | ReadOnly | DontEnum);
        return true;
    }

    return QtRuntimeMethod::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void QtRuntimeConnectionMethod::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    if (mode == IncludeDontEnumProperties)
        propertyNames.add(exec->propertyNames().length);

    QtRuntimeMethod::getOwnPropertyNames(exec, propertyNames, mode);
}

JSValue QtRuntimeConnectionMethod::lengthGetter(ExecState*, JSValue, const Identifier&)
{
    // we have one formal argument, and one optional
    return jsNumber(1);
}

// ===============

QtConnectionObject::QtConnectionObject(JSGlobalData& globalData, PassRefPtr<QtInstance> instance, int signalIndex, JSObject* thisObject, JSObject* funcObject)
    : m_instance(instance)
    , m_signalIndex(signalIndex)
    , m_originalObject(m_instance->getObject())
    , m_thisObject(globalData, thisObject)
    , m_funcObject(globalData, funcObject)
{
    setParent(m_originalObject);
    ASSERT(JSLock::currentThreadIsHoldingLock()); // so our ProtectedPtrs are safe
}

QtConnectionObject::~QtConnectionObject()
{
    // Remove us from the map of active connections
    QtRuntimeConnectionMethod::connections.remove(m_originalObject, this);
}

static const uint qt_meta_data_QtConnectionObject[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      28,   27,   27,   27, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QtConnectionObject[] = {
    "JSC::Bindings::QtConnectionObject\0\0execute()\0"
};

const QMetaObject QtConnectionObject::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QtConnectionObject,
      qt_meta_data_QtConnectionObject, 0 }
};

const QMetaObject *QtConnectionObject::metaObject() const
{
    return &staticMetaObject;
}

void *QtConnectionObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtConnectionObject))
        return static_cast<void*>(const_cast<QtConnectionObject*>(this));
    return QObject::qt_metacast(_clname);
}

int QtConnectionObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: execute(_a); break;
        }
        _id -= 1;
    }
    return _id;
}

void QtConnectionObject::execute(void **argv)
{
    QObject* obj = m_instance->getObject();
    if (obj) {
        const QMetaObject* meta = obj->metaObject();
        const QMetaMethod method = meta->method(m_signalIndex);

        QList<QByteArray> parameterTypes = method.parameterTypes();

        int argc = parameterTypes.count();

        JSLock lock(SilenceAssertionsOnly);

        // ### Should the Interpreter/ExecState come from somewhere else?
        RefPtr<RootObject> ro = m_instance->rootObject();
        if (ro) {
            JSGlobalObject* globalobj = ro->globalObject();
            if (globalobj) {
                ExecState* exec = globalobj->globalExec();
                if (exec) {
                    // Build the argument list (up to the formal argument length of the slot)
                    MarkedArgumentBuffer l;
                    // ### DropAllLocks?
                    int funcArgC = m_funcObject->get(exec, exec->propertyNames().length).toInt32(exec);
                    int argTotal = qMax(funcArgC, argc);
                    for(int i=0; i < argTotal; i++) {
                        if (i < argc) {
                            int argType = QMetaType::type(parameterTypes.at(i));
                            l.append(convertQVariantToValue(exec, ro, QVariant(argType, argv[i+1])));
                        } else {
                            l.append(jsUndefined());
                        }
                    }
                    // Stuff in the __qt_sender property, if we can
                    ScopeChainNode* oldsc = 0;
                    JSFunction* fimp = 0;
                    if (m_funcObject->inherits(&JSFunction::s_info)) {
                        fimp = static_cast<JSFunction*>(m_funcObject.get());

                        JSObject* qt_sender = QtInstance::getQtInstance(sender(), ro, QScriptEngine::QtOwnership)->createRuntimeObject(exec);
                        JSObject* wrapper = constructEmptyObject(exec, createEmptyObjectStructure(exec->globalData(), jsNull()));
                        PutPropertySlot slot;
                        wrapper->put(exec, Identifier(exec, "__qt_sender__"), qt_sender, slot);
                        oldsc = fimp->scope();
                        fimp->setScope(exec->globalData(), oldsc->push(wrapper));
                    }

                    CallData callData;
                    CallType callType = m_funcObject->getCallData(callData);
                    call(exec, m_funcObject.get(), callType, callData, m_thisObject.get(), l);

                    if (fimp)
                        fimp->setScope(exec->globalData(), oldsc);
                }
            }
        }
    } else {
        // A strange place to be - a deleted object emitted a signal here.
        qWarning() << "sender deleted, cannot deliver signal";
    }
}

bool QtConnectionObject::match(QObject* sender, int signalIndex, JSObject* thisObject, JSObject *funcObject)
{
    if (m_originalObject == sender && m_signalIndex == signalIndex
        && thisObject == (JSObject*)m_thisObject.get() && funcObject == (JSObject*)m_funcObject.get())
        return true;
    return false;
}

// ===============

template <typename T> QtArray<T>::QtArray(QList<T> list, QMetaType::Type type, PassRefPtr<RootObject> rootObject)
    : Array(rootObject)
    , m_list(list)
    , m_type(type)
{
    m_length = m_list.count();
}

template <typename T> QtArray<T>::~QtArray ()
{
}

template <typename T> RootObject* QtArray<T>::rootObject() const
{
    return m_rootObject && m_rootObject->isValid() ? m_rootObject.get() : 0;
}

template <typename T> void QtArray<T>::setValueAt(ExecState* exec, unsigned index, JSValue aValue) const
{
    // QtScript sets the value, but doesn't forward it to the original source
    // (e.g. if you do 'object.intList[5] = 6', the object is not updated, but the
    // copy of the list is).
    int dist = -1;
    QVariant val = convertValueToQVariant(exec, aValue, m_type, &dist);

    if (dist >= 0) {
        m_list[index] = val.value<T>();
    }
}


template <typename T> JSValue QtArray<T>::valueAt(ExecState *exec, unsigned int index) const
{
    if (index < m_length) {
        T val = m_list.at(index);
        return convertQVariantToValue(exec, rootObject(), QVariant::fromValue(val));
    }

    return jsUndefined();
}

// ===============

} }
