/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "InputHandler.h"

#include "BackingStore.h"
#include "BackingStoreClient.h"
#include "CSSStyleDeclaration.h"
#include "Chrome.h"
#include "ColorPickerClient.h"
#include "DOMSupport.h"
#include "DatePickerClient.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DocumentMarkerController.h"
#include "EditorClientBlackBerry.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLSelectElement.h"
#include "HTMLTextAreaElement.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PluginView.h"
#include "Range.h"
#include "RenderLayer.h"
#include "RenderMenuList.h"
#include "RenderPart.h"
#include "RenderText.h"
#include "RenderTextControl.h"
#include "RenderWidget.h"
#include "RenderedDocumentMarker.h"
#include "ScopePointer.h"
#include "SelectPopupClient.h"
#include "SelectionHandler.h"
#include "SpellChecker.h"
#include "SpellingHandler.h"
#include "SuggestionBoxHandler.h"
#include "TextCheckerClient.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "VisibleUnits.h"
#include "WebKitThreadViewportAccessor.h"
#include "WebPageClient.h"
#include "WebPage_p.h"
#include "WebSettings.h"
#include "htmlediting.h"

#include <BlackBerryPlatformDeviceInfo.h>
#include <BlackBerryPlatformIMF.h>
#include <BlackBerryPlatformKeyboardEvent.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformSettings.h>
#include <cmath>
#include <sys/keycodes.h>
#include <wtf/text/CString.h>

#define ENABLE_INPUT_LOG 0
#define ENABLE_FOCUS_LOG 0
#define ENABLE_SPELLING_LOG 0

using namespace BlackBerry::Platform;
using namespace WebCore;

#if ENABLE_INPUT_LOG
#define InputLog(severity, format, ...) Platform::logAlways(severity, format, ## __VA_ARGS__)
#else
#define InputLog(severity, format, ...)
#endif // ENABLE_INPUT_LOG

#if ENABLE_FOCUS_LOG
#define FocusLog(severity, format, ...) Platform::logAlways(severity, format, ## __VA_ARGS__)
#else
#define FocusLog(severity, format, ...)
#endif // ENABLE_FOCUS_LOG

#if ENABLE_SPELLING_LOG
#define SpellingLog(severity, format, ...) Platform::logAlways(severity, format, ## __VA_ARGS__)
#else
#define SpellingLog(severity, format, ...)
#endif // ENABLE_SPELLING_LOG

namespace BlackBerry {
namespace WebKit {

static const float zoomAnimationThreshold = 0.5;

class ProcessingChangeGuard {
public:
    ProcessingChangeGuard(InputHandler* inputHandler)
        : m_inputHandler(inputHandler)
        , m_savedProcessingChange(false)
    {
        ASSERT(m_inputHandler);

        m_savedProcessingChange = m_inputHandler->processingChange();
        m_inputHandler->setProcessingChange(true);
    }

    ~ProcessingChangeGuard()
    {
        m_inputHandler->setProcessingChange(m_savedProcessingChange);
    }

private:
    InputHandler* m_inputHandler;
    bool m_savedProcessingChange;
};

InputHandler::InputHandler(WebPagePrivate* page)
    : m_webPage(page)
    , m_currentFocusElement(0)
    , m_previousFocusableTextElement(0)
    , m_nextFocusableTextElement(0)
    , m_hasSubmitButton(false)
    , m_inputModeEnabled(false)
    , m_processingChange(false)
    , m_shouldEnsureFocusTextElementVisibleOnSelectionChanged(false)
    , m_currentFocusElementType(TextEdit)
    , m_currentFocusElementTextEditMask(DEFAULT_STYLE)
    , m_composingTextStart(0)
    , m_composingTextEnd(0)
    , m_pendingKeyboardVisibilityChange(NoChange)
    , m_delayKeyboardVisibilityChange(false)
    , m_sendFormStateOnNextKeyboardRequest(false)
    , m_request(0)
    , m_processingTransactionId(-1)
    , m_shouldNotifyWebView(true)
    , m_expectedKeyUpChar(0)
    , m_didSpellCheckWord(false)
    , m_spellingHandler(new SpellingHandler(this))
    , m_spellCheckStatusConfirmed(false)
    , m_globalSpellCheckStatus(false)
    , m_minimumSpellCheckingRequestSequence(-1)
    , m_elementTouchedIsCrossFrame(false)
{
}

InputHandler::~InputHandler()
{
    delete m_spellingHandler;
}

static BlackBerryInputType convertInputType(const HTMLInputElement* inputElement)
{
    if (inputElement->isPasswordField())
        return InputTypePassword;
    if (inputElement->isSearchField())
        return InputTypeSearch;
    if (inputElement->isEmailField())
        return InputTypeEmail;
    if (inputElement->isMonthField())
        return InputTypeMonth;
    if (inputElement->isNumberField())
        return InputTypeNumber;
    if (inputElement->isTelephoneField())
        return InputTypeTelephone;
    if (inputElement->isURLField())
        return InputTypeURL;
#if ENABLE(INPUT_TYPE_COLOR)
    if (inputElement->isColorControl())
        return InputTypeColor;
#endif
    if (inputElement->isDateField())
        return InputTypeDate;
    if (inputElement->isDateTimeField())
        return InputTypeDateTime;
    if (inputElement->isDateTimeLocalField())
        return InputTypeDateTimeLocal;
    if (inputElement->isTimeField())
        return InputTypeTime;
    // FIXME: missing WEEK popup selector
    if (DOMSupport::elementIdOrNameIndicatesEmail(inputElement))
        return InputTypeEmail;
    if (DOMSupport::elementIdOrNameIndicatesUrl(inputElement))
        return InputTypeURL;
    if (DOMSupport::elementPatternIndicatesNumber(inputElement))
        return InputTypeNumber;
    if (DOMSupport::elementPatternIndicatesHexadecimal(inputElement))
        return InputTypeHexadecimal;

    return InputTypeText;
}

static int64_t inputStyle(BlackBerryInputType type, const Element* element)
{
    switch (type) {
    case InputTypeEmail:
    case InputTypeURL:
    case InputTypeSearch:
    case InputTypeText:
    case InputTypeTextArea:
        {
            DOMSupport::AttributeState autoCompleteState = DOMSupport::elementSupportsAutocomplete(element);
            DOMSupport::AttributeState autoCorrectState = DOMSupport::elementSupportsAutocorrect(element);

            // Autocomplete disabled explicitly.
            if (autoCompleteState == DOMSupport::Off) {
                if (autoCorrectState == DOMSupport::On)
                    return NO_PREDICTION;
                return NO_AUTO_TEXT | NO_PREDICTION | NO_AUTO_CORRECTION;
            }

            // Autocomplete enabled explicitly.
            if (autoCompleteState == DOMSupport::On) {
                if (autoCorrectState == DOMSupport::Off)
                    return NO_AUTO_TEXT | NO_AUTO_CORRECTION;
                return DEFAULT_STYLE;
            }

            // Check for reserved strings and known types.
            if (type == InputTypeEmail || type == InputTypeURL || DOMSupport::elementIdOrNameIndicatesNoAutocomplete(element)) {
                if (autoCorrectState == DOMSupport::On)
                    return NO_PREDICTION;
                return NO_AUTO_TEXT | NO_PREDICTION | NO_AUTO_CORRECTION;
            }

            // Autocomplete state wasn't provided if it is a text area or autocorrect is on, apply support.
            if (autoCorrectState == DOMSupport::On || (type == InputTypeTextArea && autoCorrectState != DOMSupport::Off))
                return DEFAULT_STYLE;

            // Single line text input, without special features explicitly turned on or a textarea
            // with autocorrect disabled explicitly. Only enable predictions.
            return NO_AUTO_TEXT | NO_AUTO_CORRECTION;
        }
    case InputTypePassword:
    case InputTypeNumber:
    case InputTypeTelephone:
    case InputTypeHexadecimal:
        // Disable special handling.
        return NO_AUTO_TEXT | NO_PREDICTION | NO_AUTO_CORRECTION;
    default:
        break;
    }
    return DEFAULT_STYLE;
}

static VirtualKeyboardType convertInputTypeToVKBType(BlackBerryInputType inputType)
{
    switch (inputType) {
    case InputTypeURL:
        return VKBTypeUrl;
    case InputTypeEmail:
        return VKBTypeEmail;
    case InputTypeTelephone:
        return VKBTypePhone;
    case InputTypePassword:
        return VKBTypePassword;
    case InputTypeNumber:
    case InputTypeHexadecimal:
        return VKBTypePin;
    default:
        // All other types are text based use default keyboard.
        return VKBTypeDefault;
    }
}

static VirtualKeyboardType convertStringToKeyboardType(const AtomicString& string)
{
    DEFINE_STATIC_LOCAL(AtomicString, Default, ("default"));
    DEFINE_STATIC_LOCAL(AtomicString, Url, ("url"));
    DEFINE_STATIC_LOCAL(AtomicString, Email, ("email"));
    DEFINE_STATIC_LOCAL(AtomicString, Password, ("password"));
    DEFINE_STATIC_LOCAL(AtomicString, Web, ("web"));
    DEFINE_STATIC_LOCAL(AtomicString, Number, ("number"));
    DEFINE_STATIC_LOCAL(AtomicString, Symbol, ("symbol"));
    DEFINE_STATIC_LOCAL(AtomicString, Phone, ("phone"));
    DEFINE_STATIC_LOCAL(AtomicString, Pin, ("pin"));
    DEFINE_STATIC_LOCAL(AtomicString, Hex, ("hexadecimal"));

    if (string.isEmpty())
        return VKBTypeNotSet;
    if (equalIgnoringCase(string, Default))
        return VKBTypeDefault;
    if (equalIgnoringCase(string, Url))
        return VKBTypeUrl;
    if (equalIgnoringCase(string, Email))
        return VKBTypeEmail;
    if (equalIgnoringCase(string, Password))
        return VKBTypePassword;
    if (equalIgnoringCase(string, Web))
        return VKBTypeWeb;
    if (equalIgnoringCase(string, Number))
        return VKBTypeNumPunc;
    if (equalIgnoringCase(string, Symbol))
        return VKBTypeSymbol;
    if (equalIgnoringCase(string, Phone))
        return VKBTypePhone;
    if (equalIgnoringCase(string, Pin) || equalIgnoringCase(string, Hex))
        return VKBTypePin;
    return VKBTypeNotSet;
}

static VirtualKeyboardType keyboardTypeAttribute(const WebCore::Element* element)
{
    DEFINE_STATIC_LOCAL(QualifiedName, keyboardTypeAttr, (nullAtom, "data-blackberry-virtual-keyboard-type", nullAtom));

    if (element->fastHasAttribute(keyboardTypeAttr)) {
        AtomicString attributeString = element->fastGetAttribute(keyboardTypeAttr);
        return convertStringToKeyboardType(attributeString);
    }

    if (element->isFormControlElement()) {
        const HTMLFormControlElement* formElement = static_cast<const HTMLFormControlElement*>(element);
        if (formElement->form() && formElement->form()->fastHasAttribute(keyboardTypeAttr)) {
            AtomicString attributeString = formElement->form()->fastGetAttribute(keyboardTypeAttr);
            return convertStringToKeyboardType(attributeString);
        }
    }

    return VKBTypeNotSet;
}

static VirtualKeyboardEnterKeyType convertStringToKeyboardEnterKeyType(const AtomicString& string)
{
    DEFINE_STATIC_LOCAL(AtomicString, Default, ("default"));
    DEFINE_STATIC_LOCAL(AtomicString, Connect, ("connect"));
    DEFINE_STATIC_LOCAL(AtomicString, Done, ("done"));
    DEFINE_STATIC_LOCAL(AtomicString, Go, ("go"));
    DEFINE_STATIC_LOCAL(AtomicString, Join, ("join"));
    DEFINE_STATIC_LOCAL(AtomicString, Next, ("next"));
    DEFINE_STATIC_LOCAL(AtomicString, Search, ("search"));
    DEFINE_STATIC_LOCAL(AtomicString, Send, ("send"));
    DEFINE_STATIC_LOCAL(AtomicString, Submit, ("submit"));

    if (string.isEmpty())
        return VKBEnterKeyNotSet;
    if (equalIgnoringCase(string, Default))
        return VKBEnterKeyDefault;
    if (equalIgnoringCase(string, Connect))
        return VKBEnterKeyConnect;
    if (equalIgnoringCase(string, Done))
        return VKBEnterKeyDone;
    if (equalIgnoringCase(string, Go))
        return VKBEnterKeyGo;
    if (equalIgnoringCase(string, Join))
        return VKBEnterKeyJoin;
    if (equalIgnoringCase(string, Next))
        return VKBEnterKeyNext;
    if (equalIgnoringCase(string, Search))
        return VKBEnterKeySearch;
    if (equalIgnoringCase(string, Send))
        return VKBEnterKeySend;
    if (equalIgnoringCase(string, Submit))
        return VKBEnterKeySubmit;
    return VKBEnterKeyNotSet;
}

static VirtualKeyboardEnterKeyType keyboardEnterKeyTypeAttribute(const WebCore::Element* element)
{
    DEFINE_STATIC_LOCAL(QualifiedName, keyboardEnterKeyTypeAttr, (nullAtom, "data-blackberry-virtual-keyboard-enter-key", nullAtom));

    if (element->fastHasAttribute(keyboardEnterKeyTypeAttr)) {
        AtomicString attributeString = element->fastGetAttribute(keyboardEnterKeyTypeAttr);
        return convertStringToKeyboardEnterKeyType(attributeString);
    }

    if (element->isFormControlElement()) {
        const HTMLFormControlElement* formElement = static_cast<const HTMLFormControlElement*>(element);
        if (formElement->form() && formElement->form()->fastHasAttribute(keyboardEnterKeyTypeAttr)) {
            AtomicString attributeString = formElement->form()->fastGetAttribute(keyboardEnterKeyTypeAttr);
            return convertStringToKeyboardEnterKeyType(attributeString);
        }
    }

    return VKBEnterKeyNotSet;
}

void InputHandler::setProcessingChange(bool processingChange)
{
    if (processingChange == m_processingChange)
        return;

    m_processingChange = processingChange;

    if (!m_processingChange)
        m_webPage->m_selectionHandler->inputHandlerDidFinishProcessingChange();
}

WTF::String InputHandler::elementText()
{
    if (!isActiveTextEdit())
        return WTF::String();

    return DOMSupport::inputElementText(m_currentFocusElement.get());
}

BlackBerryInputType InputHandler::elementType(Element* element) const
{
    if (const HTMLInputElement* inputElement = toHTMLInputElement(element))
        return convertInputType(inputElement);

    if (isHTMLTextAreaElement(element))
        return InputTypeTextArea;

    // Default to InputTypeTextArea for content editable fields.
    return InputTypeTextArea;
}

void InputHandler::focusedNodeChanged()
{
    ASSERT(m_webPage->m_page->focusController());
    Frame* frame = m_webPage->m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->document())
        return;

    Node* node = frame->document()->focusedElement();

    if (isActiveTextEdit() && m_currentFocusElement == node) {
        notifyClientOfKeyboardVisibilityChange(true);
        return;
    }

    if (node && node->isElementNode()) {
        Element* element = toElement(node);
        if (DOMSupport::isElementTypePlugin(element)) {
            setPluginFocused(element);
            return;
        }

        if (DOMSupport::isTextBasedContentEditableElement(element) && !DOMSupport::isElementReadOnly(element)) {
            // Focused node is a text based input field, textarea or content editable field.
            setElementFocused(element);
            return;
        }
    } else if (node && DOMSupport::isTextBasedContentEditableElement(node->parentElement()) && !DOMSupport::isElementReadOnly(node->parentElement())) {
        setElementFocused(node->parentElement());
        return;
    }

    if (isActiveTextEdit() && m_currentFocusElement->isContentEditable()) {
        // This is a special handler for content editable fields. The focus node is the top most
        // field that is content editable, however, by enabling / disabling designmode and
        // content editable, it is possible through javascript or selection to trigger the focus node to
        // change even while maintaining editing on the same selection point. If the focus element
        // isn't contentEditable, but the current selection is, don't send a change notification.

        // When processing changes selection changes occur that may reset the focus element
        // and could potentially cause a crash as m_currentFocusElement should not be
        // changed during processing of an EditorCommand.
        if (processingChange())
            return;

        Frame* frame = m_currentFocusElement->document()->frame();
        ASSERT(frame);

        // Test the current selection to make sure that the content in focus is still content
        // editable. This may mean Javascript triggered a focus change by modifying the
        // top level parent of this object's content editable state without actually modifying
        // this particular object.
        // Example site: html5demos.com/contentEditable - blur event triggers focus change.
        if (frame == m_webPage->focusedOrMainFrame()
            && frame->selection()->start().anchorNode()
            && frame->selection()->start().anchorNode()->isContentEditable()
            && !m_elementTouchedIsCrossFrame)
                return;
    }

    // No valid focus element found for handling.
    setElementUnfocused();
}

void InputHandler::setPluginFocused(Element* element)
{
    ASSERT(DOMSupport::isElementTypePlugin(element));

    if (isActiveTextEdit())
        setElementUnfocused();

    m_currentFocusElementType = Plugin;
    m_currentFocusElement = element;
}

static bool convertStringToWchar(const WTF::String& string, wchar_t* dest, int destCapacity, int* destLength)
{
    ASSERT(dest);

    int length = string.length();

    if (!length) {
        destLength = 0;
        return true;
    }

    UErrorCode ec = U_ZERO_ERROR;

    // wchar_t strings sent to IMF are 32 bit so casting to UChar32 is safe.
    u_strToUTF32(reinterpret_cast<UChar32*>(dest), destCapacity, destLength, string.characters(), length, &ec);
    if (ec) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::convertStringToWchar Error converting string ec (%d).", ec);
        destLength = 0;
        return false;
    }
    return true;
}

static bool convertStringToWcharVector(const WTF::String& string, WTF::Vector<wchar_t>& wcharString)
{
    ASSERT(wcharString.isEmpty());

    int length = string.length();
    if (!length)
        return true;

    if (!wcharString.tryReserveCapacity(length + 1)) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::convertStringToWcharVector Cannot allocate memory for string.");
        return false;
    }

    int destLength = 0;
    if (!convertStringToWchar(string, wcharString.data(), length + 1, &destLength))
        return false;

    wcharString.resize(destLength);
    return true;
}

static WTF::String convertSpannableStringToString(spannable_string_t* src)
{
    if (!src || !src->str || !src->length)
        return WTF::String();

    WTF::Vector<UChar> dest;
    int destCapacity = (src->length * 2) + 1;
    if (!dest.tryReserveCapacity(destCapacity)) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::convertSpannableStringToString Cannot allocate memory for string.");
        return WTF::String();
    }

    int destLength = 0;
    UErrorCode ec = U_ZERO_ERROR;
    // wchar_t strings sent from IMF are 32 bit so casting to UChar32 is safe.
    u_strFromUTF32(dest.data(), destCapacity, &destLength, reinterpret_cast<UChar32*>(src->str), src->length, &ec);
    if (ec) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::convertSpannableStringToString Error converting string ec (%d).", ec);
        return WTF::String();
    }
    dest.resize(destLength);
    return WTF::String(dest.data(), destLength);
}

void InputHandler::sendLearnTextDetails(const WTF::String& string)
{
    Vector<wchar_t> wcharString;
    if (!convertStringToWcharVector(string, wcharString) || wcharString.isEmpty())
        return;

    m_webPage->m_client->inputLearnText(wcharString.data(), wcharString.size());
}

void InputHandler::learnText()
{
    if (!isActiveTextEdit())
        return;

    // Do not send (or calculate) the text when the field is NO_PREDICTION or NO_AUTO_TEXT.
    if (m_currentFocusElementTextEditMask & NO_PREDICTION || m_currentFocusElementTextEditMask & NO_AUTO_TEXT)
        return;

    WTF::String textInField(elementText());
    if (textInField.length() >= MaxLearnTextDataSize)
        textInField = textInField.substring(std::max(0, static_cast<int>(caretPosition() - MaxLearnTextDataSize)), std::min(textInField.length(), MaxLearnTextDataSize));

    textInField = textInField.stripWhiteSpace();

    // Build up the 500 character strings in word chunks.
    // Spec says 1000, but memory corruption has been observed.
    ASSERT(textInField.length() <= MaxLearnTextDataSize);

    if (textInField.isEmpty())
        return;

    InputLog(Platform::LogLevelInfo, "InputHandler::learnText '%s'", textInField.latin1().data());
    sendLearnTextDetails(textInField);
}

void InputHandler::callRequestCheckingFor(PassRefPtr<WebCore::SpellCheckRequest> spellCheckRequest)
{
    if (SpellChecker* spellChecker = getSpellChecker())
        spellChecker->requestCheckingFor(spellCheckRequest);
}

void InputHandler::requestCheckingOfString(PassRefPtr<WebCore::SpellCheckRequest> spellCheckRequest)
{
    SpellingLog(Platform::LogLevelInfo, "InputHandler::requestCheckingOfString '%s'", spellCheckRequest->data().text().latin1().data());

    if (!spellCheckRequest) {
        SpellingLog(Platform::LogLevelWarn, "InputHandler::requestCheckingOfString did not receive a valid request.");
        return;
    }

    if (spellCheckRequest->data().sequence() <= m_minimumSpellCheckingRequestSequence) {
        SpellingLog(Platform::LogLevelWarn, "InputHandler::requestCheckingOfString rejecting stale request with sequenceId=%d. Sentinal currently at %d."
            , spellCheckRequest->data().sequence(), m_minimumSpellCheckingRequestSequence);
        spellCheckRequest->didCancel();
        return;
    }

    unsigned requestLength = spellCheckRequest->data().text().length();

    // Check if the field should be spellchecked.
    if (!isActiveTextEdit() || !shouldSpellCheckElement(m_currentFocusElement.get()) || requestLength < 2) {
        SpellingLog(Platform::LogLevelWarn, "InputHandler::requestCheckingOfString request cancelled");
        spellCheckRequest->didCancel();
        return;
    }

    if (requestLength >= MaxSpellCheckingStringLength) {
        // Cancel this request and send it off in newly created chunks.
        spellCheckRequest->didCancel();
        m_spellingHandler->spellCheckTextBlock(m_currentFocusElement.get(), TextCheckingProcessIncremental);
        return;
    }

    wchar_t* checkingString = (wchar_t*)malloc(sizeof(wchar_t) * (requestLength + 1));
    if (!checkingString) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::requestCheckingOfString Cannot allocate memory for string.");
        spellCheckRequest->didCancel();
        return;
    }

    int paragraphLength = 0;
    if (!convertStringToWchar(spellCheckRequest->data().text(), checkingString, requestLength + 1, &paragraphLength)) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::requestCheckingOfString Failed to convert String to wchar type.");
        free(checkingString);
        spellCheckRequest->didCancel();
        return;
    }

    m_processingTransactionId = m_webPage->m_client->checkSpellingOfStringAsync(checkingString, static_cast<unsigned>(paragraphLength));
    free(checkingString);

    // If the call to the input service did not go through, then cancel the request so we don't block endlessly.
    // This should still take transactionId as a parameter to maintain the same behavior as if InputMethodSupport
    // were to cancel a request during processing.
    if (m_processingTransactionId == -1) { // Error before sending request to input service.
        spellCheckRequest->didCancel();
        return;
    }

    m_request = spellCheckRequest;
}

void InputHandler::spellCheckingRequestProcessed(int32_t transactionId, spannable_string_t* spannableString)
{
#if !ENABLE_SPELLING_LOG
    UNUSED_PARAM(transactionId)
#endif

    SpellingLog(Platform::LogLevelWarn,
        "InputHandler::spellCheckingRequestProcessed Expected transaction id %d, received %d. %s",
        m_processingTransactionId,
        transactionId,
        transactionId == m_processingTransactionId ? "" : "We are out of sync with input service.");

    if (!spannableString
        || !isActiveTextEdit()
        || !DOMSupport::elementHasContinuousSpellCheckingEnabled(m_currentFocusElement)
        || !m_processingTransactionId
        || !m_request) {
        SpellingLog(Platform::LogLevelWarn, "InputHandler::spellCheckingRequestProcessed Cancelling request with transactionId %d.", transactionId);
        if (m_request) {
            m_request->didCancel();
            m_request = 0;
        }
        m_processingTransactionId = -1;
        return;
    }

    Vector<TextCheckingResult> results;

    // Convert the spannableString to TextCheckingResult then append to results vector.
    WTF::String replacement;
    TextCheckingResult textCheckingResult;
    textCheckingResult.type = TextCheckingTypeSpelling;
    textCheckingResult.replacement = replacement;
    textCheckingResult.location = 0;
    textCheckingResult.length = 0;

    span_t* span = spannableString->spans;
    for (unsigned i = 0; i < spannableString->spans_count; i++) {
        if (!span)
            break;
        if (span->end < span->start) {
            m_request->didCancel();
            m_request = 0;
            return;
        }
        if (span->attributes_mask & MISSPELLED_WORD_ATTRIB) {
            textCheckingResult.location = span->start;
            // The end point includes the character that it is before. Ie, 0, 0
            // applies to the first character as the end point includes the character
            // at the position. This means the endPosition is always +1.
            textCheckingResult.length = span->end - span->start + 1;
            results.append(textCheckingResult);
        }
        span++;
    }

    // free data that we malloc'ed in InputMethodSupport
    free(spannableString->spans);
    free(spannableString);

    m_request->didSucceed(results);
    m_request = 0;
}

SpellChecker* InputHandler::getSpellChecker()
{
    if (!m_currentFocusElement || !m_currentFocusElement->document())
        return 0;

    if (Frame* frame = m_currentFocusElement->document()->frame())
        if (Editor* editor = frame->editor())
            return editor->spellChecker();

    return 0;
}

bool InputHandler::shouldRequestSpellCheckingOptionsForPoint(const Platform::IntPoint& documentContentPosition, const Element* touchedElement, imf_sp_text_t& spellCheckingOptionRequest)
{
    if (!isActiveTextEdit())
        return false;

    Element* currentFocusElement = m_currentFocusElement.get();
    if (!currentFocusElement || !currentFocusElement->isElementNode())
        return false;

    while (!currentFocusElement->isRootEditableElement()) {
        Element* parentElement = currentFocusElement->parentElement();
        if (!parentElement)
            break;
        currentFocusElement = parentElement;
    }

    if (touchedElement != currentFocusElement)
        return false;

    LayoutPoint contentPos(documentContentPosition);
    contentPos = DOMSupport::convertPointToFrame(m_webPage->mainFrame(), m_webPage->focusedOrMainFrame(), roundedIntPoint(contentPos));

    Document* document = currentFocusElement->document();
    ASSERT(document);

    RenderedDocumentMarker* marker = document->markers()->renderedMarkerContainingPoint(contentPos, DocumentMarker::Spelling);
    if (!marker)
        return false;

    m_didSpellCheckWord = true;

    // Populate the marker details in preparation for the request as the marker is
    // not guaranteed to be valid after the cursor is placed.
    spellCheckingOptionRequest.startTextPosition = marker->startOffset();
    spellCheckingOptionRequest.endTextPosition = marker->endOffset();

    m_spellCheckingOptionsRequest.startTextPosition = 0;
    m_spellCheckingOptionsRequest.endTextPosition = 0;

    SpellingLog(Platform::LogLevelInfo,
        "InputHandler::shouldRequestSpellCheckingOptionsForPoint Found spelling marker at point %s\nMarker start %d end %d",
        documentContentPosition.toString().c_str(),
        spellCheckingOptionRequest.startTextPosition,
        spellCheckingOptionRequest.endTextPosition);

    return true;
}

void InputHandler::requestSpellingCheckingOptions(imf_sp_text_t& spellCheckingOptionRequest, WebCore::IntSize& screenOffset, const bool shouldMoveDialog)
{
    // If the caret is no longer active, no message should be sent.
    if (m_webPage->focusedOrMainFrame()->selection()->selectionType() != VisibleSelection::CaretSelection)
        return;

    if (!m_currentFocusElement || !m_currentFocusElement->document() || !m_currentFocusElement->document()->frame())
        return;

    if (shouldMoveDialog || !(spellCheckingOptionRequest.startTextPosition || spellCheckingOptionRequest.startTextPosition)) {
        if (m_spellCheckingOptionsRequest.startTextPosition || m_spellCheckingOptionsRequest.endTextPosition)
            spellCheckingOptionRequest = m_spellCheckingOptionsRequest;
    }

    if (!shouldMoveDialog && spellCheckingOptionRequest.startTextPosition == spellCheckingOptionRequest.endTextPosition)
        return;

    if (screenOffset.width() == -1 && screenOffset.height() == -1) {
        screenOffset.setWidth(m_screenOffset.width());
        screenOffset.setHeight(m_screenOffset.height());
    } else {
        m_screenOffset.setWidth(screenOffset.width());
        m_screenOffset.setHeight(screenOffset.height());
    }

    // imf_sp_text_t should be generated in pixel viewport coordinates.
    // Caret is in document coordinates.
    WebCore::IntRect caretRect = m_webPage->focusedOrMainFrame()->selection()->selection().visibleStart().absoluteCaretBounds();

    // Shift from posible iFrame to root view/main frame.
    caretRect = m_webPage->focusedOrMainFrame()->view()->contentsToRootView(caretRect);

    // Shift to document scroll position.
    const WebCore::IntPoint scrollPosition = m_webPage->mainFrame()->view()->scrollPosition();
    caretRect.move(scrollPosition.x(), scrollPosition.y());

    // If we are only moving the dialog, we don't need to provide startTextPosition and endTextPosition so this logic can be skipped.
    if (!shouldMoveDialog) {
        // Calculate the offset for contentEditable since the marker offsets are relative to the node.
        // Get caret selection. Though the spelling markers might no longer exist, if this method is called we can assume the caret was placed on top of a marker earlier.
        VisibleSelection caretSelection = m_currentFocusElement->document()->frame()->selection()->selection();
        caretSelection = DOMSupport::visibleSelectionForClosestActualWordStart(caretSelection);
        VisiblePosition wordStart = caretSelection.visibleStart();
        VisiblePosition wordEnd = endOfWord(caretSelection.visibleStart());

        if (HTMLTextFormControlElement* controlElement = DOMSupport::toTextControlElement(m_currentFocusElement.get())) {
            spellCheckingOptionRequest.startTextPosition = controlElement->indexForVisiblePosition(wordStart);
            spellCheckingOptionRequest.endTextPosition = controlElement->indexForVisiblePosition(wordEnd);
        } else {
            unsigned location = 0;
            unsigned length = 0;

            // Create a range from the start to end of word.
            RefPtr<Range> rangeSelection = VisibleSelection(wordStart, wordEnd).toNormalizedRange();
            if (!rangeSelection)
                return;

            TextIterator::getLocationAndLengthFromRange(m_currentFocusElement.get(), rangeSelection.get(), location, length);
            spellCheckingOptionRequest.startTextPosition = location;
            spellCheckingOptionRequest.endTextPosition = location + length;
        }
    }
    m_spellCheckingOptionsRequest = spellCheckingOptionRequest;

    InputLog(Platform::LogLevelInfo,
        "InputHandler::requestSpellingCheckingOptions caretRect topLeft=%s, bottomRight=%s, startTextPosition=%d, endTextPosition=%d",
        Platform::IntPoint(caretRect.minXMinYCorner()).toString().c_str(),
        Platform::IntPoint(caretRect.maxXMaxYCorner()).toString().c_str(),
        spellCheckingOptionRequest.startTextPosition,
        spellCheckingOptionRequest.endTextPosition);

    m_webPage->m_client->requestSpellingCheckingOptions(spellCheckingOptionRequest, caretRect, screenOffset, shouldMoveDialog);
}

void InputHandler::setElementUnfocused(bool refocusOccuring)
{
    if (isActiveTextEdit() && DOMSupport::isElementAndDocumentAttached(m_currentFocusElement.get())) {
        FocusLog(Platform::LogLevelInfo, "InputHandler::setElementUnfocused");

        // Pass any text into the field to IMF to learn.
        learnText();

        // End any composition that is in progress.
        finishComposition();

        // Only hide the keyboard if we aren't refocusing on a new input field.
        if (!refocusOccuring) {
            notifyClientOfKeyboardVisibilityChange(false, true /* triggeredByFocusChange */);
            m_webPage->m_client->showFormControls(false /* visible */);
        }

        m_webPage->m_client->inputFocusLost();

        // Hide the suggestion box if it is visible.
        hideTextInputTypeSuggestionBox();

        // Repaint the element absent of the caret.
        if (m_currentFocusElement->renderer())
            m_currentFocusElement->renderer()->repaint();

        // If the frame selection isn't focused, focus it.
        FrameSelection* frameSelection = m_currentFocusElement->document()->frame()->selection();
        if (frameSelection && !frameSelection->isFocused())
            frameSelection->setFocused(true);
    }

    // Cancel any preexisting spellcheck requests.
    if (m_request) {
        stopPendingSpellCheckRequests();
        m_request->didCancel();
        m_request = 0;
    }

    // Clear the node details.
    m_currentFocusElement = 0;
    m_currentFocusElementType = TextEdit;
    m_previousFocusableTextElement = 0;
    m_nextFocusableTextElement = 0;
    m_hasSubmitButton = false;
}

bool InputHandler::isInputModeEnabled() const
{
    // Input mode is enabled when set, or when dump render tree or always show keyboard setting is enabled.
    return m_inputModeEnabled || m_webPage->m_dumpRenderTree || Platform::Settings::instance()->alwaysShowKeyboardOnFocus();
}

void InputHandler::setInputModeEnabled(bool active)
{
    FocusLog(Platform::LogLevelInfo,
        "InputHandler::setInputModeEnabled '%s', override is '%s'",
        active ? "true" : "false",
        m_webPage->m_dumpRenderTree || Platform::Settings::instance()->alwaysShowKeyboardOnFocus() ? "true" : "false");

    m_inputModeEnabled = active;

    // If the frame selection isn't focused, focus it.
    if (isInputModeEnabled()
        && isActiveTextEdit()
        && DOMSupport::isElementAndDocumentAttached(m_currentFocusElement.get())
        && !m_currentFocusElement->document()->frame()->selection()->isFocused())
        m_currentFocusElement->document()->frame()->selection()->setFocused(true);
}

void InputHandler::updateFormState()
{
    m_previousFocusableTextElement = 0;
    m_nextFocusableTextElement = 0;
    m_hasSubmitButton = false;

    if (!m_currentFocusElement || !m_currentFocusElement->isFormControlElement())
        return;

    HTMLFormElement* formElement = static_cast<HTMLFormControlElement*>(m_currentFocusElement.get())->form();
    if (!formElement)
        return;

    const Vector<FormAssociatedElement*> formElementList = formElement->associatedElements();
    int formElementCount = formElementList.size();
    if (formElementCount < 2)
        return;

    m_hasSubmitButton = true;

    // Walk all elements in the form to determine next/prev elements.
    // For each element in the form we need to do the following:
    // If it's the focus node, use render order to try to find the next/prev directly.
    // For all other nodes:
    // 1) If the focused node has a specific tab index, compare the elements tab
    //    index with the current prev/next looking for the best match.
    // 2) If the focused node does not have a tab index, update the maximum
    //    tab index value, required for prev navigation.
    int focusTabIndex = static_cast<Node*>(m_currentFocusElement.get())->tabIndex();
    int prevTabIndex = -1;
    int nextTabIndex = std::numeric_limits<short>::max() + 1;
    InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState form has %d fields and tabIndex %d", formElementCount, focusTabIndex);

    Element* firstInFieldWithoutTabIndex = 0;
    Element* highestTabIndexElement = 0;
    for (int focusElementId = 0; focusElementId < formElementCount; focusElementId++) {
        // Check for the focused element, and if we don't have any target nodes, use the form order next
        // and previous as placeholders. In a form without provided tab indices, this will determine the
        // control fields.
        Element* element = const_cast<HTMLElement*>(toHTMLElement(formElementList[focusElementId]));
        if (element == m_currentFocusElement) {
            InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState found focused element.");

            // If the focus tab index is set for the node, don't use the logical ordering.
            // Jump from last tab index to un-ordered is done separately.
            if (focusTabIndex)
                continue;

            // Get the next/prev element if we don't already have a tab index based item.
            // Previous
            if (!m_previousFocusableTextElement) {
                for (int previousElementId = focusElementId - 1; previousElementId >= 0; previousElementId--) {
                    Element* prevElement = const_cast<HTMLElement*>(toHTMLElement(formElementList[previousElementId]));
                    if (DOMSupport::isTextBasedContentEditableElement(prevElement) && !DOMSupport::isElementReadOnly(prevElement) && !static_cast<Node*>(prevElement)->tabIndex()) {
                        m_previousFocusableTextElement = prevElement;
                        InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState found previous element");
                        break;
                    }
                }
            }

            // Next
            if (!m_nextFocusableTextElement) {
                for (int nextElementId = focusElementId + 1; nextElementId < formElementCount; nextElementId++) {
                    Element* nextElement = const_cast<HTMLElement*>(toHTMLElement(formElementList[nextElementId]));
                    if (DOMSupport::isTextBasedContentEditableElement(nextElement) && !DOMSupport::isElementReadOnly(nextElement) && !static_cast<Node*>(nextElement)->tabIndex()) {
                        m_nextFocusableTextElement = nextElement;
                        InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState found next element");
                        break;
                    }
                }
            }
        } else if (focusTabIndex) {
            InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState processing element");
            if (DOMSupport::isTextBasedContentEditableElement(element) && !DOMSupport::isElementReadOnly(element)) {
                InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState processing element valid");
                if (int tabIndex = static_cast<Node*>(element)->tabIndex()) {
                    InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState processing element with tab index %d", tabIndex);
                    // Compare for the before and after form positions based on the tab index, and/or form position
                    // if tab indexes are equal form position should be used.
                    if (tabIndex && tabIndex < focusTabIndex && tabIndex > prevTabIndex) {
                        m_previousFocusableTextElement = element;
                        prevTabIndex = tabIndex;
                        InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState found previous element with tabIndex %d", tabIndex);
                    } else if (tabIndex > focusTabIndex && tabIndex < nextTabIndex) {
                        m_nextFocusableTextElement = element;
                        nextTabIndex = tabIndex;
                        InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState found next element with tabIndex %d", tabIndex);
                    }
                } else if (!firstInFieldWithoutTabIndex) {
                    // Store the first field in the form without a tab index if we have a form with some tab indexes the "next" one from
                    // the highest tab index is the first field without a tab index.
                    firstInFieldWithoutTabIndex = element;
                }
            }
        } else {
            // The field has no tab index, if it's the first node we'll need the highest tab index field
            // when navigating backwards.
            if (int tabIndex = static_cast<Node*>(element)->tabIndex()) {
                if (!highestTabIndexElement || (tabIndex > static_cast<Node*>(highestTabIndexElement)->tabIndex()))
                    highestTabIndexElement = element;
            }
        }
    }

    if (!m_nextFocusableTextElement && firstInFieldWithoutTabIndex) {
        // No next focusable field was found, but a first one without a tabindex was found, use it as the next field.
        m_nextFocusableTextElement = firstInFieldWithoutTabIndex;
    }

    if (!m_previousFocusableTextElement && highestTabIndexElement) {
        // No prev focusable field was found, use the highest tab index as previous since this field must not have
        // a tabindex, otheriwse highestTabIndexElement would be null.
        m_previousFocusableTextElement = highestTabIndexElement;
    }

    if (!m_nextFocusableTextElement && !m_previousFocusableTextElement) {
        m_hasSubmitButton = false;
        InputLog(Platform::LogLevelInfo, "InputHandler::updateFormState no valid elements found, clearing state.");
    }
}

void InputHandler::focusNextField()
{
    if (!m_nextFocusableTextElement)
        return;

    m_nextFocusableTextElement->focus();
}

void InputHandler::focusPreviousField()
{
    if (!m_previousFocusableTextElement)
        return;

    m_previousFocusableTextElement->focus();
}

void InputHandler::submitForm()
{
    if (!m_hasSubmitButton)
        return;

    HTMLFormElement* formElement = static_cast<HTMLFormControlElement*>(m_currentFocusElement.get())->form();
    if (!formElement)
        return;

    InputLog(Platform::LogLevelInfo, "InputHandler::submitForm triggered");

    if (elementType(m_currentFocusElement.get()) != InputTypeTextArea) {
        handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_RETURN, Platform::KeyboardEvent::KeyChar, 0), false /* changeIsPartOfComposition */);

        // Did this clear the focus? If so, form was submitted or invalid.
        if (!isActiveTextEdit())
            return;
    }

    // Validate form data and if valid, submit.
    if (formElement->checkValidity())
        formElement->submit();
}

static void addInputStyleMaskForKeyboardType(int64_t& inputMask, VirtualKeyboardType keyboardType)
{
    switch (keyboardType) {
    case VKBTypeUrl:
        inputMask |= IMF_URL_TYPE;
        break;
    case VKBTypePassword:
        inputMask |= IMF_PASSWORD_TYPE;
        break;
    case VKBTypePin:
        inputMask |= IMF_PIN_TYPE;
        break;
    case VKBTypePhone:
        inputMask |= IMF_PHONE_TYPE;
        break;
    case VKBTypeEmail:
        inputMask |= IMF_EMAIL_TYPE;
        break;
    default:
        break;
    }
}

void InputHandler::setElementFocused(Element* element)
{
    ASSERT(DOMSupport::isTextBasedContentEditableElement(element));
    ASSERT(element && element->document() && element->document()->frame());

#if ENABLE_SPELLING_LOG
    BlackBerry::Platform::StopWatch timer;
    timer.start();
#endif

    if (!element || !(element->document()))
        return;

    Frame* frame = element->document()->frame();
    if (!frame)
        return;

    if (frame->selection()->isFocused() != isInputModeEnabled())
        frame->selection()->setFocused(isInputModeEnabled());

    // Ensure visible when refocusing.
    // If device does not have physical keyboard, wait to ensure visible until VKB resizes viewport so that both animations are combined into one.
    m_shouldEnsureFocusTextElementVisibleOnSelectionChanged = isActiveTextEdit() || DeviceInfo::instance()->hasPhysicalKeyboard();

    // Clear the existing focus node details.
    setElementUnfocused(true /*refocusOccuring*/);

    // Mark this element as active and add to frame set.
    m_currentFocusElement = element;
    m_currentFocusElementType = TextEdit;
    updateFormState();

    if (isInputModeEnabled() && !m_delayKeyboardVisibilityChange)
        m_webPage->m_client->showFormControls(m_hasSubmitButton /* visible */, m_previousFocusableTextElement, m_nextFocusableTextElement);
    else
        m_sendFormStateOnNextKeyboardRequest = true;

    // Send details to the client about this element.
    BlackBerryInputType type = elementType(element);
    m_currentFocusElementTextEditMask = inputStyle(type, element);

    VirtualKeyboardType keyboardType = keyboardTypeAttribute(element);
    if (keyboardType == VKBTypeNotSet)
        keyboardType = convertInputTypeToVKBType(type);

    addInputStyleMaskForKeyboardType(m_currentFocusElementTextEditMask, keyboardType);

    VirtualKeyboardEnterKeyType enterKeyType = keyboardEnterKeyTypeAttribute(element);

    if (enterKeyType == VKBEnterKeyNotSet && type != InputTypeTextArea) {
        if (element->isFormControlElement()) {
            const HTMLFormControlElement* formElement = static_cast<const HTMLFormControlElement*>(element);
            if (formElement->form() && formElement->form()->defaultButton())
                enterKeyType = VKBEnterKeySubmit;
        }
    }

    FocusLog(Platform::LogLevelInfo,
        "InputHandler::setElementFocused, Type=%d, Style=%lld, Keyboard Type=%d, Enter Key=%d",
        type, m_currentFocusElementTextEditMask, keyboardType, enterKeyType);

    m_webPage->m_client->inputFocusGained(m_currentFocusElementTextEditMask, keyboardType, enterKeyType);

    handleInputLocaleChanged(m_webPage->m_webSettings->isWritingDirectionRTL());

    // update the suggestion box
    showTextInputTypeSuggestionBox();

    if (!m_delayKeyboardVisibilityChange)
        notifyClientOfKeyboardVisibilityChange(true, true /* triggeredByFocusChange */);

#if ENABLE_SPELLING_LOG
    SpellingLog(Platform::LogLevelInfo, "InputHandler::setElementFocused Focusing the field took %f seconds.", timer.elapsed());
#endif

    // Spellcheck the field in its entirety.
    spellCheckTextBlock(element);

#if ENABLE_SPELLING_LOG
    SpellingLog(Platform::LogLevelInfo, "InputHandler::setElementFocused Spellchecking the field increased the total time to focus to %f seconds.", timer.elapsed());
#endif
}

void InputHandler::spellCheckTextBlock(Element* element)
{
    SpellingLog(Platform::LogLevelInfo, "InputHandler::spellCheckTextBlock");

    if (!element) {
        // Fall back to a valid focused element.
        if (!m_currentFocusElement)
            return;

        element = m_currentFocusElement.get();
    }

    // Check if the field should be spellchecked.
    if (!shouldSpellCheckElement(element) || !isActiveTextEdit())
        return;

    m_spellingHandler->spellCheckTextBlock(element, TextCheckingProcessBatch);
}

bool InputHandler::shouldSpellCheckElement(const Element* element) const
{
    DOMSupport::AttributeState spellCheckAttr = DOMSupport::elementSupportsSpellCheck(element);

    // Explicitly set to off.
    if (spellCheckAttr == DOMSupport::Off)
        return false;

    // Undefined and part of a set of cases which we do not wish to check. This includes user names and email addresses, so we are piggybacking on NoAutocomplete cases.
    if (spellCheckAttr == DOMSupport::Default && (m_currentFocusElementTextEditMask & NO_AUTO_TEXT))
        return false;

    // Check if the system spell check setting is off
    return m_spellCheckStatusConfirmed ? m_globalSpellCheckStatus : true;
}

void InputHandler::stopPendingSpellCheckRequests(bool isRestartRequired)
{
    m_spellingHandler->setSpellCheckActive(false);
    // Prevent response from propagating through.
    m_processingTransactionId = 0;

    // Reject requests until lastRequestSequence. This helps us clear the queue of stale requests.
    if (SpellChecker* spellChecker = getSpellChecker()) {
        if (spellChecker->lastRequestSequence() != spellChecker->lastProcessedSequence()) {
            SpellingLog(LogLevelInfo, "InputHandler::stopPendingSpellCheckRequests will block requests up to lastRequest=%d [lastProcessed=%d]"
                , spellChecker->lastRequestSequence(), spellChecker->lastProcessedSequence());
            // Prevent requests in queue from executing.
            m_minimumSpellCheckingRequestSequence = spellChecker->lastRequestSequence();
            if (isRestartRequired && !compositionActive()) {
                // Create new spellcheck requests to replace those that were invalidated.
                spellCheckTextBlock();
            }
        }
    }
}

void InputHandler::redrawSpellCheckDialogIfRequired(const bool shouldMoveDialog)
{
    if (didSpellCheckWord()) {
        imf_sp_text_t spellCheckingOptionRequest;
        spellCheckingOptionRequest.startTextPosition = 0;
        spellCheckingOptionRequest.endTextPosition = 0;
        WebCore::IntSize screenOffset(-1, -1);
        requestSpellingCheckingOptions(spellCheckingOptionRequest, screenOffset, shouldMoveDialog);
    }
}

bool InputHandler::openDatePopup(HTMLInputElement* element, BlackBerryInputType type)
{
    if (!element || element->isDisabledOrReadOnly() || !DOMSupport::isDateTimeInputField(element))
        return false;

    if (isActiveTextEdit())
        clearCurrentFocusElement();

    m_currentFocusElement = element;
    m_currentFocusElementType = TextPopup;

    switch (type) {
    case BlackBerry::Platform::InputTypeDate:
    case BlackBerry::Platform::InputTypeTime:
    case BlackBerry::Platform::InputTypeDateTime:
    case BlackBerry::Platform::InputTypeDateTimeLocal:
    case BlackBerry::Platform::InputTypeMonth: {
        // Date input have button appearance, we hide caret when they get clicked.
        element->document()->frame()->selection()->setCaretVisible(false);

        WTF::String value = element->value();
        WTF::String min = element->getAttribute(HTMLNames::minAttr).string();
        WTF::String max = element->getAttribute(HTMLNames::maxAttr).string();
        double step = element->getAttribute(HTMLNames::stepAttr).toDouble();

        DatePickerClient* client = new DatePickerClient(type, value, min, max, step,  m_webPage, element);
        return m_webPage->openPagePopup(client,  WebCore::IntRect());
        }
    default: // Other types not supported
        return false;
    }
}

bool InputHandler::openColorPopup(HTMLInputElement* element)
{
    if (!element || element->isDisabledOrReadOnly() || !DOMSupport::isColorInputField(element))
        return false;

    if (isActiveTextEdit())
        clearCurrentFocusElement();

    m_currentFocusElement = element;
    m_currentFocusElementType = TextPopup;

    ColorPickerClient* client = new ColorPickerClient(element->value(), m_webPage, element);
    return m_webPage->openPagePopup(client, WebCore::IntRect());
}

void InputHandler::setInputValue(const WTF::String& value)
{
    if (!isActiveTextPopup())
        return;

    HTMLInputElement* inputElement = toHTMLInputElement(m_currentFocusElement.get());
    inputElement->setValue(value);
    clearCurrentFocusElement();
}

void InputHandler::nodeTextChanged(const Node* node)
{
    if (processingChange() || !node || node != m_currentFocusElement || !m_shouldNotifyWebView)
        return;

    InputLog(Platform::LogLevelInfo, "InputHandler::nodeTextChanged");

    m_webPage->m_client->inputTextChanged();

    // Remove the attributed text markers as the previous call triggered an end to
    // the composition.
    removeAttributedTextMarker();
}

WebCore::IntRect InputHandler::boundingBoxForInputField()
{
    if (!isActiveTextEdit())
        return WebCore::IntRect();

    if (!m_currentFocusElement->renderer())
        return WebCore::IntRect();

    // type="search" can have a 'X', so take the inner block bounding box to not include it.
    if (HTMLInputElement* element = m_currentFocusElement->toInputElement()) {
        if (element->isSearchField())
            return element->innerBlockElement()->renderer()->absoluteBoundingBoxRect();
        return m_currentFocusElement->renderer()->absoluteBoundingBoxRect();
    }

    if (isHTMLTextAreaElement(m_currentFocusElement))
        return m_currentFocusElement->renderer()->absoluteBoundingBoxRect();

    // Content Editable can't rely on the bounding box since it isn't fixed.
    return WebCore::IntRect();
}

void InputHandler::ensureFocusTextElementVisible(CaretScrollType scrollType)
{
    if (!isActiveTextEdit() || !isInputModeEnabled() || !m_currentFocusElement->document())
        return;

    if (!(Platform::Settings::instance()->allowedScrollAdjustmentForInputFields() & scrollType))
        return;

    // Fixed position elements cannot be scrolled into view.
    if (DOMSupport::isFixedPositionOrHasFixedPositionAncestor(m_currentFocusElement->renderer()))
        return;

    Frame* elementFrame = m_currentFocusElement->document()->frame();
    if (!elementFrame)
        return;

    Frame* mainFrame = m_webPage->mainFrame();
    if (!mainFrame)
        return;

    FrameView* mainFrameView = mainFrame->view();
    if (!mainFrameView)
        return;

    WebCore::IntRect selectionFocusRect;
    switch (elementFrame->selection()->selectionType()) {
    case VisibleSelection::CaretSelection:
        selectionFocusRect = elementFrame->selection()->absoluteCaretBounds();
        break;
    case VisibleSelection::RangeSelection: {
        Position selectionPosition;
        if (m_webPage->m_selectionHandler->lastUpdatedEndPointIsValid())
            selectionPosition = elementFrame->selection()->end();
        else
            selectionPosition = elementFrame->selection()->start();
        selectionFocusRect = VisiblePosition(selectionPosition).absoluteCaretBounds();
        break;
    }
    case VisibleSelection::NoSelection:
        m_shouldEnsureFocusTextElementVisibleOnSelectionChanged = true;
        return;
    }

    int fontHeight = selectionFocusRect.height();

    // If the text is too small, zoom in to make it a minimum size.
    // The minimum size being defined as 3 mm is a good value based on my observations.
    static const int s_minimumTextHeightInPixels = Graphics::Screen::primaryScreen()->heightInMMToPixels(3);

    double zoomScaleRequired;
    if (m_webPage->isUserScalable() && fontHeight && fontHeight * m_webPage->currentScale() < s_minimumTextHeightInPixels && !isRunningDrt())
        zoomScaleRequired = static_cast<double>(s_minimumTextHeightInPixels) / fontHeight;
    else
        zoomScaleRequired = m_webPage->currentScale(); // Don't scale.

    // Zoom level difference must exceed the given threshold before we perform a zoom animation.
    if (abs(zoomScaleRequired - m_webPage->currentScale()) < zoomAnimationThreshold)
        zoomScaleRequired = m_webPage->currentScale(); // Don't scale.

    // The scroll location we should go to given the zoom required, could be adjusted later.
    WebCore::FloatPoint offset(selectionFocusRect.location().x() - m_webPage->scrollPosition().x(), selectionFocusRect.location().y() - m_webPage->scrollPosition().y());
    double inverseScale = zoomScaleRequired / m_webPage->currentScale();
    WebCore::IntPoint destinationScrollLocation = WebCore::IntPoint(
        max(0, static_cast<int>(roundf(selectionFocusRect.location().x() - offset.x() / inverseScale))),
        max(0, static_cast<int>(roundf(selectionFocusRect.location().y() - offset.y() / inverseScale))));

    if (elementFrame != mainFrame) { // Element is in a subframe.
        // Remove any scroll offset within the subframe to get the point relative to the main frame.
        selectionFocusRect.move(-elementFrame->view()->scrollPosition().x(), -elementFrame->view()->scrollPosition().y());

        // Adjust the selection rect based on the frame offset in relation to the main frame if it's a subframe.
        if (elementFrame->ownerRenderer()) {
            WebCore::IntPoint frameOffset = elementFrame->ownerRenderer()->absoluteContentBox().location();
            selectionFocusRect.move(frameOffset.x(), frameOffset.y());
        }
    }

    const Platform::ViewportAccessor* viewportAccessor = m_webPage->m_webkitThreadViewportAccessor;
    if (scrollType == EdgeIfNeeded
        && (viewportAccessor->documentViewportRect().contains(selectionFocusRect))
        && zoomScaleRequired == m_webPage->currentScale()) {
        // Already in view and no zoom is required, return early.
        return;
    }

    bool shouldConstrainScrollingToContentEdge = true;
    Position start = elementFrame->selection()->start();
    if (start.anchorNode() && start.anchorNode()->renderer()) {
        if (RenderLayer* layer = start.anchorNode()->renderer()->enclosingLayer()) {
            // Screen rect after the required zoom.
            WebCore::IntRect actualScreenRect = WebCore::IntRect(destinationScrollLocation.x(), destinationScrollLocation.y(), m_webPage->actualVisibleSize().width() / inverseScale, m_webPage->actualVisibleSize().height() / inverseScale);

            ScrollAlignment horizontalScrollAlignment = ScrollAlignment::alignToEdgeIfNeeded;
            ScrollAlignment verticalScrollAlignment = ScrollAlignment::alignToEdgeIfNeeded;

            if (scrollType != EdgeIfNeeded) {
                // Align the selection rect if possible so that we show the field's
                // outline if the caret is at the edge of the field.
                if (RenderObject* focusedRenderer = m_currentFocusElement->renderer()) {
                    WebCore::IntRect nodeOutlineBounds = focusedRenderer->absoluteOutlineBounds();
                    WebCore::IntRect caretAtEdgeRect = rectForCaret(0);
                    int paddingX = abs(caretAtEdgeRect.x() - nodeOutlineBounds.x());
                    int paddingY = abs(caretAtEdgeRect.y() - nodeOutlineBounds.y());

                    if (selectionFocusRect.x() - paddingX == nodeOutlineBounds.x())
                        selectionFocusRect.setX(nodeOutlineBounds.x());
                    else if (selectionFocusRect.maxX() + paddingX == nodeOutlineBounds.maxX())
                        selectionFocusRect.setX(nodeOutlineBounds.maxX() - selectionFocusRect.width());
                    if (selectionFocusRect.y() - paddingY == nodeOutlineBounds.y())
                        selectionFocusRect.setY(nodeOutlineBounds.y() - selectionFocusRect.height());
                    else if (selectionFocusRect.maxY() + paddingY == nodeOutlineBounds.maxY())
                        selectionFocusRect.setY(nodeOutlineBounds.maxY() - selectionFocusRect.height());

                    // If the editing point is on the left hand side of the screen when the node's
                    // rect is edge aligned, edge align the node rect.
                    if (selectionFocusRect.x() - caretAtEdgeRect.x() < actualScreenRect.width() / 2)
                        selectionFocusRect.setX(nodeOutlineBounds.x());
                    else
                        horizontalScrollAlignment = ScrollAlignment::alignCenterIfNeeded;

                }
                verticalScrollAlignment = (scrollType == CenterAlways) ? ScrollAlignment::alignCenterAlways : ScrollAlignment::alignCenterIfNeeded;
            }

            // Pad the rect to improve the visual appearance.
            // Convert the padding back from transformed to ensure a consistent padding regardless of
            // zoom level as controls do not zoom.
            static const int s_focusRectPaddingSize = Graphics::Screen::primaryScreen()->heightInMMToPixels(12);
            selectionFocusRect.inflate(std::ceilf(viewportAccessor->documentFromPixelContents(Platform::FloatSize(0, s_focusRectPaddingSize)).height()));

            WebCore::IntRect revealRect(layer->getRectToExpose(actualScreenRect, selectionFocusRect, horizontalScrollAlignment, verticalScrollAlignment));

            // Don't constrain scroll position when animation finishes.
            shouldConstrainScrollingToContentEdge = false;

            // In order to adjust the scroll position to ensure the focused input field is visible,
            // we allow overscrolling. However this overscroll has to be strictly allowed towards the
            // bottom of the page on the y axis only, where the virtual keyboard pops up from.
            destinationScrollLocation = revealRect.location();
            destinationScrollLocation.clampNegativeToZero();
            WebCore::IntPoint maximumScrollPosition = WebCore::IntPoint(mainFrameView->contentsWidth() - actualScreenRect.width(), mainFrameView->contentsHeight() - actualScreenRect.height());
            destinationScrollLocation = destinationScrollLocation.shrunkTo(maximumScrollPosition);
        }
    }

    InputLog(Platform::LogLevelInfo,
        "InputHandler::ensureFocusTextElementVisible zooming in to %f from %f and scrolling to point %s from %s",
        zoomScaleRequired, m_webPage->currentScale(),
        Platform::IntPoint(destinationScrollLocation).toString().c_str(),
        Platform::IntPoint(mainFrameView->scrollPosition()).toString().c_str());

    m_webPage->animateToScaleAndDocumentScrollPosition(zoomScaleRequired, WebCore::FloatPoint(destinationScrollLocation), shouldConstrainScrollingToContentEdge);
}

void InputHandler::ensureFocusPluginElementVisible()
{
    if (!isActivePlugin() || !m_currentFocusElement->document())
        return;

    Frame* elementFrame = m_currentFocusElement->document()->frame();
    if (!elementFrame)
        return;

    Frame* mainFrame = m_webPage->mainFrame();
    if (!mainFrame)
        return;

    FrameView* mainFrameView = mainFrame->view();
    if (!mainFrameView)
        return;

    WebCore::IntRect selectionFocusRect;

    RenderWidget* renderWidget = static_cast<RenderWidget*>(m_currentFocusElement->renderer());
    if (renderWidget) {
        PluginView* pluginView = toPluginView(renderWidget->widget());

        if (pluginView)
            selectionFocusRect = pluginView->ensureVisibleRect();
    }

    if (selectionFocusRect.isEmpty())
        return;

    // FIXME: We may need to scroll the subframe (recursively) in the future. Revisit this...
    if (elementFrame != mainFrame) { // Element is in a subframe.
        // Remove any scroll offset within the subframe to get the point relative to the main frame.
        selectionFocusRect.move(-elementFrame->view()->scrollPosition().x(), -elementFrame->view()->scrollPosition().y());

        // Adjust the selection rect based on the frame offset in relation to the main frame if it's a subframe.
        if (elementFrame->ownerRenderer()) {
            WebCore::IntPoint frameOffset = elementFrame->ownerRenderer()->absoluteContentBox().location();
            selectionFocusRect.move(frameOffset.x(), frameOffset.y());
        }
    }

    WebCore::IntRect actualScreenRect = WebCore::IntRect(mainFrameView->scrollPosition(), m_webPage->actualVisibleSize());
    if (actualScreenRect.contains(selectionFocusRect))
        return;

    // Calculate a point such that the center of the requested rectangle
    // is at the center of the screen. FIXME: If the element was partially on screen
    // we might want to just bring the offscreen portion into view, someone needs
    // to decide if that's the behavior we want or not.
    WebCore::IntPoint pos(selectionFocusRect.center());
    pos.move(-actualScreenRect.width() / 2, -actualScreenRect.height() / 2);

    mainFrameView->setScrollPosition(pos);
}

void InputHandler::ensureFocusElementVisible(bool centerInView)
{
    if (isActivePlugin())
        ensureFocusPluginElementVisible();
    else
        ensureFocusTextElementVisible(centerInView ? CenterAlways : CenterIfNeeded);
}

void InputHandler::frameUnloaded(const Frame* frame)
{
    if (!isActiveTextEdit())
        return;

    if (m_currentFocusElement->document()->frame() != frame)
        return;

    FocusLog(Platform::LogLevelInfo, "InputHandler::frameUnloaded");

    setElementUnfocused(false /*refocusOccuring*/);
}

void InputHandler::setDelayKeyboardVisibilityChange(bool value)
{
    m_delayKeyboardVisibilityChange = value;
    m_pendingKeyboardVisibilityChange = NoChange;
}

void InputHandler::processPendingKeyboardVisibilityChange()
{
    if (!m_delayKeyboardVisibilityChange) {
        ASSERT(m_pendingKeyboardVisibilityChange == NoChange);
        return;
    }

    m_delayKeyboardVisibilityChange = false;

    if (m_pendingKeyboardVisibilityChange == NoChange)
        return;

    notifyClientOfKeyboardVisibilityChange(m_pendingKeyboardVisibilityChange == Visible);
    m_pendingKeyboardVisibilityChange = NoChange;
}

void InputHandler::notifyClientOfKeyboardVisibilityChange(bool visible, bool triggeredByFocusChange)
{
    // If we aren't ready for input, keyboard changes should be ignored.
    if (!isInputModeEnabled() && visible)
        return;

    // If we are processing a change assume the keyboard is visbile to avoid
    // flooding the VKB with show requests.
    if (!triggeredByFocusChange && processingChange() && visible)
        return;

    if (!m_delayKeyboardVisibilityChange) {
        if (m_sendFormStateOnNextKeyboardRequest) {
            m_webPage->m_client->showFormControls(m_hasSubmitButton /* visible */, m_previousFocusableTextElement, m_nextFocusableTextElement);
            m_sendFormStateOnNextKeyboardRequest = false;
        }
        m_webPage->showVirtualKeyboard(visible);
        return;
    }

    m_pendingKeyboardVisibilityChange = visible ? Visible : NotVisible;
}

bool InputHandler::selectionAtStartOfElement()
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    if (!selectionStart())
        return true;

    return false;
}

bool InputHandler::selectionAtEndOfElement()
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    return selectionStart() == static_cast<int>(elementText().length());
}

int InputHandler::selectionStart() const
{
    return selectionPosition(true);
}

int InputHandler::selectionEnd() const
{
    return selectionPosition(false);
}

int InputHandler::selectionPosition(bool start) const
{
    if (!m_currentFocusElement->document() || !m_currentFocusElement->document()->frame())
        return 0;

    if (HTMLTextFormControlElement* controlElement = DOMSupport::toTextControlElement(m_currentFocusElement.get()))
        return start ? controlElement->selectionStart() : controlElement->selectionEnd();

    FrameSelection caretSelection;
    caretSelection.setSelection(m_currentFocusElement->document()->frame()->selection()->selection());
    RefPtr<Range> rangeSelection = caretSelection.selection().toNormalizedRange();
    if (!rangeSelection)
        return 0;

    int selectionPointInNode = start ? rangeSelection->startOffset() : rangeSelection->endOffset();
    Node* containerNode = start ? rangeSelection->startContainer() : rangeSelection->endContainer();

    ExceptionCode ec;
    RefPtr<Range> rangeForNode = rangeOfContents(m_currentFocusElement.get());
    rangeForNode->setEnd(containerNode, selectionPointInNode, ec);
    ASSERT(!ec);

    return TextIterator::rangeLength(rangeForNode.get());
}

void InputHandler::selectionChanged()
{
    // This method can get called during WebPage shutdown process.
    // If that is the case, just bail out since the client is not
    // in a safe state of trust to request anything else from it.
    if (!m_webPage->m_mainFrame)
        return;

    if (!isActiveTextEdit())
        return;

    if (processingChange()) {
        m_webPage->m_client->suppressCaretChangeNotification(true /*shouldClearState*/);
        return;
    }

    // Scroll the field if necessary. This must be done even if we are processing
    // a change as the text change may have moved the caret. IMF doesn't require
    // the update, but the user needs to see the caret.
    if (m_shouldEnsureFocusTextElementVisibleOnSelectionChanged) {
        ensureFocusTextElementVisible(EdgeIfNeeded);
        m_shouldEnsureFocusTextElementVisibleOnSelectionChanged = false;
    }

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    if (!m_shouldNotifyWebView)
        return;

    int newSelectionStart = selectionStart();
    int newSelectionEnd = selectionEnd();

    InputLog(Platform::LogLevelInfo,
        "InputHandler::selectionChanged selectionStart=%u, selectionEnd=%u",
        newSelectionStart, newSelectionEnd);

    m_webPage->m_client->inputSelectionChanged(newSelectionStart, newSelectionEnd);

    // Remove the attributed text markers as the previous call triggered an end to
    // the composition.
    removeAttributedTextMarker();
}

bool InputHandler::setCursorPosition(int location)
{
    return setSelection(location, location);
}

bool InputHandler::setSelection(int start, int end, bool changeIsPartOfComposition)
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    ProcessingChangeGuard guard(this);

    VisibleSelection newSelection = DOMSupport::visibleSelectionForRangeInputElement(m_currentFocusElement.get(), start, end);
    m_currentFocusElement->document()->frame()->selection()->setSelection(newSelection, changeIsPartOfComposition ? 0 : FrameSelection::CloseTyping | FrameSelection::ClearTypingStyle);

    InputLog(Platform::LogLevelInfo,
        "InputHandler::setSelection selectionStart=%u, selectionEnd=%u",
        start, end);

    return start == selectionStart() && end == selectionEnd();
}

WebCore::IntRect InputHandler::rectForCaret(int index)
{
    if (!isActiveTextEdit())
        return WebCore::IntRect();

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    if (index < 0 || index > static_cast<int>(elementText().length())) {
        // Invalid request.
        return WebCore::IntRect();
    }

    FrameSelection caretSelection;
    caretSelection.setSelection(DOMSupport::visibleSelectionForRangeInputElement(m_currentFocusElement.get(), index, index).visibleStart());
    caretSelection.modify(FrameSelection::AlterationExtend, DirectionForward, CharacterGranularity);
    return caretSelection.selection().visibleStart().absoluteCaretBounds();
}

void InputHandler::cancelSelection()
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());

    int selectionStartPosition = selectionStart();
    ProcessingChangeGuard guard(this);
    setCursorPosition(selectionStartPosition);
}

bool InputHandler::isNavigationKey(unsigned character) const
{
    return character == KEYCODE_UP
        || character == KEYCODE_DOWN
        || character == KEYCODE_LEFT
        || character == KEYCODE_RIGHT;
}

bool InputHandler::handleKeyboardInput(const Platform::KeyboardEvent& keyboardEvent, bool changeIsPartOfComposition)
{
    InputLog(Platform::LogLevelInfo,
        "InputHandler::handleKeyboardInput received character='%c', type=%d",
        keyboardEvent.character(), keyboardEvent.type());

    // Clearing the m_shouldNotifyWebView state on any KeyboardEvent.
    m_shouldNotifyWebView = true;

    // Enable input mode if we are processing a key event.
    setInputModeEnabled();

    Platform::KeyboardEvent::Type type = keyboardEvent.type();
    /*
     * IMF sends us an unadultered KeyUp for all key presses. This key event should be allowed to be processed at all times.
     * We bypass the check because the state of composition has no implication on this key event.
     * In order to ensure we allow the correct key event through, we keep track of key down events with m_expectedKeyUpChar.
    */
    if (type == Platform::KeyboardEvent::KeyUp) {
        // When IMF auto-capitalizes a KeyDown, say the first letter of a new sentence, our KeyUp will still be in lowercase.
        if (m_expectedKeyUpChar == keyboardEvent.character() || (isASCIIUpper(m_expectedKeyUpChar) && m_expectedKeyUpChar == toASCIIUpper(keyboardEvent.character()))) {
            m_expectedKeyUpChar = 0;
            changeIsPartOfComposition = true;
        }
    }

    // If we aren't specifically part of a composition, fail, IMF should never send key input
    // while composing text. If IMF has failed, we should have already finished the
    // composition manually. There is a caveat for KeyUp which is explained above.
    if (!changeIsPartOfComposition && compositionActive()) {
        if (type == Platform::KeyboardEvent::KeyDown && isNavigationKey(keyboardEvent.character()))
            removeAttributedTextMarker();
        else
            return false;
    }

    ProcessingChangeGuard guard(this);

    unsigned adjustedModifiers = keyboardEvent.modifiers();
    if (WTF::isASCIIUpper(keyboardEvent.character()))
        adjustedModifiers |= KEYMOD_SHIFT;

    ASSERT(m_webPage->m_page->focusController());
    bool keyboardEventHandled = false;
    if (Frame* focusedFrame = m_webPage->m_page->focusController()->focusedFrame()) {
        bool isKeyChar = type == Platform::KeyboardEvent::KeyChar;

        // If this is a KeyChar type then we handle it as a keydown followed by a key up.
        if (isKeyChar)
            type = Platform::KeyboardEvent::KeyDown;
        else if (type == Platform::KeyboardEvent::KeyDown) {
            m_expectedKeyUpChar = keyboardEvent.character();

            m_shouldNotifyWebView = shouldNotifyWebView(keyboardEvent);
        }

        Platform::KeyboardEvent adjustedKeyboardEvent(keyboardEvent.character(), type, adjustedModifiers, keyboardEvent.keycode(), keyboardEvent.alternateCharacter(), keyboardEvent.sourceDevice());
        keyboardEventHandled = focusedFrame->eventHandler()->keyEvent(PlatformKeyboardEvent(adjustedKeyboardEvent));

        m_shouldNotifyWebView = true;

        if (isKeyChar) {
            type = Platform::KeyboardEvent::KeyUp;
            adjustedKeyboardEvent = Platform::KeyboardEvent(keyboardEvent.character(), type, adjustedModifiers, keyboardEvent.keycode(), keyboardEvent.alternateCharacter(), keyboardEvent.sourceDevice());
            keyboardEventHandled = focusedFrame->eventHandler()->keyEvent(PlatformKeyboardEvent(adjustedKeyboardEvent)) || keyboardEventHandled;
        }

        if (!changeIsPartOfComposition && type == Platform::KeyboardEvent::KeyUp)
            ensureFocusTextElementVisible(EdgeIfNeeded);
    }

    if (m_currentFocusElement && keyboardEventHandled)
        showTextInputTypeSuggestionBox();

    return keyboardEventHandled;
}

bool InputHandler::shouldNotifyWebView(const Platform::KeyboardEvent& keyboardEvent)
{
    // If we receive the KeyDown of a Backspace or Enter key, set this flag to prevent sending unnecessary selection and caret changes to IMF.
    return !(keyboardEvent.character() == KEYCODE_BACKSPACE || keyboardEvent.character() == KEYCODE_RETURN || keyboardEvent.character() == KEYCODE_KP_ENTER);
}

bool InputHandler::deleteSelection()
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());
    Frame* frame = m_currentFocusElement->document()->frame();

    if (frame->selection()->selectionType() != VisibleSelection::RangeSelection)
        return false;

    ASSERT(frame->editor());
    if (!handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_BACKSPACE, Platform::KeyboardEvent::KeyDown, 0), false /* changeIsPartOfComposition */))
        return false;

    selectionChanged();
    return true;
}

void InputHandler::insertText(const WTF::String& string)
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame() && m_currentFocusElement->document()->frame()->editor());
    Editor* editor = m_currentFocusElement->document()->frame()->editor();
    editor->command("InsertText").execute(string);
}

void InputHandler::clearField()
{
    if (!isActiveTextEdit())
        return;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame() && m_currentFocusElement->document()->frame()->editor());
    Editor* editor = m_currentFocusElement->document()->frame()->editor();

    editor->command("SelectAll").execute();
    editor->command("DeleteBackward").execute();
}

bool InputHandler::executeTextEditCommand(const WTF::String& commandName)
{
    ASSERT(m_webPage->focusedOrMainFrame() && m_webPage->focusedOrMainFrame()->editor());
    Editor* editor = m_webPage->focusedOrMainFrame()->editor();

    return editor->command(commandName).execute();
}

void InputHandler::cut()
{
    executeTextEditCommand("Cut");
}

void InputHandler::copy()
{
    executeTextEditCommand("Copy");
}

void InputHandler::paste()
{
    executeTextEditCommand("Paste");
}

void InputHandler::selectAll()
{
    executeTextEditCommand("SelectAll");
}

void InputHandler::addAttributedTextMarker(int start, int end, const AttributeTextStyle& style)
{
    if ((end - start) < 1 || end > static_cast<int>(elementText().length()))
        return;

    RefPtr<Range> markerRange = DOMSupport::visibleSelectionForRangeInputElement(m_currentFocusElement.get(), start, end).toNormalizedRange();
    m_currentFocusElement->document()->markers()->addMarker(markerRange.get(), DocumentMarker::AttributeText, WTF::String("Input Marker"), style);
}

void InputHandler::removeAttributedTextMarker()
{
    // Remove all attribute text markers.
    if (m_currentFocusElement && m_currentFocusElement->document())
        m_currentFocusElement->document()->markers()->removeMarkers(DocumentMarker::AttributeText);

    m_composingTextStart = 0;
    m_composingTextEnd = 0;
}

void InputHandler::clearCurrentFocusElement()
{
    if (m_currentFocusElement)
        m_currentFocusElement->blur();
}

bool InputHandler::willOpenPopupForNode(Node* node)
{
    // This method must be kept synchronized with InputHandler::didNodeOpenPopup.
    if (!node)
        return false;

    ASSERT(!node->isInShadowTree());

    if (node->hasTagName(HTMLNames::selectTag) || isHTMLOptionElement(node)) {
        // We open list popups for options and selects.
        return true;
    }

    if (node->isElementNode()) {
        Element* element = toElement(node);
        if (DOMSupport::isPopupInputField(element))
            return true;
    }

    return false;
}

bool InputHandler::didNodeOpenPopup(Node* node)
{
    // This method must be kept synchronized with InputHandler::willOpenPopupForNode.
    if (!node)
        return false;

    ASSERT(!node->isInShadowTree());

    if (node->hasTagName(HTMLNames::selectTag))
        return openSelectPopup(toHTMLSelectElement(node));

    if (isHTMLOptionElement(node)) {
        HTMLOptionElement* optionElement = toHTMLOptionElement(node);
        return openSelectPopup(optionElement->ownerSelectElement());
    }

    if (HTMLInputElement* element = node->toInputElement()) {
        if (DOMSupport::isDateTimeInputField(element))
            return openDatePopup(element, elementType(element));

        if (DOMSupport::isColorInputField(element))
            return openColorPopup(element);
    }
    return false;
}

bool InputHandler::openSelectPopup(HTMLSelectElement* select)
{
    if (!select || select->isDisabledFormControl())
        return false;

    // If there's no view, do nothing and return.
    if (!select->document()->view())
        return false;

    if (isActiveTextEdit())
        clearCurrentFocusElement();

    m_currentFocusElement = select;
    m_currentFocusElementType = SelectPopup;

    const WTF::Vector<HTMLElement*>& listItems = select->listItems();
    int size = listItems.size();

    bool multiple = select->multiple();
    ScopeArray<BlackBerry::Platform::String> labels;
    labels.reset(new BlackBerry::Platform::String[size]);

    bool* enableds = 0;
    int* itemTypes = 0;
    bool* selecteds = 0;

    if (size) {
        enableds = new bool[size];
        itemTypes = new int[size];
        selecteds = new bool[size];
        for (int i = 0; i < size; i++) {
            if (isHTMLOptionElement(listItems[i])) {
                HTMLOptionElement* option = toHTMLOptionElement(listItems[i]);
                labels[i] = option->textIndentedToRespectGroupLabel();
                enableds[i] = option->isDisabledFormControl() ? 0 : 1;
                selecteds[i] = option->selected();
                itemTypes[i] = option->parentNode() && isHTMLOptGroupElement(option->parentNode()) ? TypeOptionInGroup : TypeOption;
            } else if (isHTMLOptGroupElement(listItems[i])) {
                HTMLOptGroupElement* optGroup = toHTMLOptGroupElement(listItems[i]);
                labels[i] = optGroup->groupLabelText();
                enableds[i] = optGroup->isDisabledFormControl() ? 0 : 1;
                selecteds[i] = false;
                itemTypes[i] = TypeGroup;
            } else if (listItems[i]->hasTagName(HTMLNames::hrTag)) {
                enableds[i] = false;
                selecteds[i] = false;
                itemTypes[i] = TypeSeparator;
            }
        }
    }

    SelectPopupClient* selectClient = new SelectPopupClient(multiple, size, labels, enableds, itemTypes, selecteds, m_webPage, select);
    WebCore::IntRect elementRectInRootView = select->document()->view()->contentsToRootView(enclosingIntRect(select->getRect()));
    // Fail to create HTML popup, use the old path
    if (!m_webPage->openPagePopup(selectClient, elementRectInRootView))
        m_webPage->m_client->openPopupList(multiple, size, labels, enableds, itemTypes, selecteds);
    delete[] enableds;
    delete[] itemTypes;
    delete[] selecteds;
    return true;
}

void InputHandler::setPopupListIndex(int index)
{
    if (index == -2) // Abandon
        return clearCurrentFocusElement();

    if (!isActiveSelectPopup())
        return clearCurrentFocusElement();

    RenderObject* renderer = m_currentFocusElement->renderer();
    if (renderer && renderer->isMenuList()) {
        RenderMenuList* renderMenu = toRenderMenuList(renderer);
        renderMenu->hidePopup();
    }

    HTMLSelectElement* selectElement = toHTMLSelectElement(m_currentFocusElement.get());
    int optionIndex = selectElement->listToOptionIndex(index);
    selectElement->optionSelectedByUser(optionIndex, true /* deselect = true */, true /* fireOnChangeNow = false */);
    clearCurrentFocusElement();
}

void InputHandler::setPopupListIndexes(int size, const bool* selecteds)
{
    if (!isActiveSelectPopup())
        return clearCurrentFocusElement();

    if (size < 0)
        return;

    HTMLSelectElement* selectElement = toHTMLSelectElement(m_currentFocusElement.get());
    const WTF::Vector<HTMLElement*>& items = selectElement->listItems();
    if (items.size() != static_cast<unsigned>(size))
        return;

    HTMLOptionElement* option;
    for (int i = 0; i < size; i++) {
        if (isHTMLOptionElement(items[i])) {
            option = toHTMLOptionElement(items[i]);
            option->setSelectedState(selecteds[i]);
        }
    }

    // Force repaint because we do not send mouse events to the select element
    // and the element doesn't automatically repaint itself.
    selectElement->dispatchFormControlChangeEvent();
    selectElement->renderer()->repaint();
    clearCurrentFocusElement();
}

bool InputHandler::setBatchEditingActive(bool active)
{
    if (!isActiveTextEdit())
        return false;

    ASSERT(m_currentFocusElement->document());
    ASSERT(m_currentFocusElement->document()->frame());

    // FIXME switch this to m_currentFocusElement->document()->frame() when we have separate
    // backingstore for each frame.
    BackingStoreClient* backingStoreClient = m_webPage->backingStoreClient();
    ASSERT(backingStoreClient);

    // Enable / Disable the backingstore to prevent visual updates.
    // FIXME: Do we really need to suspend/resume both backingstore and screen here?
    if (!active) {
        backingStoreClient->backingStore()->resumeBackingStoreUpdates();
        backingStoreClient->backingStore()->resumeScreenUpdates(BackingStore::RenderAndBlit);
    } else {
        backingStoreClient->backingStore()->suspendBackingStoreUpdates();
        backingStoreClient->backingStore()->suspendScreenUpdates();
    }

    return true;
}

bool InputHandler::isCaretAtEndOfText()
{
    return caretPosition() == static_cast<int>(elementText().length());
}

int InputHandler::caretPosition() const
{
    if (!isActiveTextEdit())
        return -1;

    // NOTE: This function is expected to return the start of the selection if
    // a selection is applied.
    return selectionStart();
}

int relativeLeftOffset(int caretPosition, int leftOffset)
{
    ASSERT(caretPosition >= 0);

    return std::max(0, caretPosition - leftOffset);
}

int relativeRightOffset(int caretPosition, unsigned totalLengthOfText, int rightOffset)
{
    ASSERT(caretPosition >= 0);
    ASSERT(totalLengthOfText < INT_MAX);

    return std::min(caretPosition + rightOffset, static_cast<int>(totalLengthOfText));
}

bool InputHandler::deleteTextRelativeToCursor(int leftOffset, int rightOffset)
{
    if (!isActiveTextEdit() || compositionActive())
        return false;

    InputLog(Platform::LogLevelInfo,
        "InputHandler::deleteTextRelativeToCursor left %d right %d",
        leftOffset, rightOffset);

    int caretOffset = caretPosition();
    int start = relativeLeftOffset(caretOffset, leftOffset);
    int end = relativeRightOffset(caretOffset, elementText().length(), rightOffset);

    // If we have backspace in a single character, send this to webkit as a KeyboardEvent. Otherwise, call deleteText.
    if (leftOffset == 1 && !rightOffset) {
        if (selectionActive())
            return deleteSelection();

        if (!handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_BACKSPACE, Platform::KeyboardEvent::KeyDown, 0), true /* changeIsPartOfComposition */))
            return false;
    } else if (!deleteText(start, end))
        return false;

    ProcessingChangeGuard guard(this);

    // Scroll the field if necessary. The automatic update is suppressed
    // by the processing change guard.
    ensureFocusTextElementVisible(EdgeIfNeeded);

    return true;
}

bool InputHandler::deleteText(int start, int end)
{
    if (!isActiveTextEdit())
        return false;

    {
        ProcessingChangeGuard guard(this);

        if (end - start == 1)
            return handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_BACKSPACE, Platform::KeyboardEvent::KeyDown, 0), true /* changeIsPartOfComposition */);

        if (!setSelection(start, end, true /*changeIsPartOfComposition*/))
            return false;
    }

    InputLog(Platform::LogLevelInfo, "InputHandler::deleteText start %d end %d", start, end);

    return deleteSelection();
}

spannable_string_t* InputHandler::spannableTextInRange(int start, int end, int32_t)
{
    if (!isActiveTextEdit())
        return 0;

    if (start >= end)
        return 0;

    int length = end - start;

    WTF::String textString = elementText().substring(start, length);

    spannable_string_t* pst = (spannable_string_t*)fastMalloc(sizeof(spannable_string_t));

    // Don't use fastMalloc in case the string is unreasonably long. fastMalloc will
    // crash immediately on failure.
    pst->str = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));
    if (!pst->str) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::spannableTextInRange Cannot allocate memory for string.");
        free(pst);
        return 0;
    }

    int stringLength = 0;
    if (!convertStringToWchar(textString, pst->str, length + 1, &stringLength)) {
        Platform::logAlways(Platform::LogLevelCritical, "InputHandler::spannableTextInRange failed to convert string.");
        free(pst->str);
        free(pst);
        return 0;
    }

    pst->length = stringLength;
    pst->spans_count = 0;
    pst->spans = 0;

    return pst;
}

spannable_string_t* InputHandler::selectedText(int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    return spannableTextInRange(selectionStart(), selectionEnd(), flags);
}

spannable_string_t* InputHandler::textBeforeCursor(int32_t length, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    int caretOffset = caretPosition();
    int start = relativeLeftOffset(caretOffset, length);
    int end = caretOffset;

    return spannableTextInRange(start, end, flags);
}

spannable_string_t* InputHandler::textAfterCursor(int32_t length, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    int caretOffset = caretPosition();
    int start = caretOffset;
    int end = relativeRightOffset(caretOffset, elementText().length(), length);

    return spannableTextInRange(start, end, flags);
}

extracted_text_t* InputHandler::extractedTextRequest(extracted_text_request_t*, int32_t flags)
{
    if (!isActiveTextEdit())
        return 0;

    extracted_text_t* extractedText = (extracted_text_t *)fastMalloc(sizeof(extracted_text_t));

    // 'flags' indicates whether the text is being monitored. This is not currently used.

    // FIXME add smart limiting based on the hint sizes. Currently return all text.

    extractedText->text = spannableTextInRange(0, elementText().length(), flags);

    // FIXME confirm these should be 0 on this requested. Text changes will likely require
    // the end be the length.
    extractedText->partial_start_offset = 0;
    extractedText->partial_end_offset = 0;
    extractedText->start_offset = 0;

    // Adjust selection values relative to the start offset, which may be a subset
    // of the text in the field.
    extractedText->selection_start = selectionStart() - extractedText->start_offset;
    extractedText->selection_end = selectionEnd() - extractedText->start_offset;

    // selectionActive is not limited to inside the extracted text.
    bool selectionActive = extractedText->selection_start != extractedText->selection_end;
    bool singleLine = isHTMLInputElement(m_currentFocusElement);

    // FIXME flags has two values in doc, enum not in header yet.
    extractedText->flags = selectionActive & singleLine;

    return extractedText;
}

static void addCompositionTextStyleToAttributeTextStyle(AttributeTextStyle& style)
{
    style.setUnderline(AttributeTextStyle::StandardUnderline);
}

static void addActiveTextStyleToAttributeTextStyle(AttributeTextStyle& style)
{
    style.setBackgroundColor(Color("blue"));
    style.setTextColor(Color("white"));
}

static AttributeTextStyle compositionTextStyle()
{
    AttributeTextStyle style;
    addCompositionTextStyleToAttributeTextStyle(style);
    return style;
}

static AttributeTextStyle textStyleFromMask(int64_t mask)
{
    AttributeTextStyle style;
    if (mask & COMPOSED_TEXT_ATTRIB)
        addCompositionTextStyleToAttributeTextStyle(style);

    if (mask & ACTIVE_REGION_ATTRIB)
        addActiveTextStyleToAttributeTextStyle(style);

    return style;
}

bool InputHandler::removeComposedText()
{
    if (compositionActive()) {
        if (!deleteText(m_composingTextStart, m_composingTextEnd)) {
            // Could not remove the existing composition region.
            return false;
        }
        removeAttributedTextMarker();
    }

    return true;
}

int32_t InputHandler::setComposingRegion(int32_t start, int32_t end)
{
    if (!isActiveTextEdit())
        return -1;

    if (!removeComposedText()) {
        // Could not remove the existing composition region.
        return -1;
    }

    m_composingTextStart = start;
    m_composingTextEnd = end;

    if (compositionActive())
        addAttributedTextMarker(start, end, compositionTextStyle());

    InputLog(Platform::LogLevelInfo, "InputHandler::setComposingRegion start %d end %d", start, end);

    return 0;
}

int32_t InputHandler::finishComposition()
{
    if (!isActiveTextEdit())
        return -1;

    // FIXME if no composition is active, should we return failure -1?
    if (!compositionActive())
        return 0;

    // Remove all markers.
    removeAttributedTextMarker();

    InputLog(Platform::LogLevelInfo, "InputHandler::finishComposition completed");

    return 0;
}

span_t* InputHandler::firstSpanInString(spannable_string_t* spannableString, SpannableStringAttribute attrib)
{
    span_t* span = spannableString->spans;
    for (unsigned i = 0; i < spannableString->spans_count; i++) {
        if (span->attributes_mask & attrib)
            return span;
        span++;
    }

    return 0;
}

bool InputHandler::isTrailingSingleCharacter(span_t* span, unsigned stringLength, unsigned composingTextLength)
{
    // Make sure the new string is one character larger than the old.
    if (composingTextLength != stringLength - 1)
        return false;

    if (!span)
        return false;

    // Has only 1 character changed?
    if (span->start == span->end) {
        // Is this character the last character in the string?
        if (span->start == stringLength - 1)
            return true;
    }
    // Return after the first changed_attrib is found.
    return false;
}

bool InputHandler::setText(spannable_string_t* spannableString)
{
    if (!isActiveTextEdit() || !spannableString)
        return false;

    ASSERT(m_currentFocusElement->document() && m_currentFocusElement->document()->frame());
    Frame* frame = m_currentFocusElement->document()->frame();

    Editor* editor = frame->editor();
    ASSERT(editor);

    // Disable selectionHandler's active selection as we will be resetting and these
    // changes should not be handled as notification event.
    m_webPage->m_selectionHandler->setSelectionActive(false);

    WTF::String textToInsert = convertSpannableStringToString(spannableString);
    int textLength = textToInsert.length();

    InputLog(Platform::LogLevelInfo,
        "InputHandler::setText spannableString is '%s', of length %d",
        textToInsert.latin1().data(), textLength);

    span_t* changedSpan = firstSpanInString(spannableString, CHANGED_ATTRIB);
    int composingTextStart = m_composingTextStart;
    int composingTextEnd = m_composingTextEnd;
    int composingTextLength = compositionLength();
    removeAttributedTextMarker();

    if (isTrailingSingleCharacter(changedSpan, textLength, composingTextLength)) {
        // If the text is unconverted, do not allow JS processing as it is not a "real"
        // character in the field.
        if (firstSpanInString(spannableString, UNCONVERTED_TEXT_ATTRIB)) {
            InputLog(Platform::LogLevelInfo, "InputHandler::setText Single trailing character detected. Text is unconverted.");
            return editor->command("InsertText").execute(textToInsert.right(1));
        }
        InputLog(Platform::LogLevelInfo, "InputHandler::setText Single trailing character detected.");
        return handleKeyboardInput(Platform::KeyboardEvent(textToInsert[textLength - 1], Platform::KeyboardEvent::KeyDown, 0), false /* changeIsPartOfComposition */);
    }

    // If no spans have changed, treat it as a delete operation.
    if (!changedSpan) {
        // If the composition length is the same as our string length, then we don't need to do anything.
        if (composingTextLength == textLength) {
            InputLog(Platform::LogLevelInfo, "InputHandler::setText No spans have changed. New text is the same length as the old. Nothing to do.");
            return true;
        }

        if (composingTextLength - textLength == 1) {
            InputLog(Platform::LogLevelInfo, "InputHandler::setText No spans have changed. New text is one character shorter than the old. Treating as 'delete'.");
            return handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_BACKSPACE, Platform::KeyboardEvent::KeyDown, 0), true /* changeIsPartOfComposition */);
        }
    }

    if (composingTextLength && !setSelection(composingTextStart, composingTextEnd, true /* changeIsPartOfComposition */))
        return false;

    // If there is no text to add just delete.
    if (!textLength) {
        if (selectionActive())
            return handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_BACKSPACE, Platform::KeyboardEvent::KeyDown, 0), true /* changeIsPartOfComposition */);

        // Nothing to do.
        return true;
    }

    // Triggering an insert of the text with a space character trailing
    // causes new text to adopt the previous text style.
    // Remove it and apply it as a keypress later.
    // Upstream Webkit bug created https://bugs.webkit.org/show_bug.cgi?id=70823
    bool requiresSpaceKeyPress = false;
    if (textLength > 0 && textToInsert[textLength - 1] == KEYCODE_SPACE) {
        requiresSpaceKeyPress = true;
        textLength--;
        textToInsert.remove(textLength, 1);
    }

    InputLog(Platform::LogLevelInfo,
        "InputHandler::setText Request being processed. Text before processing: '%s'",
        elementText().latin1().data());

    if (textLength == 1 && !spannableString->spans_count) {
        // Handle single key non-attributed entry as key press rather than insert to allow
        // triggering of javascript events.
        InputLog(Platform::LogLevelInfo, "InputHandler::setText Single character entry treated as key-press in the absense of spans.");
        return handleKeyboardInput(Platform::KeyboardEvent(textToInsert[0], Platform::KeyboardEvent::KeyDown, 0), true /* changeIsPartOfComposition */);
    }

    // Perform the text change as a single command if there is one.
    if (!textToInsert.isEmpty() && !editor->command("InsertText").execute(textToInsert)) {
        InputLog(Platform::LogLevelWarn,
            "InputHandler::setText Failed to insert text '%s'",
            textToInsert.latin1().data());
        return false;
    }

    if (requiresSpaceKeyPress)
        handleKeyboardInput(Platform::KeyboardEvent(KEYCODE_SPACE, Platform::KeyboardEvent::KeyDown, 0), true /* changeIsPartOfComposition */);

    InputLog(Platform::LogLevelInfo,
        "InputHandler::setText Request being processed. Text after processing '%s'",
        elementText().latin1().data());

    return true;
}

bool InputHandler::setTextAttributes(int insertionPoint, spannable_string_t* spannableString)
{
    // Apply the attributes to the field.
    span_t* span = spannableString->spans;
    for (unsigned i = 0; i < spannableString->spans_count; i++) {
        unsigned startPosition = insertionPoint + span->start;
        // The end point includes the character that it is before. Ie, 0, 0
        // applies to the first character as the end point includes the character
        // at the position. This means the endPosition is always +1.
        unsigned endPosition = insertionPoint + span->end + 1;
        if (endPosition < startPosition || endPosition > elementText().length())
            return false;

        if (!span->attributes_mask)
            continue; // Nothing to do.

        // MISSPELLED_WORD_ATTRIB is present as an option, but it is not currently
        // used by IMF. When they add support for on the fly spell checking we can
        // use it to apply spelling markers and disable continuous spell checking.

        InputLog(Platform::LogLevelInfo,
            "InputHandler::setTextAttributes adding marker %d to %d - %llu",
            startPosition, endPosition, span->attributes_mask);
        addAttributedTextMarker(startPosition, endPosition, textStyleFromMask(span->attributes_mask));

        span++;
    }

    InputLog(Platform::LogLevelInfo, "InputHandler::setTextAttributes attribute count %d", spannableString->spans_count);

    return true;
}

bool InputHandler::setRelativeCursorPosition(int insertionPoint, int relativeCursorPosition)
{
    if (!isActiveTextEdit())
        return false;

    // 1 place cursor at end of insertion text.
    if (relativeCursorPosition == 1) {
        m_currentFocusElement->document()->frame()->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
        return true;
    }

    int cursorPosition = 0;
    if (relativeCursorPosition <= 0) {
        // Zero = insertionPoint
        // Negative value, move the cursor the requested number of characters before
        // the start of the inserted text.
        cursorPosition = insertionPoint + relativeCursorPosition;
    } else {
        // Positive value, move the cursor the requested number of characters after
        // the end of the inserted text minus 1.
        cursorPosition = caretPosition() + relativeCursorPosition - 1;
    }

    if (cursorPosition < 0 || cursorPosition > (int)elementText().length())
        return false;

    InputLog(Platform::LogLevelInfo,
        "InputHandler::setRelativeCursorPosition cursor position %d",
        cursorPosition);

    return setCursorPosition(cursorPosition);
}

bool InputHandler::setSpannableTextAndRelativeCursor(spannable_string_t* spannableString, int relativeCursorPosition, bool markTextAsComposing)
{
    InputLog(Platform::LogLevelInfo,
        "InputHandler::setSpannableTextAndRelativeCursor(%d, %d, %d)",
        spannableString->length, relativeCursorPosition, markTextAsComposing);

    int insertionPoint = compositionActive() ? m_composingTextStart : selectionStart();

    ProcessingChangeGuard guard(this);

    if (!setText(spannableString))
        return false;

    if (!setTextAttributes(insertionPoint, spannableString))
        return false;

    if (!setRelativeCursorPosition(insertionPoint, relativeCursorPosition))
        return false;

    if (markTextAsComposing) {
        m_composingTextStart = insertionPoint;
        m_composingTextEnd = insertionPoint + spannableString->length;
    }

    return true;
}

int32_t InputHandler::setComposingText(spannable_string_t* spannableString, int32_t relativeCursorPosition)
{
    if (!isActiveTextEdit())
        return -1;

    if (!spannableString)
        return -1;

    InputLog(Platform::LogLevelInfo,
        "InputHandler::setComposingText at relativeCursorPosition: %d",
        relativeCursorPosition);

    // Enable input mode if we are processing a key event.
    setInputModeEnabled();

    return setSpannableTextAndRelativeCursor(spannableString, relativeCursorPosition, true /* markTextAsComposing */) ? 0 : -1;
}

int32_t InputHandler::commitText(spannable_string_t* spannableString, int32_t relativeCursorPosition)
{
    if (!isActiveTextEdit())
        return -1;

    if (!spannableString)
        return -1;

    InputLog(Platform::LogLevelInfo, "InputHandler::commitText");

    return setSpannableTextAndRelativeCursor(spannableString, relativeCursorPosition, false /* markTextAsComposing */) ? 0 : -1;
}

void InputHandler::restoreViewState()
{
    setInputModeEnabled();

    // Make sure we reset the selection / FCC state.
    m_webPage->m_selectionHandler->selectionPositionChanged();
}

void InputHandler::showTextInputTypeSuggestionBox(bool allowEmptyPrefix)
{
    if (!isActiveTextEdit())
        return;

    HTMLInputElement* focusedInputElement = toHTMLInputElement(m_currentFocusElement->toInputElement());
    if (!focusedInputElement)
        return;

    if (!m_suggestionDropdownBoxHandler)
        m_suggestionDropdownBoxHandler = SuggestionBoxHandler::create(focusedInputElement);

    // If the focused input element isn't the same as the one inside the handler, reset and display.
    // If the focused element is the same, display the suggestions.
    if ((m_suggestionDropdownBoxHandler->focusedElement()) && m_suggestionDropdownBoxHandler->focusedElement() != focusedInputElement)
        m_suggestionDropdownBoxHandler->setInputElementAndUpdateDisplay(focusedInputElement);
    else
        m_suggestionDropdownBoxHandler->showDropdownBox(allowEmptyPrefix);
}

void InputHandler::hideTextInputTypeSuggestionBox()
{
    if (m_suggestionDropdownBoxHandler)
        m_suggestionDropdownBoxHandler->hideDropdownBox();
}

void InputHandler::elementTouched(WebCore::Element* nonShadowElementUnderFatFinger)
{
    // Attempt to show all suggestions when the input field is empty and a tap is registered when the element is focused.
    if (isActiveTextEdit() && nonShadowElementUnderFatFinger == m_currentFocusElement)
        showTextInputTypeSuggestionBox(true /* allowEmptyPrefix */);

    m_elementTouchedIsCrossFrame = nonShadowElementUnderFatFinger
        && nonShadowElementUnderFatFinger->document()
        && nonShadowElementUnderFatFinger->document()->frame() != m_webPage->focusedOrMainFrame();
}

}
}
