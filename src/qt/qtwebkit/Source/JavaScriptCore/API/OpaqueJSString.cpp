/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "OpaqueJSString.h"

#include <interpreter/CallFrame.h>
#include <runtime/JSGlobalObject.h>
#include <runtime/Identifier.h>

using namespace JSC;

PassRefPtr<OpaqueJSString> OpaqueJSString::create(const String& string)
{
    if (!string.isNull())
        return adoptRef(new OpaqueJSString(string));
    return 0;
}

String OpaqueJSString::string() const
{
    if (!this)
        return String();

    // Return a copy of the wrapped string, because the caller may make it an Identifier.
    return m_string.isolatedCopy();
}

Identifier OpaqueJSString::identifier(VM* vm) const
{
    if (!this || m_string.isNull())
        return Identifier();

    if (m_string.isEmpty())
        return Identifier(Identifier::EmptyIdentifier);

    if (m_string.is8Bit())
        return Identifier(vm, m_string.characters8(), m_string.length());

    return Identifier(vm, m_string.characters16(), m_string.length());
}
