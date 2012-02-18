/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "MediaQueryListListener.h"

#include "MediaQueryList.h"
#include "ScriptFunctionCall.h"

#if USE(JSC)
#include "JSMediaQueryList.h"
#else
#include "V8MediaQueryList.h"
#endif

namespace WebCore {

void MediaQueryListListener::queryChanged(ScriptState* state, MediaQueryList* query)
{
    ScriptCallback callback(state, m_value);
#if USE(JSC)
    callback.appendArgument(toJS(state, deprecatedGlobalObjectForPrototype(state), query));
#else
    v8::HandleScope handleScope;
    v8::Handle<v8::Context> context = state->context();
    if (context.IsEmpty())
       return; // JS may not be enabled.
    v8::Context::Scope scope(context);
    callback.appendArgument(toV8(query));
#endif
    callback.call();
}

}
