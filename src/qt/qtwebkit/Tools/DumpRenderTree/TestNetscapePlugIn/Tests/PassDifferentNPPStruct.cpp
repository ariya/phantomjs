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

// Passing a different NPP struct that has the same ndata value as the one passed to NPP_New should
// not trigger an assertion failure.

class PassDifferentNPPStruct : public PluginTest {
public:
    PassDifferentNPPStruct(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
        , m_didReceiveInitialSetWindowCall(false)
    {
    }

private:
    virtual NPError NPP_SetWindow(NPWindow* window)
    {
        if (m_didReceiveInitialSetWindowCall)
            return NPERR_NO_ERROR;
        m_didReceiveInitialSetWindowCall = true;

        NPP oldNPP = m_npp;
        NPP_t differentNPP = *m_npp;
        m_npp = &differentNPP;

        NPBool privateMode;
        NPError error = NPN_GetValue(NPNVprivateModeBool, &privateMode);

        m_npp = oldNPP;

        if (error != NPERR_NO_ERROR) {
            log("NPN_GetValue(NPNVprivateModeBool) with a different NPP struct failed with error %d", error);
            notifyDone();
            return NPERR_GENERIC_ERROR;
        }
        log("NPN_GetValue(NPNVprivateModeBool) with a different NPP struct succeeded");
        notifyDone();
        return NPERR_NO_ERROR;
    }

    bool m_didReceiveInitialSetWindowCall;
};

static PluginTest::Register<PassDifferentNPPStruct> getValueNetscapeWindow("pass-different-npp-struct");
