/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef DOMSupport_h
#define DOMSupport_h

#include "IntPoint.h"
#include "IntRect.h"

#include <BlackBerryPlatformInputEvents.h>
#include <wtf/Vector.h>

namespace WTF {
class String;
}

namespace WebCore {
class Element;
class FloatQuad;
class Frame;
class HTMLInputElement;
class HTMLTextFormControlElement;
class Node;
class Position;
class QualifiedName;
class Range;
class RenderObject;
class VisiblePosition;
class VisibleSelection;
}

namespace BlackBerry {
namespace WebKit {
namespace DOMSupport {

enum AttributeState { On, Off, Default };

bool isElementTypePlugin(const WebCore::Element*);

bool isTextInputElement(WebCore::Element*);
bool isShadowHostTextInputElement(WebCore::Node*);
bool isTextBasedContentEditableElement(WebCore::Element*);
bool isPasswordElement(const WebCore::Element*);

bool isPopupInputField(const WebCore::Element*);
bool isDateTimeInputField(const WebCore::Element*);
bool isColorInputField(const WebCore::Element*);

AttributeState elementAttributeState(const WebCore::Element*, const WebCore::QualifiedName&);
AttributeState elementSupportsAutocorrect(const WebCore::Element*);
AttributeState elementSupportsAutocomplete(const WebCore::Element*);
AttributeState elementSupportsSpellCheck(const WebCore::Element*);
bool isElementReadOnly(const WebCore::Element*);

bool elementHasContinuousSpellCheckingEnabled(const PassRefPtr<WebCore::Element>);

WTF::String inputElementText(WebCore::Element*);
WTF::String webWorksContext(const WebCore::Element*);

WebCore::HTMLTextFormControlElement* toTextControlElement(WebCore::Node*);

WebCore::IntRect transformedBoundingBoxForRange(const WebCore::Range&);
void visibleTextQuads(const WebCore::Range&, WTF::Vector<WebCore::FloatQuad>& quads, bool useSelectionHeight = false);
void visibleTextQuads(const WebCore::VisibleSelection&, WTF::Vector<WebCore::FloatQuad>& quads);

WebCore::VisibleSelection visibleSelectionForRangeInputElement(WebCore::Element*, int start, int end);
WebCore::VisibleSelection visibleSelectionForInputElement(WebCore::Element*);

bool elementIdOrNameIndicatesNoAutocomplete(const WebCore::Element*);
bool elementIdOrNameIndicatesEmail(const WebCore::HTMLInputElement*);
bool elementIdOrNameIndicatesUrl(const WebCore::HTMLInputElement*);
bool elementPatternMatches(const char*, const WebCore::HTMLInputElement*);
bool elementPatternIndicatesNumber(const WebCore::HTMLInputElement*);
bool elementPatternIndicatesHexadecimal(const WebCore::HTMLInputElement*);

WebCore::IntPoint convertPointToFrame(const WebCore::Frame* sourceFrame, const WebCore::Frame* targetFrame, const WebCore::IntPoint& sourcePoint, const bool clampToTargetFrame = false);

static const WebCore::IntPoint InvalidPoint = WebCore::IntPoint(-1, -1);

WebCore::VisibleSelection visibleSelectionForClosestActualWordStart(const WebCore::VisibleSelection&);
WebCore::VisibleSelection visibleSelectionForFocusedBlock(WebCore::Element*);
int offsetFromStartOfBlock(const WebCore::VisiblePosition offset);

WebCore::Frame* incrementFrame(WebCore::Frame* curr, bool forward, bool wrapFlag);

PassRefPtr<WebCore::Range> trimWhitespaceFromRange(PassRefPtr<WebCore::Range>);
PassRefPtr<WebCore::Range> trimWhitespaceFromRange(WebCore::VisiblePosition startPosition, WebCore::VisiblePosition endPosition);
bool isRangeTextAllWhitespace(WebCore::VisiblePosition startPosition, WebCore::VisiblePosition endPosition);

bool isFixedPositionOrHasFixedPositionAncestor(WebCore::RenderObject*);

WebCore::Element* selectionContainerElement(const WebCore::VisibleSelection&);
BlackBerry::Platform::RequestedHandlePosition elementHandlePositionAttribute(const WebCore::Element*);

bool isElementAndDocumentAttached(const WebCore::Element*);

} // DOMSupport
} // WebKit
} // BlackBerry

#endif // DOMSupport_h
