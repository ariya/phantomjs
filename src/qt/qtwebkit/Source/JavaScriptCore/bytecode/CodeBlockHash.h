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

#ifndef CodeBlockHash_h
#define CodeBlockHash_h

#include "CodeSpecializationKind.h"
#include <wtf/PrintStream.h>

// CodeBlock hashes are useful for informally identifying code blocks. They correspond
// to the low 32 bits of a SHA1 hash of the source code with two low bit flipped
// according to the role that the code block serves (call, construct). Additionally, the
// hashes are typically operated over using a string in which the hash is transformed
// into a 6-byte alphanumeric representation. This can be retrieved by using
// toString(const CodeBlockHash&). Finally, we support CodeBlockHashes for native
// functions, in which case the hash is replaced by the function address.

namespace JSC {

class SourceCode;

class CodeBlockHash {
public:
    CodeBlockHash()
        : m_hash(0)
    {
    }
    
    explicit CodeBlockHash(unsigned hash)
        : m_hash(hash)
    {
    }
    
    CodeBlockHash(const SourceCode&, CodeSpecializationKind);
    
    explicit CodeBlockHash(const char*);
    
    unsigned hash() const { return m_hash; }
    
    void dump(PrintStream&) const;
    
    // Comparison methods useful for bisection.
    bool operator==(const CodeBlockHash& other) const { return hash() == other.hash(); }
    bool operator!=(const CodeBlockHash& other) const { return hash() != other.hash(); }
    bool operator<(const CodeBlockHash& other) const { return hash() < other.hash(); }
    bool operator>(const CodeBlockHash& other) const { return hash() > other.hash(); }
    bool operator<=(const CodeBlockHash& other) const { return hash() <= other.hash(); }
    bool operator>=(const CodeBlockHash& other) const { return hash() >= other.hash(); }
    
private:
    unsigned m_hash;
};

} // namespace JSC

#endif // CodeBlockHash_h
