/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "IdentifierRep.h"
#include "JSDOMBinding.h"

#include "PlatformString.h"
#include <runtime/UString.h>
#include <wtf/HashMap.h>
#include <wtf/StdLibExtras.h>

using namespace JSC;

namespace WebCore {

typedef HashSet<IdentifierRep*> IdentifierSet;

static IdentifierSet& identifierSet()
{
    DEFINE_STATIC_LOCAL(IdentifierSet, identifierSet, ());
    return identifierSet;
}
    
typedef HashMap<int, IdentifierRep*> IntIdentifierMap;

static IntIdentifierMap& intIdentifierMap()
{
    DEFINE_STATIC_LOCAL(IntIdentifierMap, intIdentifierMap, ());
    return intIdentifierMap;
}

IdentifierRep* IdentifierRep::get(int intID)
{
    if (intID == 0 || intID == -1) {
        static IdentifierRep* negativeOneAndZeroIdentifiers[2];

        IdentifierRep* identifier = negativeOneAndZeroIdentifiers[intID + 1];
        if (!identifier) {
            identifier = new IdentifierRep(intID);

            negativeOneAndZeroIdentifiers[intID + 1] = identifier;
        }
        
        return identifier;
    }
    
    pair<IntIdentifierMap::iterator, bool> result = intIdentifierMap().add(intID, 0); 
    if (result.second) {
        ASSERT(!result.first->second);
        result.first->second = new IdentifierRep(intID);
        
        identifierSet().add(result.first->second);
    }
    
    return result.first->second;
}

typedef HashMap<RefPtr<StringImpl>, IdentifierRep*> StringIdentifierMap;

static StringIdentifierMap& stringIdentifierMap()
{
    DEFINE_STATIC_LOCAL(StringIdentifierMap, stringIdentifierMap, ());
    return stringIdentifierMap;
}

IdentifierRep* IdentifierRep::get(const char* name)
{
    ASSERT(name);
    if (!name)
        return 0;
  
    UString string = stringToUString(String::fromUTF8WithLatin1Fallback(name, strlen(name)));
    pair<StringIdentifierMap::iterator, bool> result = stringIdentifierMap().add(string.impl(), 0);
    if (result.second) {
        ASSERT(!result.first->second);
        result.first->second = new IdentifierRep(name);
        
        identifierSet().add(result.first->second);
    }
    
    return result.first->second;
}

bool IdentifierRep::isValid(IdentifierRep* identifier)
{
    return identifierSet().contains(identifier);
}
    
} // namespace WebCore
