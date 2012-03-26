/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef CompositeEditCommand_h
#define CompositeEditCommand_h

#include "EditCommand.h"
#include "CSSPropertyNames.h"
#include <wtf/Vector.h>

namespace WebCore {

class EditingStyle;
class HTMLElement;
class StyledElement;
class Text;

class CompositeEditCommand : public EditCommand {
public:
    virtual ~CompositeEditCommand();

    bool isFirstCommand(EditCommand* command) { return !m_commands.isEmpty() && m_commands.first() == command; }

protected:
    explicit CompositeEditCommand(Document*);

    //
    // sugary-sweet convenience functions to help create and apply edit commands in composite commands
    //
    void appendNode(PassRefPtr<Node>, PassRefPtr<ContainerNode> parent);
    void applyCommandToComposite(PassRefPtr<EditCommand>);
    void applyStyle(const EditingStyle*, EditAction = EditActionChangeAttributes);
    void applyStyle(const EditingStyle*, const Position& start, const Position& end, EditAction = EditActionChangeAttributes);
    void applyStyledElement(PassRefPtr<Element>);
    void removeStyledElement(PassRefPtr<Element>);
    void deleteSelection(bool smartDelete = false, bool mergeBlocksAfterDelete = true, bool replace = false, bool expandForSpecialElements = true);
    void deleteSelection(const VisibleSelection&, bool smartDelete = false, bool mergeBlocksAfterDelete = true, bool replace = false, bool expandForSpecialElements = true);
    virtual void deleteTextFromNode(PassRefPtr<Text>, unsigned offset, unsigned count);
    void inputText(const String&, bool selectInsertedText = false);
    void insertNodeAfter(PassRefPtr<Node>, PassRefPtr<Node> refChild);
    void insertNodeAt(PassRefPtr<Node>, const Position&);
    void insertNodeAtTabSpanPosition(PassRefPtr<Node>, const Position&);
    void insertNodeBefore(PassRefPtr<Node>, PassRefPtr<Node> refChild);
    void insertParagraphSeparator(bool useDefaultParagraphElement = false);
    void insertLineBreak();
    void insertTextIntoNode(PassRefPtr<Text>, unsigned offset, const String& text);
    void joinTextNodes(PassRefPtr<Text>, PassRefPtr<Text>);
    void mergeIdenticalElements(PassRefPtr<Element>, PassRefPtr<Element>);
    void rebalanceWhitespace();
    void rebalanceWhitespaceAt(const Position&);
    void rebalanceWhitespaceOnTextSubstring(RefPtr<Text>, int startOffset, int endOffset);
    void prepareWhitespaceAtPositionForSplit(Position&);
    bool canRebalance(const Position&) const;
    bool shouldRebalanceLeadingWhitespaceFor(const String&) const;
    void removeCSSProperty(PassRefPtr<StyledElement>, CSSPropertyID);
    void removeNodeAttribute(PassRefPtr<Element>, const QualifiedName& attribute);
    void removeChildrenInRange(PassRefPtr<Node>, unsigned from, unsigned to);
    virtual void removeNode(PassRefPtr<Node>);
    HTMLElement* replaceElementWithSpanPreservingChildrenAndAttributes(PassRefPtr<HTMLElement>);
    void removeNodePreservingChildren(PassRefPtr<Node>);
    void removeNodeAndPruneAncestors(PassRefPtr<Node>);
    void prune(PassRefPtr<Node>);
    void replaceTextInNode(PassRefPtr<Text>, unsigned offset, unsigned count, const String& replacementText);
    Position positionOutsideTabSpan(const Position&);
    void setNodeAttribute(PassRefPtr<Element>, const QualifiedName& attribute, const AtomicString& value);
    void splitElement(PassRefPtr<Element>, PassRefPtr<Node> atChild);
    void splitTextNode(PassRefPtr<Text>, unsigned offset);
    void splitTextNodeContainingElement(PassRefPtr<Text>, unsigned offset);
    void wrapContentsInDummySpan(PassRefPtr<Element>);

    void deleteInsignificantText(PassRefPtr<Text>, unsigned start, unsigned end);
    void deleteInsignificantText(const Position& start, const Position& end);
    void deleteInsignificantTextDownstream(const Position&);

    PassRefPtr<Node> appendBlockPlaceholder(PassRefPtr<Element>);
    PassRefPtr<Node> insertBlockPlaceholder(const Position&);
    PassRefPtr<Node> addBlockPlaceholderIfNeeded(Element*);
    void removePlaceholderAt(const Position&);

    PassRefPtr<Node> insertNewDefaultParagraphElementAt(const Position&);

    PassRefPtr<Node> moveParagraphContentsToNewBlockIfNecessary(const Position&);
    
    void pushAnchorElementDown(Node*);
    
    void moveParagraph(const VisiblePosition&, const VisiblePosition&, const VisiblePosition&, bool preserveSelection = false, bool preserveStyle = true);
    void moveParagraphs(const VisiblePosition&, const VisiblePosition&, const VisiblePosition&, bool preserveSelection = false, bool preserveStyle = true);
    void moveParagraphWithClones(const VisiblePosition& startOfParagraphToMove, const VisiblePosition& endOfParagraphToMove, Element* blockElement, Node* outerNode);
    void cloneParagraphUnderNewElement(Position& start, Position& end, Node* outerNode, Element* blockElement);
    void cleanupAfterDeletion(VisiblePosition destination = VisiblePosition());
    
    bool breakOutOfEmptyListItem();
    bool breakOutOfEmptyMailBlockquotedParagraph();
    
    Position positionAvoidingSpecialElementBoundary(const Position&);
    
    PassRefPtr<Node> splitTreeToNode(Node*, Node*, bool splitAncestor = false);

    Vector<RefPtr<EditCommand> > m_commands;

private:
    virtual void doUnapply();
    virtual void doReapply();
};

} // namespace WebCore

#endif // CompositeEditCommand_h
