/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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
#include "qt_pixmapruntime.h"

#include "CachedImage.h"
#include "HTMLImageElement.h"
#include "JSGlobalObject.h"
#include "JSHTMLImageElement.h"
#include "JSLock.h"
#include "ObjectPrototype.h"
#include "StillImageQt.h"
#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QVariant>
#include <runtime_method.h>
#include <runtime_object.h>
#include <runtime_root.h>
#include "runtime/FunctionPrototype.h"

using namespace WebCore;
namespace JSC {

namespace Bindings {

class QtPixmapClass : public Class {
public:
    QtPixmapClass();
    virtual MethodList methodsNamed(const Identifier&, Instance*) const;
    virtual Field* fieldNamed(const Identifier&, Instance*) const;
};


class QtPixmapWidthField : public Field {
public:
    static const char* name() { return "width"; }
    virtual JSValue valueFromInstance(ExecState*, const Instance* instance) const
    {
        return jsNumber(static_cast<const QtPixmapInstance*>(instance)->width());
    }
    virtual void setValueToInstance(ExecState*, const Instance*, JSValue) const {}
};

class QtPixmapHeightField : public Field {
public:
    static const char* name() { return "height"; }
    virtual JSValue valueFromInstance(ExecState*, const Instance* instance) const
    {
        return jsNumber(static_cast<const QtPixmapInstance*>(instance)->height());
    }
    virtual void setValueToInstance(ExecState*, const Instance*, JSValue) const {}
};

class QtPixmapRuntimeMethod : public Method {
public:
    virtual int numParameters() const
    {
        return 0;
    }
    virtual JSValue invoke(ExecState* exec, QtPixmapInstance*) = 0;

};

// this function receives an HTML image element as a parameter, makes it display the pixmap/image from Qt
class QtPixmapAssignToElementMethod : public QtPixmapRuntimeMethod {
public:
    static const char* name() { return "assignToHTMLImageElement"; }
    JSValue invoke(ExecState* exec, QtPixmapInstance* instance)
    {
        if (!exec->argumentCount())
            return jsUndefined();

        JSObject* objectArg = exec->argument(0).toObject(exec);
        if (!objectArg)
            return jsUndefined();

        if (!objectArg->inherits(&JSHTMLImageElement::s_info))
            return jsUndefined();

        // we now know that we have a valid <img> element as the argument, we can attach the pixmap to it.
        PassRefPtr<StillImage> stillImage = WebCore::StillImage::create(instance->toPixmap());
        HTMLImageElement* imageElement = static_cast<HTMLImageElement*>(static_cast<JSHTMLImageElement*>(objectArg)->impl());
        imageElement->setCachedImage(new CachedImage(stillImage.get()));
        JSDOMGlobalObject* global = static_cast<JSDOMGlobalObject*>(instance->rootObject()->globalObject());
        toJS(exec, global, imageElement->document());
        return jsUndefined();
    }

    virtual int numParameters() const
    {
        return 1;
    }
};

// this function encodes the image to a dataUrl, to be used in background etc. Note: very slow.
class QtPixmapToDataUrlMethod : public QtPixmapRuntimeMethod {
public:
    static const char* name() { return "toDataUrl"; }
    JSValue invoke(ExecState* exec, QtPixmapInstance* instance)
    {
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        instance->toImage().save(&buffer, "PNG");
        const QString encodedString = QLatin1String("data:image/png;base64,") + QLatin1String(byteArray.toBase64());
        const UString ustring((UChar*)encodedString.utf16(), encodedString.length());
        return jsString(exec, ustring);
    }
};

class QtPixmapToStringMethod : public QtPixmapRuntimeMethod {
    public:
    static const char* name() { return "toString"; }
    JSValue invoke(ExecState* exec, QtPixmapInstance* instance)
    {
        return instance->valueOf(exec);
    }
};

struct QtPixmapMetaData {
    QtPixmapToDataUrlMethod toDataUrlMethod;
    QtPixmapAssignToElementMethod assignToElementMethod;
    QtPixmapToStringMethod toStringMethod;
    QtPixmapHeightField heightField;
    QtPixmapWidthField widthField;
    QtPixmapClass cls;
} qt_pixmap_metaData;

// Derived RuntimeObject
class QtPixmapRuntimeObject : public RuntimeObject {
public:
    QtPixmapRuntimeObject(ExecState*, JSGlobalObject*, PassRefPtr<Instance>);

    static const ClassInfo s_info;

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType,  StructureFlags), AnonymousSlotCount, &s_info);
    }

protected:
    static const unsigned StructureFlags = RuntimeObject::StructureFlags | OverridesVisitChildren;
};

QtPixmapRuntimeObject::QtPixmapRuntimeObject(ExecState* exec, JSGlobalObject* globalObject, PassRefPtr<Instance> instance)
    : RuntimeObject(exec, globalObject, WebCore::deprecatedGetDOMStructure<QtPixmapRuntimeObject>(exec), instance)
{
}

const ClassInfo QtPixmapRuntimeObject::s_info = { "QtPixmapRuntimeObject", &RuntimeObject::s_info, 0, 0 };

QtPixmapClass::QtPixmapClass()
{
}


Class* QtPixmapInstance::getClass() const
{
    return &qt_pixmap_metaData.cls;
}

JSValue QtPixmapInstance::getMethod(ExecState* exec, const Identifier& propertyName)
{
    MethodList methodList = getClass()->methodsNamed(propertyName, this);
    return new (exec) RuntimeMethod(exec, exec->lexicalGlobalObject(), WebCore::deprecatedGetDOMStructure<RuntimeMethod>(exec), propertyName, methodList);
}

JSValue QtPixmapInstance::invokeMethod(ExecState* exec, RuntimeMethod* runtimeMethod)
{
    const MethodList& methods = *runtimeMethod->methods();

    if (methods.size() == 1) {
        QtPixmapRuntimeMethod* method = static_cast<QtPixmapRuntimeMethod*>(methods[0]);
        return method->invoke(exec, this);
    }
    return jsUndefined();
}

MethodList QtPixmapClass::methodsNamed(const Identifier& identifier, Instance*) const
{
    MethodList methods;
    if (identifier == QtPixmapToDataUrlMethod::name())
        methods.append(&qt_pixmap_metaData.toDataUrlMethod);
    else if (identifier == QtPixmapAssignToElementMethod::name())
        methods.append(&qt_pixmap_metaData.assignToElementMethod);
    else if (identifier == QtPixmapToStringMethod::name())
        methods.append(&qt_pixmap_metaData.toStringMethod);
    return methods;
}

Field* QtPixmapClass::fieldNamed(const Identifier& identifier, Instance*) const
{
    if (identifier == QtPixmapWidthField::name())
        return &qt_pixmap_metaData.widthField;
    if (identifier == QtPixmapHeightField::name())
        return &qt_pixmap_metaData.heightField;
    return 0;
}

void QtPixmapInstance::getPropertyNames(ExecState*exec, PropertyNameArray& arr)
{
    arr.add(Identifier(exec, UString(QtPixmapToDataUrlMethod::name())));
    arr.add(Identifier(exec, UString(QtPixmapAssignToElementMethod::name())));
    arr.add(Identifier(exec, UString(QtPixmapToStringMethod::name())));
    arr.add(Identifier(exec, UString(QtPixmapWidthField::name())));
    arr.add(Identifier(exec, UString(QtPixmapHeightField::name())));
}

JSValue QtPixmapInstance::defaultValue(ExecState* exec, PreferredPrimitiveType ptype) const
{
    if (ptype == PreferNumber) {
        return jsBoolean(
                (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()) && !(data.value<QImage>()).isNull())
                || (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()) && !data.value<QPixmap>().isNull()));
    }

    if (ptype == PreferString)
        return valueOf(exec);

    return jsUndefined();
}

JSValue QtPixmapInstance::valueOf(ExecState* exec) const
{
    const QString stringValue = QString::fromLatin1("[Qt Native Pixmap %1,%2]").arg(width()).arg(height());
    UString ustring((UChar*)stringValue.utf16(), stringValue.length());
    return jsString(exec, ustring);
}

QtPixmapInstance::QtPixmapInstance(PassRefPtr<RootObject> rootObj, const QVariant& d)
        :Instance(rootObj), data(d)
{
}

int QtPixmapInstance::width() const
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()))
        return data.value<QPixmap>().width();
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()))
        return data.value<QImage>().width();
    return 0;
}

int QtPixmapInstance::height() const
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()))
        return data.value<QPixmap>().height();
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()))
        return data.value<QImage>().height();
    return 0;
}

QPixmap QtPixmapInstance::toPixmap()
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()))
        return data.value<QPixmap>();

    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>())) {
        const QPixmap pixmap = QPixmap::fromImage(data.value<QImage>());
        data = QVariant::fromValue<QPixmap>(pixmap);
        return pixmap;
    }

    return QPixmap();
}

QImage QtPixmapInstance::toImage()
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()))
        return data.value<QImage>();

    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>())) {
        const QImage image = data.value<QPixmap>().toImage();
        data = QVariant::fromValue<QImage>(image);
        return image;
    }

    return QImage();
}

QVariant QtPixmapInstance::variantFromObject(JSObject* object, QMetaType::Type hint)
{
    if (!object)
        goto returnEmptyVariant;

    if (object->inherits(&JSHTMLImageElement::s_info)) {
        JSHTMLImageElement* elementJSWrapper = static_cast<JSHTMLImageElement*>(object);
        HTMLImageElement* imageElement = static_cast<HTMLImageElement*>(elementJSWrapper->impl());

        if (!imageElement)
            goto returnEmptyVariant;

        CachedImage* cachedImage = imageElement->cachedImage();
        if (!cachedImage)
            goto returnEmptyVariant;

        Image* image = cachedImage->image();
        if (!image)
            goto returnEmptyVariant;

        QPixmap* pixmap = image->nativeImageForCurrentFrame();
        if (!pixmap)
            goto returnEmptyVariant;

        return (hint == static_cast<QMetaType::Type>(qMetaTypeId<QPixmap>()))
                  ? QVariant::fromValue<QPixmap>(*pixmap)
                  : QVariant::fromValue<QImage>(pixmap->toImage());
    }

    if (object->inherits(&QtPixmapRuntimeObject::s_info)) {
        QtPixmapRuntimeObject* runtimeObject = static_cast<QtPixmapRuntimeObject*>(object);
        QtPixmapInstance* instance = static_cast<QtPixmapInstance*>(runtimeObject->getInternalInstance());
        if (!instance)
            goto returnEmptyVariant;

        if (hint == qMetaTypeId<QPixmap>())
            return QVariant::fromValue<QPixmap>(instance->toPixmap());

        if (hint == qMetaTypeId<QImage>())
            return QVariant::fromValue<QImage>(instance->toImage());
    }

returnEmptyVariant:
    if (hint == qMetaTypeId<QPixmap>())
        return QVariant::fromValue<QPixmap>(QPixmap());
    if (hint == qMetaTypeId<QImage>())
        return QVariant::fromValue<QImage>(QImage());
    return QVariant();
}

RuntimeObject* QtPixmapInstance::newRuntimeObject(ExecState* exec)
{
    return new(exec) QtPixmapRuntimeObject(exec, exec->lexicalGlobalObject(), this);
}

JSObject* QtPixmapInstance::createPixmapRuntimeObject(ExecState* exec, PassRefPtr<RootObject> root, const QVariant& data)
{
    JSLock lock(SilenceAssertionsOnly);
    RefPtr<QtPixmapInstance> instance = adoptRef(new QtPixmapInstance(root, data));
    return instance->createRuntimeObject(exec);
}

bool QtPixmapInstance::canHandle(QMetaType::Type hint)
{
    return hint == qMetaTypeId<QImage>() || hint == qMetaTypeId<QPixmap>();
}

}

}
