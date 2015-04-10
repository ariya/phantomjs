/*
 * Copyright (C) 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef ApplyStyleCommand_h
#define ApplyStyleCommand_h

#include "CompositeEditCommand.h"
#include "HTMLElement.h"
#include "WritingDirection.h"

namespace WebCore {

class CSSPrimitiveValue;
class EditingStyle;
class StyleChange;

enum ShouldIncludeTypingStyle {
    IncludeTypingStyle,
    IgnoreTypingStyle
};

class ApplyStyleCommand : public CompositeEditCommand {
public:
    enum EPropertyLevel { PropertyDefault, ForceBlockProperties };
    enum InlineStyleRemovalMode { RemoveIfNeeded, RemoveAlways, RemoveNone };
    enum EAddStyledElement { AddStyledElement, DoNotAddStyledElement };
    typedef bool (*IsInlineElementToRemoveFunction)(const Element*);

    static PassRefPtr<ApplyStyleCommand> create(Document* document, const EditingStyle* style, EditAction action = EditActionChangeAttributes, EPropertyLevel level = PropertyDefault)
    {
        return adoptRef(new ApplyStyleCommand(document, style, action, level));
    }
    static PassRefPtr<ApplyStyleCommand> create(Document* document, const EditingStyle* style, const Position& start, const Position& end, EditAction action = EditActionChangeAttributes, EPropertyLevel level = PropertyDefault)
    {
        return adoptRef(new ApplyStyleCommand(document, style, start, end, action, level));
    }
    static PassRefPtr<ApplyStyleCommand> create(PassRefPtr<Element> element, bool removeOnly = false, EditAction action = EditActionChangeAttributes)
    {
        return adoptRef(new ApplyStyleCommand(element, removeOnly, action));
    }
    static PassRefPtr<ApplyStyleCommand> create(Document* document, const EditingStyle* style, IsInlineElementToRemoveFunction isInlineElementToRemoveFunction, EditAction action = EditActionChangeAttributes)
    {
        return adoptRef(new ApplyStyleCommand(document, style, isInlineElementToRemoveFunction, action));
    }

private:
    ApplyStyleCommand(Document*, const EditingStyle*, EditAction, EPropertyLevel);
    ApplyStyleCommand(Document*, const EditingStyle*, const Position& start, const Position& end, EditAction, EPropertyLevel);
    ApplyStyleCommand(PassRefPtr<Element>, bool removeOnly, EditAction);
    ApplyStyleCommand(Document*, const EditingStyle*, bool (*isInlineElementToRemove)(const Element*), EditAction);

    virtual void doApply();
    virtual EditAction editingAction() const;

    // style-removal helpers
    bool isStyledInlineElementToRemove(Element*) const;
    bool shouldApplyInlineStyleToRun(EditingStyle*, Node* runStart, Node* pastEndNode);
    void removeConflictingInlineStyleFromRun(EditingStyle*, RefPtr<Node>& runStart, RefPtr<Node>& runEnd, PassRefPtr<Node> pastEndNode);
    bool removeInlineStyleFromElement(EditingStyle*, PassRefPtr<HTMLElement>, InlineStyleRemovalMode = RemoveIfNeeded, EditingStyle* extractedStyle = 0);
    inline bool shouldRemoveInlineStyleFromElement(EditingStyle* style, HTMLElement* element) {return removeInlineStyleFromElement(style, element, RemoveNone);}
    void replaceWithSpanOrRemoveIfWithoutAttributes(HTMLElement*&);
    bool removeImplicitlyStyledElement(EditingStyle*, HTMLElement*, InlineStyleRemovalMode, EditingStyle* extractedStyle);
    bool removeCSSStyle(EditingStyle*, HTMLElement*, InlineStyleRemovalMode = RemoveIfNeeded, EditingStyle* extractedStyle = 0);
    HTMLElement* highestAncestorWithConflictingInlineStyle(EditingStyle*, Node*);
    void applyInlineStyleToPushDown(Node*, EditingStyle*);
    void pushDownInlineStyleAroundNode(EditingStyle*, Node*);
    void removeInlineStyle(EditingStyle* , const Position& start, const Position& end);
    bool nodeFullySelected(Node*, const Position& start, const Position& end) const;
    bool nodeFullyUnselected(Node*, const Position& start, const Position& end) const;

    // style-application helpers
    void applyBlockStyle(EditingStyle*);
    void applyRelativeFontStyleChange(EditingStyle*);
    void applyInlineStyle(EditingStyle*);
    void fixRangeAndApplyInlineStyle(EditingStyle*, const Position& start, const Position& end);
    void applyInlineStyleToNodeRange(EditingStyle*, PassRefPtr<Node> startNode, PassRefPtr<Node> pastEndNode);
    void addBlockStyle(const StyleChange&, HTMLElement*);
    void addInlineStyleIfNeeded(EditingStyle*, PassRefPtr<Node> start, PassRefPtr<Node> end, EAddStyledElement = AddStyledElement);
    Position positionToComputeInlineStyleChange(PassRefPtr<Node>, RefPtr<Node>& dummyElement);
    void applyInlineStyleChange(PassRefPtr<Node> startNode, PassRefPtr<Node> endNode, StyleChange&, EAddStyledElement);
    void splitTextAtStart(const Position& start, const Position& end);
    void splitTextAtEnd(const Position& start, const Position& end);
    void splitTextElementAtStart(const Position& start, const Position& end);
    void splitTextElementAtEnd(const Position& start, const Position& end);
    bool shouldSplitTextElement(Element*, EditingStyle*);
    bool isValidCaretPositionInTextNode(const Position& position);
    bool mergeStartWithPreviousIfIdentical(const Position& start, const Position& end);
    bool mergeEndWithNextIfIdentical(const Position& start, const Position& end);
    void cleanupUnstyledAppleStyleSpans(Node* dummySpanAncestor);

    void surroundNodeRangeWithElement(PassRefPtr<Node> start, PassRefPtr<Node> end, PassRefPtr<Element>);
    float computedFontSize(Node*);
    void joinChildTextNodes(Node*, const Position& start, const Position& end);

    HTMLElement* splitAncestorsWithUnicodeBidi(Node*, bool before, WritingDirection allowedDirection);
    void removeEmbeddingUpToEnclosingBlock(Node* node, Node* unsplitAncestor);

    void updateStartEnd(const Position& newStart, const Position& newEnd);
    Position startPosition();
    Position endPosition();

    RefPtr<EditingStyle> m_style;
    EditAction m_editingAction;
    EPropertyLevel m_propertyLevel;
    Position m_start;
    Position m_end;
    bool m_useEndingSelection;
    RefPtr<Element> m_styledInlineElement;
    bool m_removeOnly;
    IsInlineElementToRemoveFunction m_isInlineElementToRemoveFunction;
};

enum ShouldStyleAttributeBeEmpty { AllowNonEmptyStyleAttribute, StyleAttributeShouldBeEmpty };
bool isEmptyFontTag(const Element*, ShouldStyleAttributeBeEmpty = StyleAttributeShouldBeEmpty);
bool isLegacyAppleStyleSpan(const Node*);
bool isStyleSpanOrSpanWithOnlyStyleAttribute(const Element*);
PassRefPtr<HTMLElement> createStyleSpanElement(Document*);

} // namespace WebCore

#endif
