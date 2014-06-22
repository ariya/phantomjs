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

#include "config.h"
#include "LazyOperandValueProfile.h"

#if ENABLE(VALUE_PROFILER)

#include "Operations.h"

namespace JSC {

CompressedLazyOperandValueProfileHolder::CompressedLazyOperandValueProfileHolder() { }
CompressedLazyOperandValueProfileHolder::~CompressedLazyOperandValueProfileHolder() { }

void CompressedLazyOperandValueProfileHolder::computeUpdatedPredictions(OperationInProgress operation)
{
    if (!m_data)
        return;
    
    for (unsigned i = 0; i < m_data->size(); ++i)
        m_data->at(i).computeUpdatedPrediction(operation);
}

LazyOperandValueProfile* CompressedLazyOperandValueProfileHolder::add(
    const LazyOperandValueProfileKey& key)
{
    if (!m_data)
        m_data = adoptPtr(new LazyOperandValueProfile::List());
    else {
        for (unsigned i = 0; i < m_data->size(); ++i) {
            if (m_data->at(i).key() == key)
                return &m_data->at(i);
        }
    }
    
    m_data->append(LazyOperandValueProfile(key));
    return &m_data->last();
}

LazyOperandValueProfileParser::LazyOperandValueProfileParser(
    CompressedLazyOperandValueProfileHolder& holder)
    : m_holder(holder)
{
    if (!m_holder.m_data)
        return;
    
    LazyOperandValueProfile::List& data = *m_holder.m_data;
    for (unsigned i = 0; i < data.size(); ++i)
        m_map.add(data[i].key(), &data[i]);
}

LazyOperandValueProfileParser::~LazyOperandValueProfileParser() { }

LazyOperandValueProfile* LazyOperandValueProfileParser::getIfPresent(
    const LazyOperandValueProfileKey& key) const
{
    HashMap<LazyOperandValueProfileKey, LazyOperandValueProfile*>::const_iterator iter =
        m_map.find(key);
    
    if (iter == m_map.end())
        return 0;
    
    return iter->value;
}

SpeculatedType LazyOperandValueProfileParser::prediction(
    const LazyOperandValueProfileKey& key) const
{
    LazyOperandValueProfile* profile = getIfPresent(key);
    if (!profile)
        return SpecNone;
    
    return profile->computeUpdatedPrediction();
}

} // namespace JSC

#endif // ENABLE(VALUE_PROFILER)

