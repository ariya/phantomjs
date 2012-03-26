/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple, Inc. All rights reserved.
 * Copyright (C) 2005, 2006 Alexey Proskuryakov <ap@webkit.org>
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
 */

#include "config.h"

#if ENABLE(XSLT)

#include "XSLTProcessor.h"

#include "CachedResourceLoader.h"
#include "Console.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include "TransformSource.h"
#include "XMLDocumentParser.h"
#include "XSLStyleSheet.h"
#include "XSLTExtensions.h"
#include "XSLTUnicodeSort.h"
#include "markup.h"
#include <libxslt/imports.h>
#include <libxslt/security.h>
#include <libxslt/variables.h>
#include <libxslt/xsltutils.h>
#include <wtf/Assertions.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuffer.h>
#include <wtf/unicode/UTF8.h>

#if PLATFORM(MAC)
#include "SoftLinking.h"

SOFT_LINK_LIBRARY(libxslt);
SOFT_LINK(libxslt, xsltFreeStylesheet, void, (xsltStylesheetPtr sheet), (sheet))
SOFT_LINK(libxslt, xsltFreeTransformContext, void, (xsltTransformContextPtr ctxt), (ctxt))
SOFT_LINK(libxslt, xsltNewTransformContext, xsltTransformContextPtr, (xsltStylesheetPtr style, xmlDocPtr doc), (style, doc))
SOFT_LINK(libxslt, xsltApplyStylesheetUser, xmlDocPtr, (xsltStylesheetPtr style, xmlDocPtr doc, const char** params, const char* output, FILE* profile, xsltTransformContextPtr userCtxt), (style, doc, params, output, profile, userCtxt))
SOFT_LINK(libxslt, xsltQuoteUserParams, int, (xsltTransformContextPtr ctxt, const char** params), (ctxt, params))
SOFT_LINK(libxslt, xsltSetCtxtSortFunc, void, (xsltTransformContextPtr ctxt, xsltSortFunc handler), (ctxt, handler))
SOFT_LINK(libxslt, xsltSetLoaderFunc, void, (xsltDocLoaderFunc f), (f))
SOFT_LINK(libxslt, xsltSaveResultTo, int, (xmlOutputBufferPtr buf, xmlDocPtr result, xsltStylesheetPtr style), (buf, result, style))
SOFT_LINK(libxslt, xsltNextImport, xsltStylesheetPtr, (xsltStylesheetPtr style), (style))
SOFT_LINK(libxslt, xsltNewSecurityPrefs, xsltSecurityPrefsPtr, (), ())
SOFT_LINK(libxslt, xsltFreeSecurityPrefs, void, (xsltSecurityPrefsPtr sec), (sec))
SOFT_LINK(libxslt, xsltSetSecurityPrefs, int, (xsltSecurityPrefsPtr sec, xsltSecurityOption option, xsltSecurityCheck func), (sec, option, func))
SOFT_LINK(libxslt, xsltSetCtxtSecurityPrefs, int, (xsltSecurityPrefsPtr sec, xsltTransformContextPtr ctxt), (sec, ctxt))
SOFT_LINK(libxslt, xsltSecurityForbid, int, (xsltSecurityPrefsPtr sec, xsltTransformContextPtr ctxt, const char* value), (sec, ctxt, value))

#endif

namespace WebCore {

void XSLTProcessor::genericErrorFunc(void*, const char*, ...)
{
    // It would be nice to do something with this error message.
}

void XSLTProcessor::parseErrorFunc(void* userData, xmlError* error)
{
    Console* console = static_cast<Console*>(userData);
    if (!console)
        return;

    MessageLevel level;
    switch (error->level) {
    case XML_ERR_NONE:
        level = TipMessageLevel;
        break;
    case XML_ERR_WARNING:
        level = WarningMessageLevel;
        break;
    case XML_ERR_ERROR:
    case XML_ERR_FATAL:
    default:
        level = ErrorMessageLevel;
        break;
    }

    console->addMessage(XMLMessageSource, LogMessageType, level, error->message, error->line, error->file);
}

// FIXME: There seems to be no way to control the ctxt pointer for loading here, thus we have globals.
static XSLTProcessor* globalProcessor = 0;
static CachedResourceLoader* globalCachedResourceLoader = 0;
static xmlDocPtr docLoaderFunc(const xmlChar* uri,
                               xmlDictPtr,
                               int options,
                               void* ctxt,
                               xsltLoadType type)
{
    if (!globalProcessor)
        return 0;

    switch (type) {
    case XSLT_LOAD_DOCUMENT: {
        xsltTransformContextPtr context = (xsltTransformContextPtr)ctxt;
        xmlChar* base = xmlNodeGetBase(context->document->doc, context->node);
        KURL url(KURL(ParsedURLString, reinterpret_cast<const char*>(base)), reinterpret_cast<const char*>(uri));
        xmlFree(base);
        ResourceError error;
        ResourceResponse response;

        Vector<char> data;

        bool requestAllowed = globalCachedResourceLoader->frame() && globalCachedResourceLoader->document()->securityOrigin()->canRequest(url);
        if (requestAllowed) {
            globalCachedResourceLoader->frame()->loader()->loadResourceSynchronously(url, AllowStoredCredentials, error, response, data);
            requestAllowed = globalCachedResourceLoader->document()->securityOrigin()->canRequest(response.url());
        }
        if (!requestAllowed) {
            data.clear();
            globalCachedResourceLoader->printAccessDeniedMessage(url);
        }

        Console* console = 0;
        if (Frame* frame = globalProcessor->xslStylesheet()->ownerDocument()->frame())
            console = frame->domWindow()->console();
        xmlSetStructuredErrorFunc(console, XSLTProcessor::parseErrorFunc);
        xmlSetGenericErrorFunc(console, XSLTProcessor::genericErrorFunc);

        // We don't specify an encoding here. Neither Gecko nor WinIE respects
        // the encoding specified in the HTTP headers.
        xmlDocPtr doc = xmlReadMemory(data.data(), data.size(), (const char*)uri, 0, options);

        xmlSetStructuredErrorFunc(0, 0);
        xmlSetGenericErrorFunc(0, 0);

        return doc;
    }
    case XSLT_LOAD_STYLESHEET:
        return globalProcessor->xslStylesheet()->locateStylesheetSubResource(((xsltStylesheetPtr)ctxt)->doc, uri);
    default:
        break;
    }

    return 0;
}

static inline void setXSLTLoadCallBack(xsltDocLoaderFunc func, XSLTProcessor* processor, CachedResourceLoader* cachedResourceLoader)
{
    xsltSetLoaderFunc(func);
    globalProcessor = processor;
    globalCachedResourceLoader = cachedResourceLoader;
}

static int writeToVector(void* context, const char* buffer, int len)
{
    Vector<UChar>& resultOutput = *static_cast<Vector<UChar>*>(context);

    if (!len)
        return 0;

    StringBuffer stringBuffer(len);
    UChar* bufferUChar = stringBuffer.characters();
    UChar* bufferUCharEnd = bufferUChar + len;

    const char* stringCurrent = buffer;
    WTF::Unicode::ConversionResult result = WTF::Unicode::convertUTF8ToUTF16(&stringCurrent, buffer + len, &bufferUChar, bufferUCharEnd);
    if (result != WTF::Unicode::conversionOK && result != WTF::Unicode::sourceExhausted) {
        ASSERT_NOT_REACHED();
        return -1;
    }

    int utf16Length = bufferUChar - stringBuffer.characters();
    resultOutput.append(stringBuffer.characters(), utf16Length);
    return stringCurrent - buffer;
}

static bool saveResultToString(xmlDocPtr resultDoc, xsltStylesheetPtr sheet, String& resultString)
{
    xmlOutputBufferPtr outputBuf = xmlAllocOutputBuffer(0);
    if (!outputBuf)
        return false;

    Vector<UChar> resultVector;
    outputBuf->context = &resultVector;
    outputBuf->writecallback = writeToVector;

    int retval = xsltSaveResultTo(outputBuf, resultDoc, sheet);
    xmlOutputBufferClose(outputBuf);
    if (retval < 0)
        return false;

    // Workaround for <http://bugzilla.gnome.org/show_bug.cgi?id=495668>: libxslt appends an extra line feed to the result.
    if (resultVector.size() > 0 && resultVector[resultVector.size() - 1] == '\n')
        resultVector.removeLast();

    resultString = String::adopt(resultVector);

    return true;
}

static const char** xsltParamArrayFromParameterMap(XSLTProcessor::ParameterMap& parameters)
{
    if (parameters.isEmpty())
        return 0;

    const char** parameterArray = (const char**)fastMalloc(((parameters.size() * 2) + 1) * sizeof(char*));

    XSLTProcessor::ParameterMap::iterator end = parameters.end();
    unsigned index = 0;
    for (XSLTProcessor::ParameterMap::iterator it = parameters.begin(); it != end; ++it) {
        parameterArray[index++] = fastStrDup(it->first.utf8().data());
        parameterArray[index++] = fastStrDup(it->second.utf8().data());
    }
    parameterArray[index] = 0;

    return parameterArray;
}

static void freeXsltParamArray(const char** params)
{
    const char** temp = params;
    if (!params)
        return;

    while (*temp) {
        fastFree((void*)*(temp++));
        fastFree((void*)*(temp++));
    }
    fastFree(params);
}

static xsltStylesheetPtr xsltStylesheetPointer(RefPtr<XSLStyleSheet>& cachedStylesheet, Node* stylesheetRootNode)
{
    if (!cachedStylesheet && stylesheetRootNode) {
        cachedStylesheet = XSLStyleSheet::createForXSLTProcessor(stylesheetRootNode->parentNode() ? stylesheetRootNode->parentNode() : stylesheetRootNode,
            stylesheetRootNode->document()->url().string(),
            stylesheetRootNode->document()->url()); // FIXME: Should we use baseURL here?

        // According to Mozilla documentation, the node must be a Document node, an xsl:stylesheet or xsl:transform element.
        // But we just use text content regardless of node type.
        cachedStylesheet->parseString(createMarkup(stylesheetRootNode));
    }

    if (!cachedStylesheet || !cachedStylesheet->document())
        return 0;

    return cachedStylesheet->compileStyleSheet();
}

static inline xmlDocPtr xmlDocPtrFromNode(Node* sourceNode, bool& shouldDelete)
{
    RefPtr<Document> ownerDocument = sourceNode->document();
    bool sourceIsDocument = (sourceNode == ownerDocument.get());

    xmlDocPtr sourceDoc = 0;
    if (sourceIsDocument && ownerDocument->transformSource())
        sourceDoc = (xmlDocPtr)ownerDocument->transformSource()->platformSource();
    if (!sourceDoc) {
        sourceDoc = (xmlDocPtr)xmlDocPtrForString(ownerDocument->cachedResourceLoader(), createMarkup(sourceNode),
            sourceIsDocument ? ownerDocument->url().string() : String());
        shouldDelete = sourceDoc;
    }
    return sourceDoc;
}

static inline String resultMIMEType(xmlDocPtr resultDoc, xsltStylesheetPtr sheet)
{
    // There are three types of output we need to be able to deal with:
    // HTML (create an HTML document), XML (create an XML document),
    // and text (wrap in a <pre> and create an XML document).

    const xmlChar* resultType = 0;
    XSLT_GET_IMPORT_PTR(resultType, sheet, method);
    if (!resultType && resultDoc->type == XML_HTML_DOCUMENT_NODE)
        resultType = (const xmlChar*)"html";

    if (xmlStrEqual(resultType, (const xmlChar*)"html"))
        return "text/html";
    if (xmlStrEqual(resultType, (const xmlChar*)"text"))
        return "text/plain";

    return "application/xml";
}

bool XSLTProcessor::transformToString(Node* sourceNode, String& mimeType, String& resultString, String& resultEncoding)
{
    RefPtr<Document> ownerDocument = sourceNode->document();

    setXSLTLoadCallBack(docLoaderFunc, this, ownerDocument->cachedResourceLoader());
    xsltStylesheetPtr sheet = xsltStylesheetPointer(m_stylesheet, m_stylesheetRootNode.get());
    if (!sheet) {
        setXSLTLoadCallBack(0, 0, 0);
        return false;
    }
    m_stylesheet->clearDocuments();

    xmlChar* origMethod = sheet->method;
    if (!origMethod && mimeType == "text/html")
        sheet->method = (xmlChar*)"html";

    bool success = false;
    bool shouldFreeSourceDoc = false;
    if (xmlDocPtr sourceDoc = xmlDocPtrFromNode(sourceNode, shouldFreeSourceDoc)) {
        // The XML declaration would prevent parsing the result as a fragment, and it's not needed even for documents,
        // as the result of this function is always immediately parsed.
        sheet->omitXmlDeclaration = true;

        xsltTransformContextPtr transformContext = xsltNewTransformContext(sheet, sourceDoc);
        registerXSLTExtensions(transformContext);

        xsltSecurityPrefsPtr securityPrefs = xsltNewSecurityPrefs();
        // Read permissions are checked by docLoaderFunc.
        if (0 != xsltSetSecurityPrefs(securityPrefs, XSLT_SECPREF_WRITE_FILE, xsltSecurityForbid))
            CRASH();
        if (0 != xsltSetSecurityPrefs(securityPrefs, XSLT_SECPREF_CREATE_DIRECTORY, xsltSecurityForbid))
            CRASH();
        if (0 != xsltSetSecurityPrefs(securityPrefs, XSLT_SECPREF_WRITE_NETWORK, xsltSecurityForbid))
            CRASH();
        if (0 != xsltSetCtxtSecurityPrefs(securityPrefs, transformContext))
            CRASH();

        // <http://bugs.webkit.org/show_bug.cgi?id=16077>: XSLT processor <xsl:sort> algorithm only compares by code point.
        xsltSetCtxtSortFunc(transformContext, xsltUnicodeSortFunction);

        // This is a workaround for a bug in libxslt.
        // The bug has been fixed in version 1.1.13, so once we ship that this can be removed.
        if (!transformContext->globalVars)
           transformContext->globalVars = xmlHashCreate(20);

        const char** params = xsltParamArrayFromParameterMap(m_parameters);
        xsltQuoteUserParams(transformContext, params);
        xmlDocPtr resultDoc = xsltApplyStylesheetUser(sheet, sourceDoc, 0, 0, 0, transformContext);

        xsltFreeTransformContext(transformContext);
        xsltFreeSecurityPrefs(securityPrefs);
        freeXsltParamArray(params);

        if (shouldFreeSourceDoc)
            xmlFreeDoc(sourceDoc);

        if ((success = saveResultToString(resultDoc, sheet, resultString))) {
            mimeType = resultMIMEType(resultDoc, sheet);
            resultEncoding = (char*)resultDoc->encoding;
        }
        xmlFreeDoc(resultDoc);
    }

    sheet->method = origMethod;
    setXSLTLoadCallBack(0, 0, 0);
    xsltFreeStylesheet(sheet);
    m_stylesheet = 0;

    return success;
}

} // namespace WebCore

#endif // ENABLE(XSLT)
