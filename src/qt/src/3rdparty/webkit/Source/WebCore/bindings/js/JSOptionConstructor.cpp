/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSOptionConstructor.h"

#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "JSHTMLOptionElement.h"
#include "ScriptExecutionContext.h"
#include "Text.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSOptionConstructor);

const ClassInfo JSOptionConstructor::s_info = { "OptionConstructor", &DOMConstructorWithDocument::s_info, 0, 0 };

JSOptionConstructor::JSOptionConstructor(ExecState* exec, Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorWithDocument(structure, globalObject)
{
    ASSERT(inherits(&s_info));
    putDirect(exec->globalData(), exec->propertyNames().prototype, JSHTMLOptionElementPrototype::self(exec, globalObject), None);
    putDirect(exec->globalData(), exec->propertyNames().length, jsNumber(4), ReadOnly | DontDelete | DontEnum);
}

static EncodedJSValue JSC_HOST_CALL constructHTMLOptionElement(ExecState* exec)
{
    JSOptionConstructor* jsConstructor = static_cast<JSOptionConstructor*>(exec->callee());
    Document* document = jsConstructor->document();
    if (!document)
        return throwVMError(exec, createReferenceError(exec, "Option constructor associated document is unavailable"));

    String data;
    if ((exec->argumentCount() >= 1) && !exec->argument(0).isUndefined())
        data = ustringToString(exec->argument(0).toString(exec));

    String value;
    if ((exec->argumentCount() >= 2)  && !exec->argument(1).isUndefined())
        value = ustringToString(exec->argument(1).toString(exec));
    bool defaultSelected = (exec->argumentCount() >= 3) && exec->argument(2).toBoolean(exec);
    bool selected = (exec->argumentCount() >= 4) && exec->argument(3).toBoolean(exec);

    ExceptionCode ec = 0;
    RefPtr<HTMLOptionElement> element = HTMLOptionElement::createForJSConstructor(document, data, value, defaultSelected, selected, ec);
    if (ec) {
        setDOMException(exec, ec);
        return JSValue::encode(JSValue());
    }

    return JSValue::encode(asObject(toJS(exec, jsConstructor->globalObject(), element.release())));
}

ConstructType JSOptionConstructor::getConstructData(ConstructData& constructData)
{
    constructData.native.function = constructHTMLOptionElement;
    return ConstructTypeHost;
}


} // namespace WebCore
