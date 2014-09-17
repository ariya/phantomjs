/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef IndentOutdentCommand_h
#define IndentOutdentCommand_h

#include "ApplyBlockElementCommand.h"
#include "CompositeEditCommand.h"

namespace WebCore {

class IndentOutdentCommand : public ApplyBlockElementCommand {
public:
    enum EIndentType { Indent, Outdent };
    static PassRefPtr<IndentOutdentCommand> create(Document* document, EIndentType type, int marginInPixels = 0)
    {
        return adoptRef(new IndentOutdentCommand(document, type, marginInPixels));
    }

    virtual bool preservesTypingStyle() const { return true; }

private:
    IndentOutdentCommand(Document*, EIndentType, int marginInPixels);

    virtual EditAction editingAction() const { return m_typeOfAction == Indent ? EditActionIndent : EditActionOutdent; }

    void indentRegion(const VisiblePosition&, const VisiblePosition&);
    void outdentRegion(const VisiblePosition&, const VisiblePosition&);
    void outdentParagraph();
    bool tryIndentingAsListItem(const Position&, const Position&);
    void indentIntoBlockquote(const Position&, const Position&, RefPtr<Element>&);

    void formatSelection(const VisiblePosition& startOfSelection, const VisiblePosition& endOfSelection);
    void formatRange(const Position& start, const Position& end, const Position& endOfSelection, RefPtr<Element>& blockquoteForNextIndent);

    EIndentType m_typeOfAction;
    int m_marginInPixels;
};

} // namespace WebCore

#endif // IndentOutdentCommand_h
