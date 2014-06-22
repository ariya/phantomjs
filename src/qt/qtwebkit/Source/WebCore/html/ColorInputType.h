/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ColorInputType_h
#define ColorInputType_h

#if ENABLE(INPUT_TYPE_COLOR)
#include "BaseClickableWithKeyInputType.h"
#include "ColorChooserClient.h"

namespace WebCore {

class ColorInputType : public BaseClickableWithKeyInputType, public ColorChooserClient {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);
    virtual ~ColorInputType();

    // ColorChooserClient implementation.
    virtual void didChooseColor(const Color&) OVERRIDE;
    virtual void didEndChooser() OVERRIDE;
    virtual IntRect elementRectRelativeToRootView() const OVERRIDE;
    virtual Color currentColor() OVERRIDE;
    virtual bool shouldShowSuggestions() const OVERRIDE;
    virtual Vector<Color> suggestions() const OVERRIDE;

private:
    ColorInputType(HTMLInputElement* element) : BaseClickableWithKeyInputType(element) { }
    virtual void attach() OVERRIDE;
    virtual bool isColorControl() const OVERRIDE;
    virtual const AtomicString& formControlType() const OVERRIDE;
    virtual bool supportsRequired() const OVERRIDE;
    virtual String fallbackValue() const OVERRIDE;
    virtual String sanitizeValue(const String&) const OVERRIDE;
    virtual void createShadowSubtree() OVERRIDE;
    virtual void setValue(const String&, bool valueChanged, TextFieldEventBehavior) OVERRIDE;
    virtual void handleDOMActivateEvent(Event*) OVERRIDE;
    virtual void detach() OVERRIDE;
    virtual bool shouldRespectListAttribute() OVERRIDE;
    virtual bool typeMismatchFor(const String&) const OVERRIDE;

    Color valueAsColor() const;
    void endColorChooser();
    void updateColorSwatch();
    HTMLElement* shadowColorSwatch() const;

    OwnPtr<ColorChooser> m_chooser;
};

} // namespace WebCore

#endif // ENABLE(INPUT_TYPE_COLOR)

#endif // ColorInputType_h
