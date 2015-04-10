/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include <string.h>
#include <vector>

using namespace std;

const char *javaScriptURL = "javascript:'Hello, ' + 'World!'";
const char *javaScriptResult = "Hello, World!";

// Test that evaluating a javascript: URL will send a stream with the result of the evaluation.
// Test that evaluating JavaScript using NPN_GetURL will a stream with result of the evaluation.
class GetURLWithJavaScriptURL : public PluginTest {
public:
    GetURLWithJavaScriptURL(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
        , m_didFail(false)
    {
    }

private:
    virtual NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
    {
        NPN_GetURL(javaScriptURL, 0);
        return NPERR_NO_ERROR;
    }

    NPError NPP_NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
    {
        stream->pdata = this;

        if (strcmp(stream->url, javaScriptURL))
            m_didFail = true;

        if (stream->end != strlen(javaScriptResult))
            m_didFail = true;

        *stype = NP_NORMAL;
        return NPERR_NO_ERROR;
    }

    NPError NPP_DestroyStream(NPStream* stream, NPReason reason)
    {
        if (stream->pdata != this)
            m_didFail = true;

        if (reason != NPRES_DONE)
            m_didFail = true;

        if (m_data.size() != stream->end)
            m_didFail = true;

        m_data.push_back('\0');

        if (strcmp(&m_data[0], javaScriptResult))
            m_didFail = true;

        if (!m_didFail)
            executeScript("testSucceeded()");
        else
            executeScript("notifyDone()");

        return NPERR_NO_ERROR;
    }

    int32_t NPP_WriteReady(NPStream* stream)
    {
        if (stream->pdata != this)
            m_didFail = true;

        return 2;
    }

    int32_t NPP_Write(NPStream* stream, int32_t offset, int32_t len, void* buffer)
    {
        if (stream->pdata != this)
            m_didFail = true;
        
        m_data.insert(m_data.end(), static_cast<char*>(buffer), static_cast<char*>(buffer) + len);
        return len;
    }

    vector<char> m_data;
    bool m_didFail;
};

static PluginTest::Register<GetURLWithJavaScriptURL> getURLWithJavaScriptURLDestroyingPlugin("get-url-with-javascript-url");
