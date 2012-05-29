/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef BINDINGS_C_RUNTIME_H_
#define BINDINGS_C_RUNTIME_H_

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "BridgeJSC.h"
#include "npruntime_internal.h"

namespace JSC {
namespace Bindings {

class CField : public Field {
public:
    CField(NPIdentifier ident) : _fieldIdentifier(ident) { }

    virtual JSValue valueFromInstance(ExecState*, const Instance*) const;
    virtual void setValueToInstance(ExecState*, const Instance*, JSValue) const;

    NPIdentifier identifier() const { return _fieldIdentifier; }

private:
    NPIdentifier _fieldIdentifier;
};


class CMethod : public Method
{
public:
    CMethod(NPIdentifier ident) : _methodIdentifier(ident) { }

    NPIdentifier identifier() const { return _methodIdentifier; }
    virtual int numParameters() const { return 0; }

private:
    NPIdentifier _methodIdentifier;
};

} // namespace Bindings
} // namespace JSC

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif
