/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2005 Alexey Proskuryakov.
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
#include "TextIterator.h"

#include "Document.h"
#include "Frame.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "htmlediting.h"
#include "InlineTextBox.h"
#include "Range.h"
#include "RenderTableCell.h"
#include "RenderTableRow.h"
#include "RenderTextControl.h"
#include "RenderTextFragment.h"
#include "TextBoundaries.h"
#include "TextBreakIterator.h"
#include "VisiblePosition.h"
#include "visible_units.h"
#include <wtf/unicode/CharacterNames.h>

#if USE(ICU_UNICODE) && !UCONFIG_NO_COLLATION
#include "TextBreakIteratorInternalICU.h"
#include <unicode/usearch.h>
#endif

using namespace WTF::Unicode;
using namespace std;

namespace WebCore {

using namespace HTMLNames;

// Buffer that knows how to compare with a search target.
// Keeps enough of the previous text to be able to search in the future, but no more.
// Non-breaking spaces are always equal to normal spaces.
// Case folding is also done if the CaseInsensitive option is specified.
// Matches are further filtered if the AtWordStarts option is specified, although some
// matches inside a word are permitted if TreatMedialCapitalAsWordStart is specified as well.
class SearchBuffer {
    WTF_MAKE_NONCOPYABLE(SearchBuffer);
public:
    SearchBuffer(const String& target, FindOptions);
    ~SearchBuffer();

    // Returns number of characters appended; guaranteed to be in the range [1, length].
    size_t append(const UChar*, size_t length);
    bool needsMoreContext() const;
    void prependContext(const UChar*, size_t length);
    void reachedBreak();

    // Result is the size in characters of what was found.
    // And <startOffset> is the number of characters back to the start of what was found.
    size_t search(size_t& startOffset);
    bool atBreak() const;

#if USE(ICU_UNICODE) && !UCONFIG_NO_COLLATION

private:
    bool isBadMatch(const UChar*, size_t length) const;
    bool isWordStartMatch(size_t start, size_t length) const;

    String m_target;
    FindOptions m_options;

    Vector<UChar> m_buffer;
    size_t m_overlap;
    size_t m_prefixLength;
    bool m_atBreak;
    bool m_needsMoreContext;

    bool m_targetRequiresKanaWorkaround;
    Vector<UChar> m_normalizedTarget;
    mutable Vector<UChar> m_normalizedMatch;

#else

private:
    void append(UChar, bool isCharacterStart);
    size_t length() const;

    String m_target;
    FindOptions m_options;

    Vector<UChar> m_buffer;
    Vector<bool> m_isCharacterStartBuffer;
    bool m_isBufferFull;
    size_t m_cursor;

#endif
};

// --------

static const unsigned bitsInWord = sizeof(unsigned) * 8;
static const unsigned bitInWordMask = bitsInWord - 1;

BitStack::BitStack()
    : m_size(0)
{
}

BitStack::~BitStack()
{
}

void BitStack::push(bool bit)
{
    unsigned index = m_size / bitsInWord;
    unsigned shift = m_size & bitInWordMask;
    if (!shift && index == m_words.size()) {
        m_words.grow(index + 1);
        m_words[index] = 0;
    }
    unsigned& word = m_words[index];
    unsigned mask = 1U << shift;
    if (bit)
        word |= mask;
    else
        word &= ~mask;
    ++m_size;
}

void BitStack::pop()
{
    if (m_size)
        --m_size;
}

bool BitStack::top() const
{
    if (!m_size)
        return false;
    unsigned shift = (m_size - 1) & bitInWordMask;
    return m_words.last() & (1U << shift);
}

unsigned BitStack::size() const
{
    return m_size;
}

// --------

#if !ASSERT_DISABLED

static unsigned depthCrossingShadowBoundaries(Node* node)
{
    unsigned depth = 0;
    for (Node* parent = node->parentOrHostNode(); parent; parent = parent->parentOrHostNode())
        ++depth;
    return depth;
}

#endif

// This function is like Range::pastLastNode, except for the fact that it can climb up out of shadow trees.
static Node* nextInPreOrderCrossingShadowBoundaries(Node* rangeEndContainer, int rangeEndOffset)
{
    if (!rangeEndContainer)
        return 0;
    if (rangeEndOffset >= 0 && !rangeEndContainer->offsetInCharacters()) {
        if (Node* next = rangeEndContainer->childNode(rangeEndOffset))
            return next;
    }
    for (Node* node = rangeEndContainer; node; node = node->parentOrHostNode()) {
        if (Node* next = node->nextSibling())
            return next;
    }
    return 0;
}

// --------

static inline bool fullyClipsContents(Node* node)
{
    RenderObject* renderer = node->renderer();
    if (!renderer || !renderer->isBox() || !renderer->hasOverflowClip())
        return false;
    return toRenderBox(renderer)->size().isEmpty();
}

static inline bool ignoresContainerClip(Node* node)
{
    RenderObject* renderer = node->renderer();
    if (!renderer || renderer->isText())
        return false;
    EPosition position = renderer->style()->position();
    return position == AbsolutePosition || position == FixedPosition;
}

static void pushFullyClippedState(BitStack& stack, Node* node)
{
    ASSERT(stack.size() == depthCrossingShadowBoundaries(node));

    // Push true if this node full clips its contents, or if a parent already has fully
    // clipped and this is not a node that ignores its container's clip.
    stack.push(fullyClipsContents(node) || (stack.top() && !ignoresContainerClip(node)));
}

static void setUpFullyClippedStack(BitStack& stack, Node* node)
{
    // Put the nodes in a vector so we can iterate in reverse order.
    Vector<Node*, 100> ancestry;
    for (Node* parent = node->parentOrHostNode(); parent; parent = parent->parentOrHostNode())
        ancestry.append(parent);

    // Call pushFullyClippedState on each node starting with the earliest ancestor.
    size_t size = ancestry.size();
    for (size_t i = 0; i < size; ++i)
        pushFullyClippedState(stack, ancestry[size - i - 1]);
    pushFullyClippedState(stack, node);

    ASSERT(stack.size() == 1 + depthCrossingShadowBoundaries(node));
}

// --------

TextIterator::TextIterator()
    : m_startContainer(0)
    , m_startOffset(0)
    , m_endContainer(0)
    , m_endOffset(0)
    , m_positionNode(0)
    , m_textCharacters(0)
    , m_textLength(0)
    , m_remainingTextBox(0)
    , m_firstLetterText(0)
    , m_lastCharacter(0)
    , m_emitsCharactersBetweenAllVisiblePositions(false)
    , m_entersTextControls(false)
    , m_emitsTextWithoutTranscoding(false)
    , m_handledFirstLetter(false)
    , m_ignoresStyleVisibility(false)
    , m_emitsObjectReplacementCharacters(false)
{
}

TextIterator::TextIterator(const Range* r, TextIteratorBehavior behavior)
    : m_startContainer(0)
    , m_startOffset(0)
    , m_endContainer(0)
    , m_endOffset(0)
    , m_positionNode(0)
    , m_textCharacters(0)
    , m_textLength(0)
    , m_remainingTextBox(0)
    , m_firstLetterText(0)
    , m_emitsCharactersBetweenAllVisiblePositions(behavior & TextIteratorEmitsCharactersBetweenAllVisiblePositions)
    , m_entersTextControls(behavior & TextIteratorEntersTextControls)
    , m_emitsTextWithoutTranscoding(behavior & TextIteratorEmitsTextsWithoutTranscoding)
    , m_handledFirstLetter(false)
    , m_ignoresStyleVisibility(behavior & TextIteratorIgnoresStyleVisibility)
    , m_emitsObjectReplacementCharacters(behavior & TextIteratorEmitsObjectReplacementCharacters)
{
    if (!r)
        return;

    // get and validate the range endpoints
    Node* startContainer = r->startContainer();
    if (!startContainer)
        return;
    int startOffset = r->startOffset();
    Node* endContainer = r->endContainer();
    int endOffset = r->endOffset();

    // Callers should be handing us well-formed ranges. If we discover that this isn't
    // the case, we could consider changing this assertion to an early return.
    ASSERT(r->boundaryPointsValid());

    // remember range - this does not change
    m_startContainer = startContainer;
    m_startOffset = startOffset;
    m_endContainer = endContainer;
    m_endOffset = endOffset;

    // set up the current node for processing
    m_node = r->firstNode();
    if (!m_node)
        return;
    setUpFullyClippedStack(m_fullyClippedStack, m_node);
    m_offset = m_node == m_startContainer ? m_startOffset : 0;
    m_handledNode = false;
    m_handledChildren = false;

    // calculate first out of bounds node
    m_pastEndNode = nextInPreOrderCrossingShadowBoundaries(endContainer, endOffset);

    // initialize node processing state
    m_needsAnotherNewline = false;
    m_textBox = 0;

    // initialize record of previous node processing
    m_hasEmitted = false;
    m_lastTextNode = 0;
    m_lastTextNodeEndedWithCollapsedSpace = false;
    m_lastCharacter = 0;

#ifndef NDEBUG
    // need this just because of the assert in advance()
    m_positionNode = m_node;
#endif

    // identify the first run
    advance();
}

TextIterator::~TextIterator()
{
}

void TextIterator::advance()
{
    // reset the run information
    m_positionNode = 0;
    m_textLength = 0;

    // handle remembered node that needed a newline after the text node's newline
    if (m_needsAnotherNewline) {
        // Emit the extra newline, and position it *inside* m_node, after m_node's 
        // contents, in case it's a block, in the same way that we position the first 
        // newline.  The range for the emitted newline should start where the line 
        // break begins.
        // FIXME: It would be cleaner if we emitted two newlines during the last 
        // iteration, instead of using m_needsAnotherNewline.
        Node* baseNode = m_node->lastChild() ? m_node->lastChild() : m_node;
        emitCharacter('\n', baseNode->parentNode(), baseNode, 1, 1);
        m_needsAnotherNewline = false;
        return;
    }

    if (!m_textBox && m_remainingTextBox) {
        m_textBox = m_remainingTextBox;
        m_remainingTextBox = 0;
        m_firstLetterText = 0;
        m_offset = 0;
    }
    // handle remembered text box
    if (m_textBox) {
        handleTextBox();
        if (m_positionNode)
            return;
    }

    while (m_node && m_node != m_pastEndNode) {
        // if the range ends at offset 0 of an element, represent the
        // position, but not the content, of that element e.g. if the
        // node is a blockflow element, emit a newline that
        // precedes the element
        if (m_node == m_endContainer && m_endOffset == 0) {
            representNodeOffsetZero();
            m_node = 0;
            return;
        }
        
        RenderObject* renderer = m_node->renderer();
        if (!renderer) {
            m_handledNode = true;
            m_handledChildren = true;
        } else {
            // handle current node according to its type
            if (!m_handledNode) {
                if (renderer->isText() && m_node->nodeType() == Node::TEXT_NODE) // FIXME: What about CDATA_SECTION_NODE?
                    m_handledNode = handleTextNode();
                else if (renderer && (renderer->isImage() || renderer->isWidget() ||
                         (renderer->node() && renderer->node()->isElementNode() &&
                          static_cast<Element*>(renderer->node())->isFormControlElement())))
                    m_handledNode = handleReplacedElement();
                else
                    m_handledNode = handleNonTextNode();
                if (m_positionNode)
                    return;
            }
        }

        // find a new current node to handle in depth-first manner,
        // calling exitNode() as we come back thru a parent node
        Node* next = m_handledChildren ? 0 : m_node->firstChild();
        m_offset = 0;
        if (!next) {
            next = m_node->nextSibling();
            if (!next) {
                bool pastEnd = m_node->traverseNextNode() == m_pastEndNode;
                Node* parentNode = m_node->parentOrHostNode();
                while (!next && parentNode) {
                    if ((pastEnd && parentNode == m_endContainer) || m_endContainer->isDescendantOf(parentNode))
                        return;
                    bool haveRenderer = m_node->renderer();
                    m_node = parentNode;
                    m_fullyClippedStack.pop();
                    parentNode = m_node->parentOrHostNode();
                    if (haveRenderer)
                        exitNode();
                    if (m_positionNode) {
                        m_handledNode = true;
                        m_handledChildren = true;
                        return;
                    }
                    next = m_node->nextSibling();
                }
            }
            m_fullyClippedStack.pop();            
        }

        // set the new current node
        m_node = next;
        if (m_node)
            pushFullyClippedState(m_fullyClippedStack, m_node);
        m_handledNode = false;
        m_handledChildren = false;
        m_handledFirstLetter = false;
        m_firstLetterText = 0;

        // how would this ever be?
        if (m_positionNode)
            return;
    }
}

bool TextIterator::handleTextNode()
{
    if (m_fullyClippedStack.top() && !m_ignoresStyleVisibility)
        return false;

    RenderText* renderer = toRenderText(m_node->renderer());
        
    m_lastTextNode = m_node;
    String str = renderer->text();

    // handle pre-formatted text
    if (!renderer->style()->collapseWhiteSpace()) {
        int runStart = m_offset;
        if (m_lastTextNodeEndedWithCollapsedSpace && hasVisibleTextNode(renderer)) {
            emitCharacter(' ', m_node, 0, runStart, runStart);
            return false;
        }
        if (!m_handledFirstLetter && renderer->isTextFragment() && !m_offset) {
            handleTextNodeFirstLetter(static_cast<RenderTextFragment*>(renderer));
            if (m_firstLetterText) {
                String firstLetter = m_firstLetterText->text();
                emitText(m_node, m_firstLetterText, m_offset, m_offset + firstLetter.length());
                m_firstLetterText = 0;
                m_textBox = 0;
                return false;
            }
        }
        if (renderer->style()->visibility() != VISIBLE && !m_ignoresStyleVisibility)
            return false;
        int strLength = str.length();
        int end = (m_node == m_endContainer) ? m_endOffset : INT_MAX;
        int runEnd = min(strLength, end);

        if (runStart >= runEnd)
            return true;

        emitText(m_node, runStart, runEnd);
        return true;
    }

    if (!renderer->firstTextBox() && str.length() > 0) {
        if (!m_handledFirstLetter && renderer->isTextFragment()) {
            handleTextNodeFirstLetter(static_cast<RenderTextFragment*>(renderer));
            if (m_firstLetterText) {
                handleTextBox();
                return false;
            }
        }
        if (renderer->style()->visibility() != VISIBLE && !m_ignoresStyleVisibility)
            return false;
        m_lastTextNodeEndedWithCollapsedSpace = true; // entire block is collapsed space
        return true;
    }

    
    m_textBox = renderer->firstTextBox();
    if (!m_handledFirstLetter && renderer->isTextFragment() && !m_offset)
        handleTextNodeFirstLetter(static_cast<RenderTextFragment*>(renderer));

    if (m_firstLetterText)
        renderer = m_firstLetterText;

    // Used when text boxes are out of order (Hebrew/Arabic w/ embeded LTR text)
    if (renderer->containsReversedText()) {
        m_sortedTextBoxes.clear();
        for (InlineTextBox* textBox = renderer->firstTextBox(); textBox; textBox = textBox->nextTextBox()) {
            m_sortedTextBoxes.append(textBox);
        }
        std::sort(m_sortedTextBoxes.begin(), m_sortedTextBoxes.end(), InlineTextBox::compareByStart); 
        m_sortedTextBoxesPosition = 0;
        m_textBox = m_sortedTextBoxes.isEmpty() ? 0 : m_sortedTextBoxes[0];
    }

    handleTextBox();
    return true;
}

void TextIterator::handleTextBox()
{    
    RenderText* renderer = m_firstLetterText ? m_firstLetterText : toRenderText(m_node->renderer());
    if (renderer->style()->visibility() != VISIBLE && !m_ignoresStyleVisibility) {
        m_textBox = 0;
        return;
    }
    String str = renderer->text();
    unsigned start = m_offset;
    unsigned end = (m_node == m_endContainer) ? static_cast<unsigned>(m_endOffset) : UINT_MAX;
    while (m_textBox) {
        unsigned textBoxStart = m_textBox->start();
        unsigned runStart = max(textBoxStart, start);

        // Check for collapsed space at the start of this run.
        InlineTextBox* firstTextBox = renderer->containsReversedText() ? (m_sortedTextBoxes.isEmpty() ? 0 : m_sortedTextBoxes[0]) : renderer->firstTextBox();
        bool needSpace = m_lastTextNodeEndedWithCollapsedSpace
            || (m_textBox == firstTextBox && textBoxStart == runStart && runStart > 0);
        if (needSpace && !isCollapsibleWhitespace(m_lastCharacter) && m_lastCharacter) {
            if (m_lastTextNode == m_node && runStart > 0 && str[runStart - 1] == ' ') {
                unsigned spaceRunStart = runStart - 1;
                while (spaceRunStart > 0 && str[spaceRunStart - 1] == ' ')
                    --spaceRunStart;
                emitText(m_node, renderer, spaceRunStart, spaceRunStart + 1);
            } else
                emitCharacter(' ', m_node, 0, runStart, runStart);
            return;
        }
        unsigned textBoxEnd = textBoxStart + m_textBox->len();
        unsigned runEnd = min(textBoxEnd, end);
        
        // Determine what the next text box will be, but don't advance yet
        InlineTextBox* nextTextBox = 0;
        if (renderer->containsReversedText()) {
            if (m_sortedTextBoxesPosition + 1 < m_sortedTextBoxes.size())
                nextTextBox = m_sortedTextBoxes[m_sortedTextBoxesPosition + 1];
        } else 
            nextTextBox = m_textBox->nextTextBox();

        if (runStart < runEnd) {
            // Handle either a single newline character (which becomes a space),
            // or a run of characters that does not include a newline.
            // This effectively translates newlines to spaces without copying the text.
            if (str[runStart] == '\n') {
                emitCharacter(' ', m_node, 0, runStart, runStart + 1);
                m_offset = runStart + 1;
            } else {
                size_t subrunEnd = str.find('\n', runStart);
                if (subrunEnd == notFound || subrunEnd > runEnd)
                    subrunEnd = runEnd;
    
                m_offset = subrunEnd;
                emitText(m_node, renderer, runStart, subrunEnd);
            }

            // If we are doing a subrun that doesn't go to the end of the text box,
            // come back again to finish handling this text box; don't advance to the next one.
            if (static_cast<unsigned>(m_positionEndOffset) < textBoxEnd)
                return;

            // Advance and return
            unsigned nextRunStart = nextTextBox ? nextTextBox->start() : str.length();
            if (nextRunStart > runEnd)
                m_lastTextNodeEndedWithCollapsedSpace = true; // collapsed space between runs or at the end
            m_textBox = nextTextBox;
            if (renderer->containsReversedText())
                ++m_sortedTextBoxesPosition;
            return;
        }
        // Advance and continue
        m_textBox = nextTextBox;
        if (renderer->containsReversedText())
            ++m_sortedTextBoxesPosition;
    }
    if (!m_textBox && m_remainingTextBox) {
        m_textBox = m_remainingTextBox;
        m_remainingTextBox = 0;
        m_firstLetterText = 0;
        m_offset = 0;
        handleTextBox();
    }
}

static inline RenderText* firstRenderTextInFirstLetter(RenderObject* firstLetter)
{
    if (!firstLetter)
        return 0;

    // FIXME: Should this check descendent objects?
    for (RenderObject* current = firstLetter->firstChild(); current; current = current->nextSibling()) {
        if (current->isText())
            return toRenderText(current);
    }
    return 0;
}

void TextIterator::handleTextNodeFirstLetter(RenderTextFragment* renderer)
{
    if (renderer->firstLetter()) {
        RenderObject* r = renderer->firstLetter();
        if (r->style()->visibility() != VISIBLE && !m_ignoresStyleVisibility)
            return;
        if (RenderText* firstLetter = firstRenderTextInFirstLetter(r)) {
            m_handledFirstLetter = true;
            m_remainingTextBox = m_textBox;
            m_textBox = firstLetter->firstTextBox();
            m_firstLetterText = firstLetter;
        }
    }
    m_handledFirstLetter = true;
}

bool TextIterator::handleReplacedElement()
{
    if (m_fullyClippedStack.top())
        return false;

    RenderObject* renderer = m_node->renderer();
    if (renderer->style()->visibility() != VISIBLE && !m_ignoresStyleVisibility)
        return false;

    if (m_lastTextNodeEndedWithCollapsedSpace) {
        emitCharacter(' ', m_lastTextNode->parentNode(), m_lastTextNode, 1, 1);
        return false;
    }

    if (m_entersTextControls && renderer->isTextControl()) {
        if (HTMLElement* innerTextElement = toRenderTextControl(renderer)->innerTextElement()) {
            m_node = innerTextElement->shadowTreeRootNode();
            pushFullyClippedState(m_fullyClippedStack, m_node);
            m_offset = 0;
            return false;
        }
    }

    m_hasEmitted = true;

    if (m_emitsObjectReplacementCharacters && renderer && renderer->isReplaced()) {
        emitCharacter(objectReplacementCharacter, m_node->parentNode(), m_node, 0, 1);
        return true;
    }

    if (m_emitsCharactersBetweenAllVisiblePositions) {
        // We want replaced elements to behave like punctuation for boundary 
        // finding, and to simply take up space for the selection preservation 
        // code in moveParagraphs, so we use a comma.
        emitCharacter(',', m_node->parentNode(), m_node, 0, 1);
        return true;
    }

    m_positionNode = m_node->parentNode();
    m_positionOffsetBaseNode = m_node;
    m_positionStartOffset = 0;
    m_positionEndOffset = 1;

    m_textCharacters = 0;
    m_textLength = 0;

    m_lastCharacter = 0;

    return true;
}

bool TextIterator::hasVisibleTextNode(RenderText* renderer)
{
    if (renderer->style()->visibility() == VISIBLE)
        return true;
    if (renderer->isTextFragment()) {
        RenderTextFragment* fragment = static_cast<RenderTextFragment*>(renderer);
        if (fragment->firstLetter() && fragment->firstLetter()->style()->visibility() == VISIBLE)
            return true;
    }
    return false;
}

static bool shouldEmitTabBeforeNode(Node* node)
{
    RenderObject* r = node->renderer();
    
    // Table cells are delimited by tabs.
    if (!r || !isTableCell(node))
        return false;
    
    // Want a tab before every cell other than the first one
    RenderTableCell* rc = toRenderTableCell(r);
    RenderTable* t = rc->table();
    return t && (t->cellBefore(rc) || t->cellAbove(rc));
}

static bool shouldEmitNewlineForNode(Node* node)
{
    // br elements are represented by a single newline.
    RenderObject* r = node->renderer();
    if (!r)
        return node->hasTagName(brTag);
        
    return r->isBR();
}

static bool shouldEmitNewlinesBeforeAndAfterNode(Node* node)
{
    // Block flow (versus inline flow) is represented by having
    // a newline both before and after the element.
    RenderObject* r = node->renderer();
    if (!r) {
        return (node->hasTagName(blockquoteTag)
                || node->hasTagName(ddTag)
                || node->hasTagName(divTag)
                || node->hasTagName(dlTag)
                || node->hasTagName(dtTag)
                || node->hasTagName(h1Tag)
                || node->hasTagName(h2Tag)
                || node->hasTagName(h3Tag)
                || node->hasTagName(h4Tag)
                || node->hasTagName(h5Tag)
                || node->hasTagName(h6Tag)
                || node->hasTagName(hrTag)
                || node->hasTagName(liTag)
                || node->hasTagName(listingTag)
                || node->hasTagName(olTag)
                || node->hasTagName(pTag)
                || node->hasTagName(preTag)
                || node->hasTagName(trTag)
                || node->hasTagName(ulTag));
    }
    
    // Need to make an exception for table cells, because they are blocks, but we
    // want them tab-delimited rather than having newlines before and after.
    if (isTableCell(node))
        return false;
    
    // Need to make an exception for table row elements, because they are neither
    // "inline" or "RenderBlock", but we want newlines for them.
    if (r->isTableRow()) {
        RenderTable* t = toRenderTableRow(r)->table();
        if (t && !t->isInline())
            return true;
    }
    
    return !r->isInline() && r->isRenderBlock() && !r->isFloatingOrPositioned() && !r->isBody();
}

static bool shouldEmitNewlineAfterNode(Node* node)
{
    // FIXME: It should be better but slower to create a VisiblePosition here.
    if (!shouldEmitNewlinesBeforeAndAfterNode(node))
        return false;
    // Check if this is the very last renderer in the document.
    // If so, then we should not emit a newline.
    while ((node = node->traverseNextSibling()))
        if (node->renderer())
            return true;
    return false;
}

static bool shouldEmitNewlineBeforeNode(Node* node)
{
    return shouldEmitNewlinesBeforeAndAfterNode(node); 
}

static bool shouldEmitExtraNewlineForNode(Node* node)
{
    // When there is a significant collapsed bottom margin, emit an extra
    // newline for a more realistic result.  We end up getting the right
    // result even without margin collapsing. For example: <div><p>text</p></div>
    // will work right even if both the <div> and the <p> have bottom margins.
    RenderObject* r = node->renderer();
    if (!r || !r->isBox())
        return false;
    
    // NOTE: We only do this for a select set of nodes, and fwiw WinIE appears
    // not to do this at all
    if (node->hasTagName(h1Tag)
        || node->hasTagName(h2Tag)
        || node->hasTagName(h3Tag)
        || node->hasTagName(h4Tag)
        || node->hasTagName(h5Tag)
        || node->hasTagName(h6Tag)
        || node->hasTagName(pTag)) {
        RenderStyle* style = r->style();
        if (style) {
            int bottomMargin = toRenderBox(r)->collapsedMarginAfter();
            int fontSize = style->fontDescription().computedPixelSize();
            if (bottomMargin * 2 >= fontSize)
                return true;
        }
    }
    
    return false;
}

static int collapsedSpaceLength(RenderText* renderer, int textEnd)
{
    const UChar* characters = renderer->text()->characters();
    int length = renderer->text()->length();
    for (int i = textEnd; i < length; ++i) {
        if (!renderer->style()->isCollapsibleWhiteSpace(characters[i]))
            return i - textEnd;
    }

    return length - textEnd;
}

static int maxOffsetIncludingCollapsedSpaces(Node* node)
{
    int offset = caretMaxOffset(node);

    if (node->renderer() && node->renderer()->isText())
        offset += collapsedSpaceLength(toRenderText(node->renderer()), offset);

    return offset;
}

// Whether or not we should emit a character as we enter m_node (if it's a container) or as we hit it (if it's atomic).
bool TextIterator::shouldRepresentNodeOffsetZero()
{
    if (m_emitsCharactersBetweenAllVisiblePositions && m_node->renderer() && m_node->renderer()->isTable())
        return true;
        
    // Leave element positioned flush with start of a paragraph
    // (e.g. do not insert tab before a table cell at the start of a paragraph)
    if (m_lastCharacter == '\n')
        return false;
    
    // Otherwise, show the position if we have emitted any characters
    if (m_hasEmitted)
        return true;
    
    // We've not emitted anything yet. Generally, there is no need for any positioning then.
    // The only exception is when the element is visually not in the same line as
    // the start of the range (e.g. the range starts at the end of the previous paragraph).
    // NOTE: Creating VisiblePositions and comparing them is relatively expensive, so we
    // make quicker checks to possibly avoid that. Another check that we could make is
    // is whether the inline vs block flow changed since the previous visible element.
    // I think we're already in a special enough case that that won't be needed, tho.

    // No character needed if this is the first node in the range.
    if (m_node == m_startContainer)
        return false;
    
    // If we are outside the start container's subtree, assume we need to emit.
    // FIXME: m_startContainer could be an inline block
    if (!m_node->isDescendantOf(m_startContainer))
        return true;

    // If we started as m_startContainer offset 0 and the current node is a descendant of
    // the start container, we already had enough context to correctly decide whether to
    // emit after a preceding block. We chose not to emit (m_hasEmitted is false),
    // so don't second guess that now.
    // NOTE: Is this really correct when m_node is not a leftmost descendant? Probably
    // immaterial since we likely would have already emitted something by now.
    if (m_startOffset == 0)
        return false;
        
    // If this node is unrendered or invisible the VisiblePosition checks below won't have much meaning.
    // Additionally, if the range we are iterating over contains huge sections of unrendered content, 
    // we would create VisiblePositions on every call to this function without this check.
    if (!m_node->renderer() || m_node->renderer()->style()->visibility() != VISIBLE)
        return false;
    
    // The startPos.isNotNull() check is needed because the start could be before the body,
    // and in that case we'll get null. We don't want to put in newlines at the start in that case.
    // The currPos.isNotNull() check is needed because positions in non-HTML content
    // (like SVG) do not have visible positions, and we don't want to emit for them either.
    VisiblePosition startPos = VisiblePosition(Position(m_startContainer, m_startOffset, Position::PositionIsOffsetInAnchor), DOWNSTREAM);
    VisiblePosition currPos = VisiblePosition(positionBeforeNode(m_node), DOWNSTREAM);
    return startPos.isNotNull() && currPos.isNotNull() && !inSameLine(startPos, currPos);
}

bool TextIterator::shouldEmitSpaceBeforeAndAfterNode(Node* node)
{
    return node->renderer() && node->renderer()->isTable() && (node->renderer()->isInline() || m_emitsCharactersBetweenAllVisiblePositions);
}

void TextIterator::representNodeOffsetZero()
{
    // Emit a character to show the positioning of m_node.
    
    // When we haven't been emitting any characters, shouldRepresentNodeOffsetZero() can 
    // create VisiblePositions, which is expensive.  So, we perform the inexpensive checks
    // on m_node to see if it necessitates emitting a character first and will early return 
    // before encountering shouldRepresentNodeOffsetZero()s worse case behavior.
    if (shouldEmitTabBeforeNode(m_node)) {
        if (shouldRepresentNodeOffsetZero())
            emitCharacter('\t', m_node->parentNode(), m_node, 0, 0);
    } else if (shouldEmitNewlineBeforeNode(m_node)) {
        if (shouldRepresentNodeOffsetZero())
            emitCharacter('\n', m_node->parentNode(), m_node, 0, 0);
    } else if (shouldEmitSpaceBeforeAndAfterNode(m_node)) {
        if (shouldRepresentNodeOffsetZero())
            emitCharacter(' ', m_node->parentNode(), m_node, 0, 0);
    }
}

bool TextIterator::handleNonTextNode()
{
    if (shouldEmitNewlineForNode(m_node))
        emitCharacter('\n', m_node->parentNode(), m_node, 0, 1);
    else if (m_emitsCharactersBetweenAllVisiblePositions && m_node->renderer() && m_node->renderer()->isHR())
        emitCharacter(' ', m_node->parentNode(), m_node, 0, 1);
    else
        representNodeOffsetZero();

    return true;
}

void TextIterator::exitNode()
{
    // prevent emitting a newline when exiting a collapsed block at beginning of the range
    // FIXME: !m_hasEmitted does not necessarily mean there was a collapsed block... it could
    // have been an hr (e.g.). Also, a collapsed block could have height (e.g. a table) and
    // therefore look like a blank line.
    if (!m_hasEmitted)
        return;
        
    // Emit with a position *inside* m_node, after m_node's contents, in 
    // case it is a block, because the run should start where the 
    // emitted character is positioned visually.
    Node* baseNode = m_node->lastChild() ? m_node->lastChild() : m_node;
    // FIXME: This shouldn't require the m_lastTextNode to be true, but we can't change that without making
    // the logic in _web_attributedStringFromRange match.  We'll get that for free when we switch to use
    // TextIterator in _web_attributedStringFromRange.
    // See <rdar://problem/5428427> for an example of how this mismatch will cause problems.
    if (m_lastTextNode && shouldEmitNewlineAfterNode(m_node)) {
        // use extra newline to represent margin bottom, as needed
        bool addNewline = shouldEmitExtraNewlineForNode(m_node);
        
        // FIXME: We need to emit a '\n' as we leave an empty block(s) that
        // contain a VisiblePosition when doing selection preservation.
        if (m_lastCharacter != '\n') {
            // insert a newline with a position following this block's contents.
            emitCharacter('\n', baseNode->parentNode(), baseNode, 1, 1);
            // remember whether to later add a newline for the current node
            ASSERT(!m_needsAnotherNewline);
            m_needsAnotherNewline = addNewline;
        } else if (addNewline)
            // insert a newline with a position following this block's contents.
            emitCharacter('\n', baseNode->parentNode(), baseNode, 1, 1);
    }
    
    // If nothing was emitted, see if we need to emit a space.
    if (!m_positionNode && shouldEmitSpaceBeforeAndAfterNode(m_node))
        emitCharacter(' ', baseNode->parentNode(), baseNode, 1, 1);
}

void TextIterator::emitCharacter(UChar c, Node* textNode, Node* offsetBaseNode, int textStartOffset, int textEndOffset)
{
    m_hasEmitted = true;
    
    // remember information with which to construct the TextIterator::range()
    // NOTE: textNode is often not a text node, so the range will specify child nodes of positionNode
    m_positionNode = textNode;
    m_positionOffsetBaseNode = offsetBaseNode;
    m_positionStartOffset = textStartOffset;
    m_positionEndOffset = textEndOffset;
 
    // remember information with which to construct the TextIterator::characters() and length()
    m_singleCharacterBuffer = c;
    m_textCharacters = &m_singleCharacterBuffer;
    m_textLength = 1;

    // remember some iteration state
    m_lastTextNodeEndedWithCollapsedSpace = false;
    m_lastCharacter = c;
}

void TextIterator::emitText(Node* textNode, RenderObject* renderObject, int textStartOffset, int textEndOffset)
{
    RenderText* renderer = toRenderText(renderObject);
    m_text = m_emitsTextWithoutTranscoding ? renderer->textWithoutTranscoding() : renderer->text();
    ASSERT(m_text.characters());
    ASSERT(0 <= textStartOffset && textStartOffset < static_cast<int>(m_text.length()));
    ASSERT(0 <= textEndOffset && textEndOffset <= static_cast<int>(m_text.length()));
    ASSERT(textStartOffset <= textEndOffset);

    m_positionNode = textNode;
    m_positionOffsetBaseNode = 0;
    m_positionStartOffset = textStartOffset;
    m_positionEndOffset = textEndOffset;
    m_textCharacters = m_text.characters() + textStartOffset;
    m_textLength = textEndOffset - textStartOffset;
    m_lastCharacter = m_text[textEndOffset - 1];

    m_lastTextNodeEndedWithCollapsedSpace = false;
    m_hasEmitted = true;
}

void TextIterator::emitText(Node* textNode, int textStartOffset, int textEndOffset)
{
    emitText(textNode, m_node->renderer(), textStartOffset, textEndOffset);
}

PassRefPtr<Range> TextIterator::range() const
{
    // use the current run information, if we have it
    if (m_positionNode) {
        if (m_positionOffsetBaseNode) {
            int index = m_positionOffsetBaseNode->nodeIndex();
            m_positionStartOffset += index;
            m_positionEndOffset += index;
            m_positionOffsetBaseNode = 0;
        }
        return Range::create(m_positionNode->document(), m_positionNode, m_positionStartOffset, m_positionNode, m_positionEndOffset);
    }

    // otherwise, return the end of the overall range we were given
    if (m_endContainer)
        return Range::create(m_endContainer->document(), m_endContainer, m_endOffset, m_endContainer, m_endOffset);
        
    return 0;
}
    
Node* TextIterator::node() const
{
    RefPtr<Range> textRange = range();
    if (!textRange)
        return 0;

    Node* node = textRange->startContainer();
    if (!node)
        return 0;
    if (node->offsetInCharacters())
        return node;
    
    return node->childNode(textRange->startOffset());
}

// --------

SimplifiedBackwardsTextIterator::SimplifiedBackwardsTextIterator()
    : m_behavior(TextIteratorDefaultBehavior)
    , m_node(0)
    , m_offset(0)
    , m_handledNode(false)
    , m_handledChildren(false)
    , m_startNode(0)
    , m_startOffset(0)
    , m_endNode(0)
    , m_endOffset(0)
    , m_positionNode(0)
    , m_positionStartOffset(0)
    , m_positionEndOffset(0)
    , m_textCharacters(0)
    , m_textLength(0)
    , m_lastTextNode(0)
    , m_lastCharacter(0)
    , m_singleCharacterBuffer(0)
    , m_havePassedStartNode(false)
    , m_shouldHandleFirstLetter(false)
{
}

SimplifiedBackwardsTextIterator::SimplifiedBackwardsTextIterator(const Range* r, TextIteratorBehavior behavior)
    : m_behavior(behavior)
    , m_node(0)
    , m_offset(0)
    , m_handledNode(false)
    , m_handledChildren(false)
    , m_startNode(0)
    , m_startOffset(0)
    , m_endNode(0)
    , m_endOffset(0)
    , m_positionNode(0)
    , m_positionStartOffset(0)
    , m_positionEndOffset(0)
    , m_textCharacters(0)
    , m_textLength(0)
    , m_lastTextNode(0)
    , m_lastCharacter(0)
    , m_singleCharacterBuffer(0)
    , m_havePassedStartNode(false)
    , m_shouldHandleFirstLetter(false)
{
    ASSERT(m_behavior == TextIteratorDefaultBehavior);

    if (!r)
        return;

    Node* startNode = r->startContainer();
    if (!startNode)
        return;
    Node* endNode = r->endContainer();
    int startOffset = r->startOffset();
    int endOffset = r->endOffset();

    if (!startNode->offsetInCharacters()) {
        if (startOffset >= 0 && startOffset < static_cast<int>(startNode->childNodeCount())) {
            startNode = startNode->childNode(startOffset);
            startOffset = 0;
        }
    }
    if (!endNode->offsetInCharacters()) {
        if (endOffset > 0 && endOffset <= static_cast<int>(endNode->childNodeCount())) {
            endNode = endNode->childNode(endOffset - 1);
            endOffset = lastOffsetInNode(endNode);
        }
    }

    m_node = endNode;
    setUpFullyClippedStack(m_fullyClippedStack, m_node);    
    m_offset = endOffset;
    m_handledNode = false;
    m_handledChildren = endOffset == 0;

    m_startNode = startNode;
    m_startOffset = startOffset;
    m_endNode = endNode;
    m_endOffset = endOffset;
    
#ifndef NDEBUG
    // Need this just because of the assert.
    m_positionNode = endNode;
#endif

    m_lastTextNode = 0;
    m_lastCharacter = '\n';

    m_havePassedStartNode = false;

    advance();
}

void SimplifiedBackwardsTextIterator::advance()
{
    ASSERT(m_positionNode);

    m_positionNode = 0;
    m_textLength = 0;

    while (m_node && !m_havePassedStartNode) {
        // Don't handle node if we start iterating at [node, 0].
        if (!m_handledNode && !(m_node == m_endNode && m_endOffset == 0)) {
            RenderObject* renderer = m_node->renderer();
            if (renderer && renderer->isText() && m_node->nodeType() == Node::TEXT_NODE) {
                // FIXME: What about CDATA_SECTION_NODE?
                if (renderer->style()->visibility() == VISIBLE && m_offset > 0)
                    m_handledNode = handleTextNode();
            } else if (renderer && (renderer->isImage() || renderer->isWidget())) {
                if (renderer->style()->visibility() == VISIBLE && m_offset > 0)
                    m_handledNode = handleReplacedElement();
            } else
                m_handledNode = handleNonTextNode();
            if (m_positionNode)
                return;
        }

        if (!m_handledChildren && m_node->hasChildNodes()) {
            m_node = m_node->lastChild();
            pushFullyClippedState(m_fullyClippedStack, m_node);
        } else {
            // Exit empty containers as we pass over them or containers
            // where [container, 0] is where we started iterating.
            if (!m_handledNode
                    && canHaveChildrenForEditing(m_node)
                    && m_node->parentNode()
                    && (!m_node->lastChild() || (m_node == m_endNode && !m_endOffset))) {
                exitNode();
                if (m_positionNode) {
                    m_handledNode = true;
                    m_handledChildren = true;
                    return;
                }
            }

            // Exit all other containers.
            while (!m_node->previousSibling()) {
                if (!advanceRespectingRange(m_node->parentOrHostNode()))
                    break;
                m_fullyClippedStack.pop();
                exitNode();
                if (m_positionNode) {
                    m_handledNode = true;
                    m_handledChildren = true;
                    return;
                }
            }

            m_fullyClippedStack.pop();
            if (advanceRespectingRange(m_node->previousSibling()))
                pushFullyClippedState(m_fullyClippedStack, m_node);
            else
                m_node = 0;
        }

        // For the purpose of word boundary detection,
        // we should iterate all visible text and trailing (collapsed) whitespaces. 
        m_offset = m_node ? maxOffsetIncludingCollapsedSpaces(m_node) : 0;
        m_handledNode = false;
        m_handledChildren = false;
        
        if (m_positionNode)
            return;
    }
}

bool SimplifiedBackwardsTextIterator::handleTextNode()
{
    m_lastTextNode = m_node;

    int startOffset;
    int offsetInNode;
    RenderText* renderer = handleFirstLetter(startOffset, offsetInNode);
    if (!renderer)
        return true;

    String text = renderer->text();
    if (!renderer->firstTextBox() && text.length() > 0)
        return true;

    m_positionEndOffset = m_offset;
    m_offset = startOffset + offsetInNode;
    m_positionNode = m_node;
    m_positionStartOffset = m_offset;

    ASSERT(0 <= m_positionStartOffset - offsetInNode && m_positionStartOffset - offsetInNode <= static_cast<int>(text.length()));
    ASSERT(1 <= m_positionEndOffset - offsetInNode && m_positionEndOffset - offsetInNode <= static_cast<int>(text.length()));
    ASSERT(m_positionStartOffset <= m_positionEndOffset);

    m_textLength = m_positionEndOffset - m_positionStartOffset;
    m_textCharacters = text.characters() + (m_positionStartOffset - offsetInNode);
    ASSERT(m_textCharacters >= text.characters());
    ASSERT(m_textCharacters + m_textLength <= text.characters() + static_cast<int>(text.length()));

    m_lastCharacter = text[m_positionEndOffset - 1];

    return !m_shouldHandleFirstLetter;
}

RenderText* SimplifiedBackwardsTextIterator::handleFirstLetter(int& startOffset, int& offsetInNode)
{
    RenderText* renderer = toRenderText(m_node->renderer());
    startOffset = (m_node == m_startNode) ? m_startOffset : 0;

    if (!renderer->isTextFragment()) {
        offsetInNode = 0;
        return renderer;
    }

    RenderTextFragment* fragment = toRenderTextFragment(renderer);
    int offsetAfterFirstLetter = fragment->start();
    if (startOffset >= offsetAfterFirstLetter) {
        ASSERT(!m_shouldHandleFirstLetter);
        offsetInNode = offsetAfterFirstLetter;
        return renderer;
    }

    if (!m_shouldHandleFirstLetter && offsetAfterFirstLetter < m_offset) {
        m_shouldHandleFirstLetter = true;
        offsetInNode = offsetAfterFirstLetter;
        return renderer;
    }

    m_shouldHandleFirstLetter = false;
    offsetInNode = 0;
    return firstRenderTextInFirstLetter(fragment->firstLetter());
}

bool SimplifiedBackwardsTextIterator::handleReplacedElement()
{
    unsigned index = m_node->nodeIndex();
    // We want replaced elements to behave like punctuation for boundary 
    // finding, and to simply take up space for the selection preservation 
    // code in moveParagraphs, so we use a comma.  Unconditionally emit
    // here because this iterator is only used for boundary finding.
    emitCharacter(',', m_node->parentNode(), index, index + 1);
    return true;
}

bool SimplifiedBackwardsTextIterator::handleNonTextNode()
{    
    // We can use a linefeed in place of a tab because this simple iterator is only used to
    // find boundaries, not actual content.  A linefeed breaks words, sentences, and paragraphs.
    if (shouldEmitNewlineForNode(m_node) || shouldEmitNewlineAfterNode(m_node) || shouldEmitTabBeforeNode(m_node)) {
        unsigned index = m_node->nodeIndex();
        // The start of this emitted range is wrong. Ensuring correctness would require
        // VisiblePositions and so would be slow. previousBoundary expects this.
        emitCharacter('\n', m_node->parentNode(), index + 1, index + 1);
    }
    return true;
}

void SimplifiedBackwardsTextIterator::exitNode()
{
    if (shouldEmitNewlineForNode(m_node) || shouldEmitNewlineBeforeNode(m_node) || shouldEmitTabBeforeNode(m_node)) {
        // The start of this emitted range is wrong. Ensuring correctness would require
        // VisiblePositions and so would be slow. previousBoundary expects this.
        emitCharacter('\n', m_node, 0, 0);
    }
}

void SimplifiedBackwardsTextIterator::emitCharacter(UChar c, Node* node, int startOffset, int endOffset)
{
    m_singleCharacterBuffer = c;
    m_positionNode = node;
    m_positionStartOffset = startOffset;
    m_positionEndOffset = endOffset;
    m_textCharacters = &m_singleCharacterBuffer;
    m_textLength = 1;
    m_lastCharacter = c;
}

bool SimplifiedBackwardsTextIterator::advanceRespectingRange(Node* next)
{
    if (!next)
        return false;
    m_havePassedStartNode |= m_node == m_startNode;
    if (m_havePassedStartNode)
        return false;
    m_node = next;
    return true;
}

PassRefPtr<Range> SimplifiedBackwardsTextIterator::range() const
{
    if (m_positionNode)
        return Range::create(m_positionNode->document(), m_positionNode, m_positionStartOffset, m_positionNode, m_positionEndOffset);
    
    return Range::create(m_startNode->document(), m_startNode, m_startOffset, m_startNode, m_startOffset);
}

// --------

CharacterIterator::CharacterIterator()
    : m_offset(0)
    , m_runOffset(0)
    , m_atBreak(true)
{
}

CharacterIterator::CharacterIterator(const Range* r, TextIteratorBehavior behavior)
    : m_offset(0)
    , m_runOffset(0)
    , m_atBreak(true)
    , m_textIterator(r, behavior)
{
    while (!atEnd() && m_textIterator.length() == 0)
        m_textIterator.advance();
}

PassRefPtr<Range> CharacterIterator::range() const
{
    RefPtr<Range> r = m_textIterator.range();
    if (!m_textIterator.atEnd()) {
        if (m_textIterator.length() <= 1) {
            ASSERT(m_runOffset == 0);
        } else {
            Node* n = r->startContainer();
            ASSERT(n == r->endContainer());
            int offset = r->startOffset() + m_runOffset;
            ExceptionCode ec = 0;
            r->setStart(n, offset, ec);
            r->setEnd(n, offset + 1, ec);
            ASSERT(!ec);
        }
    }
    return r.release();
}

void CharacterIterator::advance(int count)
{
    if (count <= 0) {
        ASSERT(count == 0);
        return;
    }
    
    m_atBreak = false;

    // easy if there is enough left in the current m_textIterator run
    int remaining = m_textIterator.length() - m_runOffset;
    if (count < remaining) {
        m_runOffset += count;
        m_offset += count;
        return;
    }

    // exhaust the current m_textIterator run
    count -= remaining;
    m_offset += remaining;
    
    // move to a subsequent m_textIterator run
    for (m_textIterator.advance(); !atEnd(); m_textIterator.advance()) {
        int runLength = m_textIterator.length();
        if (runLength == 0)
            m_atBreak = true;
        else {
            // see whether this is m_textIterator to use
            if (count < runLength) {
                m_runOffset = count;
                m_offset += count;
                return;
            }
            
            // exhaust this m_textIterator run
            count -= runLength;
            m_offset += runLength;
        }
    }

    // ran to the end of the m_textIterator... no more runs left
    m_atBreak = true;
    m_runOffset = 0;
}

String CharacterIterator::string(int numChars)
{
    Vector<UChar> result;
    result.reserveInitialCapacity(numChars);
    while (numChars > 0 && !atEnd()) {
        int runSize = min(numChars, length());
        result.append(characters(), runSize);
        numChars -= runSize;
        advance(runSize);
    }
    return String::adopt(result);
}

static PassRefPtr<Range> characterSubrange(CharacterIterator& it, int offset, int length)
{
    it.advance(offset);
    RefPtr<Range> start = it.range();

    if (length > 1)
        it.advance(length - 1);
    RefPtr<Range> end = it.range();

    return Range::create(start->startContainer()->document(), 
        start->startContainer(), start->startOffset(), 
        end->endContainer(), end->endOffset());
}

BackwardsCharacterIterator::BackwardsCharacterIterator()
    : m_offset(0)
    , m_runOffset(0)
    , m_atBreak(true)
{
}

BackwardsCharacterIterator::BackwardsCharacterIterator(const Range* range, TextIteratorBehavior behavior)
    : m_offset(0)
    , m_runOffset(0)
    , m_atBreak(true)
    , m_textIterator(range, behavior)
{
    while (!atEnd() && !m_textIterator.length())
        m_textIterator.advance();
}

PassRefPtr<Range> BackwardsCharacterIterator::range() const
{
    RefPtr<Range> r = m_textIterator.range();
    if (!m_textIterator.atEnd()) {
        if (m_textIterator.length() <= 1)
            ASSERT(m_runOffset == 0);
        else {
            Node* n = r->startContainer();
            ASSERT(n == r->endContainer());
            int offset = r->endOffset() - m_runOffset;
            ExceptionCode ec = 0;
            r->setStart(n, offset - 1, ec);
            r->setEnd(n, offset, ec);
            ASSERT(!ec);
        }
    }
    return r.release();
}

void BackwardsCharacterIterator::advance(int count)
{
    if (count <= 0) {
        ASSERT(!count);
        return;
    }

    m_atBreak = false;

    int remaining = m_textIterator.length() - m_runOffset;
    if (count < remaining) {
        m_runOffset += count;
        m_offset += count;
        return;
    }

    count -= remaining;
    m_offset += remaining;

    for (m_textIterator.advance(); !atEnd(); m_textIterator.advance()) {
        int runLength = m_textIterator.length();
        if (runLength == 0)
            m_atBreak = true;
        else {
            if (count < runLength) {
                m_runOffset = count;
                m_offset += count;
                return;
            }
            
            count -= runLength;
            m_offset += runLength;
        }
    }

    m_atBreak = true;
    m_runOffset = 0;
}

// --------

WordAwareIterator::WordAwareIterator()
    : m_previousText(0)
    , m_didLookAhead(false)
{
}

WordAwareIterator::WordAwareIterator(const Range* r)
    : m_previousText(0)
    , m_didLookAhead(true) // so we consider the first chunk from the text iterator
    , m_textIterator(r)
{
    advance(); // get in position over the first chunk of text
}

WordAwareIterator::~WordAwareIterator()
{
}

// We're always in one of these modes:
// - The current chunk in the text iterator is our current chunk
//      (typically its a piece of whitespace, or text that ended with whitespace)
// - The previous chunk in the text iterator is our current chunk
//      (we looked ahead to the next chunk and found a word boundary)
// - We built up our own chunk of text from many chunks from the text iterator

// FIXME: Performance could be bad for huge spans next to each other that don't fall on word boundaries.

void WordAwareIterator::advance()
{
    m_previousText = 0;
    m_buffer.clear();      // toss any old buffer we built up

    // If last time we did a look-ahead, start with that looked-ahead chunk now
    if (!m_didLookAhead) {
        ASSERT(!m_textIterator.atEnd());
        m_textIterator.advance();
    }
    m_didLookAhead = false;

    // Go to next non-empty chunk 
    while (!m_textIterator.atEnd() && m_textIterator.length() == 0)
        m_textIterator.advance();
    m_range = m_textIterator.range();

    if (m_textIterator.atEnd())
        return;
    
    while (1) {
        // If this chunk ends in whitespace we can just use it as our chunk.
        if (isSpaceOrNewline(m_textIterator.characters()[m_textIterator.length() - 1]))
            return;

        // If this is the first chunk that failed, save it in previousText before look ahead
        if (m_buffer.isEmpty()) {
            m_previousText = m_textIterator.characters();
            m_previousLength = m_textIterator.length();
        }

        // Look ahead to next chunk.  If it is whitespace or a break, we can use the previous stuff
        m_textIterator.advance();
        if (m_textIterator.atEnd() || m_textIterator.length() == 0 || isSpaceOrNewline(m_textIterator.characters()[0])) {
            m_didLookAhead = true;
            return;
        }

        if (m_buffer.isEmpty()) {
            // Start gobbling chunks until we get to a suitable stopping point
            m_buffer.append(m_previousText, m_previousLength);
            m_previousText = 0;
        }
        m_buffer.append(m_textIterator.characters(), m_textIterator.length());
        int exception = 0;
        m_range->setEnd(m_textIterator.range()->endContainer(), m_textIterator.range()->endOffset(), exception);
    }
}

int WordAwareIterator::length() const
{
    if (!m_buffer.isEmpty())
        return m_buffer.size();
    if (m_previousText)
        return m_previousLength;
    return m_textIterator.length();
}

const UChar* WordAwareIterator::characters() const
{
    if (!m_buffer.isEmpty())
        return m_buffer.data();
    if (m_previousText)
        return m_previousText;
    return m_textIterator.characters();
}

// --------

static inline UChar foldQuoteMarkOrSoftHyphen(UChar c)
{
    switch (c) {
        case hebrewPunctuationGershayim:
        case leftDoubleQuotationMark:
        case rightDoubleQuotationMark:
            return '"';
        case hebrewPunctuationGeresh:
        case leftSingleQuotationMark:
        case rightSingleQuotationMark:
            return '\'';
        case softHyphen:
            // Replace soft hyphen with an ignorable character so that their presence or absence will
            // not affect string comparison.
            return 0;
        default:
            return c;
    }
}

static inline void foldQuoteMarksAndSoftHyphens(String& s)
{
    s.replace(hebrewPunctuationGeresh, '\'');
    s.replace(hebrewPunctuationGershayim, '"');
    s.replace(leftDoubleQuotationMark, '"');
    s.replace(leftSingleQuotationMark, '\'');
    s.replace(rightDoubleQuotationMark, '"');
    s.replace(rightSingleQuotationMark, '\'');
    // Replace soft hyphen with an ignorable character so that their presence or absence will
    // not affect string comparison.
    s.replace(softHyphen, 0);
}

#if USE(ICU_UNICODE) && !UCONFIG_NO_COLLATION

static inline void foldQuoteMarksAndSoftHyphens(UChar* data, size_t length)
{
    for (size_t i = 0; i < length; ++i)
        data[i] = foldQuoteMarkOrSoftHyphen(data[i]);
}

static const size_t minimumSearchBufferSize = 8192;

#ifndef NDEBUG
static bool searcherInUse;
#endif

static UStringSearch* createSearcher()
{
    // Provide a non-empty pattern and non-empty text so usearch_open will not fail,
    // but it doesn't matter exactly what it is, since we don't perform any searches
    // without setting both the pattern and the text.
    UErrorCode status = U_ZERO_ERROR;
    UStringSearch* searcher = usearch_open(&newlineCharacter, 1, &newlineCharacter, 1, currentSearchLocaleID(), 0, &status);
    ASSERT(status == U_ZERO_ERROR || status == U_USING_FALLBACK_WARNING || status == U_USING_DEFAULT_WARNING);
    return searcher;
}

static UStringSearch* searcher()
{
    static UStringSearch* searcher = createSearcher();
    return searcher;
}

static inline void lockSearcher()
{
#ifndef NDEBUG
    ASSERT(!searcherInUse);
    searcherInUse = true;
#endif
}

static inline void unlockSearcher()
{
#ifndef NDEBUG
    ASSERT(searcherInUse);
    searcherInUse = false;
#endif
}

// ICU's search ignores the distinction between small kana letters and ones
// that are not small, and also characters that differ only in the voicing
// marks when considering only primary collation strength diffrences.
// This is not helpful for end users, since these differences make words
// distinct, so for our purposes we need these to be considered.
// The Unicode folks do not think the collation algorithm should be
// changed. To work around this, we would like to tailor the ICU searcher,
// but we can't get that to work yet. So instead, we check for cases where
// these differences occur, and skip those matches.

// We refer to the above technique as the "kana workaround". The next few
// functions are helper functinos for the kana workaround.

static inline bool isKanaLetter(UChar character)
{
    // Hiragana letters.
    if (character >= 0x3041 && character <= 0x3096)
        return true;

    // Katakana letters.
    if (character >= 0x30A1 && character <= 0x30FA)
        return true;
    if (character >= 0x31F0 && character <= 0x31FF)
        return true;

    // Halfwidth katakana letters.
    if (character >= 0xFF66 && character <= 0xFF9D && character != 0xFF70)
        return true;

    return false;
}

static inline bool isSmallKanaLetter(UChar character)
{
    ASSERT(isKanaLetter(character));

    switch (character) {
    case 0x3041: // HIRAGANA LETTER SMALL A
    case 0x3043: // HIRAGANA LETTER SMALL I
    case 0x3045: // HIRAGANA LETTER SMALL U
    case 0x3047: // HIRAGANA LETTER SMALL E
    case 0x3049: // HIRAGANA LETTER SMALL O
    case 0x3063: // HIRAGANA LETTER SMALL TU
    case 0x3083: // HIRAGANA LETTER SMALL YA
    case 0x3085: // HIRAGANA LETTER SMALL YU
    case 0x3087: // HIRAGANA LETTER SMALL YO
    case 0x308E: // HIRAGANA LETTER SMALL WA
    case 0x3095: // HIRAGANA LETTER SMALL KA
    case 0x3096: // HIRAGANA LETTER SMALL KE
    case 0x30A1: // KATAKANA LETTER SMALL A
    case 0x30A3: // KATAKANA LETTER SMALL I
    case 0x30A5: // KATAKANA LETTER SMALL U
    case 0x30A7: // KATAKANA LETTER SMALL E
    case 0x30A9: // KATAKANA LETTER SMALL O
    case 0x30C3: // KATAKANA LETTER SMALL TU
    case 0x30E3: // KATAKANA LETTER SMALL YA
    case 0x30E5: // KATAKANA LETTER SMALL YU
    case 0x30E7: // KATAKANA LETTER SMALL YO
    case 0x30EE: // KATAKANA LETTER SMALL WA
    case 0x30F5: // KATAKANA LETTER SMALL KA
    case 0x30F6: // KATAKANA LETTER SMALL KE
    case 0x31F0: // KATAKANA LETTER SMALL KU
    case 0x31F1: // KATAKANA LETTER SMALL SI
    case 0x31F2: // KATAKANA LETTER SMALL SU
    case 0x31F3: // KATAKANA LETTER SMALL TO
    case 0x31F4: // KATAKANA LETTER SMALL NU
    case 0x31F5: // KATAKANA LETTER SMALL HA
    case 0x31F6: // KATAKANA LETTER SMALL HI
    case 0x31F7: // KATAKANA LETTER SMALL HU
    case 0x31F8: // KATAKANA LETTER SMALL HE
    case 0x31F9: // KATAKANA LETTER SMALL HO
    case 0x31FA: // KATAKANA LETTER SMALL MU
    case 0x31FB: // KATAKANA LETTER SMALL RA
    case 0x31FC: // KATAKANA LETTER SMALL RI
    case 0x31FD: // KATAKANA LETTER SMALL RU
    case 0x31FE: // KATAKANA LETTER SMALL RE
    case 0x31FF: // KATAKANA LETTER SMALL RO
    case 0xFF67: // HALFWIDTH KATAKANA LETTER SMALL A
    case 0xFF68: // HALFWIDTH KATAKANA LETTER SMALL I
    case 0xFF69: // HALFWIDTH KATAKANA LETTER SMALL U
    case 0xFF6A: // HALFWIDTH KATAKANA LETTER SMALL E
    case 0xFF6B: // HALFWIDTH KATAKANA LETTER SMALL O
    case 0xFF6C: // HALFWIDTH KATAKANA LETTER SMALL YA
    case 0xFF6D: // HALFWIDTH KATAKANA LETTER SMALL YU
    case 0xFF6E: // HALFWIDTH KATAKANA LETTER SMALL YO
    case 0xFF6F: // HALFWIDTH KATAKANA LETTER SMALL TU
        return true;
    }
    return false;
}

enum VoicedSoundMarkType { NoVoicedSoundMark, VoicedSoundMark, SemiVoicedSoundMark };

static inline VoicedSoundMarkType composedVoicedSoundMark(UChar character)
{
    ASSERT(isKanaLetter(character));

    switch (character) {
    case 0x304C: // HIRAGANA LETTER GA
    case 0x304E: // HIRAGANA LETTER GI
    case 0x3050: // HIRAGANA LETTER GU
    case 0x3052: // HIRAGANA LETTER GE
    case 0x3054: // HIRAGANA LETTER GO
    case 0x3056: // HIRAGANA LETTER ZA
    case 0x3058: // HIRAGANA LETTER ZI
    case 0x305A: // HIRAGANA LETTER ZU
    case 0x305C: // HIRAGANA LETTER ZE
    case 0x305E: // HIRAGANA LETTER ZO
    case 0x3060: // HIRAGANA LETTER DA
    case 0x3062: // HIRAGANA LETTER DI
    case 0x3065: // HIRAGANA LETTER DU
    case 0x3067: // HIRAGANA LETTER DE
    case 0x3069: // HIRAGANA LETTER DO
    case 0x3070: // HIRAGANA LETTER BA
    case 0x3073: // HIRAGANA LETTER BI
    case 0x3076: // HIRAGANA LETTER BU
    case 0x3079: // HIRAGANA LETTER BE
    case 0x307C: // HIRAGANA LETTER BO
    case 0x3094: // HIRAGANA LETTER VU
    case 0x30AC: // KATAKANA LETTER GA
    case 0x30AE: // KATAKANA LETTER GI
    case 0x30B0: // KATAKANA LETTER GU
    case 0x30B2: // KATAKANA LETTER GE
    case 0x30B4: // KATAKANA LETTER GO
    case 0x30B6: // KATAKANA LETTER ZA
    case 0x30B8: // KATAKANA LETTER ZI
    case 0x30BA: // KATAKANA LETTER ZU
    case 0x30BC: // KATAKANA LETTER ZE
    case 0x30BE: // KATAKANA LETTER ZO
    case 0x30C0: // KATAKANA LETTER DA
    case 0x30C2: // KATAKANA LETTER DI
    case 0x30C5: // KATAKANA LETTER DU
    case 0x30C7: // KATAKANA LETTER DE
    case 0x30C9: // KATAKANA LETTER DO
    case 0x30D0: // KATAKANA LETTER BA
    case 0x30D3: // KATAKANA LETTER BI
    case 0x30D6: // KATAKANA LETTER BU
    case 0x30D9: // KATAKANA LETTER BE
    case 0x30DC: // KATAKANA LETTER BO
    case 0x30F4: // KATAKANA LETTER VU
    case 0x30F7: // KATAKANA LETTER VA
    case 0x30F8: // KATAKANA LETTER VI
    case 0x30F9: // KATAKANA LETTER VE
    case 0x30FA: // KATAKANA LETTER VO
        return VoicedSoundMark;
    case 0x3071: // HIRAGANA LETTER PA
    case 0x3074: // HIRAGANA LETTER PI
    case 0x3077: // HIRAGANA LETTER PU
    case 0x307A: // HIRAGANA LETTER PE
    case 0x307D: // HIRAGANA LETTER PO
    case 0x30D1: // KATAKANA LETTER PA
    case 0x30D4: // KATAKANA LETTER PI
    case 0x30D7: // KATAKANA LETTER PU
    case 0x30DA: // KATAKANA LETTER PE
    case 0x30DD: // KATAKANA LETTER PO
        return SemiVoicedSoundMark;
    }
    return NoVoicedSoundMark;
}

static inline bool isCombiningVoicedSoundMark(UChar character)
{
    switch (character) {
    case 0x3099: // COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK
    case 0x309A: // COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
        return true;
    }
    return false;
}

static inline bool containsKanaLetters(const String& pattern)
{
    const UChar* characters = pattern.characters();
    unsigned length = pattern.length();
    for (unsigned i = 0; i < length; ++i) {
        if (isKanaLetter(characters[i]))
            return true;
    }
    return false;
}

static void normalizeCharacters(const UChar* characters, unsigned length, Vector<UChar>& buffer)
{
    ASSERT(length);

    buffer.resize(length);

    UErrorCode status = U_ZERO_ERROR;
    size_t bufferSize = unorm_normalize(characters, length, UNORM_NFC, 0, buffer.data(), length, &status);
    ASSERT(status == U_ZERO_ERROR || status == U_STRING_NOT_TERMINATED_WARNING || status == U_BUFFER_OVERFLOW_ERROR);
    ASSERT(bufferSize);

    buffer.resize(bufferSize);

    if (status == U_ZERO_ERROR || status == U_STRING_NOT_TERMINATED_WARNING)
        return;

    status = U_ZERO_ERROR;
    unorm_normalize(characters, length, UNORM_NFC, 0, buffer.data(), bufferSize, &status);
    ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
}

static bool isNonLatin1Separator(UChar32 character)
{
    ASSERT_ARG(character, character >= 256);

    return U_GET_GC_MASK(character) & (U_GC_S_MASK | U_GC_P_MASK | U_GC_Z_MASK | U_GC_CF_MASK);
}

static inline bool isSeparator(UChar32 character)
{
    static const bool latin1SeparatorTable[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // space ! " # $ % & ' ( ) * + , - . /
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, //                         : ; < = > ?
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //   @
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, //                         [ \ ] ^ _
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //   `
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, //                           { | } ~
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0
    };

    if (character < 256)
        return latin1SeparatorTable[character];

    return isNonLatin1Separator(character);
}

inline SearchBuffer::SearchBuffer(const String& target, FindOptions options)
    : m_target(target)
    , m_options(options)
    , m_prefixLength(0)
    , m_atBreak(true)
    , m_needsMoreContext(options & AtWordStarts)
    , m_targetRequiresKanaWorkaround(containsKanaLetters(m_target))
{
    ASSERT(!m_target.isEmpty());

    // FIXME: We'd like to tailor the searcher to fold quote marks for us instead
    // of doing it in a separate replacement pass here, but ICU doesn't offer a way
    // to add tailoring on top of the locale-specific tailoring as of this writing.
    foldQuoteMarksAndSoftHyphens(m_target);

    size_t targetLength = m_target.length();
    m_buffer.reserveInitialCapacity(max(targetLength * 8, minimumSearchBufferSize));
    m_overlap = m_buffer.capacity() / 4;

    if ((m_options & AtWordStarts) && targetLength) {
        UChar32 targetFirstCharacter;
        U16_GET(m_target.characters(), 0, 0, targetLength, targetFirstCharacter);
        // Characters in the separator category never really occur at the beginning of a word,
        // so if the target begins with such a character, we just ignore the AtWordStart option.
        if (isSeparator(targetFirstCharacter)) {
            m_options &= ~AtWordStarts;
            m_needsMoreContext = false;
        }
    }

    // Grab the single global searcher.
    // If we ever have a reason to do more than once search buffer at once, we'll have
    // to move to multiple searchers.
    lockSearcher();

    UStringSearch* searcher = WebCore::searcher();
    UCollator* collator = usearch_getCollator(searcher);

    UCollationStrength strength = m_options & CaseInsensitive ? UCOL_PRIMARY : UCOL_TERTIARY;
    if (ucol_getStrength(collator) != strength) {
        ucol_setStrength(collator, strength);
        usearch_reset(searcher);
    }

    UErrorCode status = U_ZERO_ERROR;
    usearch_setPattern(searcher, m_target.characters(), targetLength, &status);
    ASSERT(status == U_ZERO_ERROR);

    // The kana workaround requires a normalized copy of the target string.
    if (m_targetRequiresKanaWorkaround)
        normalizeCharacters(m_target.characters(), m_target.length(), m_normalizedTarget);
}

inline SearchBuffer::~SearchBuffer()
{
    // Leave the static object pointing to a valid string.
    UErrorCode status = U_ZERO_ERROR;
    usearch_setPattern(WebCore::searcher(), &newlineCharacter, 1, &status);
    ASSERT(status == U_ZERO_ERROR);

    unlockSearcher();
}

inline size_t SearchBuffer::append(const UChar* characters, size_t length)
{
    ASSERT(length);

    if (m_atBreak) {
        m_buffer.shrink(0);
        m_prefixLength = 0;
        m_atBreak = false;
    } else if (m_buffer.size() == m_buffer.capacity()) {
        memcpy(m_buffer.data(), m_buffer.data() + m_buffer.size() - m_overlap, m_overlap * sizeof(UChar));
        m_prefixLength -= min(m_prefixLength, m_buffer.size() - m_overlap);
        m_buffer.shrink(m_overlap);
    }

    size_t oldLength = m_buffer.size();
    size_t usableLength = min(m_buffer.capacity() - oldLength, length);
    ASSERT(usableLength);
    m_buffer.append(characters, usableLength);
    foldQuoteMarksAndSoftHyphens(m_buffer.data() + oldLength, usableLength);
    return usableLength;
}

inline bool SearchBuffer::needsMoreContext() const
{
    return m_needsMoreContext;
}

inline void SearchBuffer::prependContext(const UChar* characters, size_t length)
{
    ASSERT(m_needsMoreContext);
    ASSERT(m_prefixLength == m_buffer.size());

    if (!length)
        return;

    m_atBreak = false;

    size_t wordBoundaryContextStart = length;
    if (wordBoundaryContextStart) {
        U16_BACK_1(characters, 0, wordBoundaryContextStart);
        wordBoundaryContextStart = startOfLastWordBoundaryContext(characters, wordBoundaryContextStart);
    }

    size_t usableLength = min(m_buffer.capacity() - m_prefixLength, length - wordBoundaryContextStart);
    m_buffer.prepend(characters + length - usableLength, usableLength);
    m_prefixLength += usableLength;

    if (wordBoundaryContextStart || m_prefixLength == m_buffer.capacity())
        m_needsMoreContext = false;
}

inline bool SearchBuffer::atBreak() const
{
    return m_atBreak;
}

inline void SearchBuffer::reachedBreak()
{
    m_atBreak = true;
}

inline bool SearchBuffer::isBadMatch(const UChar* match, size_t matchLength) const
{
    // This function implements the kana workaround. If usearch treats
    // it as a match, but we do not want to, then it's a "bad match".
    if (!m_targetRequiresKanaWorkaround)
        return false;

    // Normalize into a match buffer. We reuse a single buffer rather than
    // creating a new one each time.
    normalizeCharacters(match, matchLength, m_normalizedMatch);

    const UChar* a = m_normalizedTarget.begin();
    const UChar* aEnd = m_normalizedTarget.end();

    const UChar* b = m_normalizedMatch.begin();
    const UChar* bEnd = m_normalizedMatch.end();

    while (true) {
        // Skip runs of non-kana-letter characters. This is necessary so we can
        // correctly handle strings where the target and match have different-length
        // runs of characters that match, while still double checking the correctness
        // of matches of kana letters with other kana letters.
        while (a != aEnd && !isKanaLetter(*a))
            ++a;
        while (b != bEnd && !isKanaLetter(*b))
            ++b;

        // If we reached the end of either the target or the match, we should have
        // reached the end of both; both should have the same number of kana letters.
        if (a == aEnd || b == bEnd) {
            ASSERT(a == aEnd);
            ASSERT(b == bEnd);
            return false;
        }

        // Check for differences in the kana letter character itself.
        if (isSmallKanaLetter(*a) != isSmallKanaLetter(*b))
            return true;
        if (composedVoicedSoundMark(*a) != composedVoicedSoundMark(*b))
            return true;
        ++a;
        ++b;

        // Check for differences in combining voiced sound marks found after the letter.
        while (1) {
            if (!(a != aEnd && isCombiningVoicedSoundMark(*a))) {
                if (b != bEnd && isCombiningVoicedSoundMark(*b))
                    return true;
                break;
            }
            if (!(b != bEnd && isCombiningVoicedSoundMark(*b)))
                return true;
            if (*a != *b)
                return true;
            ++a;
            ++b;
        }
    }
}

inline bool SearchBuffer::isWordStartMatch(size_t start, size_t length) const
{
    ASSERT(m_options & AtWordStarts);

    if (!start)
        return true;

    if (m_options & TreatMedialCapitalAsWordStart) {
        int size = m_buffer.size();
        int offset = start;
        UChar32 firstCharacter;
        U16_GET(m_buffer.data(), 0, offset, size, firstCharacter);
        UChar32 previousCharacter;
        U16_PREV(m_buffer.data(), 0, offset, previousCharacter);

        if (isSeparator(firstCharacter)) {
            // The start of a separator run is a word start (".org" in "webkit.org").
            if (!isSeparator(previousCharacter))
                return true;
        } else if (isASCIIUpper(firstCharacter)) {
            // The start of an uppercase run is a word start ("Kit" in "WebKit").
            if (!isASCIIUpper(previousCharacter))
                return true;
            // The last character of an uppercase run followed by a non-separator, non-digit
            // is a word start ("Request" in "XMLHTTPRequest").
            offset = start;
            U16_FWD_1(m_buffer.data(), offset, size);
            UChar32 nextCharacter = 0;
            if (offset < size)
                U16_GET(m_buffer.data(), 0, offset, size, nextCharacter);
            if (!isASCIIUpper(nextCharacter) && !isASCIIDigit(nextCharacter) && !isSeparator(nextCharacter))
                return true;
        } else if (isASCIIDigit(firstCharacter)) {
            // The start of a digit run is a word start ("2" in "WebKit2").
            if (!isASCIIDigit(previousCharacter))
                return true;
        } else if (isSeparator(previousCharacter) || isASCIIDigit(previousCharacter)) {
            // The start of a non-separator, non-uppercase, non-digit run is a word start,
            // except after an uppercase. ("org" in "webkit.org", but not "ore" in "WebCore").
            return true;
        }
    }

    size_t wordBreakSearchStart = start + length;
    while (wordBreakSearchStart > start)
        wordBreakSearchStart = findNextWordFromIndex(m_buffer.data(), m_buffer.size(), wordBreakSearchStart, false /* backwards */);
    return wordBreakSearchStart == start;
}

inline size_t SearchBuffer::search(size_t& start)
{
    size_t size = m_buffer.size();
    if (m_atBreak) {
        if (!size)
            return 0;
    } else {
        if (size != m_buffer.capacity())
            return 0;
    }

    UStringSearch* searcher = WebCore::searcher();

    UErrorCode status = U_ZERO_ERROR;
    usearch_setText(searcher, m_buffer.data(), size, &status);
    ASSERT(status == U_ZERO_ERROR);

    usearch_setOffset(searcher, m_prefixLength, &status);
    ASSERT(status == U_ZERO_ERROR);

    int matchStart = usearch_next(searcher, &status);
    ASSERT(status == U_ZERO_ERROR);

nextMatch:
    if (!(matchStart >= 0 && static_cast<size_t>(matchStart) < size)) {
        ASSERT(matchStart == USEARCH_DONE);
        return 0;
    }

    // Matches that start in the overlap area are only tentative.
    // The same match may appear later, matching more characters,
    // possibly including a combining character that's not yet in the buffer.
    if (!m_atBreak && static_cast<size_t>(matchStart) >= size - m_overlap) {
        size_t overlap = m_overlap;
        if (m_options & AtWordStarts) {
            // Ensure that there is sufficient context before matchStart the next time around for
            // determining if it is at a word boundary.
            int wordBoundaryContextStart = matchStart;
            U16_BACK_1(m_buffer.data(), 0, wordBoundaryContextStart);
            wordBoundaryContextStart = startOfLastWordBoundaryContext(m_buffer.data(), wordBoundaryContextStart);
            overlap = min(size - 1, max(overlap, size - wordBoundaryContextStart));
        }
        memcpy(m_buffer.data(), m_buffer.data() + size - overlap, overlap * sizeof(UChar));
        m_prefixLength -= min(m_prefixLength, size - overlap);
        m_buffer.shrink(overlap);
        return 0;
    }

    size_t matchedLength = usearch_getMatchedLength(searcher);
    ASSERT(matchStart + matchedLength <= size);

    // If this match is "bad", move on to the next match.
    if (isBadMatch(m_buffer.data() + matchStart, matchedLength) || ((m_options & AtWordStarts) && !isWordStartMatch(matchStart, matchedLength))) {
        matchStart = usearch_next(searcher, &status);
        ASSERT(status == U_ZERO_ERROR);
        goto nextMatch;
    }

    size_t newSize = size - (matchStart + 1);
    memmove(m_buffer.data(), m_buffer.data() + matchStart + 1, newSize * sizeof(UChar));
    m_prefixLength -= min<size_t>(m_prefixLength, matchStart + 1);
    m_buffer.shrink(newSize);

    start = size - matchStart;
    return matchedLength;
}

#else // !ICU_UNICODE

inline SearchBuffer::SearchBuffer(const String& target, FindOptions options)
    : m_target(options & CaseInsensitive ? target.foldCase() : target)
    , m_options(options)
    , m_buffer(m_target.length())
    , m_isCharacterStartBuffer(m_target.length())
    , m_isBufferFull(false)
    , m_cursor(0)
{
    ASSERT(!m_target.isEmpty());
    m_target.replace(noBreakSpace, ' ');
    foldQuoteMarksAndSoftHyphens(m_target);
}

inline SearchBuffer::~SearchBuffer()
{
}

inline void SearchBuffer::reachedBreak()
{
    m_cursor = 0;
    m_isBufferFull = false;
}

inline bool SearchBuffer::atBreak() const
{
    return !m_cursor && !m_isBufferFull;
}

inline void SearchBuffer::append(UChar c, bool isStart)
{
    m_buffer[m_cursor] = c == noBreakSpace ? ' ' : foldQuoteMarkOrSoftHyphen(c);
    m_isCharacterStartBuffer[m_cursor] = isStart;
    if (++m_cursor == m_target.length()) {
        m_cursor = 0;
        m_isBufferFull = true;
    }
}

inline size_t SearchBuffer::append(const UChar* characters, size_t length)
{
    ASSERT(length);
    if (!(m_options & CaseInsensitive)) {
        append(characters[0], true);
        return 1;
    }
    const int maxFoldedCharacters = 16; // sensible maximum is 3, this should be more than enough
    UChar foldedCharacters[maxFoldedCharacters];
    bool error;
    int numFoldedCharacters = foldCase(foldedCharacters, maxFoldedCharacters, characters, 1, &error);
    ASSERT(!error);
    ASSERT(numFoldedCharacters);
    ASSERT(numFoldedCharacters <= maxFoldedCharacters);
    if (!error && numFoldedCharacters) {
        numFoldedCharacters = min(numFoldedCharacters, maxFoldedCharacters);
        append(foldedCharacters[0], true);
        for (int i = 1; i < numFoldedCharacters; ++i)
            append(foldedCharacters[i], false);
    }
    return 1;
}

inline bool SearchBuffer::needsMoreContext() const
{
    return false;
}

void SearchBuffer::prependContext(const UChar*, size_t)
{
    ASSERT_NOT_REACHED();
}

inline size_t SearchBuffer::search(size_t& start)
{
    if (!m_isBufferFull)
        return 0;
    if (!m_isCharacterStartBuffer[m_cursor])
        return 0;

    size_t tailSpace = m_target.length() - m_cursor;
    if (memcmp(&m_buffer[m_cursor], m_target.characters(), tailSpace * sizeof(UChar)) != 0)
        return 0;
    if (memcmp(&m_buffer[0], m_target.characters() + tailSpace, m_cursor * sizeof(UChar)) != 0)
        return 0;

    start = length();

    // Now that we've found a match once, we don't want to find it again, because those
    // are the SearchBuffer semantics, allowing for a buffer where you append more than one
    // character at a time. To do this we take advantage of m_isCharacterStartBuffer, but if
    // we want to get rid of that in the future we could track this with a separate boolean
    // or even move the characters to the start of the buffer and set m_isBufferFull to false.
    m_isCharacterStartBuffer[m_cursor] = false;

    return start;
}

// Returns the number of characters that were appended to the buffer (what we are searching in).
// That's not necessarily the same length as the passed-in target string, because case folding
// can make two strings match even though they're not the same length.
size_t SearchBuffer::length() const
{
    size_t bufferSize = m_target.length();
    size_t length = 0;
    for (size_t i = 0; i < bufferSize; ++i)
        length += m_isCharacterStartBuffer[i];
    return length;
}

#endif // !ICU_UNICODE

// --------

int TextIterator::rangeLength(const Range* r, bool forSelectionPreservation)
{
    int length = 0;
    for (TextIterator it(r, forSelectionPreservation ? TextIteratorEmitsCharactersBetweenAllVisiblePositions : TextIteratorDefaultBehavior); !it.atEnd(); it.advance())
        length += it.length();
    
    return length;
}

PassRefPtr<Range> TextIterator::subrange(Range* entireRange, int characterOffset, int characterCount)
{
    CharacterIterator entireRangeIterator(entireRange);
    return characterSubrange(entireRangeIterator, characterOffset, characterCount);
}

PassRefPtr<Range> TextIterator::rangeFromLocationAndLength(Element* scope, int rangeLocation, int rangeLength, bool forSelectionPreservation)
{
    RefPtr<Range> resultRange = scope->document()->createRange();

    int docTextPosition = 0;
    int rangeEnd = rangeLocation + rangeLength;
    bool startRangeFound = false;

    RefPtr<Range> textRunRange;

    TextIterator it(rangeOfContents(scope).get(), forSelectionPreservation ? TextIteratorEmitsCharactersBetweenAllVisiblePositions : TextIteratorDefaultBehavior);
    
    // FIXME: the atEnd() check shouldn't be necessary, workaround for <http://bugs.webkit.org/show_bug.cgi?id=6289>.
    if (rangeLocation == 0 && rangeLength == 0 && it.atEnd()) {
        textRunRange = it.range();
        
        ExceptionCode ec = 0;
        resultRange->setStart(textRunRange->startContainer(), 0, ec);
        ASSERT(!ec);
        resultRange->setEnd(textRunRange->startContainer(), 0, ec);
        ASSERT(!ec);
        
        return resultRange.release();
    }

    for (; !it.atEnd(); it.advance()) {
        int len = it.length();
        textRunRange = it.range();
        
        bool foundStart = rangeLocation >= docTextPosition && rangeLocation <= docTextPosition + len;
        bool foundEnd = rangeEnd >= docTextPosition && rangeEnd <= docTextPosition + len;
        
        // Fix textRunRange->endPosition(), but only if foundStart || foundEnd, because it is only
        // in those cases that textRunRange is used.
        if (foundEnd) {
            // FIXME: This is a workaround for the fact that the end of a run is often at the wrong
            // position for emitted '\n's.
            if (len == 1 && it.characters()[0] == '\n') {
                scope->document()->updateLayoutIgnorePendingStylesheets();
                it.advance();
                if (!it.atEnd()) {
                    RefPtr<Range> range = it.range();
                    ExceptionCode ec = 0;
                    textRunRange->setEnd(range->startContainer(), range->startOffset(), ec);
                    ASSERT(!ec);
                } else {
                    Position runStart = textRunRange->startPosition();
                    Position runEnd = VisiblePosition(runStart).next().deepEquivalent();
                    if (runEnd.isNotNull()) {
                        ExceptionCode ec = 0;
                        textRunRange->setEnd(runEnd.containerNode(), runEnd.computeOffsetInContainerNode(), ec);
                        ASSERT(!ec);
                    }
                }
            }
        }

        if (foundStart) {
            startRangeFound = true;
            int exception = 0;
            if (textRunRange->startContainer()->isTextNode()) {
                int offset = rangeLocation - docTextPosition;
                resultRange->setStart(textRunRange->startContainer(), offset + textRunRange->startOffset(), exception);
            } else {
                if (rangeLocation == docTextPosition)
                    resultRange->setStart(textRunRange->startContainer(), textRunRange->startOffset(), exception);
                else
                    resultRange->setStart(textRunRange->endContainer(), textRunRange->endOffset(), exception);
            }
        }

        if (foundEnd) {
            int exception = 0;
            if (textRunRange->startContainer()->isTextNode()) {
                int offset = rangeEnd - docTextPosition;
                resultRange->setEnd(textRunRange->startContainer(), offset + textRunRange->startOffset(), exception);
            } else {
                if (rangeEnd == docTextPosition)
                    resultRange->setEnd(textRunRange->startContainer(), textRunRange->startOffset(), exception);
                else
                    resultRange->setEnd(textRunRange->endContainer(), textRunRange->endOffset(), exception);
            }
            docTextPosition += len;
            break;
        }
        docTextPosition += len;
    }
    
    if (!startRangeFound)
        return 0;
    
    if (rangeLength != 0 && rangeEnd > docTextPosition) { // rangeEnd is out of bounds
        int exception = 0;
        resultRange->setEnd(textRunRange->endContainer(), textRunRange->endOffset(), exception);
    }
    
    return resultRange.release();
}

bool TextIterator::locationAndLengthFromRange(const Range* range, size_t& location, size_t& length)
{
    location = notFound;
    length = 0;

    if (!range->startContainer())
        return false;

    Element* selectionRoot = range->ownerDocument()->frame()->selection()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : range->ownerDocument()->documentElement();

    // The critical assumption is that this only gets called with ranges that
    // concentrate on a given area containing the selection root. This is done
    // because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so ensure that the range does not cross a
    // boundary of one of those.
    if (range->startContainer() != scope && !range->startContainer()->isDescendantOf(scope))
        return false;
    if (range->endContainer() != scope && !range->endContainer()->isDescendantOf(scope))
        return false;

    RefPtr<Range> testRange = Range::create(scope->document(), scope, 0, range->startContainer(), range->startOffset());
    ASSERT(testRange->startContainer() == scope);
    location = TextIterator::rangeLength(testRange.get());
    
    ExceptionCode ec;
    testRange->setEnd(range->endContainer(), range->endOffset(), ec);
    ASSERT(testRange->startContainer() == scope);
    length = TextIterator::rangeLength(testRange.get()) - location;
    return true;
}

// --------
    
UChar* plainTextToMallocAllocatedBuffer(const Range* r, unsigned& bufferLength, bool isDisplayString, TextIteratorBehavior defaultBehavior)
{
    UChar* result = 0;

    // Do this in pieces to avoid massive reallocations if there is a large amount of text.
    // Use system malloc for buffers since they can consume lots of memory and current TCMalloc is unable return it back to OS.
    static const unsigned cMaxSegmentSize = 1 << 16;
    bufferLength = 0;
    typedef pair<UChar*, unsigned> TextSegment;
    OwnPtr<Vector<TextSegment> > textSegments;
    Vector<UChar> textBuffer;
    textBuffer.reserveInitialCapacity(cMaxSegmentSize);
    TextIteratorBehavior behavior = defaultBehavior;
    if (!isDisplayString)
        behavior = static_cast<TextIteratorBehavior>(behavior | TextIteratorEmitsTextsWithoutTranscoding);
    
    for (TextIterator it(r, behavior); !it.atEnd(); it.advance()) {
        if (textBuffer.size() && textBuffer.size() + it.length() > cMaxSegmentSize) {
            UChar* newSegmentBuffer = static_cast<UChar*>(malloc(textBuffer.size() * sizeof(UChar)));
            if (!newSegmentBuffer)
                goto exit;
            memcpy(newSegmentBuffer, textBuffer.data(), textBuffer.size() * sizeof(UChar));
            if (!textSegments)
                textSegments = adoptPtr(new Vector<TextSegment>);
            textSegments->append(make_pair(newSegmentBuffer, (unsigned)textBuffer.size()));
            textBuffer.clear();
        }
        textBuffer.append(it.characters(), it.length());
        bufferLength += it.length();
    }

    if (!bufferLength)
        return 0;

    // Since we know the size now, we can make a single buffer out of the pieces with one big alloc
    result = static_cast<UChar*>(malloc(bufferLength * sizeof(UChar)));
    if (!result)
        goto exit;

    {
        UChar* resultPos = result;
        if (textSegments) {
            unsigned size = textSegments->size();
            for (unsigned i = 0; i < size; ++i) {
                const TextSegment& segment = textSegments->at(i);
                memcpy(resultPos, segment.first, segment.second * sizeof(UChar));
                resultPos += segment.second;
            }
        }
        memcpy(resultPos, textBuffer.data(), textBuffer.size() * sizeof(UChar));
    }

exit:
    if (textSegments) {
        unsigned size = textSegments->size();
        for (unsigned i = 0; i < size; ++i)
            free(textSegments->at(i).first);
    }
    
    if (isDisplayString && r->ownerDocument())
        r->ownerDocument()->displayBufferModifiedByEncoding(result, bufferLength);

    return result;
}

String plainText(const Range* r, TextIteratorBehavior defaultBehavior)
{
    unsigned length;
    UChar* buf = plainTextToMallocAllocatedBuffer(r, length, false, defaultBehavior);
    if (!buf)
        return "";
    String result(buf, length);
    free(buf);
    return result;
}

static inline bool isAllCollapsibleWhitespace(const String& string)
{
    const UChar* characters = string.characters();
    unsigned length = string.length();
    for (unsigned i = 0; i < length; ++i) {
        if (!isCollapsibleWhitespace(characters[i]))
            return false;
    }
    return true;
}

static PassRefPtr<Range> collapsedToBoundary(const Range* range, bool forward)
{
    ExceptionCode ec = 0;
    RefPtr<Range> result = range->cloneRange(ec);
    ASSERT(!ec);
    result->collapse(!forward, ec);
    ASSERT(!ec);
    return result.release();
}

static size_t findPlainText(CharacterIterator& it, const String& target, FindOptions options, size_t& matchStart)
{
    matchStart = 0;
    size_t matchLength = 0;

    SearchBuffer buffer(target, options);

    if (buffer.needsMoreContext()) {
        RefPtr<Range> startRange = it.range();
        RefPtr<Range> beforeStartRange = startRange->ownerDocument()->createRange();
        ExceptionCode ec = 0;
        beforeStartRange->setEnd(startRange->startContainer(), startRange->startOffset(), ec);
        for (SimplifiedBackwardsTextIterator backwardsIterator(beforeStartRange.get()); !backwardsIterator.atEnd(); backwardsIterator.advance()) {
            buffer.prependContext(backwardsIterator.characters(), backwardsIterator.length());
            if (!buffer.needsMoreContext())
                break;
        }
    }

    while (!it.atEnd()) {
        it.advance(buffer.append(it.characters(), it.length()));
tryAgain:
        size_t matchStartOffset;
        if (size_t newMatchLength = buffer.search(matchStartOffset)) {
            // Note that we found a match, and where we found it.
            size_t lastCharacterInBufferOffset = it.characterOffset();
            ASSERT(lastCharacterInBufferOffset >= matchStartOffset);
            matchStart = lastCharacterInBufferOffset - matchStartOffset;
            matchLength = newMatchLength;
            // If searching forward, stop on the first match.
            // If searching backward, don't stop, so we end up with the last match.
            if (!(options & Backwards))
                break;
            goto tryAgain;
        }
        if (it.atBreak() && !buffer.atBreak()) {
            buffer.reachedBreak();
            goto tryAgain;
        }
    }

    return matchLength;
}

PassRefPtr<Range> findPlainText(const Range* range, const String& target, FindOptions options)
{
    // CharacterIterator requires renderers to be up-to-date
    range->ownerDocument()->updateLayout();

    // First, find the text.
    size_t matchStart;
    size_t matchLength;
    {
        CharacterIterator findIterator(range, TextIteratorEntersTextControls);
        matchLength = findPlainText(findIterator, target, options, matchStart);
        if (!matchLength)
            return collapsedToBoundary(range, !(options & Backwards));
    }

    // Then, find the document position of the start and the end of the text.
    CharacterIterator computeRangeIterator(range, TextIteratorEntersTextControls);
    return characterSubrange(computeRangeIterator, matchStart, matchLength);
}

}
