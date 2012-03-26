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

#ifndef InsertListCommand_h
#define InsertListCommand_h

#include "CompositeEditCommand.h"

namespace WebCore {

class HTMLElement;

class InsertListCommand : public CompositeEditCommand {
public:
    enum Type { OrderedList, UnorderedList };

    static PassRefPtr<InsertListCommand> create(Document* document, Type listType)
    {
        return adoptRef(new InsertListCommand(document, listType));
    }

    static PassRefPtr<HTMLElement> insertList(Document*, Type);
    
    virtual bool preservesTypingStyle() const { return true; }

private:
    InsertListCommand(Document*, Type);

    virtual void doApply();
    virtual EditAction editingAction() const { return EditActionInsertList; }

    HTMLElement* fixOrphanedListChild(Node*);
    bool selectionHasListOfType(const VisibleSelection& selection, const QualifiedName&);
    PassRefPtr<HTMLElement> mergeWithNeighboringLists(PassRefPtr<HTMLElement>);
    void doApplyForSingleParagraph(bool forceCreateList, const QualifiedName&, Range* currentSelection);
    void unlistifyParagraph(const VisiblePosition& originalStart, HTMLElement* listNode, Node* listChildNode);
    PassRefPtr<HTMLElement> listifyParagraph(const VisiblePosition& originalStart, const QualifiedName& listTag);
    RefPtr<HTMLElement> m_listElement;
    Type m_type;
};

} // namespace WebCore

#endif // InsertListCommand_h
