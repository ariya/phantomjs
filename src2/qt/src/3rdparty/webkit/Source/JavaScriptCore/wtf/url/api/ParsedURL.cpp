/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
#include "ParsedURL.h"

#include "URLComponent.h"
#include "URLParser.h"

namespace WTF {

ParsedURL::ParsedURL(const URLString& spec)
    : m_spec(spec)
{
    // FIXME: Handle non-standard URLs.
    if (spec.string().isEmpty())
        return;
    URLParser<UChar>::parseStandardURL(spec.string().characters(), spec.string().length(), m_segments);
}

String ParsedURL::scheme() const
{
    return segment(m_segments.scheme);
}

String ParsedURL::username() const
{
    return segment(m_segments.username);
}

String ParsedURL::password() const
{
    return segment(m_segments.password);
}

String ParsedURL::host() const
{
    return segment(m_segments.host);
}

String ParsedURL::port() const
{
    return segment(m_segments.port);
}

String ParsedURL::path() const
{
    return segment(m_segments.path);
}

String ParsedURL::query() const
{
    return segment(m_segments.query);
}

String ParsedURL::fragment() const
{
    return segment(m_segments.fragment);
}

String ParsedURL::segment(const URLComponent& component) const
{
    if (!component.isValid())
        return String();
    return m_spec.string().substring(component.begin(), component.length());
}

}
