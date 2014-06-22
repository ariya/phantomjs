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

#include "APICast.h"
#include "APIShims.h"
#include "BooleanObject.h"
#include "DateInstance.h"
#include "DatePrototype.h"
#include "FunctionPrototype.h"
#include "Interpreter.h"
#include "JSArray.h"
#include "JSContextRefPrivate.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "JSDocument.h"
#include "JSGlobalObject.h"
#include "JSHTMLElement.h"
#include "JSLock.h"
#include "JSObject.h"
#include "JSRetainPtr.h"
#include "JSUint8ClampedArray.h"
#include "ObjectPrototype.h"
#include "PropertyNameArray.h"
#include "qdatetime.h"
#include "qdebug.h"
#include "qmetaobject.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qstringlist.h"
#include "qt_instance.h"
#include "qt_pixmapruntime.h"
#include "qvarlengtharray.h"
#include <JSFunction.h>

#include <wtf/DateMath.h>

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
    RTString,
    Date,
    Array,
    QObj,
    Object,
    Null,
    RTUint8Array
} JSRealType;

#if defined(QTWK_RUNTIME_CONVERSION_DEBUG) || defined(QTWK_RUNTIME_MATCH_DEBUG)
QDebug operator<<(QDebug dbg, const JSRealType &c)
{
     const char *map[] = { "Variant", "Number", "Boolean", "RTString", "Date",
         "Array", "RTObject", "Object", "Null"};

     dbg.nospace() << "JSType(" << ((int)c) << ", " <<  map[c] << ")";

     return dbg.space();
}
#endif

void setException(JSContextRef context, JSValueRef* exception, const QString& text)
{
    if (!exception)
        return;

    JSStringRef errorStr = JSStringCreateWithUTF8CString(text.toUtf8());
    JSValueRef errorVal[] = { JSValueMakeString(context, errorStr) };
    *exception = JSObjectMakeError(context, 1, errorVal, 0);
    JSStringRelease(errorStr);
}

struct RuntimeConversion {
    ConvertToJSValueFunction toJSValueFunc;
    ConvertToVariantFunction toVariantFunc;
};

typedef QHash<int, RuntimeConversion> RuntimeConversionTable;
Q_GLOBAL_STATIC(RuntimeConversionTable, customRuntimeConversions)

void registerCustomType(int qtMetaTypeId, ConvertToVariantFunction toVariantFunc, ConvertToJSValueFunction toJSValueFunc)
{
    RuntimeConversion conversion;
    conversion.toJSValueFunc = toJSValueFunc;
    conversion.toVariantFunc = toVariantFunc;
    customRuntimeConversions()->insert(qtMetaTypeId, conversion);
}

static bool isJSUint8Array(JSObjectRef object)
{
    return toJS(object)->inherits(&JSUint8Array::s_info);
}

static bool isJSArray(JSObjectRef object)
{
    return toJS(object)->inherits(&JSArray::s_info);
}

static bool isJSDate(JSObjectRef object)
{
    return toJS(object)->inherits(&DateInstance::s_info);
}

static bool isQtObject(JSObjectRef object)
{
    return toJS(object)->inherits(&RuntimeObject::s_info);
}

static JSRealType valueRealType(JSContextRef context, JSValueRef value, JSValueRef* exception)
{
    if (JSValueIsNumber(context, value))
        return Number;
    if (JSValueIsString(context, value))
        return RTString;
    if (JSValueIsBoolean(context, value))
        return Boolean;
    if (JSValueIsNull(context, value))
        return Null;
    if (!JSValueIsObject(context, value))
        return RTString; // I don't know.

    JSObjectRef object = JSValueToObject(context, value, exception);

    if (isJSUint8Array(object))
        return RTUint8Array;
    if (isJSArray(object))
            return Array;
    if (isJSDate(object))
            return Date;
    if (isQtObject(object))
            return QObj;

    return Object;
}

static QString toString(JSStringRef stringRef)
{
    return QString(reinterpret_cast<const QChar*>(JSStringGetCharactersPtr(stringRef)), JSStringGetLength(stringRef));
}

static JSValueRef unwrapBoxedPrimitive(JSContextRef context, JSValueRef value, JSObjectRef obj)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    JSObject* object = toJS(obj);
    if (object->inherits(&NumberObject::s_info))
        return toRef(exec, jsNumber(object->toNumber(exec)));
    if (object->inherits(&StringObject::s_info))
        return toRef(exec, object->toString(exec));
    if (object->inherits(&BooleanObject::s_info))
        return toRef(exec, object->toPrimitive(exec));
    return value;
}

QVariant convertValueToQVariant(JSContextRef, JSValueRef, QMetaType::Type, int*, HashSet<JSObjectRef>*, int, JSValueRef *exception);

static QVariantMap convertValueToQVariantMap(JSContextRef context, JSObjectRef object, HashSet<JSObjectRef>* visitedObjects, int recursionLimit, JSValueRef* exception)
{
    QVariantMap result;
    JSPropertyNameArrayRef properties = JSObjectCopyPropertyNames(context, object);
    size_t propertyCount = JSPropertyNameArrayGetCount(properties);

    for (size_t i = 0; i < propertyCount; ++i) {
        JSStringRef name = JSPropertyNameArrayGetNameAtIndex(properties, i);

        int propertyConversionDistance = 0;
        JSValueRef property = JSObjectGetProperty(context, object, name, exception);
        QVariant v = convertValueToQVariant(context, property, QMetaType::Void, &propertyConversionDistance, visitedObjects, recursionLimit, exception);
        if (exception && *exception)
            *exception = 0;
        else if (propertyConversionDistance >= 0) {
            result.insert(toString(name), v);
        }
    }
    JSPropertyNameArrayRelease(properties);
    return result;
}

template <typename ItemType>
QList<ItemType> convertToList(JSContextRef context, JSRealType type, JSObjectRef object,
                              JSValueRef value, int* distance, HashSet<JSObjectRef>* visitedObjects, int recursionLimit, JSValueRef* exception,
                              const QMetaType::Type typeId = static_cast<QMetaType::Type>(qMetaTypeId<ItemType>()))
{
    QList<ItemType> list;
    if (type == Array) {
        static JSStringRef lengthStr = JSStringCreateWithUTF8CString("length");
        JSValueRef lengthVal = JSObjectGetProperty(context, object, lengthStr, exception);
        size_t length = JSValueToNumber(context, lengthVal, exception);
        list.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            JSValueRef value = JSObjectGetPropertyAtIndex(context, object, i, exception);
            int itemDistance = -1;
            QVariant variant = convertValueToQVariant(context, value, typeId, &itemDistance, visitedObjects, recursionLimit, exception);
            if (itemDistance >= 0)
                list << variant.value<ItemType>();
            else
                break;
        }
        if (list.count() != length)
            list.clear();
        else if (distance)
            *distance = 5;
    } else {
        int itemDistance = -1;
        QVariant variant = convertValueToQVariant(context, value, typeId, &itemDistance, visitedObjects, recursionLimit, exception);
        if (itemDistance >= 0) {
            list << variant.value<ItemType>();
            if (distance)
                *distance = 10;
        }
    }
    return list;
}

static QString toQString(JSContextRef context, JSValueRef value)
{
    JSRetainPtr<JSStringRef> string(Adopt, JSValueToStringCopy(context, value, 0));
    if (!string)
        return QString();
    return QString(reinterpret_cast<const QChar*>(JSStringGetCharactersPtr(string.get())), JSStringGetLength(string.get()));
}

static void getGregorianDateTimeUTC(JSContextRef context, JSRealType type, JSValueRef value, JSObjectRef object, JSValueRef* exception, GregorianDateTime* gdt)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    if (type == Date) {
        JSObject* jsObject = toJS(object);
        DateInstance* date = asDateInstance(jsObject);
        gdt->copyFrom(*date->gregorianDateTimeUTC(exec));
    } else {
        double ms = JSValueToNumber(context, value, exception);
        GregorianDateTime convertedGdt;
        msToGregorianDateTime(exec, ms, /*utc*/ true, convertedGdt);
        gdt->copyFrom(convertedGdt);
    }
}

static QDateTime toQDateTimeUTC(JSContextRef context, JSRealType type, JSValueRef value, JSObjectRef object, JSValueRef* exception)
{
    GregorianDateTime gdt;
    getGregorianDateTimeUTC(context, type, value, object, exception, &gdt);
    QDate date(gdt.year(), gdt.month() + 1, gdt.monthDay());
    QTime time(gdt.hour(), gdt.minute(), gdt.second());
    return QDateTime(date, time, Qt::UTC);
}

QVariant convertValueToQVariant(JSContextRef context, JSValueRef value, QMetaType::Type hint, int *distance, HashSet<JSObjectRef>* visitedObjects, int recursionLimit, JSValueRef* exception)
{
    --recursionLimit;

    if (!value || !recursionLimit)
        return QVariant();

    JSObjectRef object = 0;
    if (JSValueIsObject(context, value)) {
        object = JSValueToObject(context, value, 0);
        if (visitedObjects->contains(object))
            return QVariant();

        visitedObjects->add(object);

        value = unwrapBoxedPrimitive(context, value, object);
    }

    // check magic pointer values before dereferencing value
    if (JSValueIsNumber(context, value)
        && std::isnan(JSValueToNumber(context, value, exception))) {
        if (distance)
            *distance = -1;
        return QVariant();
    }

    if (JSValueIsUndefined(context, value) && hint != QMetaType::QString && hint != (QMetaType::Type) qMetaTypeId<QVariant>()) {
        if (distance)
            *distance = -1;
        return QVariant();
    }

    JSRealType type = valueRealType(context, value, exception);
    if (hint == QMetaType::Void) {
        switch(type) {
            case Number:
                hint = QMetaType::Double;
                break;
            case Boolean:
                hint = QMetaType::Bool;
                break;
            case RTString:
            default:
                hint = QMetaType::QString;
                break;
            case Date:
                hint = QMetaType::QDateTime;
                break;
            case Object:
                hint = QMetaType::QVariantMap;
                break;
            case QObj:
                hint = QMetaType::QObjectStar;
                break;
            case RTUint8Array:
                hint = QMetaType::QByteArray;
                break;
            case Array:
                hint = QMetaType::QVariantList;
                break;
        }
    }

    qConvDebug() << "convertValueToQVariant: jstype is " << type << ", hint is" << hint;

    if (JSValueIsNull(context, value)
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
            ret = QVariant(JSValueToBoolean(context, value));
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
            ret = QVariant(JSValueToNumber(context, value, 0));
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
                ret = QVariant(QChar((ushort)JSValueToNumber(context, value, 0)));
                if (type == Boolean)
                    dist = 3;
                else
                    dist = 6;
            } else {
                JSRetainPtr<JSStringRef> str(Adopt, JSValueToStringCopy(context, value, exception));
                QChar ch;
                if (str && JSStringGetLength(str.get()) > 0)
                    ch = *reinterpret_cast<const QChar*>(JSStringGetCharactersPtr(str.get()));
                ret = QVariant(ch);
                if (type == RTString)
                    dist = 3;
                else
                    dist = 10;
            }
            break;

        case QMetaType::QString: {
            if (JSValueIsNull(context, value) || JSValueIsUndefined(context, value)) {
                if (distance)
                    *distance = 1;
                return QString();
            }
            JSRetainPtr<JSStringRef> str(Adopt, JSValueToStringCopy(context, value, exception));
            if (str) {
                QString string(reinterpret_cast<const QChar*>(JSStringGetCharactersPtr(str.get())), JSStringGetLength(str.get()));
                ret = QVariant(string);
                if (type == RTString)
                    dist = 0;
                else
                    dist = 10;
            }
            break;
        }

        case QMetaType::QVariantMap:
            if (type == Object || type == Array) {
                ret = QVariant(convertValueToQVariantMap(context, object, visitedObjects, recursionLimit, exception));
                // Those types can still have perfect matches, e.g. 'bool' if value is a Boolean Object.
                dist = 1;
            }
            break;

        case QMetaType::QVariantList:
            ret = QVariant(convertToList<QVariant>(context, type, object, value, &dist, visitedObjects, recursionLimit, exception, QMetaType::Void));
            break;

        case QMetaType::QStringList: {
            ret = QVariant(convertToList<QString>(context, type, object, value, &dist, visitedObjects, recursionLimit, exception));
            break;
        }

        case QMetaType::QByteArray: {
            if (type == RTUint8Array) {
                WTF::Uint8Array* arr = toUint8Array(toJS(toJS(context), value));
                ret = QVariant(QByteArray(reinterpret_cast<const char*>(arr->data()), arr->length()));
                dist = 0;
            } else {
                ret = QVariant(toQString(context, value).toLatin1());
                if (type == RTString)
                    dist = 5;
                else
                    dist = 10;
            }
            break;
        }

        case QMetaType::QDateTime:
        case QMetaType::QDate:
        case QMetaType::QTime:
            if (type == Date || type == Number) {
                QDateTime dt = toQDateTimeUTC(context, type, value, object, exception);
                const bool isNumber = (type == Number);
                if (hint == QMetaType::QDateTime) {
                    ret = dt;
                    dist = isNumber ? 6 : 0;
                } else if (hint == QMetaType::QDate) {
                    ret = dt.date();
                    dist = isNumber ? 8 : 1;
                } else {
                    ret = dt.time();
                    dist = isNumber ? 10 : 2;
                }
            } else if (type == RTString) {
                QString qstring = toQString(context, value);
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
            }
            break;

        case QMetaType::QObjectStar:
            if (type == QObj) {
                QtInstance* qtinst = QtInstance::getInstance(toJS(object));
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
                QtInstance* qtinst = QtInstance::getInstance(toJS(object));
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
                qConvDebug() << "got number for void * - not converting, seems unsafe:" << JSValueToNumber(context, value, 0);
            } else {
                qConvDebug() << "void* - unhandled type" << type;
            }
            break;

        default:
            // Non const type ids
            if (hint == (QMetaType::Type) qMetaTypeId<QObjectList>()) {
                ret = QVariant::fromValue(convertToList<QObject*>(context, type, object, value, &dist, visitedObjects, recursionLimit, exception));
                break;
            }
            if (hint == (QMetaType::Type) qMetaTypeId<QList<int> >()) {
                ret = QVariant::fromValue(convertToList<int>(context, type, object, value, &dist, visitedObjects, recursionLimit, exception));
                break;
            }
            if (QtPixmapRuntime::canHandle(static_cast<QMetaType::Type>(hint))) {
                ret = QtPixmapRuntime::toQt(context, object, static_cast<QMetaType::Type>(hint), exception);
            } else if (customRuntimeConversions()->contains(hint)) {
                ret = customRuntimeConversions()->value(hint).toVariantFunc(toJS(object), &dist, visitedObjects);
                if (dist == 0)
                    break;
            } else if (hint == (QMetaType::Type) qMetaTypeId<QVariant>()) {
                if (JSValueIsNull(context, value) || JSValueIsUndefined(context, value)) {
                    if (distance)
                        *distance = 1;
                    return QVariant();
                }
                if (type == Object) {
                    // Since we haven't really visited this object yet, we remove it
                    visitedObjects->remove(object);
                }

                // And then recurse with the autodetect flag
                ret = convertValueToQVariant(context, value, QMetaType::Void, distance, visitedObjects, recursionLimit, exception);
                dist = 10;
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

QVariant convertValueToQVariant(JSContextRef context, JSValueRef value, QMetaType::Type hint, int *distance, JSValueRef *exception)
{
    const int recursionLimit = 200;
    HashSet<JSObjectRef> visitedObjects;
    return convertValueToQVariant(context, value, hint, distance, &visitedObjects, recursionLimit, exception);
}

JSValueRef convertQVariantToValue(JSContextRef context, PassRefPtr<RootObject> root, const QVariant& variant, JSValueRef *exception)
{
    // Variants with QObject * can be isNull but not a null pointer
    // An empty QString variant is also null
    QMetaType::Type type = (QMetaType::Type) variant.userType();

    qConvDebug() << "convertQVariantToValue: metatype:" << type << ", isnull: " << variant.isNull();
    if (variant.isNull() &&
        !QMetaType::typeFlags(type).testFlag(QMetaType::PointerToQObject) &&
        type != QMetaType::VoidStar &&
        type != QMetaType::QString) {
        return JSValueMakeNull(context);
    }

    if (type == QMetaType::Bool)
        return JSValueMakeBoolean(context, variant.toBool());

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
        return JSValueMakeNumber(context, variant.toDouble());

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
        const JSValueRef arguments[] = {
            JSValueMakeNumber(context, date.year()),
            JSValueMakeNumber(context, date.month() - 1),
            JSValueMakeNumber(context, date.day()),
            JSValueMakeNumber(context, time.hour()),
            JSValueMakeNumber(context, time.minute()),
            JSValueMakeNumber(context, time.second()),
            JSValueMakeNumber(context, time.msec())
        };
        return JSObjectMakeDate(context, 7, arguments, exception);
    }

    if (type == QMetaType::QByteArray) {
        QByteArray qtByteArray = variant.value<QByteArray>();
        WTF::RefPtr<WTF::Uint8ClampedArray> wtfByteArray = WTF::Uint8ClampedArray::createUninitialized(qtByteArray.length());
        memcpy(wtfByteArray->data(), qtByteArray.constData(), qtByteArray.length());
        ExecState* exec = toJS(context);
        APIEntryShim entryShim(exec);
        return toRef(exec, toJS(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), wtfByteArray.get()));
    }

    if (QMetaType::typeFlags(type).testFlag(QMetaType::PointerToQObject)) {
        QObject* obj = variant.value<QObject*>();
        if (!obj)
            return JSValueMakeNull(context);
        ExecState* exec = toJS(context);
        APIEntryShim entryShim(exec);
        return toRef(exec, QtInstance::getQtInstance(obj, root.get(), QtInstance::QtOwnership)->createRuntimeObject(exec));
    }

    if (QtPixmapRuntime::canHandle(static_cast<QMetaType::Type>(variant.type())))
        return QtPixmapRuntime::toJS(context, variant, exception);

    if (customRuntimeConversions()->contains(type)) {
        if (!root->globalObject()->inherits(&JSDOMWindow::s_info))
            return JSValueMakeUndefined(context);

        Document* document = (static_cast<JSDOMWindow*>(root->globalObject()))->impl()->document();
        if (!document)
            return JSValueMakeUndefined(context);
        ExecState* exec = toJS(context);
        APIEntryShim entryShim(exec);
        return toRef(exec, customRuntimeConversions()->value(type).toJSValueFunc(exec, toJSDOMGlobalObject(document, exec), variant));
    }

    if (type == QMetaType::QVariantMap) {
        // create a new object, and stuff properties into it
        JSObjectRef ret = JSObjectMake(context, 0, 0);
        QVariantMap map = variant.value<QVariantMap>();
        QVariantMap::const_iterator i = map.constBegin();
        while (i != map.constEnd()) {
            QString s = i.key();
            JSStringRef propertyName = JSStringCreateWithCharacters(reinterpret_cast<const JSChar*>(s.constData()), s.length());
            JSValueRef propertyValue = convertQVariantToValue(context, root.get(), i.value(), /*ignored exception*/0);
            if (propertyValue)
                JSObjectSetProperty(context, ret, propertyName, propertyValue, kJSPropertyAttributeNone, /*ignored exception*/0);
            JSStringRelease(propertyName);
            ++i;
        }

        return ret;
    }

    // List types
    if (type == QMetaType::QVariantList) {
        // ### TODO: Could use special array class that lazily converts.
        // See https://bugs.webkit.org/show_bug.cgi?id=94691
        QVariantList vl = variant.toList();
        JSObjectRef array = JSObjectMakeArray(context, 0, 0, exception);
        if (exception && *exception)
            return array;
        for (int i = 0; i < vl.count(); ++i) {
            JSValueRef property = convertQVariantToValue(context, root.get(), vl.at(i), /*ignored exception*/0);
            if (property)
                JSObjectSetPropertyAtIndex(context, array, i, property, /*ignored exception*/0);
        }
        return array;
    } else if (type == QMetaType::QStringList) {
        QStringList sl = variant.value<QStringList>();
        JSObjectRef array = JSObjectMakeArray(context, 0, 0, exception);
        for (int i = 0; i < sl.count(); ++i) {
            const QString& s = sl.at(i);
            JSStringRef jsString = JSStringCreateWithCharacters(reinterpret_cast<const JSChar*>(s.constData()), s.length());
            JSObjectSetPropertyAtIndex(context, array, i, JSValueMakeString(context, jsString), /*ignored exception*/0);
            JSStringRelease(jsString);
        }
        return array;
    } else if (type == static_cast<QMetaType::Type>(qMetaTypeId<QObjectList>())) {
        QObjectList ol = variant.value<QObjectList>();
        JSObjectRef array = JSObjectMakeArray(context, 0, 0, exception);
        RefPtr<RootObject> rootRef(root); // We need a real reference, since PassRefPtr may only be passed on to one call.
        ExecState* exec = toJS(context);
        APIEntryShim entryShim(exec);
        for (int i = 0; i < ol.count(); ++i) {
            JSValueRef jsObject = toRef(exec, QtInstance::getQtInstance(ol.at(i), rootRef, QtInstance::QtOwnership)->createRuntimeObject(exec));
            JSObjectSetPropertyAtIndex(context, array, i, jsObject, /*ignored exception*/0);
        }
        return array;
    } else if (type == static_cast<QMetaType::Type>(qMetaTypeId<QList<int> >())) {
        QList<int> il = variant.value<QList<int> >();
        JSObjectRef array = JSObjectMakeArray(context, 0, 0, exception);
        for (int i = 0; i < il.count(); ++i)
            JSObjectSetPropertyAtIndex(context, array, i, JSValueMakeNumber(context, il.at(i)), /*ignored exception*/0);
        return array;
    }

    if (type == (QMetaType::Type)qMetaTypeId<QVariant>()) {
        QVariant real = variant.value<QVariant>();
        qConvDebug() << "real variant is:" << real;
        return convertQVariantToValue(context, root.get(), real, exception);
    }

    qConvDebug() << "fallback path for" << variant << variant.userType();

    QString string = variant.toString();
    JSStringRef jsstring = JSStringCreateWithCharacters(reinterpret_cast<const JSChar*>(string.constData()), string.length());
    JSValueRef value = JSValueMakeString(context, jsstring);
    JSStringRelease(jsstring);
    return value;
}

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
        return (QMetaType::Type) qMetaTypeId<QVariant>();
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
static int findMethodIndex(JSContextRef context,
                           const QMetaObject* meta,
                           const QByteArray& signature,
                           int argumentCount,
                           const JSValueRef arguments[],
                           bool allowPrivate,
                           QVarLengthArray<QVariant, 10> &vars,
                           void** vvars,
                           JSValueRef* exception)
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
        if (!overloads && m.methodSignature() == signature)
            matchingIndices.append(i);
        else if (overloads && m.name() == signature)
            matchingIndices.append(i);
    }

    int chosenIndex = -1;
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
        int rtype = method.returnType();
        if (rtype == QMetaType::UnknownType) {
            if (returnTypeName.endsWith('*')) {
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
            if (rtype == QMetaType::QVariant)
                types.append(QtMethodMatchType::variant());
            else
                types.append(QtMethodMatchType::metaType(rtype, returnTypeName));
        }

        // resolve argument types
        QList<QByteArray> parameterTypeNames = method.parameterTypes();
        for (int i = 0; i < parameterTypeNames.count(); ++i) {
            QByteArray argTypeName = parameterTypeNames.at(i);
            int atype = method.parameterType(i);
            if (atype == QMetaType::UnknownType) {
                int enumIndex = indexOfMetaEnum(meta, argTypeName);
                if (enumIndex != -1)
                    types.append(QtMethodMatchType::metaEnum(enumIndex, argTypeName));
                else {
                    unresolvedTypes = true;
                    types.append(QtMethodMatchType::unresolved(argTypeName));
                }
            } else {
                if (atype == QMetaType::QVariant)
                    types.append(QtMethodMatchType::variant());
                else
                    types.append(QtMethodMatchType::metaType(atype, argTypeName));
            }
        }

        // If the native method requires more arguments than what was passed from JavaScript
        if (argumentCount + 1 < static_cast<unsigned>(types.count())) {
            qMatchDebug() << "Match:too few args for" << method.methodSignature();
            tooFewArgs.append(index);
            continue;
        }

        if (unresolvedTypes) {
            qMatchDebug() << "Match:unresolved arg types for" << method.methodSignature();
            // remember it so we can give an error message later, if necessary
            unresolved.append(QtMethodMatchData(/*matchDistance=*/INT_MAX, index,
                                                   types, QVarLengthArray<QVariant, 10>()));
            continue;
        }

        // Now convert arguments
        if (args.count() != types.count())
            args.resize(types.count());

        QtMethodMatchType retType = types[0];
        if (retType.typeId() != QMetaType::Void)
            args[0] = QVariant(retType.typeId(), (void *)0); // the return value

        bool converted = true;
        int matchDistance = 0;
        for (unsigned i = 0; converted && i + 1 < static_cast<unsigned>(types.count()); ++i) {
            JSValueRef arg = i < argumentCount ? arguments[i] : JSValueMakeUndefined(context);

            int argdistance = -1;
            QVariant v = convertValueToQVariant(context, arg, types.at(i+1).typeId(), &argdistance, exception);
            if (argdistance >= 0) {
                matchDistance += argdistance;
                args[i+1] = v;
            } else {
                qMatchDebug() << "failed to convert argument " << i << "type" << types.at(i+1).typeId() << QMetaType::typeName(types.at(i+1).typeId());
                converted = false;
            }
        }

        qMatchDebug() << "Match: " << method.methodSignature() << (converted ? "converted":"failed to convert") << "distance " << matchDistance;

        if (converted) {
            if ((argumentCount + 1 == static_cast<unsigned>(types.count()))
                && (matchDistance == 0)) {
                // perfect match, use this one
                chosenIndex = index;
                chosenTypes = types;
                break;
            }
            QtMethodMatchData currentMatch(matchDistance, index, types, args);
            if (candidates.isEmpty())
                candidates.append(currentMatch);
            else {
                QtMethodMatchData bestMatchSoFar = candidates.at(0);
                if ((args.count() > bestMatchSoFar.args.count())
                    || ((args.count() == bestMatchSoFar.args.count())
                    && (matchDistance <= bestMatchSoFar.matchDistance)))
                    candidates.prepend(currentMatch);
                else
                    candidates.append(currentMatch);
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
                              .arg(QString::fromLatin1(signature));
            for (int i = 0; i < conversionFailed.size(); ++i) {
                if (i > 0)
                    message += QLatin1String("\n");
                QMetaMethod mtd = meta->method(conversionFailed.at(i));
                message += QString::fromLatin1("    %0").arg(QString::fromLatin1(mtd.methodSignature()));
            }
            setException(context, exception, message);
        } else if (!unresolved.isEmpty()) {
            QtMethodMatchData argsInstance = unresolved.first();
            int unresolvedIndex = argsInstance.firstUnresolvedIndex();
            Q_ASSERT(unresolvedIndex != -1);
            QtMethodMatchType unresolvedType = argsInstance.types.at(unresolvedIndex);
            QString message = QString::fromLatin1("cannot call %0(): unknown type `%1'")
                .arg(QString::fromLatin1(signature))
                .arg(QLatin1String(unresolvedType.name()));
            setException(context, exception, message);
        } else {
            QString message = QString::fromLatin1("too few arguments in call to %0(); candidates are\n")
                              .arg(QString::fromLatin1(signature));
            for (int i = 0; i < tooFewArgs.size(); ++i) {
                if (i > 0)
                    message += QLatin1String("\n");
                QMetaMethod mtd = meta->method(tooFewArgs.at(i));
                message += QString::fromLatin1("    %0").arg(QString::fromLatin1(mtd.methodSignature()));
            }
            setException(context, exception, message);
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
                    message += QString::fromLatin1("    %0").arg(QString::fromLatin1(mtd.methodSignature()));
                }
            }
            setException(context, exception, message);
        } else {
            chosenIndex = bestMatch.index;
            chosenTypes = bestMatch.types;
            args = bestMatch.args;
        }
    }

    if (chosenIndex != -1) {
        /* Copy the stuff over */
        int i;
        vars.resize(args.count());
        for (i=0; i < args.count(); i++) {
            vars[i] = args[i];
            if (chosenTypes[i].isVariant())
                vvars[i] = &vars[i];
            else
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

static JSClassRef prototypeForSignalsAndSlots()
{
    static JSClassDefinition classDef = {
        0, kJSClassAttributeNoAutomaticPrototype, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static JSClassRef cls = JSClassCreate(&classDef);
    return cls;
}

QtRuntimeMethod::QtRuntimeMethod(JSContextRef ctx, QObject* object, const QByteArray& identifier, int index, int flags, QtInstance* instance)
    : m_object(object)
    , m_identifier(identifier)
    , m_index(index)
    , m_flags(flags)
    , m_instance(instance)
{
}

QtRuntimeMethod::~QtRuntimeMethod()
{
}

JSValueRef QtRuntimeMethod::call(JSContextRef context, JSObjectRef function, JSObjectRef /*thisObject*/, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    QtRuntimeMethod* d = toRuntimeMethod(context, function);
    if (!d) {
        setException(context, exception, QStringLiteral("cannot call function of deleted runtime method"));
        return JSValueMakeUndefined(context);
    }
    QObject* obj = d->m_object;

    if (!obj) {
        setException(context, exception, QStringLiteral("cannot call function of deleted QObject"));
        return JSValueMakeUndefined(context);
    }

    // Allow for maximum of 10 arguments and size stack arrays accordingly.
    if (argumentCount > 10)
        return JSValueMakeUndefined(context);

    QVarLengthArray<QVariant, 10> vargs;
    void* qargs[11];
    const QMetaObject* metaObject = obj->metaObject();

    int methodIndex = findMethodIndex(context, metaObject, d->m_identifier,  argumentCount, arguments,
                                      (d->m_flags & AllowPrivate), vargs, (void **)qargs, exception);

    if (QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, methodIndex, qargs) >= 0)
        return JSValueMakeUndefined(context);

    if (vargs.size() > 0 && metaObject->method(methodIndex).returnType() != QMetaType::Void)
        return convertQVariantToValue(context, d->m_instance->rootObject(), vargs[0], exception);

    return JSValueMakeUndefined(context);
}

JSValueRef QtRuntimeMethod::connect(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return connectOrDisconnect(context, function, thisObject, argumentCount, arguments, exception, true);
}

JSValueRef QtRuntimeMethod::disconnect(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return connectOrDisconnect(context, function, thisObject, argumentCount, arguments, exception, false);
}

JSObjectRef QtRuntimeMethod::jsObjectRef(JSContextRef context, JSValueRef* exception)
{
    if (m_jsObject)
        return toRef(m_jsObject.get());

    static JSStringRef connectStr = JSStringCreateWithUTF8CString("connect");
    static JSStringRef disconnectStr = JSStringCreateWithUTF8CString("disconnect");
    JSRetainPtr<JSStringRef> actualNameStr(Adopt, JSStringCreateWithUTF8CString(m_identifier.constData()));

    JSObjectRef object = JSObjectMakeFunctionWithCallback(context, actualNameStr.get(), call);

    JSObjectRef generalFunctionProto = JSValueToObject(context, JSObjectGetPrototype(context, object), 0);
    JSObjectRef runtimeMethodProto = JSObjectMake(context, prototypeForSignalsAndSlots(), this);
    JSObjectSetPrototype(context, runtimeMethodProto, generalFunctionProto);

    JSObjectSetPrototype(context, object, runtimeMethodProto);

    JSObjectRef connectFunction = JSObjectMakeFunctionWithCallback(context, connectStr, connect);
    JSObjectSetPrototype(context, connectFunction, runtimeMethodProto);

    JSObjectRef disconnectFunction = JSObjectMakeFunctionWithCallback(context, disconnectStr, disconnect);
    JSObjectSetPrototype(context, disconnectFunction, runtimeMethodProto);

    const JSPropertyAttributes attributes = kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete;
    JSObjectSetProperty(context, object, connectStr, connectFunction, attributes, exception);
    JSObjectSetProperty(context, object, disconnectStr, disconnectFunction, attributes, exception);

    m_jsObject = PassWeak<JSObject>(toJS(object));

    return object;
}

QtRuntimeMethod* QtRuntimeMethod::toRuntimeMethod(JSContextRef context, JSObjectRef object)
{
    JSObjectRef proto = JSValueToObject(context, JSObjectGetPrototype(context, object), 0);
    if (!proto)
        return 0;
    if (!JSValueIsObjectOfClass(context, proto, prototypeForSignalsAndSlots()))
        return 0;
    return static_cast<QtRuntimeMethod*>(JSObjectGetPrivate(proto));
}

JSValueRef QtRuntimeMethod::connectOrDisconnect(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception, bool connect)
{
    QtRuntimeMethod* d = toRuntimeMethod(context, thisObject);
    if (!d)
        d = toRuntimeMethod(context, function);
    if (!d) {
        QString errorStr = QStringLiteral("QtMetaMethod.%1: Cannot connect to/from deleted QObject").arg(connect ?  QStringLiteral("connect") : QStringLiteral("disconnect"));
        setException(context, exception, errorStr);
        return JSValueMakeUndefined(context);
    }

    QString functionName = connect ? QStringLiteral("connect") : QStringLiteral("disconnect");

    if (!argumentCount) {
        QString errorStr = QStringLiteral("QtMetaMethod.%1: no arguments given").arg(connect ?  QStringLiteral("connect") : QStringLiteral("disconnect"));
        setException(context, exception, errorStr);
        return JSValueMakeUndefined(context);
    }

    if ((!(d->m_flags & QtRuntimeMethod::MethodIsSignal))) {
        setException(context, exception, QStringLiteral("QtMetaMethod.%3: %1::%2() is not a signal").arg(QString::fromUtf8(d->m_object.data()->metaObject()->className())).arg(QString::fromLatin1(d->m_identifier)).arg(functionName));
        return JSValueMakeUndefined(context);
    }

    QObject* sender = d->m_object.data();

    if (!sender) {
        setException(context, exception, QStringLiteral("cannot call function of deleted QObject"));
        return JSValueMakeUndefined(context);
    }

    int signalIndex = findSignalIndex(sender->metaObject(), d->m_index, d->m_identifier);

    JSObjectRef targetObject = 0;
    JSObjectRef targetFunction = 0;

    if (argumentCount == 1) {
        if (!JSValueIsObject(context, arguments[0])) {
            setException(context, exception, QStringLiteral("QtMetaMethod.%1: target is not a function").arg(functionName));
            return JSValueMakeUndefined(context);
        }
        targetFunction = JSValueToObject(context, arguments[0], exception);

        // object.signal.connect(someFunction);
        if (JSObjectIsFunction(context, targetFunction)) {
            // object.signal.connect(otherObject.slot);
            if (QtRuntimeMethod* targetMethod = toRuntimeMethod(context, targetFunction))
                targetObject = toRef(QtInstance::getQtInstance(targetMethod->m_object.data(), d->m_instance->rootObject(), QtInstance::QtOwnership)->createRuntimeObject(toJS(context)));
        } else
            targetFunction = 0;
    } else {
        // object.signal.connect(object, someFunction);
        targetObject = JSValueToObject(context, arguments[0], exception);
        if (JSValueIsObject(context, arguments[1])) {
            JSObjectRef obj = JSValueToObject(context, arguments[1], exception);
            if (JSObjectIsFunction(context, obj))
                targetFunction = obj;
        }
        if (!targetFunction) {
            // Maybe the second argument is a string
            JSValueRef conversionException = 0;
            JSRetainPtr<JSStringRef> functionName(Adopt, JSValueToStringCopy(context, arguments[1], &conversionException));
            if (functionName && !conversionException) {
                JSValueRef functionProperty = JSObjectGetProperty(context, targetObject, functionName.get(), &conversionException);
                if (!conversionException && functionProperty && JSValueIsObject(context, functionProperty)) {
                    targetFunction = JSValueToObject(context, functionProperty, 0);
                    if (!JSObjectIsFunction(context, targetFunction))
                        targetFunction = 0;
                }
            }
        }
    }

    // object.signal.connect(someObject);
    if (!targetFunction) {
        QString message = QStringLiteral("QtMetaMethod.%1: target is not a function");
        if (connect)
            message = message.arg(QStringLiteral("connect"));
        else
            message = message.arg(QStringLiteral("disconnect"));
        setException(context, exception, message);
        return JSValueMakeUndefined(context);
    }

    if (connect) {
        // to connect, we need:
        //  target object [from ctor]
        //  target signal index etc. [from ctor]
        //  receiver function [from arguments]
        //  receiver this object [from arguments]

        QtConnectionObject* conn = new QtConnectionObject(context, QtInstance::getQtInstance(sender, d->m_instance->rootObject(), QtInstance::QtOwnership), signalIndex, targetObject, targetFunction);
        bool ok = QMetaObject::connect(sender, signalIndex, conn, conn->metaObject()->methodOffset());
        if (!ok) {
            delete conn;
            QString msg = QString(QLatin1String("QtMetaMethod.connect: failed to connect to %1::%2()"))
                    .arg(QLatin1String(sender->metaObject()->className()))
                    .arg(QLatin1String(d->m_identifier));
            setException(context, exception, msg);
            return JSValueMakeUndefined(context);
        }

        // Store connection
        QtConnectionObject::connections.insert(sender, conn);

        return JSValueMakeUndefined(context);
    }

    // Now to find our previous connection object.
    QList<QtConnectionObject*> conns = QtConnectionObject::connections.values(sender);

    foreach (QtConnectionObject* conn, conns) {
        // Is this the right connection?
        if (!conn->match(context, sender, signalIndex, targetObject, targetFunction))
            continue;

        // Yep, disconnect it
        QMetaObject::disconnect(sender, signalIndex, conn, conn->metaObject()->methodOffset());
        delete conn; // this will also remove it from the map
        return JSValueMakeUndefined(context);
    }

    QString msg = QStringLiteral("QtMetaMethod.disconnect: failed to disconnect from %1::%2()")
            .arg(QLatin1String(sender->metaObject()->className()))
            .arg(QLatin1String(d->m_identifier));

    setException(context, exception, msg);
    return JSValueMakeUndefined(context);
}

// ===============

QMultiMap<QObject*, QtConnectionObject*> QtConnectionObject::connections;

QtConnectionObject::QtConnectionObject(JSContextRef context, PassRefPtr<QtInstance> senderInstance, int signalIndex, JSObjectRef receiver, JSObjectRef receiverFunction)
    : QObject(senderInstance->getObject())
    , m_context(JSContextGetGlobalContext(context))
    , m_rootObject(senderInstance->rootObject())
    , m_signalIndex(signalIndex)
    , m_receiver(receiver)
    , m_receiverFunction(receiverFunction)
{
    if (m_receiver)
        JSValueProtect(m_context, m_receiver);
    JSValueProtect(m_context, m_receiverFunction);
}

QtConnectionObject::~QtConnectionObject()
{
    connections.remove(parent(), this);

    if (m_receiver)
        JSValueUnprotect(m_context, m_receiver);
    JSValueUnprotect(m_context, m_receiverFunction);
}

// Begin moc-generated code -- modify with care! Check "HAND EDIT" parts
struct qt_meta_stringdata_QtConnectionObject_t {
    QByteArrayData data[3];
    char stringdata[44];
};
#define QT_MOC_LITERAL(idx, ofs, len) { \
    Q_REFCOUNT_INITIALIZE_STATIC, len, 0, 0, \
    offsetof(qt_meta_stringdata_QtConnectionObject_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData) \
    }
static const qt_meta_stringdata_QtConnectionObject_t qt_meta_stringdata_QtConnectionObject = {
    {
QT_MOC_LITERAL(0, 0, 33),
QT_MOC_LITERAL(1, 34, 7),
QT_MOC_LITERAL(2, 42, 0)
    },
    "JSC::Bindings::QtConnectionObject\0"
    "execute\0\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_QtConnectionObject[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x0a,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void QtConnectionObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QtConnectionObject *_t = static_cast<QtConnectionObject *>(_o);
        switch (_id) {
        case 0: _t->execute(_a); break; // HAND EDIT: add _a parameter
        default: ;
        }
    }
}

const QMetaObject QtConnectionObject::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QtConnectionObject.data,
      qt_meta_data_QtConnectionObject, qt_static_metacall, 0, 0 }
};

const QMetaObject *QtConnectionObject::metaObject() const
{
    return &staticMetaObject;
}

void *QtConnectionObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtConnectionObject.stringdata))
        return static_cast<void*>(const_cast<QtConnectionObject*>(this));
    return QObject::qt_metacast(_clname);
}

int QtConnectionObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}
// End of moc-generated code

void QtConnectionObject::execute(void** argv)
{
    QObject* sender = parent();
    const QMetaObject* meta = sender->metaObject();
    const QMetaMethod method = meta->method(m_signalIndex);

    JSValueRef* ignoredException = 0;
    JSRetainPtr<JSStringRef> lengthProperty(Adopt, JSStringCreateWithUTF8CString("length"));
    int receiverLength = int(JSValueToNumber(m_context, JSObjectGetProperty(m_context, m_receiverFunction, lengthProperty.get(), ignoredException), ignoredException));
    int argc = qMax(method.parameterCount(), receiverLength);
    WTF::Vector<JSValueRef> args(argc);

    for (int i = 0; i < argc; i++) {
        int argType = method.parameterType(i);
        args[i] = convertQVariantToValue(m_context, m_rootObject.get(), QVariant(argType, argv[i+1]), ignoredException);
    }

    JSObjectCallAsFunction(m_context, m_receiverFunction, m_receiver, argc, args.data(), 0);
}

bool QtConnectionObject::match(JSContextRef context, QObject* sender, int signalIndex, JSObjectRef receiver, JSObjectRef receiverFunction)
{
    if (sender != parent() || signalIndex != m_signalIndex)
        return false;
    JSValueRef* ignoredException = 0;
    const bool receiverMatch = (!receiver && !m_receiver) || (receiver && m_receiver && JSValueIsEqual(context, receiver, m_receiver, ignoredException));
    return receiverMatch && JSValueIsEqual(context, receiverFunction, m_receiverFunction, ignoredException);
}

} }
