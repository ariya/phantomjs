/*
 * Copyright (C) 2000 Peter Kelly <pmk@post.com>
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
#include "XMLDocumentParser.h"

#include "CDATASection.h"
#include "CachedScript.h"
#include "Comment.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLEntityParser.h"
#include "HTMLHtmlElement.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "HTMLTemplateElement.h"
#include "ProcessingInstruction.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "SecurityOrigin.h"
#include "TextResourceDecoder.h"
#include "TransformSource.h"
#include "XMLNSNames.h"
#include "XMLDocumentParserScope.h"
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <wtf/text/CString.h>
#include <wtf/StringExtras.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/unicode/UTF8.h>

#if ENABLE(XSLT)
#include "XMLTreeViewer.h"
#include <libxslt/xslt.h>
#endif

using namespace std;

namespace WebCore {

class PendingCallbacks {
    WTF_MAKE_NONCOPYABLE(PendingCallbacks); WTF_MAKE_FAST_ALLOCATED;
public:
    ~PendingCallbacks() { }
    static PassOwnPtr<PendingCallbacks> create()
    {
        return adoptPtr(new PendingCallbacks);
    }
    
    void appendStartElementNSCallback(const xmlChar* xmlLocalName, const xmlChar* xmlPrefix, const xmlChar* xmlURI, int nb_namespaces,
                                      const xmlChar** namespaces, int nb_attributes, int nb_defaulted, const xmlChar** attributes)
    {
        OwnPtr<PendingStartElementNSCallback> callback = adoptPtr(new PendingStartElementNSCallback);

        callback->xmlLocalName = xmlStrdup(xmlLocalName);
        callback->xmlPrefix = xmlStrdup(xmlPrefix);
        callback->xmlURI = xmlStrdup(xmlURI);
        callback->nb_namespaces = nb_namespaces;
        callback->namespaces = static_cast<xmlChar**>(xmlMalloc(sizeof(xmlChar*) * nb_namespaces * 2));
        for (int i = 0; i < nb_namespaces * 2 ; i++)
            callback->namespaces[i] = xmlStrdup(namespaces[i]);
        callback->nb_attributes = nb_attributes;
        callback->nb_defaulted = nb_defaulted;
        callback->attributes = static_cast<xmlChar**>(xmlMalloc(sizeof(xmlChar*) * nb_attributes * 5));
        for (int i = 0; i < nb_attributes; i++) {
            // Each attribute has 5 elements in the array:
            // name, prefix, uri, value and an end pointer.

            for (int j = 0; j < 3; j++)
                callback->attributes[i * 5 + j] = xmlStrdup(attributes[i * 5 + j]);

            int len = attributes[i * 5 + 4] - attributes[i * 5 + 3];

            callback->attributes[i * 5 + 3] = xmlStrndup(attributes[i * 5 + 3], len);
            callback->attributes[i * 5 + 4] = callback->attributes[i * 5 + 3] + len;
        }

        m_callbacks.append(callback.release());
    }

    void appendEndElementNSCallback()
    {
        m_callbacks.append(adoptPtr(new PendingEndElementNSCallback));
    }

    void appendCharactersCallback(const xmlChar* s, int len)
    {
        OwnPtr<PendingCharactersCallback> callback = adoptPtr(new PendingCharactersCallback);

        callback->s = xmlStrndup(s, len);
        callback->len = len;

        m_callbacks.append(callback.release());
    }

    void appendProcessingInstructionCallback(const xmlChar* target, const xmlChar* data)
    {
        OwnPtr<PendingProcessingInstructionCallback> callback = adoptPtr(new PendingProcessingInstructionCallback);

        callback->target = xmlStrdup(target);
        callback->data = xmlStrdup(data);

        m_callbacks.append(callback.release());
    }

    void appendCDATABlockCallback(const xmlChar* s, int len)
    {
        OwnPtr<PendingCDATABlockCallback> callback = adoptPtr(new PendingCDATABlockCallback);

        callback->s = xmlStrndup(s, len);
        callback->len = len;

        m_callbacks.append(callback.release());
    }

    void appendCommentCallback(const xmlChar* s)
    {
        OwnPtr<PendingCommentCallback> callback = adoptPtr(new PendingCommentCallback);

        callback->s = xmlStrdup(s);

        m_callbacks.append(callback.release());
    }

    void appendInternalSubsetCallback(const xmlChar* name, const xmlChar* externalID, const xmlChar* systemID)
    {
        OwnPtr<PendingInternalSubsetCallback> callback = adoptPtr(new PendingInternalSubsetCallback);

        callback->name = xmlStrdup(name);
        callback->externalID = xmlStrdup(externalID);
        callback->systemID = xmlStrdup(systemID);

        m_callbacks.append(callback.release());
    }

    void appendErrorCallback(XMLErrors::ErrorType type, const xmlChar* message, OrdinalNumber lineNumber, OrdinalNumber columnNumber)
    {
        OwnPtr<PendingErrorCallback> callback = adoptPtr(new PendingErrorCallback);

        callback->message = xmlStrdup(message);
        callback->type = type;
        callback->lineNumber = lineNumber;
        callback->columnNumber = columnNumber;

        m_callbacks.append(callback.release());
    }

    void callAndRemoveFirstCallback(XMLDocumentParser* parser)
    {
        OwnPtr<PendingCallback> callback = m_callbacks.takeFirst();
        callback->call(parser);
    }

    bool isEmpty() const { return m_callbacks.isEmpty(); }

private:
    PendingCallbacks() { }

    struct PendingCallback {
        virtual ~PendingCallback() { }
        virtual void call(XMLDocumentParser* parser) = 0;
    };

    struct PendingStartElementNSCallback : public PendingCallback {
        virtual ~PendingStartElementNSCallback()
        {
            xmlFree(xmlLocalName);
            xmlFree(xmlPrefix);
            xmlFree(xmlURI);
            for (int i = 0; i < nb_namespaces * 2; i++)
                xmlFree(namespaces[i]);
            xmlFree(namespaces);
            for (int i = 0; i < nb_attributes; i++)
                for (int j = 0; j < 4; j++)
                    xmlFree(attributes[i * 5 + j]);
            xmlFree(attributes);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->startElementNs(xmlLocalName, xmlPrefix, xmlURI,
                                      nb_namespaces, const_cast<const xmlChar**>(namespaces),
                                      nb_attributes, nb_defaulted, const_cast<const xmlChar**>(attributes));
        }

        xmlChar* xmlLocalName;
        xmlChar* xmlPrefix;
        xmlChar* xmlURI;
        int nb_namespaces;
        xmlChar** namespaces;
        int nb_attributes;
        int nb_defaulted;
        xmlChar** attributes;
    };

    struct PendingEndElementNSCallback : public PendingCallback {
        virtual void call(XMLDocumentParser* parser)
        {
            parser->endElementNs();
        }
    };

    struct PendingCharactersCallback : public PendingCallback {
        virtual ~PendingCharactersCallback()
        {
            xmlFree(s);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->characters(s, len);
        }

        xmlChar* s;
        int len;
    };

    struct PendingProcessingInstructionCallback : public PendingCallback {
        virtual ~PendingProcessingInstructionCallback()
        {
            xmlFree(target);
            xmlFree(data);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->processingInstruction(target, data);
        }

        xmlChar* target;
        xmlChar* data;
    };

    struct PendingCDATABlockCallback : public PendingCallback {
        virtual ~PendingCDATABlockCallback()
        {
            xmlFree(s);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->cdataBlock(s, len);
        }

        xmlChar* s;
        int len;
    };

    struct PendingCommentCallback : public PendingCallback {
        virtual ~PendingCommentCallback()
        {
            xmlFree(s);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->comment(s);
        }

        xmlChar* s;
    };

    struct PendingInternalSubsetCallback : public PendingCallback {
        virtual ~PendingInternalSubsetCallback()
        {
            xmlFree(name);
            xmlFree(externalID);
            xmlFree(systemID);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->internalSubset(name, externalID, systemID);
        }

        xmlChar* name;
        xmlChar* externalID;
        xmlChar* systemID;
    };

    struct PendingErrorCallback: public PendingCallback {
        virtual ~PendingErrorCallback()
        {
            xmlFree(message);
        }

        virtual void call(XMLDocumentParser* parser)
        {
            parser->handleError(type, reinterpret_cast<char*>(message), TextPosition(lineNumber, columnNumber));
        }

        XMLErrors::ErrorType type;
        xmlChar* message;
        OrdinalNumber lineNumber;
        OrdinalNumber columnNumber;
    };

    Deque<OwnPtr<PendingCallback> > m_callbacks;
};
// --------------------------------

static int globalDescriptor = 0;
static ThreadIdentifier libxmlLoaderThread = 0;

static int matchFunc(const char*)
{
    // Only match loads initiated due to uses of libxml2 from within XMLDocumentParser to avoid
    // interfering with client applications that also use libxml2.  http://bugs.webkit.org/show_bug.cgi?id=17353
    return XMLDocumentParserScope::currentCachedResourceLoader && currentThread() == libxmlLoaderThread;
}

class OffsetBuffer {
    WTF_MAKE_FAST_ALLOCATED;
public:
    OffsetBuffer(const Vector<char>& b) : m_buffer(b), m_currentOffset(0) { }

    int readOutBytes(char* outputBuffer, unsigned askedToRead)
    {
        unsigned bytesLeft = m_buffer.size() - m_currentOffset;
        unsigned lenToCopy = min(askedToRead, bytesLeft);
        if (lenToCopy) {
            memcpy(outputBuffer, m_buffer.data() + m_currentOffset, lenToCopy);
            m_currentOffset += lenToCopy;
        }
        return lenToCopy;
    }

private:
    Vector<char> m_buffer;
    unsigned m_currentOffset;
};

static inline void setAttributes(Element* element, Vector<Attribute>& attributeVector, ParserContentPolicy parserContentPolicy)
{
    if (!scriptingContentIsAllowed(parserContentPolicy))
        element->stripScriptingAttributes(attributeVector);
    element->parserSetAttributes(attributeVector);
}

static void switchToUTF16(xmlParserCtxtPtr ctxt)
{
    // Hack around libxml2's lack of encoding overide support by manually
    // resetting the encoding to UTF-16 before every chunk.  Otherwise libxml
    // will detect <?xml version="1.0" encoding="<encoding name>"?> blocks
    // and switch encodings, causing the parse to fail.

    // FIXME: Can we just use XML_PARSE_IGNORE_ENC now?

    const UChar BOM = 0xFEFF;
    const unsigned char BOMHighByte = *reinterpret_cast<const unsigned char*>(&BOM);
    xmlSwitchEncoding(ctxt, BOMHighByte == 0xFF ? XML_CHAR_ENCODING_UTF16LE : XML_CHAR_ENCODING_UTF16BE);
}

static bool shouldAllowExternalLoad(const KURL& url)
{
    String urlString = url.string();

    // On non-Windows platforms libxml asks for this URL, the
    // "XML_XML_DEFAULT_CATALOG", on initialization.
    if (urlString == "file:///etc/xml/catalog")
        return false;

    // On Windows, libxml computes a URL relative to where its DLL resides.
    if (urlString.startsWith("file:///", false) && urlString.endsWith("/etc/catalog", false))
        return false;

    // The most common DTD.  There isn't much point in hammering www.w3c.org
    // by requesting this URL for every XHTML document.
    if (urlString.startsWith("http://www.w3.org/TR/xhtml", false))
        return false;

    // Similarly, there isn't much point in requesting the SVG DTD.
    if (urlString.startsWith("http://www.w3.org/Graphics/SVG", false))
        return false;

    // The libxml doesn't give us a lot of context for deciding whether to
    // allow this request.  In the worst case, this load could be for an
    // external entity and the resulting document could simply read the
    // retrieved content.  If we had more context, we could potentially allow
    // the parser to load a DTD.  As things stand, we take the conservative
    // route and allow same-origin requests only.
    if (!XMLDocumentParserScope::currentCachedResourceLoader->document()->securityOrigin()->canRequest(url)) {
        XMLDocumentParserScope::currentCachedResourceLoader->printAccessDeniedMessage(url);
        return false;
    }

    return true;
}

static void* openFunc(const char* uri)
{
    ASSERT(XMLDocumentParserScope::currentCachedResourceLoader);
    ASSERT(currentThread() == libxmlLoaderThread);

    KURL url(KURL(), uri);

    if (!shouldAllowExternalLoad(url))
        return &globalDescriptor;

    ResourceError error;
    ResourceResponse response;
    Vector<char> data;


    {
        CachedResourceLoader* cachedResourceLoader = XMLDocumentParserScope::currentCachedResourceLoader;
        XMLDocumentParserScope scope(0);
        // FIXME: We should restore the original global error handler as well.

        if (cachedResourceLoader->frame())
            cachedResourceLoader->frame()->loader()->loadResourceSynchronously(url, AllowStoredCredentials, DoNotAskClientForCrossOriginCredentials, error, response, data);
    }

    // We have to check the URL again after the load to catch redirects.
    // See <https://bugs.webkit.org/show_bug.cgi?id=21963>.
    if (!shouldAllowExternalLoad(response.url()))
        return &globalDescriptor;

    return new OffsetBuffer(data);
}

static int readFunc(void* context, char* buffer, int len)
{
    // Do 0-byte reads in case of a null descriptor
    if (context == &globalDescriptor)
        return 0;

    OffsetBuffer* data = static_cast<OffsetBuffer*>(context);
    return data->readOutBytes(buffer, len);
}

static int writeFunc(void*, const char*, int)
{
    // Always just do 0-byte writes
    return 0;
}

static int closeFunc(void* context)
{
    if (context != &globalDescriptor) {
        OffsetBuffer* data = static_cast<OffsetBuffer*>(context);
        delete data;
    }
    return 0;
}

#if ENABLE(XSLT)
static void errorFunc(void*, const char*, ...)
{
    // FIXME: It would be nice to display error messages somewhere.
}
#endif

static bool didInit = false;

PassRefPtr<XMLParserContext> XMLParserContext::createStringParser(xmlSAXHandlerPtr handlers, void* userData)
{
    if (!didInit) {
        xmlInitParser();
        xmlRegisterInputCallbacks(matchFunc, openFunc, readFunc, closeFunc);
        xmlRegisterOutputCallbacks(matchFunc, openFunc, writeFunc, closeFunc);
        libxmlLoaderThread = currentThread();
        didInit = true;
    }

    xmlParserCtxtPtr parser = xmlCreatePushParserCtxt(handlers, 0, 0, 0, 0);
    parser->_private = userData;

    // Substitute entities.
    xmlCtxtUseOptions(parser, XML_PARSE_NOENT);

    switchToUTF16(parser);

    return adoptRef(new XMLParserContext(parser));
}


// Chunk should be encoded in UTF-8
PassRefPtr<XMLParserContext> XMLParserContext::createMemoryParser(xmlSAXHandlerPtr handlers, void* userData, const CString& chunk)
{
    if (!didInit) {
        xmlInitParser();
        xmlRegisterInputCallbacks(matchFunc, openFunc, readFunc, closeFunc);
        xmlRegisterOutputCallbacks(matchFunc, openFunc, writeFunc, closeFunc);
        libxmlLoaderThread = currentThread();
        didInit = true;
    }

    // appendFragmentSource() checks that the length doesn't overflow an int.
    xmlParserCtxtPtr parser = xmlCreateMemoryParserCtxt(chunk.data(), chunk.length());

    if (!parser)
        return 0;

    memcpy(parser->sax, handlers, sizeof(xmlSAXHandler));

    // Substitute entities.
    // FIXME: Why is XML_PARSE_NODICT needed? This is different from what createStringParser does.
    xmlCtxtUseOptions(parser, XML_PARSE_NODICT | XML_PARSE_NOENT);

    // Internal initialization
    parser->sax2 = 1;
    parser->instate = XML_PARSER_CONTENT; // We are parsing a CONTENT
    parser->depth = 0;
    parser->str_xml = xmlDictLookup(parser->dict, BAD_CAST "xml", 3);
    parser->str_xmlns = xmlDictLookup(parser->dict, BAD_CAST "xmlns", 5);
    parser->str_xml_ns = xmlDictLookup(parser->dict, XML_XML_NAMESPACE, 36);
    parser->_private = userData;

    return adoptRef(new XMLParserContext(parser));
}

// --------------------------------

bool XMLDocumentParser::supportsXMLVersion(const String& version)
{
    return version == "1.0";
}

XMLDocumentParser::XMLDocumentParser(Document* document, FrameView* frameView)
    : ScriptableDocumentParser(document)
    , m_view(frameView)
    , m_context(0)
    , m_pendingCallbacks(PendingCallbacks::create())
    , m_depthTriggeringEntityExpansion(-1)
    , m_isParsingEntityDeclaration(false)
    , m_currentNode(document)
    , m_sawError(false)
    , m_sawCSS(false)
    , m_sawXSLTransform(false)
    , m_sawFirstElement(false)
    , m_isXHTMLDocument(false)
    , m_parserPaused(false)
    , m_requestingScript(false)
    , m_finishCalled(false)
    , m_xmlErrors(document)
    , m_pendingScript(0)
    , m_scriptStartPosition(TextPosition::belowRangePosition())
    , m_parsingFragment(false)
{
}

XMLDocumentParser::XMLDocumentParser(DocumentFragment* fragment, Element* parentElement, ParserContentPolicy parserContentPolicy)
    : ScriptableDocumentParser(fragment->document(), parserContentPolicy)
    , m_view(0)
    , m_context(0)
    , m_pendingCallbacks(PendingCallbacks::create())
    , m_depthTriggeringEntityExpansion(-1)
    , m_isParsingEntityDeclaration(false)
    , m_currentNode(fragment)
    , m_sawError(false)
    , m_sawCSS(false)
    , m_sawXSLTransform(false)
    , m_sawFirstElement(false)
    , m_isXHTMLDocument(false)
    , m_parserPaused(false)
    , m_requestingScript(false)
    , m_finishCalled(false)
    , m_xmlErrors(fragment->document())
    , m_pendingScript(0)
    , m_scriptStartPosition(TextPosition::belowRangePosition())
    , m_parsingFragment(true)
{
    fragment->ref();

    // Add namespaces based on the parent node
    Vector<Element*> elemStack;
    while (parentElement) {
        elemStack.append(parentElement);

        ContainerNode* n = parentElement->parentNode();
        if (!n || !n->isElementNode())
            break;
        parentElement = toElement(n);
    }

    if (elemStack.isEmpty())
        return;

    for (; !elemStack.isEmpty(); elemStack.removeLast()) {
        Element* element = elemStack.last();
        if (element->hasAttributes()) {
            for (unsigned i = 0; i < element->attributeCount(); i++) {
                const Attribute* attribute = element->attributeItem(i);
                if (attribute->localName() == xmlnsAtom)
                    m_defaultNamespaceURI = attribute->value();
                else if (attribute->prefix() == xmlnsAtom)
                    m_prefixToNamespaceMap.set(attribute->localName(), attribute->value());
            }
        }
    }

    // If the parent element is not in document tree, there may be no xmlns attribute; just default to the parent's namespace.
    if (m_defaultNamespaceURI.isNull() && !parentElement->inDocument())
        m_defaultNamespaceURI = parentElement->namespaceURI();
}

XMLParserContext::~XMLParserContext()
{
    if (m_context->myDoc)
        xmlFreeDoc(m_context->myDoc);
    xmlFreeParserCtxt(m_context);
}

XMLDocumentParser::~XMLDocumentParser()
{
    // The XMLDocumentParser will always be detached before being destroyed.
    ASSERT(m_currentNodeStack.isEmpty());
    ASSERT(!m_currentNode);

    // FIXME: m_pendingScript handling should be moved into XMLDocumentParser.cpp!
    if (m_pendingScript)
        m_pendingScript->removeClient(this);
}

void XMLDocumentParser::doWrite(const String& parseString)
{
    ASSERT(!isDetached());
    if (!m_context)
        initializeParserContext();

    // Protect the libxml context from deletion during a callback
    RefPtr<XMLParserContext> context = m_context;

    // libXML throws an error if you try to switch the encoding for an empty string.
    if (parseString.length()) {
        // JavaScript may cause the parser to detach during xmlParseChunk
        // keep this alive until this function is done.
        RefPtr<XMLDocumentParser> protect(this);

        switchToUTF16(context->context());
        XMLDocumentParserScope scope(document()->cachedResourceLoader());
        xmlParseChunk(context->context(), reinterpret_cast<const char*>(parseString.characters()), sizeof(UChar) * parseString.length(), 0);

        // JavaScript (which may be run under the xmlParseChunk callstack) may
        // cause the parser to be stopped or detached.
        if (isStopped())
            return;
    }

    // FIXME: Why is this here?  And why is it after we process the passed source?
    if (document()->decoder() && document()->decoder()->sawError()) {
        // If the decoder saw an error, report it as fatal (stops parsing)
        TextPosition position(OrdinalNumber::fromOneBasedInt(context->context()->input->line), OrdinalNumber::fromOneBasedInt(context->context()->input->col));
        handleError(XMLErrors::fatal, "Encoding error", position);
    }
}

static inline String toString(const xmlChar* string, size_t size)
{
    return String::fromUTF8(reinterpret_cast<const char*>(string), size);
}

static inline String toString(const xmlChar* string)
{
    return String::fromUTF8(reinterpret_cast<const char*>(string));
}

static inline AtomicString toAtomicString(const xmlChar* string, size_t size)
{
    return AtomicString::fromUTF8(reinterpret_cast<const char*>(string), size);
}

static inline AtomicString toAtomicString(const xmlChar* string)
{
    return AtomicString::fromUTF8(reinterpret_cast<const char*>(string));
}

struct _xmlSAX2Namespace {
    const xmlChar* prefix;
    const xmlChar* uri;
};
typedef struct _xmlSAX2Namespace xmlSAX2Namespace;

static inline void handleNamespaceAttributes(Vector<Attribute>& prefixedAttributes, const xmlChar** libxmlNamespaces, int nb_namespaces, ExceptionCode& ec)
{
    xmlSAX2Namespace* namespaces = reinterpret_cast<xmlSAX2Namespace*>(libxmlNamespaces);
    for (int i = 0; i < nb_namespaces; i++) {
        AtomicString namespaceQName = xmlnsAtom;
        AtomicString namespaceURI = toAtomicString(namespaces[i].uri);
        if (namespaces[i].prefix)
            namespaceQName = "xmlns:" + toString(namespaces[i].prefix);

        QualifiedName parsedName = anyName;
        if (!Element::parseAttributeName(parsedName, XMLNSNames::xmlnsNamespaceURI, namespaceQName, ec))
            return;
        
        prefixedAttributes.append(Attribute(parsedName, namespaceURI));
    }
}

struct _xmlSAX2Attributes {
    const xmlChar* localname;
    const xmlChar* prefix;
    const xmlChar* uri;
    const xmlChar* value;
    const xmlChar* end;
};
typedef struct _xmlSAX2Attributes xmlSAX2Attributes;

static inline void handleElementAttributes(Vector<Attribute>& prefixedAttributes, const xmlChar** libxmlAttributes, int nb_attributes, ExceptionCode& ec)
{
    xmlSAX2Attributes* attributes = reinterpret_cast<xmlSAX2Attributes*>(libxmlAttributes);
    for (int i = 0; i < nb_attributes; i++) {
        int valueLength = static_cast<int>(attributes[i].end - attributes[i].value);
        AtomicString attrValue = toAtomicString(attributes[i].value, valueLength);
        String attrPrefix = toString(attributes[i].prefix);
        AtomicString attrURI = attrPrefix.isEmpty() ? AtomicString() : toAtomicString(attributes[i].uri);
        AtomicString attrQName = attrPrefix.isEmpty() ? toAtomicString(attributes[i].localname) : attrPrefix + ":" + toString(attributes[i].localname);

        QualifiedName parsedName = anyName;
        if (!Element::parseAttributeName(parsedName, attrURI, attrQName, ec))
            return;

        prefixedAttributes.append(Attribute(parsedName, attrValue));
    }
}

// This is a hack around https://bugzilla.gnome.org/show_bug.cgi?id=502960
// Otherwise libxml doesn't include namespace for parsed entities, breaking entity
// expansion for all entities containing elements.
static inline bool hackAroundLibXMLEntityParsingBug()
{
#if LIBXML_VERSION >= 20704
    // This bug has been fixed in libxml 2.7.4.
    return false;
#else
    return true;
#endif
}

void XMLDocumentParser::startElementNs(const xmlChar* xmlLocalName, const xmlChar* xmlPrefix, const xmlChar* xmlURI, int nb_namespaces,
                                  const xmlChar** libxmlNamespaces, int nb_attributes, int nb_defaulted, const xmlChar** libxmlAttributes)
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendStartElementNSCallback(xmlLocalName, xmlPrefix, xmlURI, nb_namespaces, libxmlNamespaces,
                                                         nb_attributes, nb_defaulted, libxmlAttributes);
        return;
    }

    exitText();

    AtomicString localName = toAtomicString(xmlLocalName);
    AtomicString uri = toAtomicString(xmlURI);
    AtomicString prefix = toAtomicString(xmlPrefix);

    if (m_parsingFragment && uri.isNull()) {
        if (!prefix.isNull())
            uri = m_prefixToNamespaceMap.get(prefix);
        else
            uri = m_defaultNamespaceURI;
    }

    // If libxml entity parsing is broken, transfer the currentNodes' namespaceURI to the new node,
    // if we're currently expanding elements which originate from an entity declaration.
    if (hackAroundLibXMLEntityParsingBug() && depthTriggeringEntityExpansion() != -1 && context()->depth > depthTriggeringEntityExpansion() && uri.isNull() && prefix.isNull())
        uri = m_currentNode->namespaceURI();

    bool isFirstElement = !m_sawFirstElement;
    m_sawFirstElement = true;

    QualifiedName qName(prefix, localName, uri);
    RefPtr<Element> newElement = m_currentNode->document()->createElement(qName, true);
    if (!newElement) {
        stopParsing();
        return;
    }

    Vector<Attribute> prefixedAttributes;
    ExceptionCode ec = 0;
    handleNamespaceAttributes(prefixedAttributes, libxmlNamespaces, nb_namespaces, ec);
    if (ec) {
        setAttributes(newElement.get(), prefixedAttributes, parserContentPolicy());
        stopParsing();
        return;
    }

    handleElementAttributes(prefixedAttributes, libxmlAttributes, nb_attributes, ec);
    setAttributes(newElement.get(), prefixedAttributes, parserContentPolicy());
    if (ec) {
        stopParsing();
        return;
    }

    newElement->beginParsingChildren();

    ScriptElement* scriptElement = toScriptElementIfPossible(newElement.get());
    if (scriptElement)
        m_scriptStartPosition = textPosition();

    m_currentNode->parserAppendChild(newElement.get());

    const ContainerNode* currentNode = m_currentNode;
#if ENABLE(TEMPLATE_ELEMENT)
    if (newElement->hasTagName(HTMLNames::templateTag))
        pushCurrentNode(toHTMLTemplateElement(newElement.get())->content());
    else
        pushCurrentNode(newElement.get());
#else
    pushCurrentNode(newElement.get());
#endif

    if (m_view && currentNode->attached() && !newElement->attached())
        newElement->attach();

    if (newElement->hasTagName(HTMLNames::htmlTag))
        static_cast<HTMLHtmlElement*>(newElement.get())->insertedByParser();

    if (!m_parsingFragment && isFirstElement && document()->frame())
        document()->frame()->loader()->dispatchDocumentElementAvailable();
}

void XMLDocumentParser::endElementNs()
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendEndElementNSCallback();
        return;
    }

    // JavaScript can detach the parser.  Make sure this is not released
    // before the end of this method.
    RefPtr<XMLDocumentParser> protect(this);

    exitText();

    RefPtr<ContainerNode> n = m_currentNode;
    n->finishParsingChildren();

    // Once we reach the depth again where entity expansion started, stop executing the work-around.
    if (hackAroundLibXMLEntityParsingBug() && context()->depth <= depthTriggeringEntityExpansion())
        setDepthTriggeringEntityExpansion(-1);

    if (!scriptingContentIsAllowed(parserContentPolicy()) && n->isElementNode() && toScriptElementIfPossible(toElement(n.get()))) {
        popCurrentNode();
        n->remove(IGNORE_EXCEPTION);
        return;
    }

    if (!n->isElementNode() || !m_view) {
        popCurrentNode();
        return;
    }

    Element* element = toElement(n.get());

    // The element's parent may have already been removed from document.
    // Parsing continues in this case, but scripts aren't executed.
    if (!element->inDocument()) {
        popCurrentNode();
        return;
    }

    ScriptElement* scriptElement = toScriptElementIfPossible(element);
    if (!scriptElement) {
        popCurrentNode();
        return;
    }

    // Don't load external scripts for standalone documents (for now).
    ASSERT(!m_pendingScript);
    m_requestingScript = true;

    if (scriptElement->prepareScript(m_scriptStartPosition, ScriptElement::AllowLegacyTypeInTypeAttribute)) {
        // FIXME: Script execution should be shared between
        // the libxml2 and Qt XMLDocumentParser implementations.

        if (scriptElement->readyToBeParserExecuted())
            scriptElement->executeScript(ScriptSourceCode(scriptElement->scriptContent(), document()->url(), m_scriptStartPosition));
        else if (scriptElement->willBeParserExecuted()) {
            m_pendingScript = scriptElement->cachedScript();
            m_scriptElement = element;
            m_pendingScript->addClient(this);

            // m_pendingScript will be 0 if script was already loaded and addClient() executed it.
            if (m_pendingScript)
                pauseParsing();
        } else
            m_scriptElement = 0;

        // JavaScript may have detached the parser
        if (isDetached())
            return;
    }
    m_requestingScript = false;
    popCurrentNode();
}

void XMLDocumentParser::characters(const xmlChar* s, int len)
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendCharactersCallback(s, len);
        return;
    }

    if (!m_leafTextNode)
        enterText();
    m_bufferedText.append(s, len);
}

void XMLDocumentParser::error(XMLErrors::ErrorType type, const char* message, va_list args)
{
    if (isStopped())
        return;

#if HAVE(VASPRINTF)
    char* m;
    if (vasprintf(&m, message, args) == -1)
        return;
#else
    char m[1024];
    vsnprintf(m, sizeof(m) - 1, message, args);
#endif

    if (m_parserPaused)
        m_pendingCallbacks->appendErrorCallback(type, reinterpret_cast<const xmlChar*>(m), lineNumber(), columnNumber());
    else
        handleError(type, m, textPosition());

#if HAVE(VASPRINTF)
    free(m);
#endif
}

void XMLDocumentParser::processingInstruction(const xmlChar* target, const xmlChar* data)
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendProcessingInstructionCallback(target, data);
        return;
    }

    exitText();

    // ### handle exceptions
    ExceptionCode ec = 0;
    RefPtr<ProcessingInstruction> pi = m_currentNode->document()->createProcessingInstruction(
        toString(target), toString(data), ec);
    if (ec)
        return;

    pi->setCreatedByParser(true);

    m_currentNode->parserAppendChild(pi.get());
    if (m_view && !pi->attached())
        pi->attach();

    pi->finishParsingChildren();

    if (pi->isCSS())
        m_sawCSS = true;
#if ENABLE(XSLT)
    m_sawXSLTransform = !m_sawFirstElement && pi->isXSL();
    if (m_sawXSLTransform && !document()->transformSourceDocument())
        stopParsing();
#endif
}

void XMLDocumentParser::cdataBlock(const xmlChar* s, int len)
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendCDATABlockCallback(s, len);
        return;
    }

    exitText();

    RefPtr<CDATASection> newNode = CDATASection::create(m_currentNode->document(), toString(s, len));
    m_currentNode->parserAppendChild(newNode.get());
    if (m_view && !newNode->attached())
        newNode->attach();
}

void XMLDocumentParser::comment(const xmlChar* s)
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendCommentCallback(s);
        return;
    }

    exitText();

    RefPtr<Comment> newNode = Comment::create(m_currentNode->document(), toString(s));
    m_currentNode->parserAppendChild(newNode.get());
    if (m_view && !newNode->attached())
        newNode->attach();
}

enum StandaloneInfo {
    StandaloneUnspecified = -2,
    NoXMlDeclaration,
    StandaloneNo,
    StandaloneYes
};

void XMLDocumentParser::startDocument(const xmlChar* version, const xmlChar* encoding, int standalone)
{
    StandaloneInfo standaloneInfo = (StandaloneInfo)standalone;
    if (standaloneInfo == NoXMlDeclaration) {
        document()->setHasXMLDeclaration(false);
        return;
    }

    if (version)
        document()->setXMLVersion(toString(version), ASSERT_NO_EXCEPTION);
    if (standalone != StandaloneUnspecified)
        document()->setXMLStandalone(standaloneInfo == StandaloneYes, ASSERT_NO_EXCEPTION);
    if (encoding)
        document()->setXMLEncoding(toString(encoding));
    document()->setHasXMLDeclaration(true);
}

void XMLDocumentParser::endDocument()
{
    exitText();
}

void XMLDocumentParser::internalSubset(const xmlChar* name, const xmlChar* externalID, const xmlChar* systemID)
{
    if (isStopped())
        return;

    if (m_parserPaused) {
        m_pendingCallbacks->appendInternalSubsetCallback(name, externalID, systemID);
        return;
    }

    if (document())
        document()->parserAppendChild(DocumentType::create(document(), toString(name), toString(externalID), toString(systemID)));
}

static inline XMLDocumentParser* getParser(void* closure)
{
    xmlParserCtxtPtr ctxt = static_cast<xmlParserCtxtPtr>(closure);
    return static_cast<XMLDocumentParser*>(ctxt->_private);
}

// This is a hack around http://bugzilla.gnome.org/show_bug.cgi?id=159219
// Otherwise libxml seems to call all the SAX callbacks twice for any replaced entity.
static inline bool hackAroundLibXMLEntityBug(void* closure)
{
#if LIBXML_VERSION >= 20627
    UNUSED_PARAM(closure);

    // This bug has been fixed in libxml 2.6.27.
    return false;
#else
    return static_cast<xmlParserCtxtPtr>(closure)->node;
#endif
}

static void startElementNsHandler(void* closure, const xmlChar* localname, const xmlChar* prefix, const xmlChar* uri, int nb_namespaces, const xmlChar** namespaces, int nb_attributes, int nb_defaulted, const xmlChar** libxmlAttributes)
{
    if (hackAroundLibXMLEntityBug(closure))
        return;

    getParser(closure)->startElementNs(localname, prefix, uri, nb_namespaces, namespaces, nb_attributes, nb_defaulted, libxmlAttributes);
}

static void endElementNsHandler(void* closure, const xmlChar*, const xmlChar*, const xmlChar*)
{
    if (hackAroundLibXMLEntityBug(closure))
        return;

    getParser(closure)->endElementNs();
}

static void charactersHandler(void* closure, const xmlChar* s, int len)
{
    if (hackAroundLibXMLEntityBug(closure))
        return;

    getParser(closure)->characters(s, len);
}

static void processingInstructionHandler(void* closure, const xmlChar* target, const xmlChar* data)
{
    if (hackAroundLibXMLEntityBug(closure))
        return;

    getParser(closure)->processingInstruction(target, data);
}

static void cdataBlockHandler(void* closure, const xmlChar* s, int len)
{
    if (hackAroundLibXMLEntityBug(closure))
        return;

    getParser(closure)->cdataBlock(s, len);
}

static void commentHandler(void* closure, const xmlChar* comment)
{
    if (hackAroundLibXMLEntityBug(closure))
        return;

    getParser(closure)->comment(comment);
}

WTF_ATTRIBUTE_PRINTF(2, 3)
static void warningHandler(void* closure, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    getParser(closure)->error(XMLErrors::warning, message, args);
    va_end(args);
}

WTF_ATTRIBUTE_PRINTF(2, 3)
static void fatalErrorHandler(void* closure, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    getParser(closure)->error(XMLErrors::fatal, message, args);
    va_end(args);
}

WTF_ATTRIBUTE_PRINTF(2, 3)
static void normalErrorHandler(void* closure, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    getParser(closure)->error(XMLErrors::nonFatal, message, args);
    va_end(args);
}

// Using a static entity and marking it XML_INTERNAL_PREDEFINED_ENTITY is
// a hack to avoid malloc/free. Using a global variable like this could cause trouble
// if libxml implementation details were to change
static xmlChar sharedXHTMLEntityResult[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

static xmlEntityPtr sharedXHTMLEntity()
{
    static xmlEntity entity;
    if (!entity.type) {
        entity.type = XML_ENTITY_DECL;
        entity.orig = sharedXHTMLEntityResult;
        entity.content = sharedXHTMLEntityResult;
        entity.etype = XML_INTERNAL_PREDEFINED_ENTITY;
    }
    return &entity;
}

static size_t convertUTF16EntityToUTF8(const UChar* utf16Entity, size_t numberOfCodeUnits, char* target, size_t targetSize)
{
    const char* originalTarget = target;
    WTF::Unicode::ConversionResult conversionResult = WTF::Unicode::convertUTF16ToUTF8(&utf16Entity,
        utf16Entity + numberOfCodeUnits, &target, target + targetSize);
    if (conversionResult != WTF::Unicode::conversionOK)
        return 0;

    // Even though we must pass the length, libxml expects the entity string to be null terminated.
    ASSERT(target > originalTarget + 1);
    *target = '\0';
    return target - originalTarget;
}

static xmlEntityPtr getXHTMLEntity(const xmlChar* name)
{
    UChar utf16DecodedEntity[4];
    size_t numberOfCodeUnits = decodeNamedEntityToUCharArray(reinterpret_cast<const char*>(name), utf16DecodedEntity);
    if (!numberOfCodeUnits)
        return 0;

    ASSERT(numberOfCodeUnits <= 4);
    size_t entityLengthInUTF8 = convertUTF16EntityToUTF8(utf16DecodedEntity, numberOfCodeUnits,
        reinterpret_cast<char*>(sharedXHTMLEntityResult), WTF_ARRAY_LENGTH(sharedXHTMLEntityResult));
    if (!entityLengthInUTF8)
        return 0;

    xmlEntityPtr entity = sharedXHTMLEntity();
    entity->length = entityLengthInUTF8;
    entity->name = name;
    return entity;
}

static void entityDeclarationHandler(void* closure, const xmlChar* name, int type, const xmlChar* publicId, const xmlChar* systemId, xmlChar* content)
{
    // Prevent the next call to getEntityHandler() to record the entity expansion depth.
    // We're parsing the entity declaration, so there's no need to record anything.
    // We only need to record the depth, if we're actually expanding the entity, when it's referenced.
    if (hackAroundLibXMLEntityParsingBug())
        getParser(closure)->setIsParsingEntityDeclaration(true);
    xmlSAX2EntityDecl(closure, name, type, publicId, systemId, content);
}

static xmlEntityPtr getEntityHandler(void* closure, const xmlChar* name)
{
    xmlParserCtxtPtr ctxt = static_cast<xmlParserCtxtPtr>(closure);

    XMLDocumentParser* parser = getParser(closure);
    if (hackAroundLibXMLEntityParsingBug()) {
        if (parser->isParsingEntityDeclaration()) {
            // We're parsing the entity declarations (not an entity reference), no need to do anything special.
            parser->setIsParsingEntityDeclaration(false);
            ASSERT(parser->depthTriggeringEntityExpansion() == -1);
        } else {
            // The entity will be used and eventually expanded. Record the current parser depth
            // so the next call to startElementNs() knows that the new element originates from
            // an entity declaration.
            parser->setDepthTriggeringEntityExpansion(ctxt->depth);
        }
    }

    xmlEntityPtr ent = xmlGetPredefinedEntity(name);
    if (ent) {
        ent->etype = XML_INTERNAL_PREDEFINED_ENTITY;
        return ent;
    }

    ent = xmlGetDocEntity(ctxt->myDoc, name);
    if (!ent && parser->isXHTMLDocument()) {
        ent = getXHTMLEntity(name);
        if (ent)
            ent->etype = XML_INTERNAL_GENERAL_ENTITY;
    }

    return ent;
}

static void startDocumentHandler(void* closure)
{
    xmlParserCtxt* ctxt = static_cast<xmlParserCtxt*>(closure);
    switchToUTF16(ctxt);
    getParser(closure)->startDocument(ctxt->version, ctxt->encoding, ctxt->standalone);
    xmlSAX2StartDocument(closure);
}

static void endDocumentHandler(void* closure)
{
    getParser(closure)->endDocument();
    xmlSAX2EndDocument(closure);
}

static void internalSubsetHandler(void* closure, const xmlChar* name, const xmlChar* externalID, const xmlChar* systemID)
{
    getParser(closure)->internalSubset(name, externalID, systemID);
    xmlSAX2InternalSubset(closure, name, externalID, systemID);
}

static void externalSubsetHandler(void* closure, const xmlChar*, const xmlChar* externalId, const xmlChar*)
{
    String extId = toString(externalId);
    if ((extId == "-//W3C//DTD XHTML 1.0 Transitional//EN")
        || (extId == "-//W3C//DTD XHTML 1.1//EN")
        || (extId == "-//W3C//DTD XHTML 1.0 Strict//EN")
        || (extId == "-//W3C//DTD XHTML 1.0 Frameset//EN")
        || (extId == "-//W3C//DTD XHTML Basic 1.0//EN")
        || (extId == "-//W3C//DTD XHTML 1.1 plus MathML 2.0//EN")
        || (extId == "-//W3C//DTD XHTML 1.1 plus MathML 2.0 plus SVG 1.1//EN")
        || (extId == "-//WAPFORUM//DTD XHTML Mobile 1.0//EN")
        || (extId == "-//WAPFORUM//DTD XHTML Mobile 1.1//EN")
        || (extId == "-//WAPFORUM//DTD XHTML Mobile 1.2//EN"))
        getParser(closure)->setIsXHTMLDocument(true); // controls if we replace entities or not.
}

static void ignorableWhitespaceHandler(void*, const xmlChar*, int)
{
    // nothing to do, but we need this to work around a crasher
    // http://bugzilla.gnome.org/show_bug.cgi?id=172255
    // http://bugs.webkit.org/show_bug.cgi?id=5792
}

void XMLDocumentParser::initializeParserContext(const CString& chunk)
{
    xmlSAXHandler sax;
    memset(&sax, 0, sizeof(sax));

    sax.error = normalErrorHandler;
    sax.fatalError = fatalErrorHandler;
    sax.characters = charactersHandler;
    sax.processingInstruction = processingInstructionHandler;
    sax.cdataBlock = cdataBlockHandler;
    sax.comment = commentHandler;
    sax.warning = warningHandler;
    sax.startElementNs = startElementNsHandler;
    sax.endElementNs = endElementNsHandler;
    sax.getEntity = getEntityHandler;
    sax.startDocument = startDocumentHandler;
    sax.endDocument = endDocumentHandler;
    sax.internalSubset = internalSubsetHandler;
    sax.externalSubset = externalSubsetHandler;
    sax.ignorableWhitespace = ignorableWhitespaceHandler;
    sax.entityDecl = entityDeclarationHandler;
    sax.initialized = XML_SAX2_MAGIC;
    DocumentParser::startParsing();
    m_sawError = false;
    m_sawCSS = false;
    m_sawXSLTransform = false;
    m_sawFirstElement = false;

    XMLDocumentParserScope scope(document()->cachedResourceLoader());
    if (m_parsingFragment)
        m_context = XMLParserContext::createMemoryParser(&sax, this, chunk);
    else {
        ASSERT(!chunk.data());
        m_context = XMLParserContext::createStringParser(&sax, this);
    }
}

void XMLDocumentParser::doEnd()
{
    if (!isStopped()) {
        if (m_context) {
            // Tell libxml we're done.
            {
                XMLDocumentParserScope scope(document()->cachedResourceLoader());
                xmlParseChunk(context(), 0, 0, 1);
            }

            m_context = 0;
        }
    }

#if ENABLE(XSLT)
    XMLTreeViewer xmlTreeViewer(document());
    bool xmlViewerMode = !m_sawError && !m_sawCSS && !m_sawXSLTransform && xmlTreeViewer.hasNoStyleInformation();
    if (xmlViewerMode)
        xmlTreeViewer.transformDocumentToTreeView();

    if (m_sawXSLTransform) {
        void* doc = xmlDocPtrForString(document()->cachedResourceLoader(), m_originalSourceForTransform.toString(), document()->url().string());
        document()->setTransformSource(adoptPtr(new TransformSource(doc)));

        document()->setParsing(false); // Make the document think it's done, so it will apply XSL stylesheets.
        document()->styleResolverChanged(RecalcStyleImmediately);

        // styleResolverChanged() call can detach the parser and null out its document.
        // In that case, we just bail out.
        if (isDetached())
            return;

        document()->setParsing(true);
        DocumentParser::stopParsing();
    }
#endif
}

#if ENABLE(XSLT)
static inline const char* nativeEndianUTF16Encoding()
{
    const UChar BOM = 0xFEFF;
    const unsigned char BOMHighByte = *reinterpret_cast<const unsigned char*>(&BOM);
    return BOMHighByte == 0xFF ? "UTF-16LE" : "UTF-16BE";
}

void* xmlDocPtrForString(CachedResourceLoader* cachedResourceLoader, const String& source, const String& url)
{
    if (source.isEmpty())
        return 0;

    // Parse in a single chunk into an xmlDocPtr
    // FIXME: Hook up error handlers so that a failure to parse the main document results in
    // good error messages.

    const bool is8Bit = source.is8Bit();
    const char* characters = is8Bit ? reinterpret_cast<const char*>(source.characters8()) : reinterpret_cast<const char*>(source.characters16());
    size_t sizeInBytes = source.length() * (is8Bit ? sizeof(LChar) : sizeof(UChar));
    const char* encoding = is8Bit ? "iso-8859-1" : nativeEndianUTF16Encoding();

    XMLDocumentParserScope scope(cachedResourceLoader, errorFunc, 0);
    return xmlReadMemory(characters, sizeInBytes, url.latin1().data(), encoding, XSLT_PARSE_OPTIONS);
}
#endif

OrdinalNumber XMLDocumentParser::lineNumber() const
{
    return OrdinalNumber::fromOneBasedInt(context() ? context()->input->line : 1);
}

OrdinalNumber XMLDocumentParser::columnNumber() const
{
    return OrdinalNumber::fromOneBasedInt(context() ? context()->input->col : 1);
}

TextPosition XMLDocumentParser::textPosition() const
{
    xmlParserCtxtPtr context = this->context();
    if (!context)
        return TextPosition::minimumPosition();
    return TextPosition(OrdinalNumber::fromOneBasedInt(context->input->line),
                        OrdinalNumber::fromOneBasedInt(context->input->col));
}

void XMLDocumentParser::stopParsing()
{
    DocumentParser::stopParsing();
    if (context())
        xmlStopParser(context());
}

void XMLDocumentParser::resumeParsing()
{
    ASSERT(!isDetached());
    ASSERT(m_parserPaused);

    m_parserPaused = false;

    // First, execute any pending callbacks
    while (!m_pendingCallbacks->isEmpty()) {
        m_pendingCallbacks->callAndRemoveFirstCallback(this);

        // A callback paused the parser
        if (m_parserPaused)
            return;
    }

    // Then, write any pending data
    SegmentedString rest = m_pendingSrc;
    m_pendingSrc.clear();
    // There is normally only one string left, so toString() shouldn't copy.
    // In any case, the XML parser runs on the main thread and it's OK if
    // the passed string has more than one reference.
    append(rest.toString().impl());

    // Finally, if finish() has been called and write() didn't result
    // in any further callbacks being queued, call end()
    if (m_finishCalled && m_pendingCallbacks->isEmpty())
        end();
}

bool XMLDocumentParser::appendFragmentSource(const String& chunk)
{
    ASSERT(!m_context);
    ASSERT(m_parsingFragment);

    CString chunkAsUtf8 = chunk.utf8();
    
    // libxml2 takes an int for a length, and therefore can't handle XML chunks larger than 2 GiB.
    if (chunkAsUtf8.length() > INT_MAX)
        return false;

    initializeParserContext(chunkAsUtf8);
    xmlParseContent(context());
    endDocument(); // Close any open text nodes.

    // FIXME: If this code is actually needed, it should probably move to finish()
    // XMLDocumentParserQt has a similar check (m_stream.error() == QXmlStreamReader::PrematureEndOfDocumentError) in doEnd().
    // Check if all the chunk has been processed.
    long bytesProcessed = xmlByteConsumed(context());
    if (bytesProcessed == -1 || ((unsigned long)bytesProcessed) != chunkAsUtf8.length()) {
        // FIXME: I don't believe we can hit this case without also having seen an error or a null byte.
        // If we hit this ASSERT, we've found a test case which demonstrates the need for this code.
        ASSERT(m_sawError || (bytesProcessed >= 0 && !chunkAsUtf8.data()[bytesProcessed]));
        return false;
    }

    // No error if the chunk is well formed or it is not but we have no error.
    return context()->wellFormed || !xmlCtxtGetLastError(context());
}

// --------------------------------

struct AttributeParseState {
    HashMap<String, String> attributes;
    bool gotAttributes;
};

static void attributesStartElementNsHandler(void* closure, const xmlChar* xmlLocalName, const xmlChar* /*xmlPrefix*/,
                                            const xmlChar* /*xmlURI*/, int /*nb_namespaces*/, const xmlChar** /*namespaces*/,
                                            int nb_attributes, int /*nb_defaulted*/, const xmlChar** libxmlAttributes)
{
    if (strcmp(reinterpret_cast<const char*>(xmlLocalName), "attrs") != 0)
        return;

    xmlParserCtxtPtr ctxt = static_cast<xmlParserCtxtPtr>(closure);
    AttributeParseState* state = static_cast<AttributeParseState*>(ctxt->_private);

    state->gotAttributes = true;

    xmlSAX2Attributes* attributes = reinterpret_cast<xmlSAX2Attributes*>(libxmlAttributes);
    for (int i = 0; i < nb_attributes; i++) {
        String attrLocalName = toString(attributes[i].localname);
        int valueLength = (int) (attributes[i].end - attributes[i].value);
        String attrValue = toString(attributes[i].value, valueLength);
        String attrPrefix = toString(attributes[i].prefix);
        String attrQName = attrPrefix.isEmpty() ? attrLocalName : attrPrefix + ":" + attrLocalName;

        state->attributes.set(attrQName, attrValue);
    }
}

HashMap<String, String> parseAttributes(const String& string, bool& attrsOK)
{
    AttributeParseState state;
    state.gotAttributes = false;

    xmlSAXHandler sax;
    memset(&sax, 0, sizeof(sax));
    sax.startElementNs = attributesStartElementNsHandler;
    sax.initialized = XML_SAX2_MAGIC;
    RefPtr<XMLParserContext> parser = XMLParserContext::createStringParser(&sax, &state);
    String parseString = "<?xml version=\"1.0\"?><attrs " + string + " />";
    xmlParseChunk(parser->context(), reinterpret_cast<const char*>(parseString.characters()), parseString.length() * sizeof(UChar), 1);
    attrsOK = state.gotAttributes;
    return state.attributes;
}

}
