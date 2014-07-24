/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSHTMLDocument.h"

#include "Frame.h"
#include "HTMLAllCollection.h"
#include "HTMLBodyElement.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "HTMLElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "JSDOMWindow.h"
#include "JSDOMWindowCustom.h"
#include "JSDOMWindowShell.h"
#include "JSHTMLCollection.h"
#include "JSMainThreadExecState.h"
#include "SegmentedString.h"
#include "DocumentParser.h"
#include <runtime/Error.h>
#include <runtime/JSCell.h>
#include <wtf/unicode/CharacterNames.h>

using namespace JSC;

namespace WebCore {

using namespace HTMLNames;

bool JSHTMLDocument::canGetItemsForName(ExecState*, HTMLDocument* document, PropertyName propertyName)
{
    AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
    return atomicPropertyName && document->documentNamedItemMap().contains(atomicPropertyName);
}

JSValue JSHTMLDocument::nameGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSHTMLDocument* thisObj = jsCast<JSHTMLDocument*>(asObject(slotBase));
    HTMLDocument* document = toHTMLDocument(thisObj->impl());

    AtomicStringImpl* atomicPropertyName = findAtomicString(propertyName);
    if (!atomicPropertyName || !document->documentNamedItemMap().contains(atomicPropertyName))
        return jsUndefined();

    if (UNLIKELY(!document->documentNamedItemMap().containsSingle(atomicPropertyName))) {
        RefPtr<HTMLCollection> collection = document->documentNamedItems(atomicPropertyName);
        ASSERT(!collection->isEmpty());
        ASSERT(!collection->hasExactlyOneItem());
        return toJS(exec, thisObj->globalObject(), WTF::getPtr(collection));
    }

    Node* node = document->documentNamedItemMap().getElementByDocumentNamedItem(atomicPropertyName, document);
    Frame* frame;
    if (node->hasTagName(iframeTag) && (frame = toHTMLIFrameElement(node)->contentFrame()))
        return toJS(exec, frame);

    return toJS(exec, thisObj->globalObject(), node);
}

// Custom attributes

JSValue JSHTMLDocument::all(ExecState* exec) const
{
    // If "all" has been overwritten, return the overwritten value
    JSValue v = getDirect(exec->vm(), Identifier(exec, "all"));
    if (v)
        return v;

    return toJS(exec, globalObject(), toHTMLDocument(impl())->all());
}

void JSHTMLDocument::setAll(ExecState* exec, JSValue value)
{
    // Add "all" to the property map.
    putDirect(exec->vm(), Identifier(exec, "all"), value);
}

// Custom functions

JSValue JSHTMLDocument::open(ExecState* exec)
{
    // For compatibility with other browsers, pass open calls with more than 2 parameters to the window.
    if (exec->argumentCount() > 2) {
        Frame* frame = toHTMLDocument(impl())->frame();
        if (frame) {
            JSDOMWindowShell* wrapper = toJSDOMWindowShell(frame, currentWorld(exec));
            if (wrapper) {
                JSValue function = wrapper->get(exec, Identifier(exec, "open"));
                CallData callData;
                CallType callType = ::getCallData(function, callData);
                if (callType == CallTypeNone)
                    return throwTypeError(exec);
                return JSMainThreadExecState::call(exec, function, callType, callData, wrapper, ArgList(exec));
            }
        }
        return jsUndefined();
    }

    // document.open clobbers the security context of the document and
    // aliases it with the active security context.
    Document* activeDocument = asJSDOMWindow(exec->lexicalGlobalObject())->impl()->document();

    // In the case of two parameters or fewer, do a normal document open.
    toHTMLDocument(impl())->open(activeDocument);
    return this;
}

enum NewlineRequirement { DoNotAddNewline, DoAddNewline };

static inline void documentWrite(ExecState* exec, HTMLDocument* document, NewlineRequirement addNewline)
{
    // DOM only specifies single string argument, but browsers allow multiple or no arguments.

    size_t size = exec->argumentCount();

    String firstString = exec->argument(0).toString(exec)->value(exec);
    SegmentedString segmentedString = firstString;
    if (size != 1) {
        if (!size)
            segmentedString.clear();
        else {
            for (size_t i = 1; i < size; ++i) {
                String subsequentString = exec->argument(i).toString(exec)->value(exec);
                segmentedString.append(SegmentedString(subsequentString));
            }
        }
    }
    if (addNewline)
        segmentedString.append(SegmentedString(String(&newlineCharacter, 1)));

    Document* activeDocument = asJSDOMWindow(exec->lexicalGlobalObject())->impl()->document();
    document->write(segmentedString, activeDocument);
}

JSValue JSHTMLDocument::write(ExecState* exec)
{
    documentWrite(exec, toHTMLDocument(impl()), DoNotAddNewline);
    return jsUndefined();
}

JSValue JSHTMLDocument::writeln(ExecState* exec)
{
    documentWrite(exec, toHTMLDocument(impl()), DoAddNewline);
    return jsUndefined();
}

} // namespace WebCore
