/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSHTMLAllCollection.h"

#include "HTMLAllCollection.h"
#include "JSDOMBinding.h"
#include "JSHTMLAllCollection.h"
#include "JSNode.h"
#include "JSNodeList.h"
#include "Node.h"
#include "StaticNodeList.h"
#include <runtime/JSValue.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>

using namespace JSC;

namespace WebCore {

static JSValue getNamedItems(ExecState* exec, JSHTMLAllCollection* collection, const Identifier& propertyName)
{
    Vector<RefPtr<Node> > namedItems;
    collection->impl()->namedItems(identifierToAtomicString(propertyName), namedItems);

    if (namedItems.isEmpty())
        return jsUndefined();
    if (namedItems.size() == 1)
        return toJS(exec, collection->globalObject(), namedItems[0].get());

    // FIXME: HTML5 specifies that this should be a DynamicNodeList.
    // FIXME: HTML5 specifies that non-HTMLOptionsCollection collections should return
    // the first matching item instead of a NodeList.
    return toJS(exec, collection->globalObject(), StaticNodeList::adopt(namedItems).get());
}

// HTMLCollections are strange objects, they support both get and call,
// so that document.forms.item(0) and document.forms(0) both work.
static EncodedJSValue JSC_HOST_CALL callHTMLAllCollection(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return JSValue::encode(jsUndefined());

    // Do not use thisObj here. It can be the JSHTMLDocument, in the document.forms(i) case.
    JSHTMLAllCollection* jsCollection = static_cast<JSHTMLAllCollection*>(exec->callee());
    HTMLAllCollection* collection = static_cast<HTMLAllCollection*>(jsCollection->impl());

    // Also, do we need the TypeError test here ?

    if (exec->argumentCount() == 1) {
        // Support for document.all(<index>) etc.
        bool ok;
        UString string = exec->argument(0).toString(exec);
        unsigned index = Identifier::toUInt32(string, ok);
        if (ok)
            return JSValue::encode(toJS(exec, jsCollection->globalObject(), collection->item(index)));

        // Support for document.images('<name>') etc.
        return JSValue::encode(getNamedItems(exec, jsCollection, Identifier(exec, string)));
    }

    // The second arg, if set, is the index of the item we want
    bool ok;
    UString string = exec->argument(0).toString(exec);
    unsigned index = Identifier::toUInt32(exec->argument(1).toString(exec), ok);
    if (ok) {
        String pstr = ustringToString(string);
        Node* node = collection->namedItem(pstr);
        while (node) {
            if (!index)
                return JSValue::encode(toJS(exec, jsCollection->globalObject(), node));
            node = collection->nextNamedItem(pstr);
            --index;
        }
    }

    return JSValue::encode(jsUndefined());
}

CallType JSHTMLAllCollection::getCallData(CallData& callData)
{
    callData.native.function = callHTMLAllCollection;
    return CallTypeHost;
}

bool JSHTMLAllCollection::canGetItemsForName(ExecState*, HTMLAllCollection* collection, const Identifier& propertyName)
{
    Vector<RefPtr<Node> > namedItems;
    collection->namedItems(identifierToAtomicString(propertyName), namedItems);
    return !namedItems.isEmpty();
}

JSValue JSHTMLAllCollection::nameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSHTMLAllCollection* thisObj = static_cast<JSHTMLAllCollection*>(asObject(slotBase));
    return getNamedItems(exec, thisObj, propertyName);
}

JSValue JSHTMLAllCollection::item(ExecState* exec)
{
    bool ok;
    uint32_t index = Identifier::toUInt32(exec->argument(0).toString(exec), ok);
    if (ok)
        return toJS(exec, globalObject(), impl()->item(index));
    return getNamedItems(exec, this, Identifier(exec, exec->argument(0).toString(exec)));
}

JSValue JSHTMLAllCollection::namedItem(ExecState* exec)
{
    return getNamedItems(exec, this, Identifier(exec, exec->argument(0).toString(exec)));
}

} // namespace WebCore
