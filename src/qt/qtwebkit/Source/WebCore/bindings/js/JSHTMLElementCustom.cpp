/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "JSHTMLElement.h"

#include "Document.h"
#include "HTMLFormElement.h"
#include <runtime/JSWithScope.h>

#if ENABLE(MICRODATA)
#include "JSMicroDataItemValue.h"
#endif

namespace WebCore {

using namespace JSC;

JSScope* JSHTMLElement::pushEventHandlerScope(ExecState* exec, JSScope* scope) const
{
    HTMLElement* element = impl();

    // The document is put on first, fall back to searching it only after the element and form.
    scope = JSWithScope::create(exec, asObject(toJS(exec, globalObject(), element->ownerDocument())), scope);

    // The form is next, searched before the document, but after the element itself.
    if (HTMLFormElement* form = element->form())
        scope = JSWithScope::create(exec, asObject(toJS(exec, globalObject(), form)), scope);

    // The element is on top, searched first.
    return JSWithScope::create(exec, asObject(toJS(exec, globalObject(), element)), scope);
}

#if ENABLE(MICRODATA)
JSValue JSHTMLElement::itemValue(ExecState* exec) const
{
    HTMLElement* element = impl();
    return toJS(exec, globalObject(), WTF::getPtr(element->itemValue()));
}

void JSHTMLElement::setItemValue(ExecState* exec, JSValue value)
{
    HTMLElement* imp = impl();
    ExceptionCode ec = 0;
    imp->setItemValue(valueToStringWithNullCheck(exec, value), ec);
    setDOMException(exec, ec);
}
#endif

} // namespace WebCore
