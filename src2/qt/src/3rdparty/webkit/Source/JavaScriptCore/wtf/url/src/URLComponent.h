// Copyright 2010, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef URLComponent_h
#define URLComponent_h

namespace WTF {

// Represents a substring for URL parsing.
class URLComponent {
public:
    URLComponent() : m_begin(0), m_length(-1) { }
    URLComponent(int begin, int length) : m_begin(begin), m_length(length) { }

    // Helper that returns a component created with the given begin and ending
    // points. The ending point is non-inclusive.
    static inline URLComponent fromRange(int begin, int end)
    {
        return URLComponent(begin, end - begin);
    }

    // Returns true if this component is valid, meaning the length is given. Even
    // valid components may be empty to record the fact that they exist.
    bool isValid() const { return m_length != -1; }

    bool isNonEmpty() const { return m_length > 0; }
    bool isEmptyOrInvalid() const { return m_length <= 0; }

    void reset()
    {
        m_begin = 0;
        m_length = -1;
    }

    bool operator==(const URLComponent& other) const { return m_begin == other.m_begin && m_length == other.m_length; }

    int begin() const { return m_begin; }
    void setBegin(int begin) { m_begin = begin; }

    int length() const { return m_length; }
    void setLength(int length) { m_length = length; }

    int end() const { return m_begin + m_length; }

private:
    int m_begin; // Byte offset in the string of this component.
    int m_length; // Will be -1 if the component is unspecified.
};

} // namespace WTF

#endif // URLComponent_h
