/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DFGDoubleFormatState_h
#define DFGDoubleFormatState_h

namespace JSC { namespace DFG {

enum DoubleFormatState {
    EmptyDoubleFormatState, // bottom
    UsingDoubleFormat,
    NotUsingDoubleFormat,
    CantUseDoubleFormat // top
};

inline DoubleFormatState mergeDoubleFormatStates(DoubleFormatState a, DoubleFormatState b)
{
    switch (a) {
    case EmptyDoubleFormatState:
        return b;
    case UsingDoubleFormat:
        switch (b) {
        case EmptyDoubleFormatState:
        case UsingDoubleFormat:
            return UsingDoubleFormat;
        case NotUsingDoubleFormat:
        case CantUseDoubleFormat:
            return CantUseDoubleFormat;
        }
    case NotUsingDoubleFormat:
        switch (b) {
        case EmptyDoubleFormatState:
        case NotUsingDoubleFormat:
            return NotUsingDoubleFormat;
        case UsingDoubleFormat:
        case CantUseDoubleFormat:
            return CantUseDoubleFormat;
        }
    case CantUseDoubleFormat:
        return CantUseDoubleFormat;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return CantUseDoubleFormat;
}

inline bool mergeDoubleFormatState(DoubleFormatState& dest, DoubleFormatState src)
{
    DoubleFormatState newState = mergeDoubleFormatStates(dest, src);
    if (newState == dest)
        return false;
    dest = newState;
    return true;
}

inline const char* doubleFormatStateToString(DoubleFormatState state)
{
    switch (state) {
    case EmptyDoubleFormatState:
        return "Empty";
    case UsingDoubleFormat:
        return "DoubleFormat";
    case NotUsingDoubleFormat:
        return "ValueFormat";
    case CantUseDoubleFormat:
        return "ForceValue";
    }
    RELEASE_ASSERT_NOT_REACHED();
    return 0;
}

} } // namespace JSC::DFG

#endif // DFGDoubleFormatState_h

