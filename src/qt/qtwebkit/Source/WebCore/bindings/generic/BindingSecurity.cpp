/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "BindingSecurity.h"

#include "BindingState.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "HTMLFrameElementBase.h"
#include "HTMLParserIdioms.h"
#include "JSDOMBinding.h"
#include "SecurityOrigin.h"
#include "Settings.h"

namespace WebCore {

static bool canAccessDocument(BindingState* state, Document* targetDocument, SecurityReportingOption reportingOption = ReportSecurityError)
{
    if (!targetDocument)
        return false;

    DOMWindow* active = activeDOMWindow(state);
    if (!active)
        return false;

    if (active->document()->securityOrigin()->canAccess(targetDocument->securityOrigin()))
        return true;

    if (reportingOption == ReportSecurityError)
        printErrorMessageForFrame(targetDocument->frame(), targetDocument->domWindow()->crossDomainAccessErrorMessage(active));

    return false;
}

bool BindingSecurity::shouldAllowAccessToDOMWindow(BindingState* state, DOMWindow* target, SecurityReportingOption reportingOption)
{
    return target && canAccessDocument(state, target->document(), reportingOption);
}

bool BindingSecurity::shouldAllowAccessToFrame(BindingState* state, Frame* target, SecurityReportingOption reportingOption)
{
    return target && canAccessDocument(state, target->document(), reportingOption);
}

bool BindingSecurity::shouldAllowAccessToNode(BindingState* state, Node* target)
{
    return target && canAccessDocument(state, target->document());
}

bool BindingSecurity::allowSettingFrameSrcToJavascriptUrl(BindingState* state, HTMLFrameElementBase* frame, const String& value)
{
    return !protocolIsJavaScript(stripLeadingAndTrailingHTMLSpaces(value)) || canAccessDocument(state, frame->contentDocument());
}

}
