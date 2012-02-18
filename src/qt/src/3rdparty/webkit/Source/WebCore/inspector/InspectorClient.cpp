/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorClient.h"

#if ENABLE(INSPECTOR)

#include "Frame.h"
#include "Page.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"

namespace WebCore {

bool InspectorClient::doDispatchMessageOnFrontendPage(Page* frontendPage, const String& message)
{
    if (!frontendPage)
        return false;

    Frame* frame = frontendPage->mainFrame();
    if (!frame)
        return false;

    ScriptController* scriptController = frame->script();
    if (!scriptController)
        return false;

    String dispatchToFrontend("WebInspector.dispatchMessageFromBackend(");
    dispatchToFrontend += message;
    dispatchToFrontend += ");";

    // Do not call executeInWorld here since it will end up calling Document::updateStyleForAllDocuments().
    // As a result we might re-enter CSSStyleSelector::styleForElement() which is terrible.
    scriptController->evaluate(ScriptSourceCode(dispatchToFrontend));
    return true;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
