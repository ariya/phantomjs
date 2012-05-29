/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "SpellingCorrectionCommand.h"

#include "SpellingCorrectionController.h"
#include "DocumentFragment.h"
#include "Frame.h"
#include "ReplaceSelectionCommand.h"
#include "SetSelectionCommand.h"
#include "TextIterator.h"
#include "markup.h"

namespace WebCore {

#if SUPPORT_AUTOCORRECTION_PANEL
// On Mac OS X, we use this command to keep track of user undoing a correction for the first time.
// This information is needed by spell checking service to update user specific data.
class SpellingCorrectionRecordUndoCommand : public SimpleEditCommand {
public:
    static PassRefPtr<SpellingCorrectionRecordUndoCommand> create(Document* document, const String& corrected, const String& correction)
    {
        return adoptRef(new SpellingCorrectionRecordUndoCommand(document, corrected, correction));
    }
private:
    SpellingCorrectionRecordUndoCommand(Document* document, const String& corrected, const String& correction)
        : SimpleEditCommand(document)
        , m_corrected(corrected)
        , m_correction(correction)
        , m_hasBeenUndone(false)
    {
    }

    virtual void doApply()
    {
    }

    virtual void doUnapply()
    {
        if (!m_hasBeenUndone) {
            document()->frame()->editor()->unappliedSpellCorrection(startingSelection(), m_corrected, m_correction);
            m_hasBeenUndone = true;
        }
        
    }

    String m_corrected;
    String m_correction;
    bool m_hasBeenUndone;
};
#endif

SpellingCorrectionCommand::SpellingCorrectionCommand(PassRefPtr<Range> rangeToBeCorrected, const String& correction)
    : CompositeEditCommand(rangeToBeCorrected->startContainer()->document())
    , m_rangeToBeCorrected(rangeToBeCorrected)
    , m_selectionToBeCorrected(m_rangeToBeCorrected.get())
    , m_correction(correction)
{
}

void SpellingCorrectionCommand::doApply()
{
    m_corrected = plainText(m_rangeToBeCorrected.get());
    if (!m_corrected.length())
        return;

    if (!document()->frame()->selection()->shouldChangeSelection(m_selectionToBeCorrected))
        return;

    RefPtr<DocumentFragment> fragment = createFragmentFromText(m_rangeToBeCorrected.get(), m_correction);
    if (!fragment)
        return;

    applyCommandToComposite(SetSelectionCommand::create(m_selectionToBeCorrected, SelectionController::SpellCorrectionTriggered | SelectionController::CloseTyping | SelectionController::ClearTypingStyle));
#if SUPPORT_AUTOCORRECTION_PANEL
    applyCommandToComposite(SpellingCorrectionRecordUndoCommand::create(document(), m_corrected, m_correction));
#endif
    applyCommandToComposite(ReplaceSelectionCommand::create(document(), fragment, ReplaceSelectionCommand::MatchStyle | ReplaceSelectionCommand::PreventNesting, EditActionPaste));
}

bool SpellingCorrectionCommand::shouldRetainAutocorrectionIndicator() const
{
    return true;
}

} // namespace WebCore
