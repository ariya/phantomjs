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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlugInAutoStartProvider_h
#define PlugInAutoStartProvider_h

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class ImmutableArray;
class ImmutableDictionary;
class WebContext;

typedef HashMap<unsigned, double> PlugInAutoStartOriginHash;
typedef Vector<String> PlugInAutoStartOrigins;

class PlugInAutoStartProvider {
    WTF_MAKE_NONCOPYABLE(PlugInAutoStartProvider);
public:
    explicit PlugInAutoStartProvider(WebContext*);

    void addAutoStartOriginHash(const String& pageOrigin, unsigned plugInOriginHash);
    void didReceiveUserInteraction(unsigned plugInOriginHash);

    PassRefPtr<ImmutableDictionary> autoStartOriginsTableCopy() const;
    void setAutoStartOriginsTable(ImmutableDictionary&);
    void setAutoStartOriginsArray(ImmutableArray&);

    PlugInAutoStartOriginHash autoStartOriginHashesCopy() const;
    const PlugInAutoStartOrigins& autoStartOrigins() const { return m_autoStartOrigins; }

private:
    WebContext* m_context;

    typedef HashMap<String, PlugInAutoStartOriginHash, CaseFoldingHash> AutoStartTable;
    AutoStartTable m_autoStartTable;

    HashMap<unsigned, String> m_hashToOriginMap;

    PlugInAutoStartOrigins m_autoStartOrigins;
};

} // namespace WebKit

#endif /* PlugInAutoStartProvider_h */
