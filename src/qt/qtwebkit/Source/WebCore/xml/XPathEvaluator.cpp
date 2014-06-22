/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XPathEvaluator.h"

#include "ExceptionCode.h"
#include "NativeXPathNSResolver.h"
#include "Node.h"
#include "XPathExpression.h"
#include "XPathResult.h"
#include "XPathUtil.h"

namespace WebCore {

using namespace XPath;

PassRefPtr<XPathExpression> XPathEvaluator::createExpression(const String& expression,
                                                             XPathNSResolver* resolver,
                                                             ExceptionCode& ec)
{
    return XPathExpression::createExpression(expression, resolver, ec);
}

PassRefPtr<XPathNSResolver> XPathEvaluator::createNSResolver(Node* nodeResolver)
{
    return NativeXPathNSResolver::create(nodeResolver);
}

PassRefPtr<XPathResult> XPathEvaluator::evaluate(const String& expression,
                                                 Node* contextNode,
                                                 XPathNSResolver* resolver,
                                                 unsigned short type,
                                                 XPathResult* result,
                                                 ExceptionCode& ec)
{
    if (!isValidContextNode(contextNode)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    ec = 0;
    RefPtr<XPathExpression> expr = createExpression(expression, resolver, ec);
    if (ec)
        return 0;
    
    return expr->evaluate(contextNode, type, result, ec);
}

}
