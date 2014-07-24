/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef EditingBehavior_h
#define EditingBehavior_h

#include "EditingBehaviorTypes.h"

namespace WebCore {

class EditingBehavior {

public:
    explicit EditingBehavior(EditingBehaviorType type)
        : m_type(type)
    {
    }

    // Individual functions for each case where we have more than one style of editing behavior.
    // Create a new function for any platform difference so we can control it here.

    // When extending a selection beyond the top or bottom boundary of an editable area,
    // maintain the horizontal position on Windows but extend it to the boundary of the editable
    // content on Mac.
    bool shouldMoveCaretToHorizontalBoundaryWhenPastTopOrBottom() const
    {
        return m_type != EditingWindowsBehavior;
    }

    // On Windows, selections should always be considered as directional, regardless if it is
    // mouse-based or keyboard-based.
    bool shouldConsiderSelectionAsDirectional() const { return m_type != EditingMacBehavior; }

    // On Mac, when revealing a selection (for example as a result of a Find operation on the Browser),
    // content should be scrolled such that the selection gets certer aligned.
    bool shouldCenterAlignWhenSelectionIsRevealed() const { return m_type == EditingMacBehavior; }

    // On Mac, style is considered present when present at the beginning of selection. On other platforms,
    // style has to be present throughout the selection.
    bool shouldToggleStyleBasedOnStartOfSelection() const { return m_type == EditingMacBehavior; }

    // Standard Mac behavior when extending to a boundary is grow the selection rather than leaving the base
    // in place and moving the extent. Matches NSTextView.
    bool shouldAlwaysGrowSelectionWhenExtendingToBoundary() const { return m_type == EditingMacBehavior; }

    // On Mac, when processing a contextual click, the object being clicked upon should be selected.
    bool shouldSelectOnContextualMenuClick() const { return m_type == EditingMacBehavior; }

    // On Linux, should be able to get and insert spelling suggestions without selecting the misspelled word.
    bool shouldAllowSpellingSuggestionsWithoutSelection() const
    {
        return m_type == EditingUnixBehavior;
    }
    
    // On Mac and Windows, pressing backspace (when it isn't handled otherwise) should navigate back.
    bool shouldNavigateBackOnBackspace() const
    {
        return m_type != EditingUnixBehavior;
    }

    // On Mac, selecting backwards by word/line from the middle of a word/line, and then going
    // forward leaves the caret back in the middle with no selection, instead of directly selecting
    // to the other end of the line/word (Unix/Windows behavior).
    bool shouldExtendSelectionByWordOrLineAcrossCaret() const { return m_type != EditingMacBehavior; }

    // Based on native behavior, when using ctrl(alt)+arrow to move caret by word, ctrl(alt)+left arrow moves caret to
    // immediately before the word in all platforms, for example, the word break positions are: "|abc |def |hij |opq".
    // But ctrl+right arrow moves caret to "abc |def |hij |opq" on Windows and "abc| def| hij| opq|" on Mac and Linux.
    bool shouldSkipSpaceWhenMovingRight() const { return m_type == EditingWindowsBehavior; }

private:
    EditingBehaviorType m_type;
};

} // namespace WebCore

#endif // EditingBehavior_h
