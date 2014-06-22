/**
 * Copyright (C) 2011 Nokia Inc.  All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "QuotesData.h"

namespace WebCore {

static size_t sizeForQuotesDataWithQuoteCount(unsigned count)
{
    return sizeof(QuotesData) + sizeof(std::pair<String, String>) * count;
}

PassRefPtr<QuotesData> QuotesData::create(const Vector<std::pair<String, String> >& quotes)
{
    void* slot = fastMalloc(sizeForQuotesDataWithQuoteCount(quotes.size()));
    return adoptRef(new (NotNull, slot) QuotesData(quotes));
}

QuotesData::QuotesData(const Vector<std::pair<String, String> >& quotes)
    : m_quoteCount(quotes.size())
{
    for (unsigned i = 0; i < m_quoteCount; ++i)
        new (NotNull, &m_quotePairs[i]) std::pair<String, String>(quotes[i]);
}

QuotesData::~QuotesData()
{
    for (unsigned i = 0; i < m_quoteCount; ++i)
        m_quotePairs[i].~pair<String, String>();
}

const String& QuotesData::openQuote(unsigned index) const
{
    if (!m_quoteCount)
        return emptyString();

    if (index >= m_quoteCount)
        return m_quotePairs[m_quoteCount - 1].first;

    return m_quotePairs[index].first;
}

const String& QuotesData::closeQuote(unsigned index) const
{
    if (!m_quoteCount)
        return emptyString();

    if (index >= m_quoteCount)
        return m_quotePairs[m_quoteCount - 1].second;

    return m_quotePairs[index].second;
}

bool operator==(const QuotesData& a, const QuotesData& b)
{
    if (a.m_quoteCount != b.m_quoteCount)
        return false;

    for (unsigned i = 0; i < a.m_quoteCount; ++i) {
        if (a.m_quotePairs[i] != b.m_quotePairs[i])
            return false;
    }

    return true;
}

} // namespace WebCore
