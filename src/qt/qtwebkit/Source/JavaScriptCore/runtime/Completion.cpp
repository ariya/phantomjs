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
#include "CodeProfiling.h"
#include "Debugger.h"
#include "Interpreter.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "Operations.h"
#include "Parser.h"
#include <wtf/WTFThreadData.h>
#include <stdio.h>

namespace JSC {

bool checkSyntax(ExecState* exec, const SourceCode& source, JSValue* returnedException)
{
    JSLockHolder lock(exec);
    RELEASE_ASSERT(exec->vm().identifierTable == wtfThreadData().currentIdentifierTable());

    ProgramExecutable* program = ProgramExecutable::create(exec, source);
    JSObject* error = program->checkSyntax(exec);
    if (error) {
        if (returnedException)
            *returnedException = error;
        return false;
    }

    return true;
}
    
bool checkSyntax(VM& vm, const SourceCode& source, ParserError& error)
{
    JSLockHolder lock(vm);
    RELEASE_ASSERT(vm.identifierTable == wtfThreadData().currentIdentifierTable());
    RefPtr<ProgramNode> programNode = parse<ProgramNode>(&vm, source, 0, Identifier(), JSParseNormal, JSParseProgramCode, error);
    return programNode;
}

JSValue evaluate(ExecState* exec, const SourceCode& source, JSValue thisValue, JSValue* returnedException)
{
    JSLockHolder lock(exec);
    RELEASE_ASSERT(exec->vm().identifierTable == wtfThreadData().currentIdentifierTable());
    RELEASE_ASSERT(!exec->vm().isCollectorBusy());

    CodeProfiling profile(source);

    ProgramExecutable* program = ProgramExecutable::create(exec, source);
    if (!program) {
        if (returnedException)
            *returnedException = exec->vm().exception;

        exec->vm().exception = JSValue();
        return jsUndefined();
    }

    if (!thisValue || thisValue.isUndefinedOrNull())
        thisValue = exec->dynamicGlobalObject();
    JSObject* thisObj = thisValue.toThisObject(exec);
    JSValue result = exec->interpreter()->execute(program, exec, thisObj);

    if (exec->hadException()) {
        if (returnedException)
            *returnedException = exec->exception();

        exec->clearException();
        return jsUndefined();
    }

    RELEASE_ASSERT(result);
    return result;
}

} // namespace JSC
