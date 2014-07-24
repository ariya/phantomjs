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

#include "config.h"
#include "ValidityState.h"

namespace WebCore {

String ValidityState::validationMessage() const
{
    return m_control->validationMessage();
}

bool ValidityState::valueMissing() const
{
    return m_control->valueMissing();
}

bool ValidityState::typeMismatch() const
{
    return m_control->typeMismatch();
}

bool ValidityState::patternMismatch() const
{
    return m_control->patternMismatch();
}

bool ValidityState::tooLong() const
{
    return m_control->tooLong();
}

bool ValidityState::rangeUnderflow() const
{
    return m_control->rangeUnderflow();
}

bool ValidityState::rangeOverflow() const
{
    return m_control->rangeOverflow();
}

bool ValidityState::stepMismatch() const
{
    return m_control->stepMismatch();
}

bool ValidityState::badInput() const
{
    return m_control->hasBadInput();
}

bool ValidityState::customError() const
{
    return m_control->customError();
}

bool ValidityState::valid() const
{
    return m_control->valid();
}

} // namespace
