/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef Gamepad_h
#define Gamepad_h

#if ENABLE(GAMEPAD)

#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Gamepad: public RefCounted<Gamepad> {
public:
    static PassRefPtr<Gamepad> create()
    {
        return adoptRef(new Gamepad);
    }
    ~Gamepad();

    typedef Vector<float> FloatVector;

    const String& id() const { return m_id; }
    void id(const String& id) { m_id = id; }

    unsigned index() const { return m_index; }
    void index(unsigned val) { m_index = val; }

    unsigned long long timestamp() const { return m_timestamp; }
    void timestamp(unsigned long long val) { m_timestamp = val; }

    const FloatVector& axes() const { return m_axes; }
    void axes(unsigned count, float* data);

    const FloatVector& buttons() const { return m_buttons; }
    void buttons(unsigned count, float* data);

private:
    Gamepad();
    String m_id;
    unsigned m_index;
    unsigned long long m_timestamp;
    FloatVector m_axes;
    FloatVector m_buttons;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD)

#endif // Gamepad_h
