/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef SelectionController_h
#define SelectionController_h

#include "EditingStyle.h"
#include "IntRect.h"
#include "Range.h"
#include "ScrollBehavior.h"
#include "Timer.h"
#include "VisibleSelection.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class CharacterData;
class CSSMutableStyleDeclaration;
class Frame;
class GraphicsContext;
class HTMLFormElement;
class RenderObject;
class RenderView;
class Settings;
class VisiblePosition;

enum DirectionalityPolicy { MakeNonDirectionalSelection, MakeDirectionalSelection };

class SelectionController {
    WTF_MAKE_NONCOPYABLE(SelectionController); WTF_MAKE_FAST_ALLOCATED;
public:
    enum EAlteration { AlterationMove, AlterationExtend };
    enum CursorAlignOnScroll { AlignCursorOnScrollIfNeeded,
                               AlignCursorOnScrollAlways };
    enum SetSelectionOption {
        CloseTyping = 1 << 0,
        ClearTypingStyle = 1 << 1,
        UserTriggered = 1 << 2,
        SpellCorrectionTriggered = 1 << 3,
    };
    typedef unsigned SetSelectionOptions;

    SelectionController(Frame* = 0, bool isDragCaretController = false);

    Element* rootEditableElement() const { return m_selection.rootEditableElement(); }
    bool isContentEditable() const { return m_selection.isContentEditable(); }
    bool isContentRichlyEditable() const { return m_selection.isContentRichlyEditable(); }
    Node* shadowTreeRootNode() const { return m_selection.shadowTreeRootNode(); }
     
    void moveTo(const Range*, EAffinity, bool userTriggered = false);
    void moveTo(const VisiblePosition&, bool userTriggered = false, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded);
    void moveTo(const VisiblePosition&, const VisiblePosition&, bool userTriggered = false);
    void moveTo(const Position&, EAffinity, bool userTriggered = false);
    void moveTo(const Position&, const Position&, EAffinity, bool userTriggered = false);

    const VisibleSelection& selection() const { return m_selection; }
    void setSelection(const VisibleSelection&, SetSelectionOptions = CloseTyping | ClearTypingStyle, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded, TextGranularity = CharacterGranularity, DirectionalityPolicy = MakeDirectionalSelection);
    void setSelection(const VisibleSelection& selection, TextGranularity granularity, DirectionalityPolicy directionality = MakeDirectionalSelection) { setSelection(selection, CloseTyping | ClearTypingStyle, AlignCursorOnScrollIfNeeded, granularity, directionality); }
    bool setSelectedRange(Range*, EAffinity, bool closeTyping);
    void selectAll();
    void clear();
    
    // Call this after doing user-triggered selections to make it easy to delete the frame you entirely selected.
    void selectFrameElementInParentIfFullySelected();

    bool contains(const IntPoint&);

    VisibleSelection::SelectionType selectionType() const { return m_selection.selectionType(); }

    EAffinity affinity() const { return m_selection.affinity(); }

    bool modify(EAlteration, SelectionDirection, TextGranularity, bool userTriggered = false);
    bool modify(EAlteration, int verticalDistance, bool userTriggered = false, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded);
    TextGranularity granularity() const { return m_granularity; }

    void setStart(const VisiblePosition &, bool userTriggered = false);
    void setEnd(const VisiblePosition &, bool userTriggered = false);
    
    void setBase(const VisiblePosition&, bool userTriggered = false);
    void setBase(const Position&, EAffinity, bool userTriggered = false);
    void setExtent(const VisiblePosition&, bool userTriggered = false);
    void setExtent(const Position&, EAffinity, bool userTriggered = false);

    Position base() const { return m_selection.base(); }
    Position extent() const { return m_selection.extent(); }
    Position start() const { return m_selection.start(); }
    Position end() const { return m_selection.end(); }

    // Return the renderer that is responsible for painting the caret (in the selection start node)
    RenderObject* caretRenderer() const;

    // Caret rect local to the caret's renderer
    IntRect localCaretRect();
    IntRect localCaretRectForPainting() const { return m_caretRect; }

    // Bounds of (possibly transformed) caret in absolute coords
    IntRect absoluteCaretBounds();
    void setCaretRectNeedsUpdate(bool flag = true);

    void setIsDirectional(bool);
    void willBeModified(EAlteration, SelectionDirection);

    bool isNone() const { return m_selection.isNone(); }
    bool isCaret() const { return m_selection.isCaret(); }
    bool isRange() const { return m_selection.isRange(); }
    bool isCaretOrRange() const { return m_selection.isCaretOrRange(); }
    bool isInPasswordField() const;
    bool isAll(EditingBoundaryCrossingRule rule = CannotCrossEditingBoundary) const { return m_selection.isAll(rule); }
    
    PassRefPtr<Range> toNormalizedRange() const { return m_selection.toNormalizedRange(); }

    void debugRenderer(RenderObject*, bool selected) const;

    void nodeWillBeRemoved(Node*);
    void textWillBeReplaced(CharacterData*, unsigned offset, unsigned oldLength, unsigned newLength);

    void setCaretVisible(bool = true);
    void clearCaretRectIfNeeded();
    bool recomputeCaretRect(); // returns true if caret rect moved
    void invalidateCaretRect();
    void paintCaret(GraphicsContext*, int tx, int ty, const IntRect& clipRect);

    // Used to suspend caret blinking while the mouse is down.
    void setCaretBlinkingSuspended(bool suspended) { m_isCaretBlinkingSuspended = suspended; }
    bool isCaretBlinkingSuspended() const { return m_isCaretBlinkingSuspended; }

    // Focus
    void setFocused(bool);
    bool isFocused() const { return m_focused; }
    bool isFocusedAndActive() const;
    void pageActivationChanged();

    // Painting.
    void updateAppearance();

    void updateSecureKeyboardEntryIfActive();

#ifndef NDEBUG
    void formatForDebugger(char* buffer, unsigned length) const;
    void showTreeForThis() const;
#endif

    bool shouldChangeSelection(const VisibleSelection&) const;
    bool shouldDeleteSelection(const VisibleSelection&) const;
    void setFocusedNodeIfNeeded();
    void notifyRendererOfSelectionChange(bool userTriggered);

    void paintDragCaret(GraphicsContext*, int tx, int ty, const IntRect& clipRect) const;

    EditingStyle* typingStyle() const;
    PassRefPtr<CSSMutableStyleDeclaration> copyTypingStyle() const;
    void setTypingStyle(PassRefPtr<EditingStyle>);
    void clearTypingStyle();

    FloatRect bounds(bool clipToVisibleContent = true) const;

    void getClippedVisibleTextRectangles(Vector<FloatRect>&) const;

    HTMLFormElement* currentForm() const;

    void revealSelection(const ScrollAlignment& = ScrollAlignment::alignCenterIfNeeded, bool revealExtent = false);
    void setSelectionFromNone();

private:
    enum EPositionType { START, END, BASE, EXTENT };

    void respondToNodeModification(Node*, bool baseRemoved, bool extentRemoved, bool startRemoved, bool endRemoved);
    TextDirection directionOfEnclosingBlock();

    VisiblePosition positionForPlatform(bool isGetStart) const;
    VisiblePosition startForPlatform() const;
    VisiblePosition endForPlatform() const;

    VisiblePosition modifyExtendingRight(TextGranularity);
    VisiblePosition modifyExtendingForward(TextGranularity);
    VisiblePosition modifyMovingRight(TextGranularity);
    VisiblePosition modifyMovingForward(TextGranularity);
    VisiblePosition modifyExtendingLeft(TextGranularity);
    VisiblePosition modifyExtendingBackward(TextGranularity);
    VisiblePosition modifyMovingLeft(TextGranularity);
    VisiblePosition modifyMovingBackward(TextGranularity);

    void updateCaretRect();
    IntRect caretRepaintRect() const;
    bool shouldRepaintCaret(const RenderView* view) const;

    int xPosForVerticalArrowNavigation(EPositionType);
    
    void notifyAccessibilityForSelectionChange();

    void focusedOrActiveStateChanged();
    bool caretRendersInsideNode(Node*) const;
    
    IntRect absoluteBoundsForLocalRect(const IntRect&) const;

    void caretBlinkTimerFired(Timer<SelectionController>*);

    void setUseSecureKeyboardEntry(bool);

    Frame* m_frame;

    int m_xPosForVerticalArrowNavigation;

    VisibleSelection m_selection;
    TextGranularity m_granularity;

    RefPtr<EditingStyle> m_typingStyle;

    Timer<SelectionController> m_caretBlinkTimer;

    IntRect m_caretRect; // caret rect in coords local to the renderer responsible for painting the caret
    IntRect m_absCaretBounds; // absolute bounding rect for the caret
    IntRect m_absoluteCaretRepaintBounds;
    
    bool m_caretRectNeedsUpdate; // true if m_caretRect and m_absCaretBounds need to be calculated
    bool m_absCaretBoundsDirty;
    bool m_isDirectional;
    bool m_isDragCaretController;
    bool m_isCaretBlinkingSuspended;
    bool m_focused;
    bool m_caretVisible;
    bool m_caretPaint;
};

inline EditingStyle* SelectionController::typingStyle() const
{
    return m_typingStyle.get();
}

inline void SelectionController::clearTypingStyle()
{
    m_typingStyle.clear();
}

inline void SelectionController::setTypingStyle(PassRefPtr<EditingStyle> style)
{
    m_typingStyle = style;
}

#if !(PLATFORM(MAC) || PLATFORM(GTK) || PLATFORM(CHROMIUM))
inline void SelectionController::notifyAccessibilityForSelectionChange()
{
}
#endif

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::SelectionController&);
void showTree(const WebCore::SelectionController*);
#endif

#endif // SelectionController_h

