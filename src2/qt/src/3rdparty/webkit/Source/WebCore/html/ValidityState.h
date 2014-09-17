/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2009 Michelangelo De Simone <micdesim@gmail.com>
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
 *
 */

#ifndef ValidityState_h
#define ValidityState_h

#include "FormAssociatedElement.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class ValidityState {
    WTF_MAKE_NONCOPYABLE(ValidityState); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<ValidityState> create(FormAssociatedElement* control)
    {
        return adoptPtr(new ValidityState(control));
    }

    void ref() { m_control->ref(); }
    void deref() { m_control->deref(); }

    String validationMessage() const;

    void setCustomErrorMessage(const String&);

    bool valueMissing() const;
    bool typeMismatch() const;
    bool patternMismatch() const;
    bool tooLong() const;
    bool rangeUnderflow() const;
    bool rangeOverflow() const;
    bool stepMismatch() const;
    bool customError() const;
    bool valid() const;

private:
    ValidityState(FormAssociatedElement* control) : m_control(control) { }

    static bool isValidColorString(const String&);
    static bool isValidEmailAddress(const String&);

    FormAssociatedElement* m_control;
    String m_customErrorMessage;
};

} // namespace WebCore

#endif // ValidityState_h
