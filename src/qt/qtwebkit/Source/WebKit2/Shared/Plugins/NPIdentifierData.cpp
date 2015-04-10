/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NPIdentifierData.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/IdentifierRep.h>

using namespace WebCore;

namespace WebKit {

NPIdentifierData::NPIdentifierData()
    : m_isString(false)
    , m_number(0)
{
}


NPIdentifierData NPIdentifierData::fromNPIdentifier(NPIdentifier npIdentifier)
{
    NPIdentifierData npIdentifierData;

    IdentifierRep* identifierRep = static_cast<IdentifierRep*>(npIdentifier);
    npIdentifierData.m_isString = identifierRep->isString();

    if (npIdentifierData.m_isString)
        npIdentifierData.m_string = identifierRep->string();
    else
        npIdentifierData.m_number = identifierRep->number();

    return npIdentifierData;
}

NPIdentifier NPIdentifierData::createNPIdentifier() const
{
    if (m_isString)
        return static_cast<NPIdentifier>(IdentifierRep::get(m_string.data()));
    
    return static_cast<NPIdentifier>(IdentifierRep::get(m_number));
}

void NPIdentifierData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_isString;
    if (m_isString)
        encoder << m_string;
    else
        encoder << m_number;
}

bool NPIdentifierData::decode(CoreIPC::ArgumentDecoder& decoder, NPIdentifierData& result)
{
    if (!decoder.decode(result.m_isString))
        return false;
        
    if (result.m_isString)
        return decoder.decode(result.m_string);

    return decoder.decode(result.m_number);
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
