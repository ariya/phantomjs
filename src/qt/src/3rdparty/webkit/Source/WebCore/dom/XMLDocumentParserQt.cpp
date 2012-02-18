/*
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLEntityParser.h"
#include "HTMLHtmlElement.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "ProcessingInstruction.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptableDocumentParser.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "TextResourceDecoder.h"
#include "TransformSource.h"
#include <QDebug>
#include <wtf/StringExtras.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

#if ENABLE(XHTMLMP)
#include "HTMLNames.h"
#include "HTMLScriptElement.h"
#endif

using namespace std;

namespace WebCore {

class EntityResolver : public QXmlStreamEntityResolver {
    virtual QString resolveUndeclaredEntity(const QString &name);
};

QString EntityResolver::resolveUndeclaredEntity(const QString &name)
{
    UChar c = decodeNamedEntity(name.toUtf8().constData());
    return QString(c);
}

// --------------------------------

bool XMLDocumentParser::supportsXMLVersion(const String& version)
{
    return version == "1.0";
}

XMLDocumentParser::XMLDocumentParser(Document* document, FrameView* frameView)
    : ScriptableDocumentParser(document)
    , m_view(frameView)
    , m_wroteText(false)
    , m_currentNode(document)
    , m_sawError(false)
    , m_sawCSS(false)
    , m_sawXSLTransform(false)
    , m_sawFirstElement(false)
    , m_isXHTMLDocument(false)
#if ENABLE(XHTMLMP)
    , m_isXHTMLMPDocument(false)
    , m_hasDocTypeDeclaration(false)
#endif
    , m_parserPaused(false)
    , m_requestingScript(false)
    , m_finishCalled(false)
    , m_errorCount(0)
    , m_lastErrorPosition(TextPosition1::belowRangePosition())
    , m_pendingScript(0)
    , m_scriptStartPosition(TextPosition1::belowRangePosition())
    , m_parsingFragment(false)
    , m_scriptingPermission(FragmentScriptingAllowed)
{
    m_stream.setEntityResolver(new EntityResolver);
}

XMLDocumentParser::XMLDocumentParser(DocumentFragment* fragment, Element* parentElement, FragmentScriptingPermission permission)
    : ScriptableDocumentParser(fragment->document())
    , m_view(0)
    , m_wroteText(false)
    , m_currentNode(fragment)
    , m_sawError(false)
    , m_sawCSS(false)
    , m_sawXSLTransform(false)
    , m_sawFirstElement(false)
    , m_isXHTMLDocument(false)
#if ENABLE(XHTMLMP)
    , m_isXHTMLMPDocument(false)
    , m_hasDocTypeDeclaration(false)
#endif
    , m_parserPaused(false)
    , m_requestingScript(false)
    , m_finishCalled(false)
    , m_errorCount(0)
    , m_lastErrorPosition(TextPosition1::belowRangePosition())
    , m_pendingScript(0)
    , m_scriptStartPosition(TextPosition1::belowRangePosition())
    , m_parsingFragment(true)
    , m_scriptingPermission(permission)
{
    fragment->ref();

    // Add namespaces based on the parent node
    Vector<Element*> elemStack;
    while (parentElement) {
        elemStack.append(parentElement);

        Node* n = parentElement->parentNode();
        if (!n || !n->isElementNode())
            break;
        parentElement = static_cast<Element*>(n);
    }

    if (elemStack.isEmpty())
        return;

    QXmlStreamNamespaceDeclarations namespaces;
    for (Element* element = elemStack.last(); !elemStack.isEmpty(); elemStack.removeLast()) {
        if (NamedNodeMap* attrs = element->attributes()) {
            for (unsigned i = 0; i < attrs->length(); i++) {
                Attribute* attr = attrs->attributeItem(i);
                if (attr->localName() == "xmlns")
                    m_defaultNamespaceURI = attr->value();
                else if (attr->prefix() == "xmlns")
                    namespaces.append(QXmlStreamNamespaceDeclaration(attr->localName(), attr->value()));
            }
        }
    }
    m_stream.addExtraNamespaceDeclarations(namespaces);
    m_stream.setEntityResolver(new EntityResolver);

    // If the parent element is not in document tree, there may be no xmlns attribute; just default to the parent's namespace.
    if (m_defaultNamespaceURI.isNull() && !parentElement->inDocument())
        m_defaultNamespaceURI = parentElement->namespaceURI();
}

XMLDocumentParser::~XMLDocumentParser()
{
    clearCurrentNodeStack();
    if (m_pendingScript)
        m_pendingScript->removeClient(this);
    delete m_stream.entityResolver();
}

void XMLDocumentParser::doWrite(const String& parseString)
{
    m_wroteText = true;

    if (document()->decoder() && document()->decoder()->sawError()) {
        // If the decoder saw an error, report it as fatal (stops parsing)
        handleError(fatal, "Encoding error", lineNumber(), columnNumber());
        return;
    }

    QString data(parseString);
    if (!data.isEmpty()) {
        // JavaScript may cause the parser to detach,
        // keep this alive until this function is done.
        RefPtr<XMLDocumentParser> protect(this);

        m_stream.addData(data);
        parse();
    }

    return;
}

void XMLDocumentParser::initializeParserContext(const CString&)
{
    DocumentParser::startParsing();
    m_sawError = false;
    m_sawCSS = false;
    m_sawXSLTransform = false;
    m_sawFirstElement = false;
}

void XMLDocumentParser::doEnd()
{
#if ENABLE(XSLT)
    if (m_sawXSLTransform) {
        document()->setTransformSource(new TransformSource(m_originalSourceForTransform));
        document()->setParsing(false); // Make the doc think it's done, so it will apply xsl sheets.
        document()->styleSelectorChanged(RecalcStyleImmediately);
        document()->setParsing(true);
        DocumentParser::stopParsing();
    }
#endif

    if (m_stream.error() == QXmlStreamReader::PrematureEndOfDocumentError
        || (m_wroteText && !m_sawFirstElement && !m_sawXSLTransform && !m_sawError))
        handleError(fatal, qPrintable(m_stream.errorString()), lineNumber(), columnNumber());
}

int XMLDocumentParser::lineNumber() const
{
    return m_stream.lineNumber();
}

int XMLDocumentParser::columnNumber() const
{
    return m_stream.columnNumber();
}

TextPosition0 XMLDocumentParser::textPosition() const
{
    return TextPosition0(WTF::ZeroBasedNumber::fromZeroBasedInt(lineNumber()), WTF::ZeroBasedNumber::fromZeroBasedInt(columnNumber()));
}

// This method incorrectly reinterprets zero-base lineNumber method as one-based number.
// FIXME: This error is kept for compatibility. We should fix it eventually. 
TextPosition1 XMLDocumentParser::textPositionOneBased() const
{
    return TextPosition1(WTF::OneBasedNumber::fromOneBasedInt(lineNumber()), WTF::OneBasedNumber::fromOneBasedInt(columnNumber()));
}

void XMLDocumentParser::stopParsing()
{
    ScriptableDocumentParser::stopParsing();
}

void XMLDocumentParser::resumeParsing()
{
    ASSERT(m_parserPaused);

    m_parserPaused = false;

    // First, execute any pending callbacks
    parse();
    if (m_parserPaused)
        return;

    // Then, write any pending data
    SegmentedString rest = m_pendingSrc;
    m_pendingSrc.clear();
    append(rest);

    // Finally, if finish() has been called and append() didn't result
    // in any further callbacks being queued, call end()
    if (m_finishCalled && !m_parserPaused && !m_pendingScript)
        end();
}

bool XMLDocumentParser::appendFragmentSource(const String& source)
{
    ASSERT(!m_sawFirstElement);
    append(String("<qxmlstreamdummyelement>"));
    append(source);
    append(String("</qxmlstreamdummyelement>"));
    return !hasError();
}

// --------------------------------

struct AttributeParseState {
    HashMap<String, String> attributes;
    bool gotAttributes;
};

static void attributesStartElementNsHandler(AttributeParseState* state, const QXmlStreamAttributes& attrs)
{
    if (attrs.count() <= 0)
        return;

    state->gotAttributes = true;

    for (int i = 0; i < attrs.count(); i++) {
        const QXmlStreamAttribute& attr = attrs[i];
        String attrLocalName = attr.name();
        String attrValue     = attr.value();
        String attrURI       = attr.namespaceUri();
        String attrQName     = attr.qualifiedName();
        state->attributes.set(attrQName, attrValue);
    }
}

HashMap<String, String> parseAttributes(const String& string, bool& attrsOK)
{
    AttributeParseState state;
    state.gotAttributes = false;

    QXmlStreamReader stream;
    QString dummy = QString(QLatin1String("<?xml version=\"1.0\"?><attrs %1 />")).arg(string);
    stream.addData(dummy);
    while (!stream.atEnd()) {
        stream.readNext();
        if (stream.isStartElement()) {
            attributesStartElementNsHandler(&state, stream.attributes());
        }
    }
    attrsOK = state.gotAttributes;
    return state.attributes;
}

static inline String prefixFromQName(const QString& qName)
{
    const int offset = qName.indexOf(QLatin1Char(':'));
    if (offset <= 0)
        return String();
    else
        return qName.left(offset);
}

static inline void handleElementNamespaces(Element* newElement, const QXmlStreamNamespaceDeclarations &ns,
                                           ExceptionCode& ec, FragmentScriptingPermission scriptingPermission)
{
    for (int i = 0; i < ns.count(); ++i) {
        const QXmlStreamNamespaceDeclaration &decl = ns[i];
        String namespaceURI = decl.namespaceUri();
        String namespaceQName = decl.prefix().isEmpty() ? String("xmlns") : String("xmlns:");
        namespaceQName.append(decl.prefix());
        newElement->setAttributeNS("http://www.w3.org/2000/xmlns/", namespaceQName, namespaceURI, ec, scriptingPermission);
        if (ec) // exception setting attributes
            return;
    }
}

static inline void handleElementAttributes(Element* newElement, const QXmlStreamAttributes &attrs, ExceptionCode& ec,
                                           FragmentScriptingPermission scriptingPermission)
{
    for (int i = 0; i < attrs.count(); ++i) {
        const QXmlStreamAttribute &attr = attrs[i];
        String attrLocalName = attr.name();
        String attrValue     = attr.value();
        String attrURI       = attr.namespaceUri().isEmpty() ? String() : String(attr.namespaceUri());
        String attrQName     = attr.qualifiedName();
        newElement->setAttributeNS(attrURI, attrQName, attrValue, ec, scriptingPermission);
        if (ec) // exception setting attributes
            return;
    }
}

void XMLDocumentParser::parse()
{
    while (!isStopped() && !m_parserPaused && !m_stream.atEnd()) {
        m_stream.readNext();
        switch (m_stream.tokenType()) {
        case QXmlStreamReader::StartDocument: {
            startDocument();
        }
            break;
        case QXmlStreamReader::EndDocument: {
            endDocument();
        }
            break;
        case QXmlStreamReader::StartElement: {
#if ENABLE(XHTMLMP)
            if (document()->isXHTMLMPDocument() && !m_hasDocTypeDeclaration) {
                handleError(fatal, "DOCTYPE declaration lost.", lineNumber(), columnNumber());
                break;
            }
#endif
            parseStartElement();
        }
            break;
        case QXmlStreamReader::EndElement: {
            parseEndElement();
        }
            break;
        case QXmlStreamReader::Characters: {
            if (m_stream.isCDATA()) {
                //cdata
                parseCdata();
            } else {
                //characters
                parseCharacters();
            }
        }
            break;
        case QXmlStreamReader::Comment: {
            parseComment();
        }
            break;
        case QXmlStreamReader::DTD: {
            //qDebug()<<"------------- DTD";
            parseDtd();
#if ENABLE(XHTMLMP)
            m_hasDocTypeDeclaration = true;
#endif
        }
            break;
        case QXmlStreamReader::EntityReference: {
            //qDebug()<<"---------- ENTITY = "<<m_stream.name().toString()
            //        <<", t = "<<m_stream.text().toString();
            if (isXHTMLDocument()
#if ENABLE(XHTMLMP)
                || isXHTMLMPDocument()
#endif
               ) {
                QString entity = m_stream.name().toString();
                UChar c = decodeNamedEntity(entity.toUtf8().constData());
                if (!m_currentNode->isTextNode())
                    enterText();
                ExceptionCode ec = 0;
                String str(&c, 1);
                // qDebug()<<" ------- adding entity "<<str;
                static_cast<Text*>(m_currentNode)->appendData(str, ec);
            }
        }
            break;
        case QXmlStreamReader::ProcessingInstruction: {
            parseProcessingInstruction();
        }
            break;
        default: {
            if (m_stream.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
                ErrorType type = (m_stream.error() == QXmlStreamReader::NotWellFormedError) ?
                                 fatal : warning;
                handleError(type, qPrintable(m_stream.errorString()), lineNumber(),
                            columnNumber());
            }
        }
            break;
        }
    }
}

void XMLDocumentParser::startDocument()
{
    initializeParserContext();
    ExceptionCode ec = 0;

    if (!m_parsingFragment) {
        document()->setXMLStandalone(m_stream.isStandaloneDocument(), ec);

        QStringRef version = m_stream.documentVersion();
        if (!version.isEmpty())
            document()->setXMLVersion(version, ec);
        QStringRef encoding = m_stream.documentEncoding();
        if (!encoding.isEmpty())
            document()->setXMLEncoding(encoding);
    }
}

void XMLDocumentParser::parseStartElement()
{
    if (!m_sawFirstElement && m_parsingFragment) {
        // skip dummy element for fragments
        m_sawFirstElement = true;
        return;
    }

    exitText();

    String localName = m_stream.name();
    String uri       = m_stream.namespaceUri();
    String prefix    = prefixFromQName(m_stream.qualifiedName().toString());

    if (m_parsingFragment && uri.isNull()) {
        Q_ASSERT(prefix.isNull());
        uri = m_defaultNamespaceURI;
    }

    QualifiedName qName(prefix, localName, uri);
    RefPtr<Element> newElement = document()->createElement(qName, true);
    if (!newElement) {
        stopParsing();
        return;
    }

#if ENABLE(XHTMLMP)
    if (!m_sawFirstElement && isXHTMLMPDocument()) {
        // As per 7.1 section of OMA-WAP-XHTMLMP-V1_1-20061020-A.pdf,
        // we should make sure that the root element MUST be 'html' and
        // ensure the name of the default namespace on the root elment 'html'
        // MUST be 'http://www.w3.org/1999/xhtml'
        if (localName != HTMLNames::htmlTag.localName()) {
            handleError(fatal, "XHTMLMP document expects 'html' as root element.", lineNumber(), columnNumber());
            return;
        }

        if (uri.isNull()) {
            m_defaultNamespaceURI = HTMLNames::xhtmlNamespaceURI;
            uri = m_defaultNamespaceURI;
            m_stream.addExtraNamespaceDeclaration(QXmlStreamNamespaceDeclaration(prefix, HTMLNames::xhtmlNamespaceURI));
        }
    }
#endif

    bool isFirstElement = !m_sawFirstElement;
    m_sawFirstElement = true;

    ExceptionCode ec = 0;
    handleElementNamespaces(newElement.get(), m_stream.namespaceDeclarations(), ec, m_scriptingPermission);
    if (ec) {
        stopParsing();
        return;
    }

    handleElementAttributes(newElement.get(), m_stream.attributes(), ec, m_scriptingPermission);
    if (ec) {
        stopParsing();
        return;
    }

    ScriptElement* scriptElement = toScriptElement(newElement.get());
    if (scriptElement)
        m_scriptStartPosition = textPositionOneBased();

    m_currentNode->deprecatedParserAddChild(newElement.get());

    pushCurrentNode(newElement.get());
    if (m_view && !newElement->attached())
        newElement->attach();

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    if (newElement->hasTagName(HTMLNames::htmlTag))
        static_cast<HTMLHtmlElement*>(newElement.get())->insertedByParser();
#endif

    if (isFirstElement && document()->frame())
        document()->frame()->loader()->dispatchDocumentElementAvailable();
}

void XMLDocumentParser::parseEndElement()
{
    exitText();

    RefPtr<Node> n = m_currentNode;
    n->finishParsingChildren();

    if (m_scriptingPermission == FragmentScriptingNotAllowed && n->isElementNode() && toScriptElement(static_cast<Element*>(n.get()))) {
        popCurrentNode();
        ExceptionCode ec;
        n->remove(ec);
        return;
    }

    if (!n->isElementNode() || !m_view) {
        if (!m_currentNodeStack.isEmpty())
            popCurrentNode();
        return;
    }

    Element* element = static_cast<Element*>(n.get());

    // The element's parent may have already been removed from document.
    // Parsing continues in this case, but scripts aren't executed.
    if (!element->inDocument()) {
        popCurrentNode();
        return;
    }

    ScriptElement* scriptElement = toScriptElement(element);
    if (!scriptElement) {
        popCurrentNode();
        return;
    }

    // don't load external scripts for standalone documents (for now)
    ASSERT(!m_pendingScript);
    m_requestingScript = true;

    bool successfullyPrepared = scriptElement->prepareScript(m_scriptStartPosition, ScriptElement::AllowLegacyTypeInTypeAttribute);
    if (!successfullyPrepared) {
#if ENABLE(XHTMLMP)
        if (!scriptElement->isScriptTypeSupported(ScriptElement::AllowLegacyTypeInTypeAttribute))
            document()->setShouldProcessNoscriptElement(true);
#endif
    } else {
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
    }
    m_requestingScript = false;
    popCurrentNode();
}

void XMLDocumentParser::parseCharacters()
{
    if (!m_currentNode->isTextNode())
        enterText();
    ExceptionCode ec = 0;
    static_cast<Text*>(m_currentNode)->appendData(m_stream.text(), ec);
}

void XMLDocumentParser::parseProcessingInstruction()
{
    exitText();

    // ### handle exceptions
    int exception = 0;
    RefPtr<ProcessingInstruction> pi = document()->createProcessingInstruction(
        m_stream.processingInstructionTarget(),
        m_stream.processingInstructionData(), exception);
    if (exception)
        return;

    pi->setCreatedByParser(true);

    m_currentNode->deprecatedParserAddChild(pi.get());
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

void XMLDocumentParser::parseCdata()
{
    exitText();

    RefPtr<Node> newNode = CDATASection::create(document(), m_stream.text());

    m_currentNode->deprecatedParserAddChild(newNode.get());
    if (m_view && !newNode->attached())
        newNode->attach();
}

void XMLDocumentParser::parseComment()
{
    exitText();

    RefPtr<Node> newNode = Comment::create(document(), m_stream.text());

    m_currentNode->deprecatedParserAddChild(newNode.get());
    if (m_view && !newNode->attached())
        newNode->attach();
}

void XMLDocumentParser::endDocument()
{
#if ENABLE(XHTMLMP)
    m_hasDocTypeDeclaration = false;
#endif
}

bool XMLDocumentParser::hasError() const
{
    return m_stream.hasError();
}

void XMLDocumentParser::parseDtd()
{
    QStringRef name = m_stream.dtdName();
    QStringRef publicId = m_stream.dtdPublicId();
    QStringRef systemId = m_stream.dtdSystemId();

    //qDebug() << dtd << name << publicId << systemId;
    if ((publicId == QLatin1String("-//W3C//DTD XHTML 1.0 Transitional//EN"))
        || (publicId == QLatin1String("-//W3C//DTD XHTML 1.1//EN"))
        || (publicId == QLatin1String("-//W3C//DTD XHTML 1.0 Strict//EN"))
        || (publicId == QLatin1String("-//W3C//DTD XHTML 1.0 Frameset//EN"))
        || (publicId == QLatin1String("-//W3C//DTD XHTML Basic 1.0//EN"))
        || (publicId == QLatin1String("-//W3C//DTD XHTML 1.1 plus MathML 2.0//EN"))
        || (publicId == QLatin1String("-//W3C//DTD XHTML 1.1 plus MathML 2.0 plus SVG 1.1//EN"))
#if !ENABLE(XHTMLMP)
        || (publicId == QLatin1String("-//WAPFORUM//DTD XHTML Mobile 1.0//EN"))
#endif
       )
        setIsXHTMLDocument(true); // controls if we replace entities or not.
#if ENABLE(XHTMLMP)
    else if ((publicId == QLatin1String("-//WAPFORUM//DTD XHTML Mobile 1.1//EN"))
             || (publicId == QLatin1String("-//WAPFORUM//DTD XHTML Mobile 1.0//EN"))) {
        if (AtomicString(name) != HTMLNames::htmlTag.localName()) {
            handleError(fatal, "Invalid DOCTYPE declaration, expected 'html' as root element.", lineNumber(), columnNumber());
            return;
        }

        if (document()->isXHTMLMPDocument()) // check if the MIME type is correct with this method
            setIsXHTMLMPDocument(true);
        else
            setIsXHTMLDocument(true);
    }
#endif
    if (!m_parsingFragment)
        document()->parserAddChild(DocumentType::create(document(), name, publicId, systemId));

}
}
