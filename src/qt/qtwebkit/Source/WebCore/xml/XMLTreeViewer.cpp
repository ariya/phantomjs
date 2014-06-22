/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XMLTreeViewer.h"

#if ENABLE(XSLT)

#include "Document.h"
#include "Element.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "Page.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "Settings.h"
#include "Text.h"
#include "XMLViewerCSS.h"
#include "XMLViewerJS.h"

using namespace std;

namespace WebCore {

XMLTreeViewer::XMLTreeViewer(Document* document)
    : m_document(document)
{
}

bool XMLTreeViewer::hasNoStyleInformation() const
{
    if (m_document->sawElementsInKnownNamespaces() || m_document->transformSourceDocument())
        return false;

    if (!m_document->frame() || !m_document->frame()->page())
        return false;

    if (!m_document->frame()->page()->settings()->developerExtrasEnabled())
        return false;

    if (m_document->frame()->tree()->parent())
        return false; // This document is not in a top frame

    return true;
}

void XMLTreeViewer::transformDocumentToTreeView()
{
    m_document->setIsViewSource(true);
    String scriptString(reinterpret_cast<const char*>(XMLViewer_js), sizeof(XMLViewer_js));
    m_document->frame()->script()->evaluate(ScriptSourceCode(scriptString));
    String noStyleMessage("This XML file does not appear to have any style information associated with it. The document tree is shown below.");
    m_document->frame()->script()->evaluate(ScriptSourceCode("prepareWebKitXMLViewer('" + noStyleMessage + "');"));

    String cssString(reinterpret_cast<const char*>(XMLViewer_css), sizeof(XMLViewer_css));
    RefPtr<Text> text = m_document->createTextNode(cssString);
    m_document->getElementById("xml-viewer-style")->appendChild(text, IGNORE_EXCEPTION);
    m_document->styleResolverChanged(RecalcStyleImmediately);
}

} // namespace WebCore

#endif // ENABLE(XSLT)
