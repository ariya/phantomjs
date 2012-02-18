/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(XSLT)

#include "JSXSLTProcessor.h"

#include "Document.h"
#include "DocumentFragment.h"
#include "JSDocument.h"
#include "JSDocumentFragment.h"
#include "JSNode.h"
#include "Node.h"
#include "PlatformString.h"
#include "XSLTProcessor.h"
#include "JSDOMBinding.h"

using namespace JSC;

namespace WebCore {

JSValue JSXSLTProcessor::importStylesheet(ExecState* exec)
{
    JSValue nodeVal = exec->argument(0);
    if (nodeVal.inherits(&JSNode::s_info)) {
        JSNode* node = static_cast<JSNode*>(asObject(nodeVal));
        impl()->importStylesheet(node->impl());
        return jsUndefined();
    }
    // Throw exception?
    return jsUndefined();
}

JSValue JSXSLTProcessor::transformToFragment(ExecState* exec)
{
    JSValue nodeVal = exec->argument(0);
    JSValue docVal = exec->argument(1);
    if (nodeVal.inherits(&JSNode::s_info) && docVal.inherits(&JSDocument::s_info)) {
        WebCore::Node* node = static_cast<JSNode*>(asObject(nodeVal))->impl();
        Document* doc = static_cast<Document*>(static_cast<JSDocument*>(asObject(docVal))->impl());
        return toJS(exec, globalObject(), impl()->transformToFragment(node, doc).get());
    }
    // Throw exception?
    return jsUndefined();
}

JSValue JSXSLTProcessor::transformToDocument(ExecState* exec)
{
    JSValue nodeVal = exec->argument(0);
    if (nodeVal.inherits(&JSNode::s_info)) {
        JSNode* node = static_cast<JSNode*>(asObject(nodeVal));
        RefPtr<Document> resultDocument = impl()->transformToDocument(node->impl());
        if (resultDocument)
            return toJS(exec, globalObject(), resultDocument.get());
        return jsUndefined();
    }
    // Throw exception?
    return jsUndefined();
}

JSValue JSXSLTProcessor::setParameter(ExecState* exec)
{
    if (exec->argument(1).isUndefinedOrNull() || exec->argument(2).isUndefinedOrNull())
        return jsUndefined(); // Throw exception?
    String namespaceURI = ustringToString(exec->argument(0).toString(exec));
    String localName = ustringToString(exec->argument(1).toString(exec));
    String value = ustringToString(exec->argument(2).toString(exec));
    impl()->setParameter(namespaceURI, localName, value);
    return jsUndefined();
}

JSValue JSXSLTProcessor::getParameter(ExecState* exec)
{
    if (exec->argument(1).isUndefinedOrNull())
        return jsUndefined();
    String namespaceURI = ustringToString(exec->argument(0).toString(exec));
    String localName = ustringToString(exec->argument(1).toString(exec));
    String value = impl()->getParameter(namespaceURI, localName);
    return jsStringOrUndefined(exec, value);
}

JSValue JSXSLTProcessor::removeParameter(ExecState* exec)
{
    if (exec->argument(1).isUndefinedOrNull())
        return jsUndefined();
    String namespaceURI = ustringToString(exec->argument(0).toString(exec));
    String localName = ustringToString(exec->argument(1).toString(exec));
    impl()->removeParameter(namespaceURI, localName);
    return jsUndefined();
}

EncodedJSValue JSC_HOST_CALL JSXSLTProcessorConstructor::constructJSXSLTProcessor(ExecState* exec)
{
    JSXSLTProcessorConstructor* jsConstructor = static_cast<JSXSLTProcessorConstructor*>(exec->callee());
    return JSValue::encode(CREATE_DOM_WRAPPER(exec, jsConstructor->globalObject(), XSLTProcessor, XSLTProcessor::create().get()));
}

} // namespace WebCore

#endif // ENABLE(XSLT)
