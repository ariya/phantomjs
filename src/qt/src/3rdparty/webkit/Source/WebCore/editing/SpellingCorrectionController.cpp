/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "SpellingCorrectionController.h"

#include "DocumentMarkerController.h"
#include "EditCommand.h"
#include "EditorClient.h"
#include "Frame.h"
#include "FrameView.h"
#include "SpellingCorrectionCommand.h"
#include "TextCheckerClient.h"
#include "TextCheckingHelper.h"
#include "TextIterator.h"
#include "htmlediting.h"
#include "markup.h"
#include "visible_units.h"


namespace WebCore {

using namespace std;
using namespace WTF;

#if SUPPORT_AUTOCORRECTION_PANEL

static const Vector<DocumentMarker::MarkerType>& markerTypesForAutocorrection()
{
    DEFINE_STATIC_LOCAL(Vector<DocumentMarker::MarkerType>, markerTypesForAutoCorrection, ());
    if (markerTypesForAutoCorrection.isEmpty()) {
        markerTypesForAutoCorrection.append(DocumentMarker::Replacement);
        markerTypesForAutoCorrection.append(DocumentMarker::CorrectionIndicator);
        markerTypesForAutoCorrection.append(DocumentMarker::SpellCheckingExemption);
        markerTypesForAutoCorrection.append(DocumentMarker::Autocorrected);
    }
    return markerTypesForAutoCorrection;
}

static const Vector<DocumentMarker::MarkerType>& markerTypesForReplacement()
{
    DEFINE_STATIC_LOCAL(Vector<DocumentMarker::MarkerType>, markerTypesForReplacement, ());
    if (markerTypesForReplacement.isEmpty()) {
        markerTypesForReplacement.append(DocumentMarker::Replacement);
        markerTypesForReplacement.append(DocumentMarker::SpellCheckingExemption);
    }
    return markerTypesForReplacement;
}

static bool markersHaveIdenticalDescription(const Vector<DocumentMarker>& markers)
{
    if (markers.isEmpty())
        return true;

    const String& description = markers[0].description;
    for (size_t i = 1; i < markers.size(); ++i) {
        if (description != markers[i].description)
            return false;
    }
    return true;
}

SpellingCorrectionController::SpellingCorrectionController(Frame* frame)
    : m_frame(frame)
    , m_correctionPanelTimer(this, &SpellingCorrectionController::correctionPanelTimerFired)
{
}

SpellingCorrectionController::~SpellingCorrectionController()
{
    dismiss(ReasonForDismissingCorrectionPanelIgnored);
}

void SpellingCorrectionController::startCorrectionPanelTimer(CorrectionPanelInfo::PanelType type)
{
    const double correctionPanelTimerInterval = 0.3;
    if (!isAutomaticSpellingCorrectionEnabled())
        return;

    // If type is PanelTypeReversion, then the new range has been set. So we shouldn't clear it.
    if (type == CorrectionPanelInfo::PanelTypeCorrection)
        m_correctionPanelInfo.rangeToBeReplaced.clear();
    m_correctionPanelInfo.panelType = type;
    m_correctionPanelTimer.startOneShot(correctionPanelTimerInterval);
}

void SpellingCorrectionController::stopCorrectionPanelTimer()
{
    m_correctionPanelTimer.stop();
    m_correctionPanelInfo.rangeToBeReplaced.clear();
}

void SpellingCorrectionController::stopPendingCorrection(const VisibleSelection& oldSelection)
{
    // Make sure there's no pending autocorrection before we call markMisspellingsAndBadGrammar() below.
    VisibleSelection currentSelection(m_frame->selection()->selection());
    if (currentSelection == oldSelection)
        return;

    stopCorrectionPanelTimer();
    dismiss(ReasonForDismissingCorrectionPanelIgnored);
}

void SpellingCorrectionController::applyPendingCorrection(const VisibleSelection& selectionAfterTyping)
{
    // Apply pending autocorrection before next round of spell checking.
    bool doApplyCorrection = true;
    VisiblePosition startOfSelection = selectionAfterTyping.visibleStart();
    VisibleSelection currentWord = VisibleSelection(startOfWord(startOfSelection, LeftWordIfOnBoundary), endOfWord(startOfSelection, RightWordIfOnBoundary));
    if (currentWord.visibleEnd() == startOfSelection) {
        String wordText = plainText(currentWord.toNormalizedRange().get());
        if (wordText.length() > 0 && isAmbiguousBoundaryCharacter(wordText[wordText.length() - 1]))
            doApplyCorrection = false;
    }
    if (doApplyCorrection)
        handleCorrectionPanelResult(dismissSoon(ReasonForDismissingCorrectionPanelAccepted)); 
    else
        m_correctionPanelInfo.rangeToBeReplaced.clear();
}

bool SpellingCorrectionController::hasPendingCorrection() const
{
    return m_correctionPanelInfo.rangeToBeReplaced;
}

bool SpellingCorrectionController::isSpellingMarkerAllowed(PassRefPtr<Range> misspellingRange) const
{
    return !m_frame->document()->markers()->hasMarkers(misspellingRange.get(), DocumentMarker::SpellCheckingExemption);
}

void SpellingCorrectionController::show(PassRefPtr<Range> rangeToReplace, const String& replacement)
{
    FloatRect boundingBox = windowRectForRange(rangeToReplace.get());
    if (boundingBox.isEmpty())
        return;
    m_correctionPanelInfo.replacedString = plainText(rangeToReplace.get());
    m_correctionPanelInfo.rangeToBeReplaced = rangeToReplace;
    m_correctionPanelInfo.replacementString = replacement;
    m_correctionPanelInfo.isActive = true;
    client()->showCorrectionPanel(m_correctionPanelInfo.panelType, boundingBox, m_correctionPanelInfo.replacedString, replacement, Vector<String>());
}

void SpellingCorrectionController::handleCancelOperation()
{
    if (!m_correctionPanelInfo.isActive)
        return;
    m_correctionPanelInfo.isActive = false;
    dismiss(ReasonForDismissingCorrectionPanelCancelled);
}

void SpellingCorrectionController::dismiss(ReasonForDismissingCorrectionPanel reasonForDismissing)
{
    if (!m_correctionPanelInfo.isActive)
        return;
    m_correctionPanelInfo.isActive = false;
    m_correctionPanelIsDismissedByEditor = true;
    if (client())
        client()->dismissCorrectionPanel(reasonForDismissing);
}

String SpellingCorrectionController::dismissSoon(ReasonForDismissingCorrectionPanel reasonForDismissing)
{
    if (!m_correctionPanelInfo.isActive)
        return String();
    m_correctionPanelInfo.isActive = false;
    m_correctionPanelIsDismissedByEditor = true;
    if (!client())
        return String();
    return client()->dismissCorrectionPanelSoon(reasonForDismissing);
}

void SpellingCorrectionController::applyCorrectionPanelInfo(const Vector<DocumentMarker::MarkerType>& markerTypesToAdd)
{
    if (!m_correctionPanelInfo.rangeToBeReplaced)
        return;

    ExceptionCode ec = 0;
    RefPtr<Range> paragraphRangeContainingCorrection = m_correctionPanelInfo.rangeToBeReplaced->cloneRange(ec);
    if (ec)
        return;

    setStart(paragraphRangeContainingCorrection.get(), startOfParagraph(m_correctionPanelInfo.rangeToBeReplaced->startPosition()));
    setEnd(paragraphRangeContainingCorrection.get(), endOfParagraph(m_correctionPanelInfo.rangeToBeReplaced->endPosition()));

    // After we replace the word at range rangeToBeReplaced, we need to add markers to that range.
    // However, once the replacement took place, the value of rangeToBeReplaced is not valid anymore.
    // So before we carry out the replacement, we need to store the start position of rangeToBeReplaced
    // relative to the start position of the containing paragraph. We use correctionStartOffsetInParagraph
    // to store this value. In order to obtain this offset, we need to first create a range
    // which spans from the start of paragraph to the start position of rangeToBeReplaced.
    RefPtr<Range> correctionStartOffsetInParagraphAsRange = Range::create(paragraphRangeContainingCorrection->startContainer(ec)->document(), paragraphRangeContainingCorrection->startPosition(), paragraphRangeContainingCorrection->startPosition());
    if (ec)
        return;

    Position startPositionOfRangeToBeReplaced = m_correctionPanelInfo.rangeToBeReplaced->startPosition();
    correctionStartOffsetInParagraphAsRange->setEnd(startPositionOfRangeToBeReplaced.containerNode(), startPositionOfRangeToBeReplaced.computeOffsetInContainerNode(), ec);
    if (ec)
        return;

    // Take note of the location of autocorrection so that we can add marker after the replacement took place.
    int correctionStartOffsetInParagraph = TextIterator::rangeLength(correctionStartOffsetInParagraphAsRange.get());

    // Clone the range, since the caller of this method may want to keep the original range around.
    RefPtr<Range> rangeToBeReplaced = m_correctionPanelInfo.rangeToBeReplaced->cloneRange(ec);
    applyCommand(SpellingCorrectionCommand::create(rangeToBeReplaced, m_correctionPanelInfo.replacementString));
    setEnd(paragraphRangeContainingCorrection.get(), m_frame->selection()->selection().start());
    RefPtr<Range> replacementRange = TextIterator::subrange(paragraphRangeContainingCorrection.get(), correctionStartOffsetInParagraph,  m_correctionPanelInfo.replacementString.length());
    String newText = plainText(replacementRange.get());

    // Check to see if replacement succeeded.
    if (newText != m_correctionPanelInfo.replacementString)
        return;

    DocumentMarkerController* markers = replacementRange->startContainer()->document()->markers();
    size_t size = markerTypesToAdd.size();
    for (size_t i = 0; i < size; ++i) {
        DocumentMarker::MarkerType markerType = markerTypesToAdd[i];
        String description;
        if (m_correctionPanelInfo.panelType != CorrectionPanelInfo::PanelTypeReversion && (markerType == DocumentMarker::Replacement || markerType == DocumentMarker::Autocorrected))
            description = m_correctionPanelInfo.replacedString;
        markers->addMarker(replacementRange.get(), markerType, description);
    }
}

bool SpellingCorrectionController::applyAutocorrectionBeforeTypingIfAppropriate()
{
    if (!m_correctionPanelInfo.rangeToBeReplaced || !m_correctionPanelInfo.isActive)
        return false;

    if (m_correctionPanelInfo.panelType != CorrectionPanelInfo::PanelTypeCorrection)
        return false;

    Position caretPosition = m_frame->selection()->selection().start();

    if (m_correctionPanelInfo.rangeToBeReplaced->endPosition() == caretPosition) {
        handleCorrectionPanelResult(dismissSoon(ReasonForDismissingCorrectionPanelAccepted));
        return true;
    } 
    
    // Pending correction should always be where caret is. But in case this is not always true, we still want to dismiss the panel without accepting the correction.
    ASSERT(m_correctionPanelInfo.rangeToBeReplaced->endPosition() == caretPosition);
    dismiss(ReasonForDismissingCorrectionPanelIgnored);
    return false;
}

void SpellingCorrectionController::respondToUnappliedSpellCorrection(const VisibleSelection& selectionOfCorrected, const String& corrected, const String& correction)
{
    client()->recordAutocorrectionResponse(EditorClient::AutocorrectionReverted, corrected, correction);
    m_frame->document()->updateLayout();
    m_frame->selection()->setSelection(selectionOfCorrected, SelectionController::CloseTyping | SelectionController::ClearTypingStyle | SelectionController::SpellCorrectionTriggered);
    RefPtr<Range> range = Range::create(m_frame->document(), m_frame->selection()->selection().start(), m_frame->selection()->selection().end());

    DocumentMarkerController* markers = m_frame->document()->markers();
    markers->removeMarkers(range.get(), DocumentMarker::Spelling | DocumentMarker::Autocorrected, DocumentMarkerController::RemovePartiallyOverlappingMarker);
    markers->addMarker(range.get(), DocumentMarker::Replacement);
    markers->addMarker(range.get(), DocumentMarker::SpellCheckingExemption);
}

void SpellingCorrectionController::correctionPanelTimerFired(Timer<SpellingCorrectionController>*)
{
    m_correctionPanelIsDismissedByEditor = false;
    switch (m_correctionPanelInfo.panelType) {
    case CorrectionPanelInfo::PanelTypeCorrection: {
        VisibleSelection selection(m_frame->selection()->selection());
        VisiblePosition start(selection.start(), selection.affinity());
        VisiblePosition p = startOfWord(start, LeftWordIfOnBoundary);
        VisibleSelection adjacentWords = VisibleSelection(p, start);
        m_frame->editor()->markAllMisspellingsAndBadGrammarInRanges(Editor::MarkSpelling | Editor::ShowCorrectionPanel, adjacentWords.toNormalizedRange().get(), 0);
    }
        break;
    case CorrectionPanelInfo::PanelTypeReversion: {
        if (!m_correctionPanelInfo.rangeToBeReplaced)
            break;
        m_correctionPanelInfo.isActive = true;
        m_correctionPanelInfo.replacedString = plainText(m_correctionPanelInfo.rangeToBeReplaced.get());
        FloatRect boundingBox = windowRectForRange(m_correctionPanelInfo.rangeToBeReplaced.get());
        if (!boundingBox.isEmpty())
            client()->showCorrectionPanel(m_correctionPanelInfo.panelType, boundingBox, m_correctionPanelInfo.replacedString, m_correctionPanelInfo.replacementString, Vector<String>());
    }
        break;
    case CorrectionPanelInfo::PanelTypeSpellingSuggestions: {
        if (!m_correctionPanelInfo.rangeToBeReplaced || plainText(m_correctionPanelInfo.rangeToBeReplaced.get()) != m_correctionPanelInfo.replacedString)
            break;
        String paragraphText = plainText(TextCheckingParagraph(m_correctionPanelInfo.rangeToBeReplaced).paragraphRange().get());
        Vector<String> suggestions;
        textChecker()->getGuessesForWord(m_correctionPanelInfo.replacedString, paragraphText, suggestions);
        if (suggestions.isEmpty()) {
            m_correctionPanelInfo.rangeToBeReplaced.clear();
            break;
        }
        String topSuggestion = suggestions.first();
        suggestions.remove(0);
        m_correctionPanelInfo.isActive = true;
        FloatRect boundingBox = windowRectForRange(m_correctionPanelInfo.rangeToBeReplaced.get());
        if (!boundingBox.isEmpty())
            client()->showCorrectionPanel(m_correctionPanelInfo.panelType, boundingBox, m_correctionPanelInfo.replacedString, topSuggestion, suggestions);
    }
        break;
    }
}

void SpellingCorrectionController::handleCorrectionPanelResult(const String& correction)
{
    Range* replacedRange = m_correctionPanelInfo.rangeToBeReplaced.get();
    if (!replacedRange || m_frame->document() != replacedRange->ownerDocument())
        return;

    String currentWord = plainText(m_correctionPanelInfo.rangeToBeReplaced.get());
    // Check to see if the word we are about to correct has been changed between timer firing and callback being triggered.
    if (currentWord != m_correctionPanelInfo.replacedString)
        return;

    m_correctionPanelInfo.isActive = false;

    switch (m_correctionPanelInfo.panelType) {
    case CorrectionPanelInfo::PanelTypeCorrection:
        if (correction.length()) {
            m_correctionPanelInfo.replacementString = correction;
            applyCorrectionPanelInfo(markerTypesForAutocorrection());
        } else if (!m_correctionPanelIsDismissedByEditor)
            replacedRange->startContainer()->document()->markers()->addMarker(replacedRange, DocumentMarker::RejectedCorrection, m_correctionPanelInfo.replacedString);
        break;
    case CorrectionPanelInfo::PanelTypeReversion:
    case CorrectionPanelInfo::PanelTypeSpellingSuggestions:
        if (correction.length()) {
            m_correctionPanelInfo.replacementString = correction;
            applyCorrectionPanelInfo(markerTypesForReplacement());
        }
        break;
    }

    m_correctionPanelInfo.rangeToBeReplaced.clear();
}

bool SpellingCorrectionController::isAutomaticSpellingCorrectionEnabled()
{
    return client() && client()->isAutomaticSpellingCorrectionEnabled();
}

FloatRect SpellingCorrectionController::windowRectForRange(const Range* range) const
{
    FrameView* view = m_frame->view();
    if (!view)
        return FloatRect();
    Vector<FloatQuad> textQuads;
    range->textQuads(textQuads);
    FloatRect boundingRect;
    size_t size = textQuads.size();
    for (size_t i = 0; i < size; ++i)
        boundingRect.unite(textQuads[i].boundingBox());
    return view->contentsToWindow(IntRect(boundingRect));
}        

void SpellingCorrectionController::respondToChangedSelection(const VisibleSelection& oldSelection)
{
    VisibleSelection currentSelection(m_frame->selection()->selection());
    // When user moves caret to the end of autocorrected word and pauses, we show the panel
    // containing the original pre-correction word so that user can quickly revert the
    // undesired autocorrection. Here, we start correction panel timer once we confirm that
    // the new caret position is at the end of a word.
    if (!currentSelection.isCaret() || currentSelection == oldSelection)
        return;

    VisiblePosition selectionPosition = currentSelection.start();
    
    // Creating a Visible position triggers a layout and there is no
    // guarantee that the selection is still valid.
    if (selectionPosition.isNull())
        return;
    
    VisiblePosition endPositionOfWord = endOfWord(selectionPosition, LeftWordIfOnBoundary);
    if (selectionPosition != endPositionOfWord)
        return;

    Position position = endPositionOfWord.deepEquivalent();
    if (position.anchorType() != Position::PositionIsOffsetInAnchor)
        return;

    Node* node = position.containerNode();
    int endOffset = position.offsetInContainerNode();
    Vector<DocumentMarker> markers = node->document()->markers()->markersForNode(node);
    size_t markerCount = markers.size();
    for (size_t i = 0; i < markerCount; ++i) {
        const DocumentMarker& marker = markers[i];
        if (!shouldStartTimerFor(marker, endOffset))
            continue;
        RefPtr<Range> wordRange = Range::create(m_frame->document(), node, marker.startOffset, node, marker.endOffset);
        String currentWord = plainText(wordRange.get());
        if (!currentWord.length())
            continue;

        m_correctionPanelInfo.rangeToBeReplaced = wordRange;
        m_correctionPanelInfo.replacedString = currentWord;
        if (marker.type == DocumentMarker::Spelling)
            startCorrectionPanelTimer(CorrectionPanelInfo::PanelTypeSpellingSuggestions);
        else {
            m_correctionPanelInfo.replacementString = marker.description;
            startCorrectionPanelTimer(CorrectionPanelInfo::PanelTypeReversion);
        }

        break;
    }
}

void SpellingCorrectionController::respondToAppliedEditing(EditCommand* command)
{
    if (command->isTopLevelCommand() && !command->shouldRetainAutocorrectionIndicator())
        m_frame->document()->markers()->removeMarkers(DocumentMarker::CorrectionIndicator);
}

void SpellingCorrectionController::respondToUnappliedEditing(EditCommand* command)
{
    if (!command->isCreateLinkCommand())
        return;
    RefPtr<Range> range = Range::create(m_frame->document(), command->startingSelection().start(), command->startingSelection().end());
    if (!range)
        return;
    DocumentMarkerController* markers = m_frame->document()->markers();
    markers->addMarker(range.get(), DocumentMarker::Replacement);
    markers->addMarker(range.get(), DocumentMarker::SpellCheckingExemption);
}

EditorClient* SpellingCorrectionController::client()
{
    return m_frame->page() ? m_frame->page()->editorClient() : 0;
}

TextCheckerClient* SpellingCorrectionController::textChecker()
{
    if (EditorClient* owner = client())
        return owner->textChecker();
    return 0;
}

void SpellingCorrectionController::recordAutocorrectionResponseReversed(const String& replacedString, const String& replacementString)
{
    client()->recordAutocorrectionResponse(EditorClient::AutocorrectionReverted, replacedString, replacementString);
}

void SpellingCorrectionController::recordAutocorrectionResponseReversed(const String& replacedString, PassRefPtr<Range> replacementRange)
{
    recordAutocorrectionResponseReversed(replacedString, plainText(replacementRange.get()));
}

void SpellingCorrectionController::markReversed(PassRefPtr<Range> changedRange)
{
    changedRange->startContainer()->document()->markers()->removeMarkers(changedRange.get(), DocumentMarker::Autocorrected, DocumentMarkerController::RemovePartiallyOverlappingMarker);
    changedRange->startContainer()->document()->markers()->addMarker(changedRange.get(), DocumentMarker::SpellCheckingExemption);
}

void SpellingCorrectionController::markCorrection(PassRefPtr<Range> replacedRange, const String& replacedString)
{
    Vector<DocumentMarker::MarkerType> markerTypesToAdd = markerTypesForAutocorrection();
    DocumentMarkerController* markers = replacedRange->startContainer()->document()->markers();
    for (size_t i = 0; i < markerTypesToAdd.size(); ++i) {
        DocumentMarker::MarkerType markerType = markerTypesToAdd[i];
        if (markerType == DocumentMarker::Replacement || markerType == DocumentMarker::Autocorrected)
            markers->addMarker(replacedRange.get(), markerType, replacedString);
        else
            markers->addMarker(replacedRange.get(), markerType);
    }
}

void SpellingCorrectionController::recordSpellcheckerResponseForModifiedCorrection(Range* rangeOfCorrection, const String& corrected, const String& correction)
{
    if (!rangeOfCorrection)
        return;
    DocumentMarkerController* markers = rangeOfCorrection->startContainer()->document()->markers();
    Vector<DocumentMarker> correctedOnceMarkers = markers->markersInRange(rangeOfCorrection, DocumentMarker::Autocorrected);
    if (correctedOnceMarkers.isEmpty())
        return;
    
    // Spelling corrected text has been edited. We need to determine whether user has reverted it to original text or
    // edited it to something else, and notify spellchecker accordingly.
    if (markersHaveIdenticalDescription(correctedOnceMarkers) && correctedOnceMarkers[0].description == corrected)
        client()->recordAutocorrectionResponse(EditorClient::AutocorrectionReverted, corrected, correction);
    else
        client()->recordAutocorrectionResponse(EditorClient::AutocorrectionEdited, corrected, correction);
    markers->removeMarkers(rangeOfCorrection, DocumentMarker::Autocorrected, DocumentMarkerController::RemovePartiallyOverlappingMarker);
}

#endif

} // namespace WebCore
