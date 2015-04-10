/*
 * Copyright (C) 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
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
#include "JSDocument.h"

#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLDocument.h"
#include "JSCanvasRenderingContext2D.h"
#if ENABLE(WEBGL)
#include "JSWebGLRenderingContext.h"
#endif
#include "JSDOMWindowCustom.h"
#include "JSHTMLDocument.h"
#include "JSLocation.h"
#include "JSTouch.h"
#include "JSTouchList.h"
#include "Location.h"
#include "NodeTraversal.h"
#include "ScriptController.h"
#include "TouchList.h"

#if ENABLE(SVG)
#include "JSSVGDocument.h"
#include "SVGDocument.h"
#endif

#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

JSValue JSDocument::location(ExecState* exec) const
{
    Frame* frame = static_cast<Document*>(impl())->frame();
    if (!frame)
        return jsNull();

    Location* location = frame->document()->domWindow()->location();
    if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), location))
        return wrapper;

    JSLocation* jsLocation = JSLocation::create(getDOMStructure<JSLocation>(exec, globalObject()), globalObject(), location);
    cacheWrapper(currentWorld(exec), location, jsLocation);
    return jsLocation;
}

void JSDocument::setLocation(ExecState* exec, JSValue value)
{
    Frame* frame = static_cast<Document*>(impl())->frame();
    if (!frame)
        return;

    String locationString = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;

    if (Location* location = frame->document()->domWindow()->location())
        location->setHref(locationString, activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, Document* document)
{
    if (!document)
        return jsNull();

    JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), document);
    if (wrapper)
        return wrapper;

    if (DOMWindow* domWindow = document->domWindow()) {
        globalObject = toJSDOMWindow(toJS(exec, domWindow));
        // Creating a wrapper for domWindow might have created a wrapper for document as well.
        wrapper = getCachedWrapper(currentWorld(exec), document);
        if (wrapper)
            return wrapper;
    }

    if (document->isHTMLDocument())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, HTMLDocument, document);
#if ENABLE(SVG)
    else if (document->isSVGDocument())
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, SVGDocument, document);
#endif
    else
        wrapper = CREATE_DOM_WRAPPER(exec, globalObject, Document, document);

    // Make sure the document is kept around by the window object, and works right with the
    // back/forward cache.
    if (!document->frame()) {
        size_t nodeCount = 0;
        for (Node* n = document; n; n = NodeTraversal::next(n))
            nodeCount++;
        
        exec->heap()->reportExtraMemoryCost(nodeCount * sizeof(Node));
    }

    return wrapper;
}

#if ENABLE(TOUCH_EVENTS)
JSValue JSDocument::createTouchList(ExecState* exec)
{
    RefPtr<TouchList> touchList = TouchList::create();

    for (size_t i = 0; i < exec->argumentCount(); i++)
        touchList->append(toTouch(exec->argument(i)));

    return toJS(exec, globalObject(), touchList.release());
}
#endif

} // namespace WebCore
