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

#ifndef BindingSecurity_h
#define BindingSecurity_h

#include "BindingSecurityBase.h"
#include "Element.h"
#include "Frame.h"
#include "GenericBinding.h"
#include "HTMLFrameElementBase.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "Settings.h"

namespace WebCore {

class DOMWindow;
class Node;

// Security functions shared by various language bindings.
template <class Binding>
class BindingSecurity : public BindingSecurityBase {
public:
    // Check if the active execution context can access the target frame.
    static bool canAccessFrame(State<Binding>*, Frame*, bool reportError);

    // Check if it is safe to access the given node from the
    // current security context.
    static bool checkNodeSecurity(State<Binding>*, Node* target);

    static bool allowPopUp(State<Binding>*);
    static bool allowSettingFrameSrcToJavascriptUrl(State<Binding>*, HTMLFrameElementBase*, String value);
    static bool allowSettingSrcToJavascriptURL(State<Binding>*, Element*, String name, String value);

    static bool shouldAllowNavigation(State<Binding>*, Frame*);

private:
    explicit BindingSecurity() {}
    ~BindingSecurity();

    // Check if the current DOMWindow's security context can access the target
    // DOMWindow.  This function does not report errors, so most callers should
    // use canAccessFrame instead.
    static bool canAccessWindow(State<Binding>*, DOMWindow* target);
};

// Implementations of templated methods must be in this file.

template <class Binding>
bool BindingSecurity<Binding>::canAccessWindow(State<Binding>* state,
                                               DOMWindow* targetWindow)
{
    DOMWindow* activeWindow = state->activeWindow();
    return canAccess(activeWindow, targetWindow);
}

template <class Binding>
bool BindingSecurity<Binding>::canAccessFrame(State<Binding>* state,
                                              Frame* target,
                                              bool reportError)
{
    // The subject is detached from a frame, deny accesses.
    if (!target)
        return false;

    if (!canAccessWindow(state, getDOMWindow(target))) {
        if (reportError)
            state->immediatelyReportUnsafeAccessTo(target);
        return false;
    }
    return true;
}

template <class Binding>
bool BindingSecurity<Binding>::checkNodeSecurity(State<Binding>* state, Node* node)
{
    if (!node)
        return false;

    Frame* target = getFrame(node);

    if (!target)
        return false;

    return canAccessFrame(state, target, true);
}

template <class Binding>
bool BindingSecurity<Binding>::allowPopUp(State<Binding>* state)
{
    if (state->processingUserGesture())
        return true;

    Frame* frame = state->firstFrame();
    ASSERT(frame);
    Settings* settings = frame->settings();
    return settings && settings->javaScriptCanOpenWindowsAutomatically();
}

template <class Binding>
bool BindingSecurity<Binding>::allowSettingFrameSrcToJavascriptUrl(State<Binding>* state, HTMLFrameElementBase* frame, String value)
{
    if (protocolIsJavaScript(stripLeadingAndTrailingHTMLSpaces(value))) {
        Node* contentDoc = frame->contentDocument();
        if (contentDoc && !checkNodeSecurity(state, contentDoc))
            return false;
    }
    return true;
}

template <class Binding>
bool BindingSecurity<Binding>::allowSettingSrcToJavascriptURL(State<Binding>* state, Element* element, String name, String value)
{
    if ((element->hasTagName(HTMLNames::iframeTag) || element->hasTagName(HTMLNames::frameTag)) && equalIgnoringCase(name, "src"))
        return allowSettingFrameSrcToJavascriptUrl(state, static_cast<HTMLFrameElementBase*>(element), value);
    return true;
}

template <class Binding>
bool BindingSecurity<Binding>::shouldAllowNavigation(State<Binding>* state, Frame* frame)
{
    Frame* activeFrame = state->activeFrame();
    return activeFrame && activeFrame->loader()->shouldAllowNavigation(frame);
}

}

#endif // BindingSecurity_h
