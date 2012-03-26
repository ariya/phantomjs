/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef FocusController_h
#define FocusController_h

#include "FocusDirection.h"
#include "SpatialNavigation.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Document;
class Frame;
class KeyboardEvent;
class Node;
class Page;

class FocusController {
    WTF_MAKE_NONCOPYABLE(FocusController); WTF_MAKE_FAST_ALLOCATED;
public:
    FocusController(Page*);

    void setFocusedFrame(PassRefPtr<Frame>);
    Frame* focusedFrame() const { return m_focusedFrame.get(); }
    Frame* focusedOrMainFrame() const;

    bool setInitialFocus(FocusDirection, KeyboardEvent*);
    bool advanceFocus(FocusDirection, KeyboardEvent*, bool initialFocus = false);
        
    bool setFocusedNode(Node*, PassRefPtr<Frame>);

    void setActive(bool);
    bool isActive() const { return m_isActive; }

    void setFocused(bool);
    bool isFocused() const { return m_isFocused; }

private:
    bool advanceFocusDirectionally(FocusDirection, KeyboardEvent*);
    bool advanceFocusInDocumentOrder(FocusDirection, KeyboardEvent*, bool initialFocus);

    bool advanceFocusDirectionallyInContainer(Node* container, const IntRect& startingRect, FocusDirection, KeyboardEvent*);
    void findFocusCandidateInContainer(Node* container, const IntRect& startingRect, FocusDirection, KeyboardEvent*, FocusCandidate& closest);

    Page* m_page;
    RefPtr<Frame> m_focusedFrame;
    bool m_isActive;
    bool m_isFocused;
    bool m_isChangingFocusedFrame;
};

} // namespace WebCore
    
#endif // FocusController_h
