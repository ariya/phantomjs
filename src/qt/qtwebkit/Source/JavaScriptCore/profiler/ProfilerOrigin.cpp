/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "ProfilerOrigin.h"

#include "JSGlobalObject.h"
#include "ObjectConstructor.h"
#include "Operations.h"
#include "ProfilerBytecodes.h"
#include "ProfilerDatabase.h"

namespace JSC { namespace Profiler {

Origin::Origin(Database& database, CodeBlock* codeBlock, unsigned bytecodeIndex)
    : m_bytecodes(database.ensureBytecodesFor(codeBlock))
    , m_bytecodeIndex(bytecodeIndex)
{
}

void Origin::dump(PrintStream& out) const
{
    out.print(*m_bytecodes, ":bc#", m_bytecodeIndex);
}

JSValue Origin::toJS(ExecState* exec) const
{
    JSObject* result = constructEmptyObject(exec);
    result->putDirect(exec->vm(), exec->propertyNames().bytecodesID, jsNumber(m_bytecodes->id()));
    result->putDirect(exec->vm(), exec->propertyNames().bytecodeIndex, jsNumber(m_bytecodeIndex));
    return result;
}

} } // namespace JSC::Profiler

