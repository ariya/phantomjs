/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007 Apple Inc.
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
 *
 */

#include "config.h"
#include "Completion.h"

#include "CallFrame.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "Interpreter.h"
#include "Parser.h"
#include "Debugger.h"
#include "WTFThreadData.h"
#include <stdio.h>

namespace JSC {

Completion checkSyntax(ExecState* exec, const SourceCode& source)
{
    JSLock lock(exec);
    ASSERT(exec->globalData().identifierTable == wtfThreadData().currentIdentifierTable());

    ProgramExecutable* program = ProgramExecutable::create(exec, source);
    JSObject* error = program->checkSyntax(exec);
    if (error)
        return Completion(Throw, error);

    return Completion(Normal);
}

Completion evaluate(ExecState* exec, ScopeChainNode* scopeChain, const SourceCode& source, JSValue thisValue)
{
    JSLock lock(exec);
    ASSERT(exec->globalData().identifierTable == wtfThreadData().currentIdentifierTable());

    ProgramExecutable* program = ProgramExecutable::create(exec, source);
    if (!program) {
        JSValue exception = exec->globalData().exception;
        exec->globalData().exception = JSValue();
        return Completion(Throw, exception);
    }

    JSObject* thisObj = (!thisValue || thisValue.isUndefinedOrNull()) ? exec->dynamicGlobalObject() : thisValue.toObject(exec);

    JSValue result = exec->interpreter()->execute(program, exec, scopeChain, thisObj);

    if (exec->hadException()) {
        JSValue exception = exec->exception();
        exec->clearException();

        ComplType exceptionType = Throw;
        if (exception.isObject())
            exceptionType = asObject(exception)->exceptionType();
        return Completion(exceptionType, exception);
    }
    return Completion(Normal, result);
}

} // namespace JSC
