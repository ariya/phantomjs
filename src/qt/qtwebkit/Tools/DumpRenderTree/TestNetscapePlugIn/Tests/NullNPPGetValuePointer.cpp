/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PluginTest.h"

#include "PluginObject.h"

using namespace std;

// Passing null for our NPP_GetValue function pointer should not crash.

class NullNPPGetValuePointer : public PluginTest {
public:
    NullNPPGetValuePointer(NPP, const string& identifier);

private:
    virtual NPError NPP_Destroy(NPSavedData**);
    virtual NPError NPP_GetValue(NPPVariable, void* value);

    NPP_GetValueProcPtr m_originalNPPGetValuePointer;
};

static PluginTest::Register<NullNPPGetValuePointer> registrar("null-npp-getvalue-pointer");

NullNPPGetValuePointer::NullNPPGetValuePointer(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
    , m_originalNPPGetValuePointer(pluginFunctions->getvalue)
{
    // Be sneaky and null out the getvalue pointer the browser is holding. This simulates a plugin
    // that doesn't implement NPP_GetValue (like Shockwave Director 10.3 on Windows). Note that if
    // WebKit copies the NPPluginFuncs struct this technique will have no effect and WebKit will
    // call into our NPP_GetValue implementation.
    pluginFunctions->getvalue = 0;
}

NPError NullNPPGetValuePointer::NPP_Destroy(NPSavedData**)
{
    // Set the NPP_GetValue pointer back the way it was before we mucked with it so we don't mess
    // up future uses of the plugin module.
    pluginFunctions->getvalue = m_originalNPPGetValuePointer;
    return NPERR_NO_ERROR;
}

NPError NullNPPGetValuePointer::NPP_GetValue(NPPVariable, void*)
{
    pluginLog(m_npp, "NPP_GetValue was called but should not have been. Maybe WebKit copied the NPPluginFuncs struct, which would invalidate this test.");
    return NPERR_GENERIC_ERROR;
}
