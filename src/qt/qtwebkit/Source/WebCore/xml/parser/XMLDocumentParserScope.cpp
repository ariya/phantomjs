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
#include "XMLDocumentParserScope.h"

namespace WebCore {

CachedResourceLoader* XMLDocumentParserScope::currentCachedResourceLoader = 0;

XMLDocumentParserScope::XMLDocumentParserScope(CachedResourceLoader* cachedResourceLoader)
    : m_oldCachedResourceLoader(currentCachedResourceLoader)
#if ENABLE(XSLT)
    , m_oldGenericErrorFunc(xmlGenericError)
    , m_oldStructuredErrorFunc(xmlStructuredError)
    , m_oldErrorContext(xmlGenericErrorContext)
#endif
{
    currentCachedResourceLoader = cachedResourceLoader;
}

#if ENABLE(XSLT)
XMLDocumentParserScope::XMLDocumentParserScope(CachedResourceLoader* cachedResourceLoader, xmlGenericErrorFunc genericErrorFunc, xmlStructuredErrorFunc structuredErrorFunc, void* errorContext)
    : m_oldCachedResourceLoader(currentCachedResourceLoader)
    , m_oldGenericErrorFunc(xmlGenericError)
    , m_oldStructuredErrorFunc(xmlStructuredError)
    , m_oldErrorContext(xmlGenericErrorContext)
{
    currentCachedResourceLoader = cachedResourceLoader;
    if (genericErrorFunc)
        xmlSetGenericErrorFunc(errorContext, genericErrorFunc);
    if (structuredErrorFunc)
        xmlSetStructuredErrorFunc(errorContext, structuredErrorFunc);
}
#endif

XMLDocumentParserScope::~XMLDocumentParserScope()
{
    currentCachedResourceLoader = m_oldCachedResourceLoader;
#if ENABLE(XSLT)
    xmlSetGenericErrorFunc(m_oldErrorContext, m_oldGenericErrorFunc);
    xmlSetStructuredErrorFunc(m_oldErrorContext, m_oldStructuredErrorFunc);
#endif
}

}
