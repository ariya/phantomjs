/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "InputType.h"

#include "BeforeTextInsertedEvent.h"
#include "ButtonInputType.h"
#include "CheckboxInputType.h"
#include "ColorInputType.h"
#include "DateComponents.h"
#include "DateInputType.h"
#include "DateTimeInputType.h"
#include "DateTimeLocalInputType.h"
#include "EmailInputType.h"
#include "ExceptionCode.h"
#include "FileInputType.h"
#include "FormDataList.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HiddenInputType.h"
#include "ImageInputType.h"
#include "IsIndexInputType.h"
#include "KeyboardEvent.h"
#include "LocalizedStrings.h"
#include "MonthInputType.h"
#include "NumberInputType.h"
#include "PasswordInputType.h"
#include "RadioInputType.h"
#include "RangeInputType.h"
#include "RegularExpression.h"
#include "RenderObject.h"
#include "ResetInputType.h"
#include "SearchInputType.h"
#include "ShadowRoot.h"
#include "SubmitInputType.h"
#include "TelephoneInputType.h"
#include "TextInputType.h"
#include "TimeInputType.h"
#include "URLInputType.h"
#include "WeekInputType.h"
#include <limits>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

using namespace std;

typedef PassOwnPtr<InputType> (*InputTypeFactoryFunction)(HTMLInputElement*);
typedef HashMap<String, InputTypeFactoryFunction, CaseFoldingHash> InputTypeFactoryMap;

static PassOwnPtr<InputTypeFactoryMap> createInputTypeFactoryMap()
{
    OwnPtr<InputTypeFactoryMap> map = adoptPtr(new InputTypeFactoryMap);
    map->add(InputTypeNames::button(), ButtonInputType::create);
    map->add(InputTypeNames::checkbox(), CheckboxInputType::create);
    map->add(InputTypeNames::color(), ColorInputType::create);
    map->add(InputTypeNames::date(), DateInputType::create);
    map->add(InputTypeNames::datetime(), DateTimeInputType::create);
    map->add(InputTypeNames::datetimelocal(), DateTimeLocalInputType::create);
    map->add(InputTypeNames::email(), EmailInputType::create);
    map->add(InputTypeNames::file(), FileInputType::create);
    map->add(InputTypeNames::hidden(), HiddenInputType::create);
    map->add(InputTypeNames::image(), ImageInputType::create);
    map->add(InputTypeNames::isindex(), IsIndexInputType::create);
    map->add(InputTypeNames::month(), MonthInputType::create);
    map->add(InputTypeNames::number(), NumberInputType::create);
    map->add(InputTypeNames::password(), PasswordInputType::create);
    map->add(InputTypeNames::radio(), RadioInputType::create);
    map->add(InputTypeNames::range(), RangeInputType::create);
    map->add(InputTypeNames::reset(), ResetInputType::create);
    map->add(InputTypeNames::search(), SearchInputType::create);
    map->add(InputTypeNames::submit(), SubmitInputType::create);
    map->add(InputTypeNames::telephone(), TelephoneInputType::create);
    map->add(InputTypeNames::time(), TimeInputType::create);
    map->add(InputTypeNames::url(), URLInputType::create);
    map->add(InputTypeNames::week(), WeekInputType::create);
    // No need to register "text" because it is the default type.
    return map.release();
}

PassOwnPtr<InputType> InputType::create(HTMLInputElement* element, const String& typeName)
{
    static const InputTypeFactoryMap* factoryMap = createInputTypeFactoryMap().leakPtr();
    PassOwnPtr<InputType> (*factory)(HTMLInputElement*) = typeName.isEmpty() ? 0 : factoryMap->get(typeName);
    if (!factory)
        factory = TextInputType::create;
    return factory(element);
}

PassOwnPtr<InputType> InputType::createText(HTMLInputElement* element)
{
    return TextInputType::create(element);
}

InputType::~InputType()
{
}

bool InputType::isTextField() const
{
    return false;
}

bool InputType::isTextType() const
{
    return false;
}

bool InputType::isRangeControl() const
{
    return false;
}

bool InputType::saveFormControlState(String& result) const
{
    String currentValue = element()->value();
    if (currentValue == element()->defaultValue())
        return false;
    result = currentValue;
    return true;
}

void InputType::restoreFormControlState(const String& state) const
{
    element()->setValue(state);
}

bool InputType::isFormDataAppendable() const
{
    // There is no form data unless there's a name for non-image types.
    return !element()->name().isEmpty();
}

bool InputType::appendFormData(FormDataList& encoding, bool) const
{
    // Always successful.
    encoding.appendData(element()->name(), element()->value());
    return true;
}

double InputType::valueAsDate() const
{
    return DateComponents::invalidMilliseconds();
}

void InputType::setValueAsDate(double, ExceptionCode& ec) const
{
    ec = INVALID_STATE_ERR;
}

double InputType::valueAsNumber() const
{
    return numeric_limits<double>::quiet_NaN();
}

void InputType::setValueAsNumber(double, ExceptionCode& ec) const
{
    ec = INVALID_STATE_ERR;
}

bool InputType::supportsValidation() const
{
    return true;
}

bool InputType::typeMismatchFor(const String&) const
{
    return false;
}

bool InputType::typeMismatch() const
{
    return false;
}

bool InputType::supportsRequired() const
{
    // Almost all validatable types support @required.
    return supportsValidation();
}

bool InputType::valueMissing(const String&) const
{
    return false;
}

bool InputType::patternMismatch(const String&) const
{
    return false;
}

bool InputType::rangeUnderflow(const String&) const
{
    return false;
}

bool InputType::rangeOverflow(const String&) const
{
    return false;
}

bool InputType::supportsRangeLimitation() const
{
    return false;
}

double InputType::defaultValueForStepUp() const
{
    return 0;
}

double InputType::minimum() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

double InputType::maximum() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

bool InputType::stepMismatch(const String&, double) const
{
    // Non-supported types should be rejected by HTMLInputElement::getAllowedValueStep().
    ASSERT_NOT_REACHED();
    return false;
}

double InputType::stepBase() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

double InputType::stepBaseWithDecimalPlaces(unsigned* decimalPlaces) const
{
    if (decimalPlaces)
        *decimalPlaces = 0;
    return stepBase();
}

double InputType::defaultStep() const
{
    return numeric_limits<double>::quiet_NaN();
}

double InputType::stepScaleFactor() const
{
    return numeric_limits<double>::quiet_NaN();
}

bool InputType::parsedStepValueShouldBeInteger() const
{
    return false;
}

bool InputType::scaledStepValueShouldBeInteger() const
{
    return false;
}

double InputType::acceptableError(double) const
{
    return 0;
}

String InputType::typeMismatchText() const
{
    return validationMessageTypeMismatchText();
}

String InputType::valueMissingText() const
{
    return validationMessageValueMissingText();
}

void InputType::handleClickEvent(MouseEvent*)
{
}

void InputType::handleMouseDownEvent(MouseEvent*)
{
}

void InputType::handleDOMActivateEvent(Event*)
{
}

void InputType::handleKeydownEvent(KeyboardEvent*)
{
}

void InputType::handleKeypressEvent(KeyboardEvent*)
{
}

void InputType::handleKeyupEvent(KeyboardEvent*)
{
}

void InputType::handleBeforeTextInsertedEvent(BeforeTextInsertedEvent* event)
{
    element()->handleBeforeTextInsertedEvent(event);
}

void InputType::handleWheelEvent(WheelEvent*)
{
}

void InputType::forwardEvent(Event*)
{
}

bool InputType::shouldSubmitImplicitly(Event* event)
{
    return event->isKeyboardEvent() && event->type() == eventNames().keypressEvent && static_cast<KeyboardEvent*>(event)->charCode() == '\r';
}

PassRefPtr<HTMLFormElement> InputType::formForSubmission() const
{
    return element()->form();
}

RenderObject* InputType::createRenderer(RenderArena*, RenderStyle* style) const
{
    return RenderObject::createObject(element(), style);
}

void InputType::createShadowSubtree()
{
}

void InputType::destroyShadowSubtree()
{
    element()->removeShadowRoot();
}

double InputType::parseToDouble(const String&, double defaultValue) const
{
    return defaultValue;
}

double InputType::parseToDoubleWithDecimalPlaces(const String& src, double defaultValue, unsigned *decimalPlaces) const
{
    if (decimalPlaces)
        *decimalPlaces = 0;
    return parseToDouble(src, defaultValue);
}

bool InputType::parseToDateComponents(const String&, DateComponents*) const
{
    ASSERT_NOT_REACHED();
    return false;
}

String InputType::serialize(double) const
{
    ASSERT_NOT_REACHED();
    return String();
}

void InputType::dispatchSimulatedClickIfActive(KeyboardEvent* event) const
{
    if (element()->active())
        element()->dispatchSimulatedClick(event);
    event->setDefaultHandled();
}

bool InputType::canSetStringValue() const
{
    return true;
}

bool InputType::isKeyboardFocusable() const
{
    return true;
}

bool InputType::shouldUseInputMethod() const
{
    return false;
}

void InputType::handleBlurEvent()
{
}

void InputType::accessKeyAction(bool)
{
    element()->focus(false);
}

void InputType::attach()
{
}

void InputType::altAttributeChanged()
{
}

void InputType::srcAttributeChanged()
{
}

void InputType::valueChanged()
{
}

void InputType::willMoveToNewOwnerDocument()
{
}

bool InputType::shouldRespectAlignAttribute()
{
    return false;
}

bool InputType::canChangeFromAnotherType() const
{
    return true;
}

void InputType::minOrMaxAttributeChanged()
{
}

bool InputType::canBeSuccessfulSubmitButton()
{
    return false;
}

bool InputType::rendererIsNeeded()
{
    return true;
}

FileList* InputType::files()
{
    return 0;
}

bool InputType::getTypeSpecificValue(String&)
{
    return false;
}

String InputType::fallbackValue()
{
    return String();
}

String InputType::defaultValue()
{
    return String();
}

bool InputType::canSetSuggestedValue()
{
    return false;
}

bool InputType::shouldSendChangeEventAfterCheckedChanged()
{
    return true;
}

bool InputType::storesValueSeparateFromAttribute()
{
    return true;
}

bool InputType::canSetValue(const String&)
{
    return true;
}

PassOwnPtr<ClickHandlingState> InputType::willDispatchClick()
{
    return nullptr;
}

void InputType::didDispatchClick(Event*, const ClickHandlingState&)
{
}

String InputType::visibleValue() const
{
    return element()->value();
}

String InputType::convertFromVisibleValue(const String& visibleValue) const
{
    return visibleValue;
}

bool InputType::isAcceptableValue(const String&)
{
    return true;
}

String InputType::sanitizeValue(const String& proposedValue)
{
    return proposedValue;
}

bool InputType::hasUnacceptableValue()
{
    return false;
}

void InputType::setFileList(const Vector<String>&)
{
    ASSERT_NOT_REACHED();
}

bool InputType::shouldResetOnDocumentActivation()
{
    return false;
}

bool InputType::shouldRespectListAttribute()
{
    return false;
}

bool InputType::shouldRespectSpeechAttribute()
{
    return false;
}

bool InputType::isTextButton() const
{
    return false;
}

bool InputType::isRadioButton() const
{
    return false;
}

bool InputType::isSearchField() const
{
    return false;
}

bool InputType::isHiddenType() const
{
    return false;
}

bool InputType::isPasswordField() const
{
    return false;
}

bool InputType::isCheckbox() const
{
    return false;
}

bool InputType::isEmailField() const
{
    return false;
}

bool InputType::isFileUpload() const
{
    return false;
}

bool InputType::isImageButton() const
{
    return false;
}

bool InputType::isNumberField() const
{
    return false;
}

bool InputType::isSubmitButton() const
{
    return false;
}

bool InputType::isTelephoneField() const
{
    return false;
}

bool InputType::isURLField() const
{
    return false;
}

bool InputType::isEnumeratable()
{
    return true;
}

bool InputType::isCheckable()
{
    return false;
}

bool InputType::isSteppable() const
{
    return false;
}

bool InputType::shouldRespectHeightAndWidthAttributes()
{
    return false;
}

namespace InputTypeNames {

// The type names must be lowercased because they will be the return values of
// input.type and input.type must be lowercase according to DOM Level 2.

const AtomicString& button()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("button"));
    return name;
}

const AtomicString& checkbox()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("checkbox"));
    return name;
}

const AtomicString& color()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("color"));
    return name;
}

const AtomicString& date()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("date"));
    return name;
}

const AtomicString& datetime()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("datetime"));
    return name;
}

const AtomicString& datetimelocal()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("datetime-local"));
    return name;
}

const AtomicString& email()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("email"));
    return name;
}

const AtomicString& file()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("file"));
    return name;
}

const AtomicString& hidden()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("hidden"));
    return name;
}

const AtomicString& image()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("image"));
    return name;
}

const AtomicString& isindex()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("khtml_isindex"));
    return name;
}

const AtomicString& month()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("month"));
    return name;
}

const AtomicString& number()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("number"));
    return name;
}

const AtomicString& password()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("password"));
    return name;
}

const AtomicString& radio()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("radio"));
    return name;
}

const AtomicString& range()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("range"));
    return name;
}

const AtomicString& reset()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("reset"));
    return name;
}

const AtomicString& search()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("search"));
    return name;
}

const AtomicString& submit()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("submit"));
    return name;
}

const AtomicString& telephone()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("tel"));
    return name;
}

const AtomicString& text()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("text"));
    return name;
}

const AtomicString& time()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("time"));
    return name;
}

const AtomicString& url()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("url"));
    return name;
}

const AtomicString& week()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("week"));
    return name;
}

} // namespace WebCore::InputTypeNames

} // namespace WebCore
