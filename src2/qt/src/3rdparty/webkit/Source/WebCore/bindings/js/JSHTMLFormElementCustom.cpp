/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSHTMLFormElement.h"

#include "Frame.h"
#include "HTMLCollection.h"
#include "HTMLFormElement.h"
#include "JSDOMWindowCustom.h"
#include "JSNodeList.h"
#include "StaticNodeList.h"

using namespace JSC;

namespace WebCore {

bool JSHTMLFormElement::canGetItemsForName(ExecState*, HTMLFormElement* form, const Identifier& propertyName)
{
    Vector<RefPtr<Node> > namedItems;
    form->getNamedElements(identifierToAtomicString(propertyName), namedItems);
    return namedItems.size();
}

JSValue JSHTMLFormElement::nameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSHTMLElement* jsForm = static_cast<JSHTMLFormElement*>(asObject(slotBase));
    HTMLFormElement* form = static_cast<HTMLFormElement*>(jsForm->impl());

    Vector<RefPtr<Node> > namedItems;
    form->getNamedElements(identifierToAtomicString(propertyName), namedItems);
    
    if (namedItems.isEmpty())
        return jsUndefined();
    if (namedItems.size() == 1)
        return toJS(exec, jsForm->globalObject(), namedItems[0].get());

    // FIXME: HTML5 specifies that this should be a RadioNodeList.
    return toJS(exec, jsForm->globalObject(), StaticNodeList::adopt(namedItems).get());
}

}
