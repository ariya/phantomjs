/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSCanvasRenderingContext2D.h"

#include "CanvasGradient.h"
#include "CanvasPattern.h"
#include "CanvasRenderingContext2D.h"
#include "CanvasStyle.h"
#include "ExceptionCode.h"
#include "FloatRect.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "HTMLVideoElement.h"
#include "ImageData.h"
#include "JSCanvasGradient.h"
#include "JSCanvasPattern.h"
#include "JSHTMLCanvasElement.h"
#include "JSHTMLImageElement.h"
#include "JSHTMLVideoElement.h"
#include "JSImageData.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

static JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, CanvasStyle* style)
{
    if (style->canvasGradient())
        return toJS(exec, globalObject, style->canvasGradient());
    if (style->canvasPattern())
        return toJS(exec, globalObject, style->canvasPattern());
    return jsString(exec, style->color());
}

static PassRefPtr<CanvasStyle> toHTMLCanvasStyle(ExecState*, JSValue value)
{
    if (!value.isObject())
        return 0;
    JSObject* object = asObject(value);
    if (object->inherits(&JSCanvasGradient::s_info))
        return CanvasStyle::createFromGradient(static_cast<JSCanvasGradient*>(object)->impl());
    if (object->inherits(&JSCanvasPattern::s_info))
        return CanvasStyle::createFromPattern(static_cast<JSCanvasPattern*>(object)->impl());
    return 0;
}

JSValue JSCanvasRenderingContext2D::strokeStyle(ExecState* exec) const
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    return toJS(exec, globalObject(), context->strokeStyle());        
}

void JSCanvasRenderingContext2D::setStrokeStyle(ExecState* exec, JSValue value)
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    if (value.isString()) {
        context->setStrokeColor(ustringToString(asString(value)->value(exec)));
        return;
    }
    context->setStrokeStyle(toHTMLCanvasStyle(exec, value));
}

JSValue JSCanvasRenderingContext2D::fillStyle(ExecState* exec) const
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    return toJS(exec, globalObject(), context->fillStyle());
}

void JSCanvasRenderingContext2D::setFillStyle(ExecState* exec, JSValue value)
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    if (value.isString()) {
        context->setFillColor(ustringToString(asString(value)->value(exec)));
        return;
    }
    context->setFillStyle(toHTMLCanvasStyle(exec, value));
}

JSValue JSCanvasRenderingContext2D::setFillColor(ExecState* exec)
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    // string arg = named color
    // number arg = gray color
    // string arg, number arg = named color, alpha
    // number arg, number arg = gray color, alpha
    // 4 args = r, g, b, a
    // 5 args = c, m, y, k, a
    switch (exec->argumentCount()) {
        case 1:
            if (exec->argument(0).isString())
                context->setFillColor(ustringToString(asString(exec->argument(0))->value(exec)));
            else
                context->setFillColor(exec->argument(0).toFloat(exec));
            break;
        case 2:
            if (exec->argument(0).isString())
                context->setFillColor(ustringToString(asString(exec->argument(0))->value(exec)), exec->argument(1).toFloat(exec));
            else
                context->setFillColor(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec));
            break;
        case 4:
            context->setFillColor(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                  exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec));
            break;
        case 5:
            context->setFillColor(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                  exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec));
            break;
        default:
            return throwSyntaxError(exec);
    }
    return jsUndefined();
}    

JSValue JSCanvasRenderingContext2D::setStrokeColor(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    // string arg = named color
    // number arg = gray color
    // string arg, number arg = named color, alpha
    // number arg, number arg = gray color, alpha
    // 4 args = r, g, b, a
    // 5 args = c, m, y, k, a
    switch (exec->argumentCount()) {
        case 1:
            if (exec->argument(0).isString())
                context->setStrokeColor(ustringToString(asString(exec->argument(0))->value(exec)));
            else
                context->setStrokeColor(exec->argument(0).toFloat(exec));
            break;
        case 2:
            if (exec->argument(0).isString())
                context->setStrokeColor(ustringToString(asString(exec->argument(0))->value(exec)), exec->argument(1).toFloat(exec));
            else
                context->setStrokeColor(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec));
            break;
        case 4:
            context->setStrokeColor(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                    exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec));
            break;
        case 5:
            context->setStrokeColor(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                    exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec));
            break;
        default:
            return throwSyntaxError(exec);
    }
    
    return jsUndefined();
}

JSValue JSCanvasRenderingContext2D::strokeRect(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    
    if (exec->argumentCount() <= 4)
        context->strokeRect(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                            exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec));
    else
        context->strokeRect(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                            exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec));

    return jsUndefined();    
}

JSValue JSCanvasRenderingContext2D::drawImage(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    // DrawImage has three variants:
    //     drawImage(img, dx, dy)
    //     drawImage(img, dx, dy, dw, dh)
    //     drawImage(img, sx, sy, sw, sh, dx, dy, dw, dh)
    // Composite operation is specified with globalCompositeOperation.
    // The img parameter can be a <img> or <canvas> element.
    JSValue value = exec->argument(0);
    if (value.isNull()) {
        setDOMException(exec, TYPE_MISMATCH_ERR);
        return jsUndefined();
    }
    if (!value.isObject())
        return throwTypeError(exec);

    JSObject* o = asObject(value);
    ExceptionCode ec = 0;
    if (o->inherits(&JSHTMLImageElement::s_info)) {
        HTMLImageElement* imgElt = static_cast<HTMLImageElement*>(static_cast<JSHTMLElement*>(o)->impl());
        switch (exec->argumentCount()) {
            case 3:
                context->drawImage(imgElt, exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), ec);
                break;
            case 5:
                context->drawImage(imgElt, exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                                   exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec), ec);
                setDOMException(exec, ec);
                break;
            case 9:
                context->drawImage(imgElt, FloatRect(exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                                   exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec)),
                                   FloatRect(exec->argument(5).toFloat(exec), exec->argument(6).toFloat(exec),
                                   exec->argument(7).toFloat(exec), exec->argument(8).toFloat(exec)), ec);
                setDOMException(exec, ec);
                break;
            default:
                return throwSyntaxError(exec);
        }
    } else if (o->inherits(&JSHTMLCanvasElement::s_info)) {
        HTMLCanvasElement* canvas = static_cast<HTMLCanvasElement*>(static_cast<JSHTMLElement*>(o)->impl());
        switch (exec->argumentCount()) {
            case 3:
                context->drawImage(canvas, exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), ec);
                setDOMException(exec, ec);
                break;
            case 5:
                context->drawImage(canvas, exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                                   exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec), ec);
                setDOMException(exec, ec);
                break;
            case 9:
                context->drawImage(canvas, FloatRect(exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                                   exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec)),
                                   FloatRect(exec->argument(5).toFloat(exec), exec->argument(6).toFloat(exec),
                                   exec->argument(7).toFloat(exec), exec->argument(8).toFloat(exec)), ec);
                setDOMException(exec, ec);
                break;
            default:
                return throwSyntaxError(exec);
        }
#if ENABLE(VIDEO)
    } else if (o->inherits(&JSHTMLVideoElement::s_info)) {
            HTMLVideoElement* video = static_cast<HTMLVideoElement*>(static_cast<JSHTMLElement*>(o)->impl());
            switch (exec->argumentCount()) {
                case 3:
                    context->drawImage(video, exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), ec);
                    break;
                case 5:
                    context->drawImage(video, exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                                       exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec), ec);
                    setDOMException(exec, ec);
                    break;
                case 9:
                    context->drawImage(video, FloatRect(exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                                       exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec)),
                                       FloatRect(exec->argument(5).toFloat(exec), exec->argument(6).toFloat(exec),
                                       exec->argument(7).toFloat(exec), exec->argument(8).toFloat(exec)), ec);
                    setDOMException(exec, ec);
                    break;
                default:
                    return throwSyntaxError(exec);
        }
#endif
    } else
        return throwTypeError(exec);

    return jsUndefined();
}

JSValue JSCanvasRenderingContext2D::drawImageFromRect(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    
    JSValue value = exec->argument(0);
    if (!value.isObject())
        return throwTypeError(exec);
    JSObject* o = asObject(value);
    
    if (!o->inherits(&JSHTMLImageElement::s_info))
        return throwTypeError(exec);
    context->drawImageFromRect(static_cast<HTMLImageElement*>(static_cast<JSHTMLElement*>(o)->impl()),
                               exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec),
                               exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec),
                               exec->argument(5).toFloat(exec), exec->argument(6).toFloat(exec),
                               exec->argument(7).toFloat(exec), exec->argument(8).toFloat(exec),
                               ustringToString(exec->argument(9).toString(exec)));    
    return jsUndefined();    
}

JSValue JSCanvasRenderingContext2D::setShadow(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    switch (exec->argumentCount()) {
        case 3:
            context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                               exec->argument(2).toFloat(exec));
            break;
        case 4:
            if (exec->argument(3).isString())
                context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                   exec->argument(2).toFloat(exec), ustringToString(asString(exec->argument(3))->value(exec)));
            else
                context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                   exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec));
            break;
        case 5:
            if (exec->argument(3).isString())
                context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                   exec->argument(2).toFloat(exec), ustringToString(asString(exec->argument(3))->value(exec)),
                                   exec->argument(4).toFloat(exec));
            else
                context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                                   exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec),
                                   exec->argument(4).toFloat(exec));
            break;
        case 7:
            context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                               exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec),
                               exec->argument(4).toFloat(exec), exec->argument(5).toFloat(exec),
                               exec->argument(6).toFloat(exec));
            break;
        case 8:
            context->setShadow(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec),
                               exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec),
                               exec->argument(4).toFloat(exec), exec->argument(5).toFloat(exec),
                               exec->argument(6).toFloat(exec), exec->argument(7).toFloat(exec));
            break;
        default:
            return throwSyntaxError(exec);
    }
    
    return jsUndefined();    
}

JSValue JSCanvasRenderingContext2D::createPattern(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    JSValue value = exec->argument(0);
    if (!value.isObject()) {
        setDOMException(exec, TYPE_MISMATCH_ERR);
        return jsUndefined();
    }
    JSObject* o = asObject(value);

    if (o->inherits(&JSHTMLImageElement::s_info)) {
        ExceptionCode ec;
        JSValue pattern = toJS(exec, globalObject(), 
            context->createPattern(static_cast<HTMLImageElement*>(static_cast<JSHTMLElement*>(o)->impl()),
                                   valueToStringWithNullCheck(exec, exec->argument(1)), ec).get());
        setDOMException(exec, ec);
        return pattern;
    }
    if (o->inherits(&JSHTMLCanvasElement::s_info)) {
        ExceptionCode ec;
        JSValue pattern = toJS(exec, globalObject(), 
            context->createPattern(static_cast<HTMLCanvasElement*>(static_cast<JSHTMLElement*>(o)->impl()),
                valueToStringWithNullCheck(exec, exec->argument(1)), ec).get());
        setDOMException(exec, ec);
        return pattern;
    }
    setDOMException(exec, TYPE_MISMATCH_ERR);
    return jsUndefined();
}

JSValue JSCanvasRenderingContext2D::createImageData(ExecState* exec)
{
    // createImageData has two variants
    // createImageData(ImageData)
    // createImageData(width, height)
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    RefPtr<ImageData> imageData = 0;

    ExceptionCode ec = 0;
    if (exec->argumentCount() == 1)
        imageData = context->createImageData(toImageData(exec->argument(0)), ec);
    else if (exec->argumentCount() == 2)
        imageData = context->createImageData(exec->argument(0).toFloat(exec), exec->argument(1).toFloat(exec), ec);

    setDOMException(exec, ec);
    return toJS(exec, globalObject(), WTF::getPtr(imageData));
}

JSValue JSCanvasRenderingContext2D::putImageData(ExecState* exec)
{
    // putImageData has two variants
    // putImageData(ImageData, x, y)
    // putImageData(ImageData, x, y, dirtyX, dirtyY, dirtyWidth, dirtyHeight)
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    ExceptionCode ec = 0;
    if (exec->argumentCount() >= 7)
        context->putImageData(toImageData(exec->argument(0)), exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), 
                              exec->argument(3).toFloat(exec), exec->argument(4).toFloat(exec), exec->argument(5).toFloat(exec), exec->argument(6).toFloat(exec), ec);
    else
        context->putImageData(toImageData(exec->argument(0)), exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), ec);

    setDOMException(exec, ec);
    return jsUndefined();
}

JSValue JSCanvasRenderingContext2D::fillText(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    // string arg = text to draw
    // number arg = x
    // number arg = y
    // optional number arg = maxWidth
    if (exec->argumentCount() < 3 || exec->argumentCount() > 4)
        return throwSyntaxError(exec);
    
    if (exec->argumentCount() == 4)
        context->fillText(ustringToString(exec->argument(0).toString(exec)), exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec));
    else
        context->fillText(ustringToString(exec->argument(0).toString(exec)), exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec));
    return jsUndefined();
}

JSValue JSCanvasRenderingContext2D::strokeText(ExecState* exec)
{ 
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());

    // string arg = text to draw
    // number arg = x
    // number arg = y
    // optional number arg = maxWidth
    if (exec->argumentCount() < 3 || exec->argumentCount() > 4)
        return throwSyntaxError(exec);
    
    if (exec->argumentCount() == 4)
        context->strokeText(ustringToString(exec->argument(0).toString(exec)), exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec), exec->argument(3).toFloat(exec));
    else
        context->strokeText(ustringToString(exec->argument(0).toString(exec)), exec->argument(1).toFloat(exec), exec->argument(2).toFloat(exec));
    return jsUndefined();
}

} // namespace WebCore
