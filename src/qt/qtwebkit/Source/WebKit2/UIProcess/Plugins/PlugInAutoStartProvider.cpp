/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "PlugInAutoStartProvider.h"

#include "ImmutableArray.h"
#include "ImmutableDictionary.h"
#include "WebContext.h"
#include "WebContextClient.h"
#include "WebProcessMessages.h"
#include <wtf/CurrentTime.h>

using namespace WebCore;

static const double plugInAutoStartExpirationTimeThreshold = 30 * 24 * 60 * 60;

namespace WebKit {

PlugInAutoStartProvider::PlugInAutoStartProvider(WebContext* context)
    : m_context(context)
{
}

static double expirationTimeFromNow()
{
    return currentTime() + plugInAutoStartExpirationTimeThreshold;
}

void PlugInAutoStartProvider::addAutoStartOriginHash(const String& pageOrigin, unsigned plugInOriginHash)
{
    if (m_hashToOriginMap.contains(plugInOriginHash))
        return;

    AutoStartTable::iterator it = m_autoStartTable.find(pageOrigin);
    if (it == m_autoStartTable.end())
        it = m_autoStartTable.add(pageOrigin, PlugInAutoStartOriginHash()).iterator;

    double expirationTime = expirationTimeFromNow();
    it->value.set(plugInOriginHash, expirationTime);
    m_hashToOriginMap.set(plugInOriginHash, pageOrigin);

    m_context->sendToAllProcesses(Messages::WebProcess::DidAddPlugInAutoStartOriginHash(plugInOriginHash, expirationTime));
    m_context->client().plugInAutoStartOriginHashesChanged(m_context);
}

PlugInAutoStartOriginHash PlugInAutoStartProvider::autoStartOriginHashesCopy() const
{
    PlugInAutoStartOriginHash copyMap;
    AutoStartTable::const_iterator end = m_autoStartTable.end();
    for (AutoStartTable::const_iterator it = m_autoStartTable.begin(); it != end; ++it) {
        PlugInAutoStartOriginHash::const_iterator mapEnd = it->value.end();
        for (PlugInAutoStartOriginHash::const_iterator mapIt = it->value.begin(); mapIt != mapEnd; ++mapIt)
            copyMap.set(mapIt->key, mapIt->value);
    }
    return copyMap;
}

PassRefPtr<ImmutableDictionary> PlugInAutoStartProvider::autoStartOriginsTableCopy() const
{
    ImmutableDictionary::MapType map;
    AutoStartTable::const_iterator end = m_autoStartTable.end();
    double now = currentTime();
    for (AutoStartTable::const_iterator it = m_autoStartTable.begin(); it != end; ++it) {
        ImmutableDictionary::MapType hashMap;
        PlugInAutoStartOriginHash::const_iterator valueEnd = it->value.end();
        for (PlugInAutoStartOriginHash::const_iterator valueIt = it->value.begin(); valueIt != valueEnd; ++valueIt) {
            if (now > valueIt->value)
                continue;
            hashMap.set(String::number(valueIt->key), WebDouble::create(valueIt->value));
        }

        if (hashMap.size())
            map.set(it->key, ImmutableDictionary::adopt(hashMap));
    }

    return ImmutableDictionary::adopt(map);
}

void PlugInAutoStartProvider::setAutoStartOriginsTable(ImmutableDictionary& table)
{
    m_hashToOriginMap.clear();
    m_autoStartTable.clear();
    HashMap<unsigned, double> hashMap;

    ImmutableDictionary::MapType::const_iterator end = table.map().end();
    for (ImmutableDictionary::MapType::const_iterator it = table.map().begin(); it != end; ++it) {
        PlugInAutoStartOriginHash hashes;
        ImmutableDictionary* hashesForPage = static_cast<ImmutableDictionary*>(it->value.get());
        ImmutableDictionary::MapType::const_iterator hashEnd = hashesForPage->map().end();
        for (ImmutableDictionary::MapType::const_iterator hashIt = hashesForPage->map().begin(); hashIt != hashEnd; ++hashIt) {
            bool ok;
            unsigned hash = hashIt->key.toUInt(&ok);
            if (!ok)
                continue;

            if (hashIt->value->type() != WebDouble::APIType)
                continue;

            double expirationTime = static_cast<WebDouble*>(hashIt->value.get())->value();
            hashes.set(hash, expirationTime);
            hashMap.set(hash, expirationTime);
            m_hashToOriginMap.set(hash, it->key);
        }

        m_autoStartTable.set(it->key, hashes);
    }

    m_context->sendToAllProcesses(Messages::WebProcess::ResetPlugInAutoStartOriginHashes(hashMap));
}

void PlugInAutoStartProvider::setAutoStartOriginsArray(ImmutableArray& originList)
{
    m_autoStartOrigins.clear();
    for (size_t i = 0, length = originList.size(); i < length; ++i) {
        if (originList.at(i)->type() != WebString::APIType)
            continue;
        m_autoStartOrigins.append(static_cast<WebString*>(originList.at(i))->string());
    }
}

void PlugInAutoStartProvider::didReceiveUserInteraction(unsigned plugInOriginHash)
{
    HashMap<unsigned, String>::const_iterator it = m_hashToOriginMap.find(plugInOriginHash);
    if (it == m_hashToOriginMap.end()) {
        ASSERT_NOT_REACHED();
        return;
    }

    double newExpirationTime = expirationTimeFromNow();
    m_autoStartTable.find(it->value)->value.set(plugInOriginHash, newExpirationTime);
    m_context->sendToAllProcesses(Messages::WebProcess::DidAddPlugInAutoStartOriginHash(plugInOriginHash, newExpirationTime));
    m_context->client().plugInAutoStartOriginHashesChanged(m_context);
}

} // namespace WebKit
