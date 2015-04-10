/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig (sam@webkit.org)
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
#include "DOMImplementation.h"

#include "ContentType.h"
#include "CSSStyleSheet.h"
#include "ContextFeatures.h"
#include "DocumentType.h"
#include "Element.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FTPDirectoryDocument.h"
#include "HTMLDocument.h"
#include "HTMLNames.h"
#include "HTMLViewSourceDocument.h"
#include "Image.h"
#include "ImageDocument.h"
#include "MediaDocument.h"
#include "MediaList.h"
#include "MediaPlayer.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "PluginData.h"
#include "PluginDocument.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "StyleSheetContents.h"
#include "TextDocument.h"
#include "ThreadGlobalData.h"
#include "XMLNames.h"
#include <wtf/StdLibExtras.h>

#if ENABLE(SVG)
#include "SVGNames.h"
#include "SVGDocument.h"
#endif

namespace WebCore {

typedef HashSet<String, CaseFoldingHash> FeatureSet;

static void addString(FeatureSet& set, const char* string)
{
    set.add(string);
}

#if ENABLE(VIDEO)
class DOMImplementationSupportsTypeClient : public MediaPlayerSupportsTypeClient {
public:
    DOMImplementationSupportsTypeClient(bool needsHacks, const String& host)
        : m_needsHacks(needsHacks)
        , m_host(host)
    {
    }

private:
    virtual bool mediaPlayerNeedsSiteSpecificHacks() const OVERRIDE { return m_needsHacks; }
    virtual String mediaPlayerDocumentHost() const OVERRIDE { return m_host; }

    bool m_needsHacks;
    String m_host;
};
#endif

#if ENABLE(SVG)

static bool isSVG10Feature(const String &feature, const String &version)
{
    if (!version.isEmpty() && version != "1.0")
        return false;

    static bool initialized = false;
    DEFINE_STATIC_LOCAL(FeatureSet, svgFeatures, ());
    if (!initialized) {
#if ENABLE(FILTERS) && ENABLE(SVG_FONTS)
        addString(svgFeatures, "svg");
        addString(svgFeatures, "svg.static");
#endif
//      addString(svgFeatures, "svg.animation");
//      addString(svgFeatures, "svg.dynamic");
//      addString(svgFeatures, "svg.dom.animation");
//      addString(svgFeatures, "svg.dom.dynamic");
#if ENABLE(FILTERS) && ENABLE(SVG_FONTS)
        addString(svgFeatures, "dom");
        addString(svgFeatures, "dom.svg");
        addString(svgFeatures, "dom.svg.static");
#endif
//      addString(svgFeatures, "svg.all");
//      addString(svgFeatures, "dom.svg.all");
        initialized = true;
    }
    return feature.startsWith("org.w3c.", false)
        && svgFeatures.contains(feature.right(feature.length() - 8));
}

static bool isSVG11Feature(const String &feature, const String &version)
{
    if (!version.isEmpty() && version != "1.1")
        return false;

    static bool initialized = false;
    DEFINE_STATIC_LOCAL(FeatureSet, svgFeatures, ());
    if (!initialized) {
        // Sadly, we cannot claim to implement any of the SVG 1.1 generic feature sets
        // lack of Font and Filter support.
        // http://bugs.webkit.org/show_bug.cgi?id=15480
#if ENABLE(FILTERS) && ENABLE(SVG_FONTS)
        addString(svgFeatures, "SVG");
        addString(svgFeatures, "SVGDOM");
        addString(svgFeatures, "SVG-static");
        addString(svgFeatures, "SVGDOM-static");
#endif
        addString(svgFeatures, "SVG-animation");
        addString(svgFeatures, "SVGDOM-animation");
//      addString(svgFeatures, "SVG-dynamic);
//      addString(svgFeatures, "SVGDOM-dynamic);
        addString(svgFeatures, "CoreAttribute");
        addString(svgFeatures, "Structure");
        addString(svgFeatures, "BasicStructure");
        addString(svgFeatures, "ContainerAttribute");
        addString(svgFeatures, "ConditionalProcessing");
        addString(svgFeatures, "Image");
        addString(svgFeatures, "Style");
        addString(svgFeatures, "ViewportAttribute");
        addString(svgFeatures, "Shape");
        addString(svgFeatures, "Text");
        addString(svgFeatures, "BasicText");
        addString(svgFeatures, "PaintAttribute");
        addString(svgFeatures, "BasicPaintAttribute");
        addString(svgFeatures, "OpacityAttribute");
        addString(svgFeatures, "GraphicsAttribute");
        addString(svgFeatures, "BaseGraphicsAttribute");
        addString(svgFeatures, "Marker");
//      addString(svgFeatures, "ColorProfile"); // requires color-profile, bug 6037
        addString(svgFeatures, "Gradient");
        addString(svgFeatures, "Pattern");
        addString(svgFeatures, "Clip");
        addString(svgFeatures, "BasicClip");
        addString(svgFeatures, "Mask");
#if ENABLE(FILTERS)
        addString(svgFeatures, "Filter");
        addString(svgFeatures, "BasicFilter");
#endif
        addString(svgFeatures, "DocumentEventsAttribute");
        addString(svgFeatures, "GraphicalEventsAttribute");
//      addString(svgFeatures, "AnimationEventsAttribute");
        addString(svgFeatures, "Cursor");
        addString(svgFeatures, "Hyperlinking");
        addString(svgFeatures, "XlinkAttribute");
        addString(svgFeatures, "ExternalResourcesRequired");
        addString(svgFeatures, "View");
        addString(svgFeatures, "Script");
        addString(svgFeatures, "Animation"); 
#if ENABLE(SVG_FONTS)
        addString(svgFeatures, "Font");
        addString(svgFeatures, "BasicFont");
#endif
        addString(svgFeatures, "Extensibility");
        initialized = true;
    }
    return feature.startsWith("http://www.w3.org/tr/svg11/feature#", false)
        && svgFeatures.contains(feature.right(feature.length() - 35));
}
#endif

static bool isEvents2Feature(const String &feature, const String &version)
{
    if (!version.isEmpty() && version != "2.0")
        return false;

    static bool initialized = false;
    DEFINE_STATIC_LOCAL(FeatureSet, events2Features, ());
    if (!initialized) {
        addString(events2Features, "Events");
        addString(events2Features, "HTMLEvents");
        addString(events2Features, "MouseEvents");
        addString(events2Features, "MutationEvents");
        addString(events2Features, "UIEvents");
        initialized = true;
    }
    return events2Features.contains(feature);
}

static bool isEvents3Feature(const String &feature, const String &version)
{
    if (!version.isEmpty() && version != "3.0")
        return false;

    static bool initialized = false;
    DEFINE_STATIC_LOCAL(FeatureSet, events3Features, ());
    if (!initialized) {
        // FIXME: We probably support many of these features.
//        addString(events3Features, "CompositionEvents");
//        addString(events3Features, "Events");
//        addString(events3Features, "FocusEvents");
//        addString(events3Features, "HTMLEvents");
//        addString(events3Features, "KeyboardEvents");
//        addString(events3Features, "MouseEvents");
//        addString(events3Features, "MutationEvents");
//        addString(events3Features, "MutationNameEvents");
        addString(events3Features, "TextEvents");
//        addString(events3Features, "UIEvents");
//        addString(events3Features, "WheelEvents");
        initialized = true;
    }
    // FIXME: We do not yet support Events 3 "extended feature strings".
    return events3Features.contains(feature);
}

DOMImplementation::DOMImplementation(Document* document)
    : m_document(document)
{
}

bool DOMImplementation::hasFeature(const String& feature, const String& version)
{
    String lower = feature.lower();
    if (lower == "core" || lower == "html" || lower == "xml" || lower == "xhtml")
        return version.isEmpty() || version == "1.0" || version == "2.0";
    if (lower == "css"
            || lower == "css2"
            || lower == "range"
            || lower == "stylesheets"
            || lower == "traversal"
            || lower == "views")
        return version.isEmpty() || version == "2.0";
    if (isEvents2Feature(feature, version))
        return true;
    if (lower == "xpath")
        return version.isEmpty() || version == "3.0";
    if (isEvents3Feature(feature, version))
        return true;

#if ENABLE(SVG)
    if (isSVG11Feature(feature, version))
        return true;
    if (isSVG10Feature(feature, version))
        return true;
#endif

    return false;
}

PassRefPtr<DocumentType> DOMImplementation::createDocumentType(const String& qualifiedName,
    const String& publicId, const String& systemId, ExceptionCode& ec)
{
    String prefix, localName;
    if (!Document::parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    return DocumentType::create(0, qualifiedName, publicId, systemId);
}

DOMImplementation* DOMImplementation::getInterface(const String& /*feature*/)
{
    return 0;
}

PassRefPtr<Document> DOMImplementation::createDocument(const String& namespaceURI,
    const String& qualifiedName, DocumentType* doctype, ExceptionCode& ec)
{
    RefPtr<Document> doc;
#if ENABLE(SVG)
    if (namespaceURI == SVGNames::svgNamespaceURI)
        doc = SVGDocument::create(0, KURL());
    else
#endif
    if (namespaceURI == HTMLNames::xhtmlNamespaceURI)
        doc = Document::createXHTML(0, KURL());
    else
        doc = Document::create(0, KURL());

    doc->setSecurityOrigin(m_document->securityOrigin());
    doc->setContextFeatures(m_document->contextFeatures());

    RefPtr<Node> documentElement;
    if (!qualifiedName.isEmpty()) {
        documentElement = doc->createElementNS(namespaceURI, qualifiedName, ec);
        if (ec)
            return 0;
    }

    // WRONG_DOCUMENT_ERR: Raised if doctype has already been used with a different document or was
    // created from a different implementation.
    // Hixie's interpretation of the DOM Core spec suggests we should prefer
    // other exceptions to WRONG_DOCUMENT_ERR (based on order mentioned in spec),
    // but this matches the new DOM Core spec (http://www.w3.org/TR/domcore/).
    if (doctype && doctype->document()) {
        ec = WRONG_DOCUMENT_ERR;
        return 0;
    }

    if (doctype)
        doc->appendChild(doctype);
    if (documentElement)
        doc->appendChild(documentElement.release());

    return doc.release();
}

PassRefPtr<CSSStyleSheet> DOMImplementation::createCSSStyleSheet(const String&, const String& media, ExceptionCode&)
{
    // FIXME: Title should be set.
    // FIXME: Media could have wrong syntax, in which case we should generate an exception.
    RefPtr<CSSStyleSheet> sheet = CSSStyleSheet::create(StyleSheetContents::create());
    sheet->setMediaQueries(MediaQuerySet::createAllowingDescriptionSyntax(media));
    return sheet;
}

static inline bool isValidXMLMIMETypeChar(UChar c)
{
    // Valid characters per RFCs 3023 and 2045:
    // 0-9a-zA-Z_-+~!$^{}|.%'`#&*
    return isASCIIAlphanumeric(c) || c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' || c == '+'
        || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '{' || c == '|' || c == '}' || c == '~';
}

bool DOMImplementation::isXMLMIMEType(const String& mimeType)
{
    if (mimeType == "text/xml" || mimeType == "application/xml" || mimeType == "text/xsl")
        return true;

    if (!mimeType.endsWith("+xml"))
        return false;

    size_t slashPosition = mimeType.find('/');
    // Take into account the '+xml' ending of mimeType.
    if (slashPosition == notFound || !slashPosition || slashPosition == mimeType.length() - 5)
        return false;

    // Again, mimeType ends with '+xml', no need to check the validity of that substring.
    for (size_t i = 0; i < mimeType.length() - 4; ++i) {
        if (!isValidXMLMIMETypeChar(mimeType[i]) && i != slashPosition)
            return false;
    }

    return true;
}

bool DOMImplementation::isTextMIMEType(const String& mimeType)
{
    if (MIMETypeRegistry::isSupportedJavaScriptMIMEType(mimeType)
        || mimeType == "application/json" // Render JSON as text/plain.
        || (mimeType.startsWith("text/") && mimeType != "text/html"
            && mimeType != "text/xml" && mimeType != "text/xsl"))
        return true;

    return false;
}

PassRefPtr<HTMLDocument> DOMImplementation::createHTMLDocument(const String& title)
{
    RefPtr<HTMLDocument> d = HTMLDocument::create(0, KURL());
    d->open();
    d->write("<!doctype html><html><body></body></html>");
    if (!title.isNull())
        d->setTitle(title);
    d->setSecurityOrigin(m_document->securityOrigin());
    d->setContextFeatures(m_document->contextFeatures());
    return d.release();
}

PassRefPtr<Document> DOMImplementation::createDocument(const String& type, Frame* frame, const KURL& url, bool inViewSourceMode)
{
    if (inViewSourceMode)
        return HTMLViewSourceDocument::create(frame, url, type);

    // Plugins cannot take HTML and XHTML from us, and we don't even need to initialize the plugin database for those.
    if (type == "text/html")
        return HTMLDocument::create(frame, url);
    if (type == "application/xhtml+xml")
        return Document::createXHTML(frame, url);

#if ENABLE(FTPDIR)
    // Plugins cannot take FTP from us either
    if (type == "application/x-ftp-directory")
        return FTPDirectoryDocument::create(frame, url);
#endif

    PluginData* pluginData = 0;
    PluginData::AllowedPluginTypes allowedPluginTypes = PluginData::OnlyApplicationPlugins;
    if (frame && frame->page()) {
        if (frame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin))
            allowedPluginTypes = PluginData::AllPlugins;

        pluginData = frame->page()->pluginData();
    }

    // PDF is one image type for which a plugin can override built-in support.
    // We do not want QuickTime to take over all image types, obviously.
    if ((MIMETypeRegistry::isPDFOrPostScriptMIMEType(type)) && pluginData && pluginData->supportsMimeType(type, allowedPluginTypes))
        return PluginDocument::create(frame, url);
    if (Image::supportsType(type))
        return ImageDocument::create(frame, url);

#if ENABLE(VIDEO)
     // Check to see if the type can be played by our MediaPlayer, if so create a MediaDocument
    // Key system is not applicable here.
    DOMImplementationSupportsTypeClient client(frame && frame->settings() && frame->settings()->needsSiteSpecificQuirks(), url.host());
    if (MediaPlayer::supportsType(ContentType(type), String(), url, &client))
         return MediaDocument::create(frame, url);
#endif

    // Everything else except text/plain can be overridden by plugins. In particular, Adobe SVG Viewer should be used for SVG, if installed.
    // Disallowing plug-ins to use text/plain prevents plug-ins from hijacking a fundamental type that the browser is expected to handle,
    // and also serves as an optimization to prevent loading the plug-in database in the common case.
    if (type != "text/plain" && ((pluginData && pluginData->supportsMimeType(type, allowedPluginTypes)) || (frame && frame->loader()->client()->shouldAlwaysUsePluginDocument(type))))
        return PluginDocument::create(frame, url);
    if (isTextMIMEType(type))
        return TextDocument::create(frame, url);

#if ENABLE(SVG)
    if (type == "image/svg+xml")
        return SVGDocument::create(frame, url);
#endif
    if (isXMLMIMEType(type))
        return Document::create(frame, url);

    return HTMLDocument::create(frame, url);
}

}
