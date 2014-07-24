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

// Executing JS after removing the plugin element from the document should not crash.

class EvaluateJSAfterRemovingPluginElement : public PluginTest {
public:
    EvaluateJSAfterRemovingPluginElement(NPP, const string& identifier);

private:
    virtual NPError NPP_DestroyStream(NPStream*, NPReason);

    bool m_didExecuteScript;
};

static PluginTest::Register<EvaluateJSAfterRemovingPluginElement> registrar("evaluate-js-after-removing-plugin-element");

EvaluateJSAfterRemovingPluginElement::EvaluateJSAfterRemovingPluginElement(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
    , m_didExecuteScript(false)
{
    waitUntilDone();
}

NPError EvaluateJSAfterRemovingPluginElement::NPP_DestroyStream(NPStream*, NPReason)
{
    if (m_didExecuteScript)
        return NPERR_NO_ERROR;
    m_didExecuteScript = true;

    executeScript("var plugin = document.getElementsByTagName('embed')[0]; plugin.parentElement.removeChild(plugin);");
    executeScript("document.body.appendChild(document.createTextNode('Executing script after removing the plugin element from the document succeeded.'));");
    notifyDone();

    return NPERR_NO_ERROR;
}
