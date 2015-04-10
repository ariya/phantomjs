/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
#include "HTMLFormControlsCollection.h"


#include "HTMLAllCollection.h"
#include "JSDOMBinding.h"
#include "JSHTMLCollection.h"
#include "JSHTMLFormControlsCollection.h"
#include "JSNode.h"
#include "JSNodeList.h"
#include "JSRadioNodeList.h"
#include "Node.h"
#include "RadioNodeList.h"
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>

using namespace JSC;

namespace WebCore {

static JSValue getNamedItems(ExecState* exec, JSHTMLFormControlsCollection* collection, PropertyName propertyName)
{
    Vector<RefPtr<Node> > namedItems;
    const AtomicString& name = propertyNameToAtomicString(propertyName);
    collection->impl()->namedItems(name, namedItems);

    if (namedItems.isEmpty())
        return jsUndefined();
    if (namedItems.size() == 1)
        return toJS(exec, collection->globalObject(), namedItems[0].get());

    ASSERT(collection->impl()->type() == FormControls);
    return toJS(exec, collection->globalObject(), collection->impl()->ownerNode()->radioNodeList(name).get());
}

bool JSHTMLFormControlsCollection::canGetItemsForName(ExecState*, HTMLFormControlsCollection* collection, PropertyName propertyName)
{
    return collection->hasNamedItem(propertyNameToAtomicString(propertyName));
}

JSValue JSHTMLFormControlsCollection::nameGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSHTMLFormControlsCollection* thisObj = jsCast<JSHTMLFormControlsCollection*>(asObject(slotBase));
    return getNamedItems(exec, thisObj, propertyName);
}

JSValue JSHTMLFormControlsCollection::namedItem(ExecState* exec)
{
    JSValue value = getNamedItems(exec, this, Identifier(exec, exec->argument(0).toString(exec)->value(exec)));
    return value.isUndefined() ? jsNull() : value;
}

} // namespace WebCore
