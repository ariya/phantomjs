/*
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "JSNodeFilterCondition.h"

#include "JSMainThreadExecState.h"
#include "JSNode.h"
#include "JSNodeFilter.h"
#include "NodeFilter.h"
#include <runtime/Error.h>
#include <runtime/JSLock.h>

namespace WebCore {

using namespace JSC;

JSNodeFilterCondition::JSNodeFilterCondition(VM&, NodeFilter* owner, JSValue filter)
    : m_filter(filter.isObject() ? PassWeak<JSObject>(jsCast<JSObject*>(filter), &m_weakOwner, owner) : nullptr)
{
}

short JSNodeFilterCondition::acceptNode(JSC::ExecState* exec, Node* filterNode) const
{
    JSLockHolder lock(exec);

    if (!m_filter)
        return NodeFilter::FILTER_ACCEPT;

    // Exec is null if we've been called from a non-JavaScript language and the document
    // is no longer able to run JavaScript (e.g., it's disconnected from its frame).
    if (!exec)
        return NodeFilter::FILTER_REJECT;

    JSValue filter = m_filter.get();
    CallData callData;
    CallType callType = getCallData(filter, callData);
    if (callType == CallTypeNone) {
        filter = filter.get(exec, Identifier(exec, "acceptNode"));
        callType = getCallData(filter, callData);
        if (callType == CallTypeNone) {
            throwError(exec, createTypeError(exec, "NodeFilter object does not have an acceptNode function"));
            return NodeFilter::FILTER_REJECT;
        }
    }

    MarkedArgumentBuffer args;
    // FIXME: The node should have the prototype chain that came from its document, not
    // whatever prototype chain might be on the window this filter came from. Bug 27662
    args.append(toJS(exec, deprecatedGlobalObjectForPrototype(exec), filterNode));
    if (exec->hadException())
        return NodeFilter::FILTER_REJECT;

    JSValue result = JSMainThreadExecState::call(exec, filter, callType, callData, m_filter.get(), args);
    if (exec->hadException())
        return NodeFilter::FILTER_REJECT;

    int intResult = result.toInt32(exec);
    if (exec->hadException())
        return NodeFilter::FILTER_REJECT;

    return intResult;
}

bool JSNodeFilterCondition::WeakOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, SlotVisitor& visitor)
{
    return visitor.containsOpaqueRoot(context);
}

} // namespace WebCore
