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

#ifndef DeleteButtonController_h
#define DeleteButtonController_h

#include "DeleteButton.h"

namespace WebCore {

class DeleteButton;
class Frame;
class HTMLElement;
class RenderObject;
class VisibleSelection;

class DeleteButtonController {
    WTF_MAKE_NONCOPYABLE(DeleteButtonController); WTF_MAKE_FAST_ALLOCATED;
public:
    DeleteButtonController(Frame*);

    static const char* const containerElementIdentifier;

    HTMLElement* target() const { return m_target.get(); }
    HTMLElement* containerElement() const { return m_containerElement.get(); }

    void respondToChangedSelection(const VisibleSelection& oldSelection);

    void show(HTMLElement*);
    void hide();

    bool enabled() const { return (m_disableStack == 0); }
    void enable();
    void disable();

    void deleteTarget();

private:
    static const char* const buttonElementIdentifier;
    static const char* const outlineElementIdentifier;

    void createDeletionUI();

    Frame* m_frame;
    RefPtr<HTMLElement> m_target;
    RefPtr<HTMLElement> m_containerElement;
    RefPtr<HTMLElement> m_outlineElement;
    RefPtr<DeleteButton> m_buttonElement;
    bool m_wasStaticPositioned;
    bool m_wasAutoZIndex;
    unsigned m_disableStack;
};

} // namespace WebCore

#endif
