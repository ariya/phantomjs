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

#include "APICast.h"
#include "APIShims.h"
#include "CachedImage.h"
#include "HTMLImageElement.h"
#include "ImageData.h"
#include "IntSize.h"
#include "JSDOMBinding.h"
#include "JSGlobalObject.h"
#include "JSHTMLImageElement.h"
#include "JSImageData.h"
#include "JSRetainPtr.h"
#include "JavaScript.h"
#include "StillImageQt.h"
#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QVariant>
#include <QtEndian>

using namespace WebCore;
namespace JSC {

namespace Bindings {

static void copyPixelsInto(const QImage& sourceImage, int width, int height, unsigned char* destPixels)
{
    QImage image(sourceImage);
    switch (image.format()) {
    case QImage::Format_RGB888:
        for (int y = 0; y < height; y++) {
            const uchar* scanLine = image.scanLine(y);
            for (int x = 0; x < width; x++) {
                *(destPixels++) = *(scanLine++);
                *(destPixels++) = *(scanLine++);
                *(destPixels++) = *(scanLine++);
                *(destPixels++) = 0xFF;
            }
        }
        break;
    default:
        image = image.convertToFormat(QImage::Format_ARGB32);
        // Fall through
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        for (int y = 0; y < height; y++) {
            const quint32* scanLine = reinterpret_cast_ptr<const quint32*>(image.scanLine(y));
            for (int x = 0; x < width; x++) {
                QRgb pixel = scanLine[x];
                qToBigEndian<quint32>((pixel << 8) | qAlpha(pixel), destPixels);
                destPixels += 4;
            }
        }
        break;
    }
}

static QPixmap toPixmap(const QVariant& data)
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()))
        return data.value<QPixmap>();

    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()))
        return QPixmap::fromImage(data.value<QImage>());

    return QPixmap();
}

static QImage toImage(const QVariant& data)
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()))
        return data.value<QImage>();

    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()))
        return data.value<QPixmap>().toImage();

    return QImage();
}

static QSize imageSizeForVariant(const QVariant& data)
{
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QPixmap>()))
        return data.value<QPixmap>().size();
    if (data.type() == static_cast<QVariant::Type>(qMetaTypeId<QImage>()))
        return data.value<QImage>().size();
    return QSize(0, 0);
}

static JSValueRef getPixmapWidth(JSContextRef context, JSObjectRef object, JSStringRef, JSValueRef*)
{
    QVariant& data = *static_cast<QVariant*>(JSObjectGetPrivate(object));
    return JSValueMakeNumber(context, imageSizeForVariant(data).width());
}

static JSValueRef getPixmapHeight(JSContextRef context, JSObjectRef object, JSStringRef, JSValueRef*)
{
    QVariant& data = *static_cast<QVariant*>(JSObjectGetPrivate(object));
    return JSValueMakeNumber(context, imageSizeForVariant(data).height());
}

static JSValueRef assignToHTMLImageElement(JSContextRef context, JSObjectRef function, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!argumentCount)
        return JSValueMakeUndefined(context);

    JSObjectRef objectArg = JSValueToObject(context, arguments[0], exception);
    if (!objectArg)
        return JSValueMakeUndefined(context);

    JSObject* jsObject = ::toJS(objectArg);

    if (!jsObject->inherits(&JSHTMLImageElement::s_info))
        return JSValueMakeUndefined(context);

    QVariant& data = *static_cast<QVariant*>(JSObjectGetPrivate(object));

    // We now know that we have a valid <img> element as the argument, we can attach the pixmap to it.
    RefPtr<StillImage> stillImage = WebCore::StillImage::create(toPixmap(data));
    HTMLImageElement* imageElement = toHTMLImageElement(static_cast<JSHTMLImageElement*>(jsObject)->impl());
    imageElement->setCachedImage(new CachedImage(stillImage.get()));
    return JSValueMakeUndefined(context);
}

static JSValueRef pixmapToImageData(JSContextRef context, JSObjectRef function, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    QVariant& data = *static_cast<QVariant*>(JSObjectGetPrivate(object));
    QImage image = toImage(data);
    int width = image.width();
    int height = image.height();

    JSC::ExecState* exec = ::toJS(context);
    APIEntryShim entryShim(exec);

    RefPtr<ImageData> imageData = ImageData::create(IntSize(width, height));
    copyPixelsInto(image, width, height, imageData->data()->data());

    JSDOMGlobalObject* globalObject = static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject());
    return ::toRef(exec, toJS(exec, globalObject, imageData.get()));
}

static JSValueRef pixmapToDataUrl(JSContextRef context, JSObjectRef function, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    QVariant& data = *static_cast<QVariant*>(JSObjectGetPrivate(object));
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    toImage(data).save(&buffer, "PNG");
    QByteArray encoded = QByteArray("data:image/png;base64,") + byteArray.toBase64();
    JSRetainPtr<JSStringRef> str(Adopt, JSStringCreateWithUTF8CString(encoded.constData()));
    JSValueRef value = JSValueMakeString(context, str.get());

    return value;
}

static JSValueRef pixmapToString(JSContextRef context, JSObjectRef function, JSObjectRef object, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    QVariant& data = *static_cast<QVariant*>(JSObjectGetPrivate(object));
    QSize size = imageSizeForVariant(data);
    QString stringValue = QString::fromLatin1("[Qt Native Pixmap %1,%2]").arg(size.width()).arg(size.height());
    JSRetainPtr<JSStringRef> str(Adopt, JSStringCreateWithUTF8CString(stringValue.toUtf8().constData()));
    JSValueRef value = JSValueMakeString(context, str.get());

    return value;
}

static void finalizePixmap(JSObjectRef object)
{
    delete static_cast<QVariant*>(JSObjectGetPrivate(object));
}

JSObjectRef QtPixmapRuntime::toJS(JSContextRef context, const QVariant& value, JSValueRef* exception)
{
    return JSObjectMake(context, getClassRef(), new QVariant(value));
}

static QVariant emptyVariantForHint(QMetaType::Type hint)
{
    if (hint == qMetaTypeId<QPixmap>())
        return QVariant::fromValue(QPixmap());
    if (hint == qMetaTypeId<QImage>())
        return QVariant::fromValue(QImage());
    return QVariant();
}

QVariant QtPixmapRuntime::toQt(JSContextRef context, JSObjectRef obj, QMetaType::Type hint, JSValueRef* exception)
{
    if (!obj)
        return emptyVariantForHint(hint);

    if (JSValueIsObjectOfClass(context, obj, QtPixmapRuntime::getClassRef())) {
        QVariant* originalVariant = static_cast<QVariant*>(JSObjectGetPrivate(obj));
        if (hint == qMetaTypeId<QPixmap>())
            return QVariant::fromValue<QPixmap>(toPixmap(*originalVariant));

        if (hint == qMetaTypeId<QImage>())
            return QVariant::fromValue<QImage>(toImage(*originalVariant));
    }

    JSObject* jsObject = ::toJS(obj);
    if (!jsObject->inherits(&JSHTMLImageElement::s_info))
        return emptyVariantForHint(hint);

    JSHTMLImageElement* elementJSWrapper = static_cast<JSHTMLImageElement*>(jsObject);
    HTMLImageElement* imageElement = toHTMLImageElement(elementJSWrapper->impl());

    if (!imageElement)
        return emptyVariantForHint(hint);

    CachedImage* cachedImage = imageElement->cachedImage();
    if (!cachedImage)
        return emptyVariantForHint(hint);

    Image* image = cachedImage->imageForRenderer(imageElement->renderer());
    if (!image)
        return emptyVariantForHint(hint);

    QPixmap* pixmap = image->nativeImageForCurrentFrame();
    if (!pixmap)
        return emptyVariantForHint(hint);

    return (hint == static_cast<QMetaType::Type>(qMetaTypeId<QPixmap>()))
        ? QVariant::fromValue<QPixmap>(*pixmap)
        : QVariant::fromValue<QImage>(pixmap->toImage());
}

bool QtPixmapRuntime::canHandle(QMetaType::Type hint)
{
    return hint == qMetaTypeId<QImage>() || hint == qMetaTypeId<QPixmap>();
}

JSClassRef QtPixmapRuntime::getClassRef()
{
    static const JSStaticValue staticValues[] = {
        { "width", getPixmapWidth, 0, 0 },
        { "height", getPixmapHeight, 0, 0 },
        { 0, 0, 0, 0}
    };

    static const JSStaticFunction staticFunctions[] = {
        { "assignToHTMLImageElement", assignToHTMLImageElement, 0 },
        { "toDataUrl", pixmapToDataUrl, 0 },
        { "toImageData", pixmapToImageData, 0 },
        { "toString", pixmapToString, 0 },
        { 0, 0, 0 }
    };

    static const JSClassDefinition classDefinition = {
        0, 0, "QtPixmapRuntimeObject", 0, staticValues, staticFunctions,
        0, finalizePixmap, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    static JSClassRef classRef = JSClassCreate(&classDefinition);
    return classRef;
}


}

}
