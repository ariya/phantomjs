/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSClipboard.h"

#include "Clipboard.h"
#include "Element.h"
#include "HTMLImageElement.h"
#include "HTMLNames.h"
#include "IntPoint.h"
#include "JSNode.h"
#include "Node.h"
#include "PlatformString.h"
#include <runtime/ArrayPrototype.h>
#include <runtime/Error.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

using namespace JSC;

namespace WebCore {

using namespace HTMLNames;

JSValue JSClipboard::types(ExecState* exec) const
{
    Clipboard* clipboard = impl();

    HashSet<String> types = clipboard->types();
    if (types.isEmpty())
        return jsNull();

    MarkedArgumentBuffer list;
    HashSet<String>::const_iterator end = types.end();
    for (HashSet<String>::const_iterator it = types.begin(); it != end; ++it)
        list.append(jsString(exec, stringToUString(*it)));
    return constructArray(exec, globalObject(), list);
}

JSValue JSClipboard::clearData(ExecState* exec)
{
    Clipboard* clipboard = impl();

    if (!exec->argumentCount()) {
        clipboard->clearAllData();
        return jsUndefined();
    }

    if (exec->argumentCount() == 1) {
        clipboard->clearData(ustringToString(exec->argument(0).toString(exec)));
        return jsUndefined();
    }

    // FIXME: It does not match the rest of the JS bindings to throw on invalid number of arguments. 
    return throwError(exec, createSyntaxError(exec, "clearData: Invalid number of arguments"));
}

JSValue JSClipboard::getData(ExecState* exec)
{
    // FIXME: It does not match the rest of the JS bindings to throw on invalid number of arguments.
    if (exec->argumentCount() != 1)
        return throwError(exec, createSyntaxError(exec, "getData: Invalid number of arguments"));

    Clipboard* clipboard = impl();

    bool success;
    String result = clipboard->getData(ustringToString(exec->argument(0).toString(exec)), success);
    if (!success)
        return jsUndefined();

    return jsString(exec, result);
}

JSValue JSClipboard::setDragImage(ExecState* exec)
{
    Clipboard* clipboard = impl();

    if (!clipboard->isForDragAndDrop())
        return jsUndefined();

    // FIXME: It does not match the rest of the JS bindings to throw on invalid number of arguments. 
    if (exec->argumentCount() != 3)
        return throwError(exec, createSyntaxError(exec, "setDragImage: Invalid number of arguments"));

    int x = exec->argument(1).toInt32(exec);
    int y = exec->argument(2).toInt32(exec);

    // See if they passed us a node
    Node* node = toNode(exec->argument(0));
    if (!node)
        return throwTypeError(exec);

    // FIXME: This should probably be a TypeError. 
    if (!node->isElementNode())
        return throwError(exec, createSyntaxError(exec, "setDragImageFromElement: Invalid first argument"));

    if (static_cast<Element*>(node)->hasLocalName(imgTag) && !node->inDocument())
        clipboard->setDragImage(static_cast<HTMLImageElement*>(node)->cachedImage(), IntPoint(x, y));
    else
        clipboard->setDragImageElement(node, IntPoint(x, y));

    return jsUndefined();
}

} // namespace WebCore
