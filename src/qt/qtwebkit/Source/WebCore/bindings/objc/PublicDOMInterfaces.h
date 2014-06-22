// Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
// Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

// This file is used by bindings/scripts/CodeGeneratorObjC.pm to determine public API.
// All public DOM class interfaces, properties and methods need to be in this file.
// Anything not in the file will be generated into the appropriate private header file.

#ifndef OBJC_CODE_GENERATION
#error Do not include this header, instead include the appropriate DOM header.
#endif

@interface DOMAttr : DOMNode WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *name;
@property(readonly) BOOL specified;
@property(copy) NSString *value;
@property(readonly, retain) DOMElement *ownerElement;
@property(readonly, retain) DOMCSSStyleDeclaration *style AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMCDATASection : DOMText WEBKIT_VERSION_1_3
@end

@interface DOMCharacterData : DOMNode WEBKIT_VERSION_1_3
@property(copy) NSString *data;
@property(readonly) unsigned length;
- (NSString *)substringData:(unsigned)offset :(unsigned)length;
- (NSString *)substringData:(unsigned)offset length:(unsigned)length AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)appendData:(NSString *)data;
- (void)insertData:(unsigned)offset :(NSString *)data;
- (void)deleteData:(unsigned)offset :(unsigned)length;
- (void)replaceData:(unsigned)offset :(unsigned)length :(NSString *)data;
- (void)insertData:(unsigned)offset data:(NSString *)data AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)deleteData:(unsigned)offset length:(unsigned)length AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)replaceData:(unsigned)offset length:(unsigned)length data:(NSString *)data AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMComment : DOMCharacterData WEBKIT_VERSION_1_3
@end

@interface DOMImplementation : DOMObject WEBKIT_VERSION_1_3
- (BOOL)hasFeature:(NSString *)feature :(NSString *)version;
- (DOMDocumentType *)createDocumentType:(NSString *)qualifiedName :(NSString *)publicId :(NSString *)systemId;
- (DOMDocument *)createDocument:(NSString *)namespaceURI :(NSString *)qualifiedName :(DOMDocumentType *)doctype;
- (DOMCSSStyleSheet *)createCSSStyleSheet:(NSString *)title :(NSString *)media;
- (BOOL)hasFeature:(NSString *)feature version:(NSString *)version AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMDocumentType *)createDocumentType:(NSString *)qualifiedName publicId:(NSString *)publicId systemId:(NSString *)systemId AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMDocument *)createDocument:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName doctype:(DOMDocumentType *)doctype AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMCSSStyleSheet *)createCSSStyleSheet:(NSString *)title media:(NSString *)media AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMHTMLDocument *)createHTMLDocument:(NSString *)title AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMAbstractView : DOMObject WEBKIT_VERSION_1_3
@property(readonly, retain) DOMDocument *document;
@end

@interface DOMDocument : DOMNode WEBKIT_VERSION_1_3
@property(readonly, retain) DOMDocumentType *doctype;
@property(readonly, retain) DOMImplementation *implementation;
@property(readonly, retain) DOMElement *documentElement;
@property(readonly, retain) DOMAbstractView *defaultView;
@property(readonly, retain) DOMStyleSheetList *styleSheets;
@property(readonly, retain) DOMHTMLCollection *images;
@property(readonly, retain) DOMHTMLCollection *applets;
@property(readonly, retain) DOMHTMLCollection *links;
@property(readonly, retain) DOMHTMLCollection *forms;
@property(readonly, retain) DOMHTMLCollection *anchors;
@property(copy) NSString *title;
@property(readonly, copy) NSString *referrer;
@property(readonly, copy) NSString *domain;
@property(readonly, copy) NSString *URL;
@property(retain) DOMHTMLElement *body;
@property(copy) NSString *cookie;
@property(readonly, copy) NSString *inputEncoding AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *xmlEncoding AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *xmlVersion AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property BOOL xmlStandalone AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *documentURI AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *charset AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *defaultCharset AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *readyState AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *characterSet AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *preferredStylesheetSet AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *selectedStylesheetSet AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *lastModified AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMElement *)createElement:(NSString *)tagName;
- (DOMDocumentFragment *)createDocumentFragment;
- (DOMText *)createTextNode:(NSString *)data;
- (DOMComment *)createComment:(NSString *)data;
- (DOMCDATASection *)createCDATASection:(NSString *)data;
- (DOMProcessingInstruction *)createProcessingInstruction:(NSString *)target :(NSString *)data;
- (DOMProcessingInstruction *)createProcessingInstruction:(NSString *)target data:(NSString *)data AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMAttr *)createAttribute:(NSString *)name;
- (DOMEntityReference *)createEntityReference:(NSString *)name;
- (DOMNodeList *)getElementsByTagName:(NSString *)tagname;
- (DOMNode *)importNode:(DOMNode *)importedNode :(BOOL)deep;
- (DOMElement *)createElementNS:(NSString *)namespaceURI :(NSString *)qualifiedName;
- (DOMAttr *)createAttributeNS:(NSString *)namespaceURI :(NSString *)qualifiedName;
- (DOMNodeList *)getElementsByTagNameNS:(NSString *)namespaceURI :(NSString *)localName;
- (DOMNode *)importNode:(DOMNode *)importedNode deep:(BOOL)deep AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)adoptNode:(DOMNode *)source AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMElement *)createElementNS:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMAttr *)createAttributeNS:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNodeList *)getElementsByTagNameNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMElement *)getElementById:(NSString *)elementId;
- (DOMEvent *)createEvent:(NSString *)eventType;
- (DOMRange *)createRange;
- (DOMCSSStyleDeclaration *)createCSSStyleDeclaration AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMCSSStyleDeclaration *)getOverrideStyle:(DOMElement *)element :(NSString *)pseudoElement;
- (DOMCSSStyleDeclaration *)getOverrideStyle:(DOMElement *)element pseudoElement:(NSString *)pseudoElement AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMCSSStyleDeclaration *)getComputedStyle:(DOMElement *)element :(NSString *)pseudoElement;
- (DOMCSSStyleDeclaration *)getComputedStyle:(DOMElement *)element pseudoElement:(NSString *)pseudoElement AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMCSSRuleList *)getMatchedCSSRules:(DOMElement *)element pseudoElement:(NSString *)pseudoElement AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMCSSRuleList *)getMatchedCSSRules:(DOMElement *)element pseudoElement:(NSString *)pseudoElement authorOnly:(BOOL)authorOnly AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNodeList *)getElementsByName:(NSString *)elementName;
- (DOMNodeIterator *)createNodeIterator:(DOMNode *)root whatToShow:(unsigned)whatToShow filter:(id <DOMNodeFilter>)filter expandEntityReferences:(BOOL)expandEntityReferences AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMTreeWalker *)createTreeWalker:(DOMNode *)root whatToShow:(unsigned)whatToShow filter:(id <DOMNodeFilter>)filter expandEntityReferences:(BOOL)expandEntityReferences AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNodeIterator *)createNodeIterator:(DOMNode *)root :(unsigned)whatToShow :(id <DOMNodeFilter>)filter :(BOOL)expandEntityReferences;
- (DOMTreeWalker *)createTreeWalker:(DOMNode *)root :(unsigned)whatToShow :(id <DOMNodeFilter>)filter :(BOOL)expandEntityReferences;
- (DOMXPathExpression *)createExpression:(NSString *)expression :(id <DOMXPathNSResolver>)resolver AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
- (DOMXPathExpression *)createExpression:(NSString *)expression resolver:(id <DOMXPathNSResolver>)resolver AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (id <DOMXPathNSResolver>)createNSResolver:(DOMNode *)nodeResolver AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMXPathResult *)evaluate:(NSString *)expression :(DOMNode *)contextNode :(id <DOMXPathNSResolver>)resolver :(unsigned short)type :(DOMXPathResult *)inResult AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
- (DOMXPathResult *)evaluate:(NSString *)expression contextNode:(DOMNode *)contextNode resolver:(id <DOMXPathNSResolver>)resolver type:(unsigned short)type inResult:(DOMXPathResult *)inResult AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)execCommand:(NSString *)command userInterface:(BOOL)userInterface value:(NSString *)value AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)execCommand:(NSString *)command userInterface:(BOOL)userInterface AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)execCommand:(NSString *)command AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)queryCommandEnabled:(NSString *)command AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)queryCommandIndeterm:(NSString *)command AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)queryCommandState:(NSString *)command AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)queryCommandSupported:(NSString *)command AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (NSString *)queryCommandValue:(NSString *)command AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMElement *)elementFromPoint:(int)x y:(int)y AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNodeList *)getElementsByClassName:(NSString *)tagname AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMElement *)querySelector:(NSString *)selectors AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMNodeList *)querySelectorAll:(NSString *)selectors AVAILABLE_IN_WEBKIT_VERSION_4_0;
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
- (void)webkitCancelFullScreen AVAILABLE_IN_WEBKIT_VERSION_4_0;
#endif
@end

@interface DOMDocumentFragment : DOMNode WEBKIT_VERSION_1_3
@end

@interface DOMDocumentType : DOMNode WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *name;
@property(readonly, retain) DOMNamedNodeMap *entities;
@property(readonly, retain) DOMNamedNodeMap *notations;
@property(readonly, copy) NSString *publicId;
@property(readonly, copy) NSString *systemId;
@property(readonly, copy) NSString *internalSubset;
@end

@interface DOMElement : DOMNode WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *tagName;
@property(readonly, retain) DOMCSSStyleDeclaration *style;
@property(copy) NSString *className;
@property(readonly) int offsetLeft;
@property(readonly) int offsetTop;
@property(readonly) int offsetWidth;
@property(readonly) int offsetHeight;
@property(readonly, retain) DOMElement *offsetParent;
@property(readonly) int clientWidth;
@property(readonly) int clientHeight;
@property int scrollLeft;
@property int scrollTop;
@property(readonly) int scrollWidth;
@property(readonly) int scrollHeight;
@property(readonly) int clientLeft AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int clientTop AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *innerText AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMElement *firstElementChild AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly, retain) DOMElement *lastElementChild AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly, retain) DOMElement *previousElementSibling AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly, retain) DOMElement *nextElementSibling AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly) unsigned childElementCount AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (NSString *)getAttribute:(NSString *)name;
- (void)setAttribute:(NSString *)name :(NSString *)value;
- (void)setAttribute:(NSString *)name value:(NSString *)value AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)removeAttribute:(NSString *)name;
- (DOMAttr *)getAttributeNode:(NSString *)name;
- (DOMAttr *)setAttributeNode:(DOMAttr *)newAttr;
- (DOMAttr *)removeAttributeNode:(DOMAttr *)oldAttr;
- (DOMNodeList *)getElementsByTagName:(NSString *)name;
- (NSString *)getAttributeNS:(NSString *)namespaceURI :(NSString *)localName;
- (void)setAttributeNS:(NSString *)namespaceURI :(NSString *)qualifiedName :(NSString *)value;
- (void)removeAttributeNS:(NSString *)namespaceURI :(NSString *)localName;
- (DOMNodeList *)getElementsByTagNameNS:(NSString *)namespaceURI :(NSString *)localName;
- (DOMAttr *)getAttributeNodeNS:(NSString *)namespaceURI :(NSString *)localName;
- (NSString *)getAttributeNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)setAttributeNS:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName value:(NSString *)value AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)removeAttributeNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNodeList *)getElementsByTagNameNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMAttr *)getAttributeNodeNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMAttr *)setAttributeNodeNS:(DOMAttr *)newAttr;
- (BOOL)hasAttribute:(NSString *)name;
- (BOOL)hasAttributeNS:(NSString *)namespaceURI :(NSString *)localName;
- (BOOL)hasAttributeNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)scrollIntoView:(BOOL)alignWithTop AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)scrollIntoViewIfNeeded:(BOOL)centerIfNeeded AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)scrollByLines:(int)lines AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)scrollByPages:(int)pages AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)focus AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)blur AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMNodeList *)getElementsByClassName:(NSString *)name AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMElement *)querySelector:(NSString *)selectors AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMNodeList *)querySelectorAll:(NSString *)selectors AVAILABLE_IN_WEBKIT_VERSION_4_0;
#if defined(ENABLE_FULLSCREEN_API) && ENABLE_FULLSCREEN_API
- (void)webkitRequestFullScreen:(unsigned short)flags AVAILABLE_IN_WEBKIT_VERSION_4_0;
#endif
@end

@interface DOMEntity : DOMNode WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *publicId;
@property(readonly, copy) NSString *systemId;
@property(readonly, copy) NSString *notationName;
@end

@interface DOMEntityReference : DOMNode WEBKIT_VERSION_1_3
@end

@interface DOMBlob : DOMObject WEBKIT_VERSION_4_0
@property(readonly) unsigned long long size;
@end

@interface DOMFile : DOMBlob WEBKIT_VERSION_4_0
@property(readonly, copy) NSString *name;
@end

@interface DOMFileList : DOMObject WEBKIT_VERSION_4_0
@property(readonly) unsigned length;
- (DOMFile *)item:(unsigned)index;
@end

@interface DOMNamedNodeMap : DOMObject WEBKIT_VERSION_1_3
@property(readonly) unsigned length;
- (DOMNode *)getNamedItem:(NSString *)name;
- (DOMNode *)setNamedItem:(DOMNode *)node;
- (DOMNode *)removeNamedItem:(NSString *)name;
- (DOMNode *)item:(unsigned)index;
- (DOMNode *)getNamedItemNS:(NSString *)namespaceURI :(NSString *)localName;
- (DOMNode *)getNamedItemNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)setNamedItemNS:(DOMNode *)node;
- (DOMNode *)removeNamedItemNS:(NSString *)namespaceURI :(NSString *)localName;
- (DOMNode *)removeNamedItemNS:(NSString *)namespaceURI localName:(NSString *)localName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMNode : DOMObject WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *nodeName;
@property(copy) NSString *nodeValue;
@property(readonly) unsigned short nodeType;
@property(readonly, retain) DOMNode *parentNode;
@property(readonly, retain) DOMNodeList *childNodes;
@property(readonly, retain) DOMNode *firstChild;
@property(readonly, retain) DOMNode *lastChild;
@property(readonly, retain) DOMNode *previousSibling;
@property(readonly, retain) DOMNode *nextSibling;
@property(readonly, retain) DOMNamedNodeMap *attributes;
@property(readonly, retain) DOMDocument *ownerDocument;
@property(readonly, copy) NSString *namespaceURI;
@property(copy) NSString *prefix;
@property(readonly, copy) NSString *localName;
@property(copy) NSString *textContent AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *baseURI AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMElement *parentElement AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) BOOL isContentEditable AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)insertBefore:(DOMNode *)newChild :(DOMNode *)refChild;
- (DOMNode *)insertBefore:(DOMNode *)newChild refChild:(DOMNode *)refChild AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)replaceChild:(DOMNode *)newChild :(DOMNode *)oldChild;
- (DOMNode *)replaceChild:(DOMNode *)newChild oldChild:(DOMNode *)oldChild AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)removeChild:(DOMNode *)oldChild;
- (DOMNode *)appendChild:(DOMNode *)newChild;
- (BOOL)hasChildNodes;
- (DOMNode *)cloneNode:(BOOL)deep;
- (void)normalize;
- (BOOL)isSupported:(NSString *)feature :(NSString *)version;
- (BOOL)isSupported:(NSString *)feature version:(NSString *)version AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)hasAttributes;
- (BOOL)isSameNode:(DOMNode *)other AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)isEqualNode:(DOMNode *)other AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (NSString *)lookupPrefix:(NSString *)namespaceURI AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)isDefaultNamespace:(NSString *)namespaceURI AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (NSString *)lookupNamespaceURI:(NSString *)prefix AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (unsigned short)compareDocumentPosition:(DOMNode *)other AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (BOOL)contains:(DOMNode *)other AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMNodeList : DOMObject WEBKIT_VERSION_1_3
@property(readonly) unsigned length;
- (DOMNode *)item:(unsigned)index;
@end

@interface DOMNotation : DOMNode WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *publicId;
@property(readonly, copy) NSString *systemId;
@end

@interface DOMProcessingInstruction : DOMNode WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *target;
@property(copy) NSString *data;
@property(readonly, retain) DOMStyleSheet *sheet AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMText : DOMCharacterData WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *wholeText AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMText *)splitText:(unsigned)offset;
- (DOMText *)replaceWholeText:(NSString *)content AVAILABLE_IN_WEBKIT_VERSION_4_0;
@end

@interface DOMHTMLAnchorElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property(copy) NSString *charset;
@property(copy) NSString *coords;
@property(copy) NSString *href;
@property(copy) NSString *hreflang;
@property(copy) NSString *name;
@property(copy) NSString *rel;
@property(copy) NSString *rev;
@property(copy) NSString *shape;
@property(copy) NSString *target;
@property(copy) NSString *type;
@property(readonly, copy) NSURL *absoluteLinkURL AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *hashName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *host AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *hostname AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *pathname AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *port AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *protocol AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *search AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *text AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLAppletElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@property(copy) NSString *alt;
@property(copy) NSString *archive;
@property(copy) NSString *code;
@property(copy) NSString *codeBase;
@property(copy) NSString *height;
@property int hspace;
@property(copy) NSString *name;
@property(copy) NSString *object;
@property int vspace;
@property(copy) NSString *width;
@end

@interface DOMHTMLAreaElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property(copy) NSString *alt;
@property(copy) NSString *coords;
@property(copy) NSString *href;
@property BOOL noHref;
@property(copy) NSString *shape;
@property(copy) NSString *target;
@property(readonly, copy) NSURL *absoluteLinkURL AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *hashName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *host AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *hostname AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *pathname AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *port AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *protocol AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSString *search AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLBRElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *clear;
@end

@interface DOMHTMLBaseElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *href;
@property(copy) NSString *target;
@end

@interface DOMHTMLBaseFontElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *color;
@property(copy) NSString *face;
@property(copy) NSString *size;
@end

@interface DOMHTMLBodyElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *aLink;
@property(copy) NSString *background;
@property(copy) NSString *bgColor;
@property(copy) NSString *link;
@property(copy) NSString *text;
@property(copy) NSString *vLink;
@end

@interface DOMHTMLButtonElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property(readonly, retain) DOMHTMLFormElement *form;
@property BOOL disabled;
@property(copy) NSString *name;
@property(copy) NSString *type;
@property(copy) NSString *value;
@property BOOL autofocus AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly) BOOL willValidate AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)click AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLCanvasElement : DOMHTMLElement WEBKIT_VERSION_3_0
@property int height;
@property int width;
@end

@interface DOMHTMLCollection : DOMObject WEBKIT_VERSION_1_3
@property(readonly) unsigned length;
- (DOMNode *)item:(unsigned)index;
- (DOMNode *)namedItem:(NSString *)name;
- (DOMNodeList *)tags:(NSString *)name AVAILABLE_IN_WEBKIT_VERSION_4_0;
@end

@interface DOMHTMLDListElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL compact;
@end

@interface DOMHTMLDirectoryElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL compact;
@end

@interface DOMHTMLDivElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@end

@interface DOMHTMLDocument : DOMDocument WEBKIT_VERSION_1_3
@property(readonly, retain) DOMHTMLCollection *embeds AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMHTMLCollection *plugins AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMHTMLCollection *scripts AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int width AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int height AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *dir AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *designMode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *bgColor AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *fgColor AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *alinkColor AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *linkColor AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *vlinkColor AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMElement *activeElement AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly, copy) NSString *compatMode AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)captureEvents AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)releaseEvents AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)clear AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (BOOL)hasFocus AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)open;
- (void)close;
- (void)write:(NSString *)text;
- (void)writeln:(NSString *)text;
@end

@interface DOMHTMLElement : DOMElement WEBKIT_VERSION_1_3
@property(copy) NSString *accessKey AVAILABLE_AFTER_WEBKIT_VERSION_5_1;
@property(copy) NSString *title;
@property(copy) NSString *idName;
@property(copy) NSString *lang;
@property(copy) NSString *dir;
@property(copy) NSString *innerHTML;
@property(copy) NSString *innerText;
@property(copy) NSString *outerHTML;
@property(copy) NSString *outerText;
@property(readonly, retain) DOMHTMLCollection *children;
@property(copy) NSString *contentEditable;
@property(readonly) BOOL isContentEditable;
@property(readonly, copy) NSString *titleDisplayString AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property int tabIndex;
- (void)click AVAILABLE_AFTER_WEBKIT_VERSION_5_1;
@end

@interface DOMHTMLEmbedElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@property int height;
@property(copy) NSString *name;
@property(copy) NSString *src;
@property(copy) NSString *type;
@property int width;
@end

@interface DOMHTMLFieldSetElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly, retain) DOMHTMLFormElement *form;
@end

@interface DOMHTMLFontElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *color;
@property(copy) NSString *face;
@property(copy) NSString *size;
@end

@interface DOMHTMLFormElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly, retain) DOMHTMLCollection *elements;
@property(readonly) int length;
@property(copy) NSString *name;
@property(copy) NSString *acceptCharset;
@property(copy) NSString *action;
@property(copy) NSString *enctype;
@property(copy) NSString *method;
@property(copy) NSString *target;
@property(copy) NSString *encoding AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)submit;
- (void)reset;
@end

@interface DOMHTMLFrameElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *frameBorder;
@property(copy) NSString *longDesc;
@property(copy) NSString *marginHeight;
@property(copy) NSString *marginWidth;
@property(copy) NSString *name;
@property BOOL noResize;
@property(copy) NSString *scrolling;
@property(copy) NSString *src;
@property(readonly, retain) DOMDocument *contentDocument;
@property(readonly, retain) DOMAbstractView *contentWindow AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *location AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int width AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int height AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLFrameSetElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *cols;
@property(copy) NSString *rows;
@end

@interface DOMHTMLHRElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@property BOOL noShade;
@property(copy) NSString *size;
@property(copy) NSString *width;
@end

@interface DOMHTMLHeadElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *profile;
@end

@interface DOMHTMLHeadingElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@end

@interface DOMHTMLHtmlElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *version;
@end

@interface DOMHTMLIFrameElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@property(copy) NSString *frameBorder;
@property(copy) NSString *height;
@property(copy) NSString *longDesc;
@property(copy) NSString *marginHeight;
@property(copy) NSString *marginWidth;
@property(copy) NSString *name;
@property(copy) NSString *scrolling;
@property(copy) NSString *src;
@property(copy) NSString *width;
@property(readonly, retain) DOMDocument *contentDocument;
@property(readonly, retain) DOMAbstractView *contentWindow AVAILABLE_IN_WEBKIT_VERSION_4_0;
@end

@interface DOMHTMLImageElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *name;
@property(copy) NSString *align;
@property(copy) NSString *alt;
@property(copy) NSString *border;
@property int height;
@property int hspace;
@property BOOL isMap;
@property(copy) NSString *longDesc;
@property(copy) NSString *src;
@property(copy) NSString *useMap;
@property int vspace;
@property int width;
@property(readonly, copy) NSString *altDisplayString AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSURL *absoluteImageURL AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) BOOL complete AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(copy) NSString *lowsrc AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int naturalHeight AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int naturalWidth AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int x AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int y AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLInputElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *defaultValue;
@property BOOL defaultChecked;
@property(readonly, retain) DOMHTMLFormElement *form;
@property(copy) NSString *accept;
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property(copy) NSString *align;
@property(copy) NSString *alt;
@property BOOL checked;
@property BOOL disabled;
@property int maxLength;
@property(copy) NSString *name;
@property BOOL readOnly;
@property(copy) NSString *size;
@property(copy) NSString *src;
@property(copy) NSString *type;
@property(copy) NSString *useMap;
@property(copy) NSString *value;
@property(readonly, copy) NSString *altDisplayString AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, copy) NSURL *absoluteImageURL AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property BOOL indeterminate AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property int selectionStart AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property int selectionEnd AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property BOOL autofocus AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property BOOL multiple AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly) BOOL willValidate AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(retain) DOMFileList *files AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)select;
- (void)click;
- (void)setSelectionRange:(int)start end:(int)end AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLLIElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *type;
@property int value;
@end

@interface DOMHTMLLabelElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property(readonly, retain) DOMHTMLFormElement *form;
@property(copy) NSString *htmlFor;
@end

@interface DOMHTMLLegendElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property(readonly, retain) DOMHTMLFormElement *form;
@property(copy) NSString *align;
@end

@interface DOMHTMLLinkElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL disabled;
@property(copy) NSString *charset;
@property(copy) NSString *href;
@property(copy) NSString *hreflang;
@property(copy) NSString *media;
@property(copy) NSString *rel;
@property(copy) NSString *rev;
@property(copy) NSString *target;
@property(copy) NSString *type;
@property(readonly, copy) NSURL *absoluteLinkURL AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMStyleSheet *sheet AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLMapElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly, retain) DOMHTMLCollection *areas;
@property(copy) NSString *name;
@end

@interface DOMHTMLMarqueeElement : DOMHTMLElement WEBKIT_VERSION_3_0
- (void)start;
- (void)stop;
@end

@interface DOMHTMLMenuElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL compact;
@end

@interface DOMHTMLMetaElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *content;
@property(copy) NSString *httpEquiv;
@property(copy) NSString *name;
@property(copy) NSString *scheme;
@end

@interface DOMHTMLModElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *cite;
@property(copy) NSString *dateTime;
@end

@interface DOMHTMLOListElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL compact;
@property int start;
@property(copy) NSString *type;
@end

@interface DOMHTMLObjectElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly, retain) DOMHTMLFormElement *form;
@property(copy) NSString *code;
@property(copy) NSString *align;
@property(copy) NSString *archive;
@property(copy) NSString *border;
@property(copy) NSString *codeBase;
@property(copy) NSString *codeType;
@property(copy) NSString *data;
@property BOOL declare;
@property(copy) NSString *height;
@property int hspace;
@property(copy) NSString *name;
@property(copy) NSString *standby;
@property(copy) NSString *type;
@property(copy) NSString *useMap;
@property int vspace;
@property(copy) NSString *width;
@property(readonly, retain) DOMDocument *contentDocument;
@property(readonly, copy) NSURL *absoluteImageURL AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLOptGroupElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL disabled;
@property(copy) NSString *label;
@end

@interface DOMHTMLOptionElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly, retain) DOMHTMLFormElement *form;
@property BOOL defaultSelected;
@property(readonly, copy) NSString *text;
@property(readonly) int index;
@property BOOL disabled;
@property(copy) NSString *label;
@property BOOL selected;
@property(copy) NSString *value;
@end

@interface DOMHTMLOptionsCollection : DOMObject WEBKIT_VERSION_1_3
@property unsigned length;
@property int selectedIndex AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)item:(unsigned)index;
- (DOMNode *)namedItem:(NSString *)name;
- (void)add:(DOMHTMLOptionElement *)option index:(unsigned)index AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)remove:(unsigned)index AVAILABLE_IN_WEBKIT_VERSION_4_0;
@end

@interface DOMHTMLParagraphElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@end

@interface DOMHTMLParamElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *name;
@property(copy) NSString *type;
@property(copy) NSString *value;
@property(copy) NSString *valueType;
@end

@interface DOMHTMLPreElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property int width;
@property BOOL wrap AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLQuoteElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *cite;
@end

@interface DOMHTMLScriptElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *text;
@property(copy) NSString *htmlFor;
@property(copy) NSString *event;
@property(copy) NSString *charset;
@property BOOL defer;
@property(copy) NSString *src;
@property(copy) NSString *type;
@end

@interface DOMHTMLSelectElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *type;
@property int selectedIndex;
@property(copy) NSString *value;
@property(readonly) int length;
@property(readonly, retain) DOMHTMLFormElement *form;
@property(readonly, retain) DOMHTMLOptionsCollection *options;
@property BOOL disabled;
@property BOOL multiple;
@property(copy) NSString *name;
@property int size;
@property(readonly) BOOL willValidate AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property BOOL autofocus AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)add:(DOMHTMLElement *)element :(DOMHTMLElement *)before;
- (void)add:(DOMHTMLElement *)element before:(DOMHTMLElement *)before AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)remove:(int)index;
- (DOMNode *)item:(unsigned)index AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (DOMNode *)namedItem:(NSString *)name AVAILABLE_IN_WEBKIT_VERSION_4_0;
@end

@interface DOMHTMLStyleElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL disabled;
@property(copy) NSString *media;
@property(copy) NSString *type;
@property(readonly, retain) DOMStyleSheet *sheet AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLTableCaptionElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@end

@interface DOMHTMLTableCellElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly) int cellIndex;
@property(copy) NSString *abbr;
@property(copy) NSString *align;
@property(copy) NSString *axis;
@property(copy) NSString *bgColor;
@property(copy) NSString *ch;
@property(copy) NSString *chOff;
@property int colSpan;
@property(copy) NSString *headers;
@property(copy) NSString *height;
@property BOOL noWrap;
@property int rowSpan;
@property(copy) NSString *scope;
@property(copy) NSString *vAlign;
@property(copy) NSString *width;
@end

@interface DOMHTMLTableColElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@property(copy) NSString *ch;
@property(copy) NSString *chOff;
@property int span;
@property(copy) NSString *vAlign;
@property(copy) NSString *width;
@end

@interface DOMHTMLTableElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(retain) DOMHTMLTableCaptionElement *caption;
@property(retain) DOMHTMLTableSectionElement *tHead;
@property(retain) DOMHTMLTableSectionElement *tFoot;
@property(readonly, retain) DOMHTMLCollection *rows;
@property(readonly, retain) DOMHTMLCollection *tBodies;
@property(copy) NSString *align;
@property(copy) NSString *bgColor;
@property(copy) NSString *border;
@property(copy) NSString *cellPadding;
@property(copy) NSString *cellSpacing;
@property(copy) NSString *frameBorders;
@property(copy) NSString *rules;
@property(copy) NSString *summary;
@property(copy) NSString *width;
- (DOMHTMLElement *)createTHead;
- (void)deleteTHead;
- (DOMHTMLElement *)createTFoot;
- (void)deleteTFoot;
- (DOMHTMLElement *)createCaption;
- (void)deleteCaption;
- (DOMHTMLElement *)insertRow:(int)index;
- (void)deleteRow:(int)index;
@end

@interface DOMHTMLTableRowElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(readonly) int rowIndex;
@property(readonly) int sectionRowIndex;
@property(readonly, retain) DOMHTMLCollection *cells;
@property(copy) NSString *align;
@property(copy) NSString *bgColor;
@property(copy) NSString *ch;
@property(copy) NSString *chOff;
@property(copy) NSString *vAlign;
- (DOMHTMLElement *)insertCell:(int)index;
- (void)deleteCell:(int)index;
@end

@interface DOMHTMLTableSectionElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *align;
@property(copy) NSString *ch;
@property(copy) NSString *chOff;
@property(copy) NSString *vAlign;
@property(readonly, retain) DOMHTMLCollection *rows;
- (DOMHTMLElement *)insertRow:(int)index;
- (void)deleteRow:(int)index;
@end

@interface DOMHTMLTextAreaElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *defaultValue;
@property(readonly, retain) DOMHTMLFormElement *form;
@property(copy) NSString *accessKey AVAILABLE_WEBKIT_VERSION_1_3_AND_LATER_BUT_DEPRECATED_AFTER_WEBKIT_VERSION_5_1;
@property int cols;
@property BOOL disabled;
@property(copy) NSString *name;
@property BOOL readOnly;
@property int rows;
@property(readonly, copy) NSString *type;
@property(copy) NSString *value;
@property int selectionStart AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property int selectionEnd AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property BOOL autofocus AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property(readonly) BOOL willValidate AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)select;
- (void)setSelectionRange:(int)start end:(int)end AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMHTMLTitleElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property(copy) NSString *text;
@end

@interface DOMHTMLUListElement : DOMHTMLElement WEBKIT_VERSION_1_3
@property BOOL compact;
@property(copy) NSString *type;
@end

@interface DOMStyleSheetList : DOMObject WEBKIT_VERSION_1_3
@property(readonly) unsigned length;
- (DOMStyleSheet *)item:(unsigned)index;
@end

@interface DOMCSSCharsetRule : DOMCSSRule WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *encoding;
@end

@interface DOMCSSFontFaceRule : DOMCSSRule WEBKIT_VERSION_1_3
@property(readonly, retain) DOMCSSStyleDeclaration *style;
@end

@interface DOMCSSImportRule : DOMCSSRule WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *href;
@property(readonly, retain) DOMMediaList *media;
@property(readonly, retain) DOMCSSStyleSheet *styleSheet;
@end

@interface DOMCSSMediaRule : DOMCSSRule WEBKIT_VERSION_1_3
@property(readonly, retain) DOMMediaList *media;
@property(readonly, retain) DOMCSSRuleList *cssRules;
- (unsigned)insertRule:(NSString *)rule :(unsigned)index;
- (unsigned)insertRule:(NSString *)rule index:(unsigned)index AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)deleteRule:(unsigned)index;
@end

@interface DOMCSSPageRule : DOMCSSRule WEBKIT_VERSION_1_3
@property(copy) NSString *selectorText;
@property(readonly, retain) DOMCSSStyleDeclaration *style;
@end

@interface DOMCSSPrimitiveValue : DOMCSSValue WEBKIT_VERSION_1_3
@property(readonly) unsigned short primitiveType;
- (void)setFloatValue:(unsigned short)unitType :(float)floatValue;
- (void)setFloatValue:(unsigned short)unitType floatValue:(float)floatValue AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (float)getFloatValue:(unsigned short)unitType;
- (void)setStringValue:(unsigned short)stringType :(NSString *)stringValue;
- (void)setStringValue:(unsigned short)stringType stringValue:(NSString *)stringValue AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (NSString *)getStringValue;
- (DOMCounter *)getCounterValue;
- (DOMRect *)getRectValue;
- (DOMRGBColor *)getRGBColorValue;
@end

@interface DOMRGBColor : DOMObject WEBKIT_VERSION_1_3
@property(readonly, retain) DOMCSSPrimitiveValue *red;
@property(readonly, retain) DOMCSSPrimitiveValue *green;
@property(readonly, retain) DOMCSSPrimitiveValue *blue;
@property(readonly, retain) DOMCSSPrimitiveValue *alpha;
@property(readonly, copy) NSColor *color AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMCSSRule : DOMObject WEBKIT_VERSION_1_3
@property(readonly) unsigned short type;
@property(copy) NSString *cssText;
@property(readonly, retain) DOMCSSStyleSheet *parentStyleSheet;
@property(readonly, retain) DOMCSSRule *parentRule;
@end

@interface DOMCSSRuleList : DOMObject WEBKIT_VERSION_1_3
@property(readonly) unsigned length;
- (DOMCSSRule *)item:(unsigned)index;
@end

@interface DOMCSSStyleDeclaration : DOMObject WEBKIT_VERSION_1_3
@property(copy) NSString *cssText;
@property(readonly) unsigned length;
@property(readonly, retain) DOMCSSRule *parentRule;
- (NSString *)getPropertyValue:(NSString *)propertyName;
- (DOMCSSValue *)getPropertyCSSValue:(NSString *)propertyName;
- (NSString *)removeProperty:(NSString *)propertyName;
- (NSString *)getPropertyPriority:(NSString *)propertyName;
- (void)setProperty:(NSString *)propertyName :(NSString *)value :(NSString *)priority;
- (void)setProperty:(NSString *)propertyName value:(NSString *)value priority:(NSString *)priority AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (NSString *)item:(unsigned)index;
- (NSString *)getPropertyShorthand:(NSString *)propertyName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
- (BOOL)isPropertyImplicit:(NSString *)propertyName AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMCSSStyleRule : DOMCSSRule WEBKIT_VERSION_1_3
@property(copy) NSString *selectorText;
@property(readonly, retain) DOMCSSStyleDeclaration *style;
@end

@interface DOMStyleSheet : DOMObject WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *type;
@property BOOL disabled;
@property(readonly, retain) DOMNode *ownerNode;
@property(readonly, retain) DOMStyleSheet *parentStyleSheet;
@property(readonly, copy) NSString *href;
@property(readonly, copy) NSString *title;
@property(readonly, retain) DOMMediaList *media;
@end

@interface DOMCSSStyleSheet : DOMStyleSheet WEBKIT_VERSION_1_3
@property(readonly, retain) DOMCSSRule *ownerRule;
@property(readonly, retain) DOMCSSRuleList *cssRules;
@property(readonly, retain) DOMCSSRuleList *rules AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (unsigned)insertRule:(NSString *)rule :(unsigned)index;
- (unsigned)insertRule:(NSString *)rule index:(unsigned)index AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)deleteRule:(unsigned)index;
- (int)addRule:(NSString *)selector style:(NSString *)style index:(unsigned)index AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)removeRule:(unsigned)index AVAILABLE_IN_WEBKIT_VERSION_4_0;
@end

@interface DOMCSSValue : DOMObject WEBKIT_VERSION_1_3
@property(copy) NSString *cssText;
@property(readonly) unsigned short cssValueType;
@end

@interface DOMCSSValueList : DOMCSSValue WEBKIT_VERSION_1_3
@property(readonly) unsigned length;
- (DOMCSSValue *)item:(unsigned)index;
@end

@interface DOMCSSUnknownRule : DOMCSSRule WEBKIT_VERSION_1_3
@end

@interface DOMCounter : DOMObject WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *identifier;
@property(readonly, copy) NSString *listStyle;
@property(readonly, copy) NSString *separator;
@end

@interface DOMRect : DOMObject WEBKIT_VERSION_1_3
@property(readonly, retain) DOMCSSPrimitiveValue *top;
@property(readonly, retain) DOMCSSPrimitiveValue *right;
@property(readonly, retain) DOMCSSPrimitiveValue *bottom;
@property(readonly, retain) DOMCSSPrimitiveValue *left;
@end

@interface DOMEvent : DOMObject WEBKIT_VERSION_1_3
@property(readonly, copy) NSString *type;
@property(readonly, retain) id <DOMEventTarget> target;
@property(readonly, retain) id <DOMEventTarget> currentTarget;
@property(readonly) unsigned short eventPhase;
@property(readonly) BOOL bubbles;
@property(readonly) BOOL cancelable;
@property(readonly) DOMTimeStamp timeStamp;
@property(readonly, retain) id <DOMEventTarget> srcElement AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property BOOL returnValue AVAILABLE_IN_WEBKIT_VERSION_4_0;
@property BOOL cancelBubble AVAILABLE_IN_WEBKIT_VERSION_4_0;
- (void)stopPropagation;
- (void)preventDefault;
- (void)initEvent:(NSString *)eventTypeArg canBubbleArg:(BOOL)canBubbleArg cancelableArg:(BOOL)cancelableArg AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initEvent:(NSString *)eventTypeArg :(BOOL)canBubbleArg :(BOOL)cancelableArg;
@end

@interface DOMUIEvent : DOMEvent WEBKIT_VERSION_1_3
@property(readonly, retain) DOMAbstractView *view;
@property(readonly) int detail;
@property(readonly) int keyCode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int charCode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int layerX AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
@property(readonly) int layerY AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
@property(readonly) int pageX AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int pageY AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int which AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initUIEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable view:(DOMAbstractView *)view detail:(int)detail AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initUIEvent:(NSString *)type :(BOOL)canBubble :(BOOL)cancelable :(DOMAbstractView *)view :(int)detail;
@end

@interface DOMMutationEvent : DOMEvent WEBKIT_VERSION_1_3
@property(readonly, retain) DOMNode *relatedNode;
@property(readonly, copy) NSString *prevValue;
@property(readonly, copy) NSString *newValue;
@property(readonly, copy) NSString *attrName;
@property(readonly) unsigned short attrChange;
- (void)initMutationEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable relatedNode:(DOMNode *)relatedNode prevValue:(NSString *)prevValue newValue:(NSString *)newValue attrName:(NSString *)attrName attrChange:(unsigned short)attrChange AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initMutationEvent:(NSString *)type :(BOOL)canBubble :(BOOL)cancelable :(DOMNode *)relatedNode :(NSString *)prevValue :(NSString *)newValue :(NSString *)attrName :(unsigned short)attrChange;
@end

@interface DOMOverflowEvent : DOMEvent WEBKIT_VERSION_3_0
@property(readonly) unsigned short orient;
@property(readonly) BOOL horizontalOverflow;
@property(readonly) BOOL verticalOverflow;
- (void)initOverflowEvent:(unsigned short)orient horizontalOverflow:(BOOL)horizontalOverflow verticalOverflow:(BOOL)verticalOverflow;
@end

@interface DOMWheelEvent : DOMMouseEvent WEBKIT_VERSION_3_0
@property(readonly) BOOL isHorizontal;
@property(readonly) int wheelDelta;
@property(readonly) int wheelDeltaX AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int wheelDeltaY AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initWheelEvent:(int)wheelDeltaX wheelDeltaY:(int)wheelDeltaY view:(DOMAbstractView *)view screenX:(int)screenX screenY:(int)screenY clientX:(int)clientX clientY:(int)clientY ctrlKey:(BOOL)ctrlKey altKey:(BOOL)altKey shiftKey:(BOOL)shiftKey metaKey:(BOOL)metaKey AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMKeyboardEvent : DOMUIEvent WEBKIT_VERSION_3_0
@property(readonly, copy) NSString *keyIdentifier;
@property(readonly) unsigned location AVAILABLE_AFTER_WEBKIT_VERSION_5_1;
@property(readonly) unsigned keyLocation AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
@property(readonly) BOOL ctrlKey;
@property(readonly) BOOL shiftKey;
@property(readonly) BOOL altKey;
@property(readonly) BOOL metaKey;
@property(readonly) int keyCode;
@property(readonly) int charCode;
@property(readonly) BOOL altGraphKey AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)getModifierState:(NSString *)keyIdentifierArg;
- (void)initKeyboardEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable view:(DOMAbstractView *)view keyIdentifier:(NSString *)keyIdentifier keyLocation:(unsigned)keyLocation ctrlKey:(BOOL)ctrlKey altKey:(BOOL)altKey shiftKey:(BOOL)shiftKey metaKey:(BOOL)metaKey altGraphKey:(BOOL)altGraphKey AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
- (void)initKeyboardEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable view:(DOMAbstractView *)view keyIdentifier:(NSString *)keyIdentifier keyLocation:(unsigned)keyLocation ctrlKey:(BOOL)ctrlKey altKey:(BOOL)altKey shiftKey:(BOOL)shiftKey metaKey:(BOOL)metaKey AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
- (void)initKeyboardEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable view:(DOMAbstractView *)view keyIdentifier:(NSString *)keyIdentifier location:(unsigned)location ctrlKey:(BOOL)ctrlKey altKey:(BOOL)altKey shiftKey:(BOOL)shiftKey metaKey:(BOOL)metaKey altGraphKey:(BOOL)altGraphKey AVAILABLE_AFTER_WEBKIT_VERSION_5_1;
- (void)initKeyboardEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable view:(DOMAbstractView *)view keyIdentifier:(NSString *)keyIdentifier location:(unsigned)location ctrlKey:(BOOL)ctrlKey altKey:(BOOL)altKey shiftKey:(BOOL)shiftKey metaKey:(BOOL)metaKey AVAILABLE_AFTER_WEBKIT_VERSION_5_1;
@end

@interface DOMMouseEvent : DOMUIEvent WEBKIT_VERSION_1_3
@property(readonly) int screenX;
@property(readonly) int screenY;
@property(readonly) int clientX;
@property(readonly) int clientY;
@property(readonly) BOOL ctrlKey;
@property(readonly) BOOL shiftKey;
@property(readonly) BOOL altKey;
@property(readonly) BOOL metaKey;
@property(readonly) unsigned short button;
@property(readonly, retain) id <DOMEventTarget> relatedTarget;
@property(readonly) int offsetX AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int offsetY AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int x AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) int y AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMNode *fromElement AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly, retain) DOMNode *toElement AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initMouseEvent:(NSString *)type canBubble:(BOOL)canBubble cancelable:(BOOL)cancelable view:(DOMAbstractView *)view detail:(int)detail screenX:(int)screenX screenY:(int)screenY clientX:(int)clientX clientY:(int)clientY ctrlKey:(BOOL)ctrlKey altKey:(BOOL)altKey shiftKey:(BOOL)shiftKey metaKey:(BOOL)metaKey button:(unsigned short)button relatedTarget:(id <DOMEventTarget>)relatedTarget AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)initMouseEvent:(NSString *)type :(BOOL)canBubble :(BOOL)cancelable :(DOMAbstractView *)view :(int)detail :(int)screenX :(int)screenY :(int)clientX :(int)clientY :(BOOL)ctrlKey :(BOOL)altKey :(BOOL)shiftKey :(BOOL)metaKey :(unsigned short)button :(id <DOMEventTarget>)relatedTarget;
@end

@interface DOMRange : DOMObject WEBKIT_VERSION_1_3
@property(readonly, retain) DOMNode *startContainer;
@property(readonly) int startOffset;
@property(readonly, retain) DOMNode *endContainer;
@property(readonly) int endOffset;
@property(readonly) BOOL collapsed;
@property(readonly, retain) DOMNode *commonAncestorContainer;
@property(readonly, copy) NSString *text AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)setStart:(DOMNode *)refNode offset:(int)offset AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)setStart:(DOMNode *)refNode :(int)offset;
- (void)setEnd:(DOMNode *)refNode offset:(int)offset AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)setEnd:(DOMNode *)refNode :(int)offset;
- (void)setStartBefore:(DOMNode *)refNode;
- (void)setStartAfter:(DOMNode *)refNode;
- (void)setEndBefore:(DOMNode *)refNode;
- (void)setEndAfter:(DOMNode *)refNode;
- (void)collapse:(BOOL)toStart;
- (void)selectNode:(DOMNode *)refNode;
- (void)selectNodeContents:(DOMNode *)refNode;
- (short)compareBoundaryPoints:(unsigned short)how sourceRange:(DOMRange *)sourceRange AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (short)compareBoundaryPoints:(unsigned short)how :(DOMRange *)sourceRange;
- (void)deleteContents;
- (DOMDocumentFragment *)extractContents;
- (DOMDocumentFragment *)cloneContents;
- (void)insertNode:(DOMNode *)newNode;
- (void)surroundContents:(DOMNode *)newParent;
- (DOMRange *)cloneRange;
- (NSString *)toString;
- (void)detach;
- (DOMDocumentFragment *)createContextualFragment:(NSString *)html AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)intersectsNode:(DOMNode *)refNode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (short)compareNode:(DOMNode *)refNode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (short)comparePoint:(DOMNode *)refNode offset:(int)offset AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)isPointInRange:(DOMNode *)refNode offset:(int)offset AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@end

@interface DOMNodeIterator : DOMObject WEBKIT_VERSION_1_3
@property(readonly, retain) DOMNode *root;
@property(readonly) unsigned whatToShow;
@property(readonly, retain) id <DOMNodeFilter> filter;
@property(readonly) BOOL expandEntityReferences;
@property(readonly, retain) DOMNode *referenceNode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
@property(readonly) BOOL pointerBeforeReferenceNode AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMNode *)nextNode;
- (DOMNode *)previousNode;
- (void)detach;
@end

@interface DOMMediaList : DOMObject WEBKIT_VERSION_1_3
@property(copy) NSString *mediaText;
@property(readonly) unsigned length;
- (NSString *)item:(unsigned)index;
- (void)deleteMedium:(NSString *)oldMedium;
- (void)appendMedium:(NSString *)newMedium;
@end

@interface DOMTreeWalker : DOMObject WEBKIT_VERSION_1_3
@property(readonly, retain) DOMNode *root;
@property(readonly) unsigned whatToShow;
@property(readonly, retain) id <DOMNodeFilter> filter;
@property(readonly) BOOL expandEntityReferences;
@property(retain) DOMNode *currentNode;
- (DOMNode *)parentNode;
- (DOMNode *)firstChild;
- (DOMNode *)lastChild;
- (DOMNode *)previousSibling;
- (DOMNode *)nextSibling;
- (DOMNode *)previousNode;
- (DOMNode *)nextNode;
@end

@interface DOMXPathResult : DOMObject WEBKIT_VERSION_3_0
@property(readonly) unsigned short resultType;
@property(readonly) double numberValue;
@property(readonly, copy) NSString *stringValue;
@property(readonly) BOOL booleanValue;
@property(readonly, retain) DOMNode *singleNodeValue;
@property(readonly) BOOL invalidIteratorState;
@property(readonly) unsigned snapshotLength;
- (DOMNode *)iterateNext;
- (DOMNode *)snapshotItem:(unsigned)index;
@end

@interface DOMXPathExpression : DOMObject WEBKIT_VERSION_3_0
- (DOMXPathResult *)evaluate:(DOMNode *)contextNode type:(unsigned short)type inResult:(DOMXPathResult *)inResult AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (DOMXPathResult *)evaluate:(DOMNode *)contextNode :(unsigned short)type :(DOMXPathResult *)inResult AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER_BUT_DEPRECATED;
@end

// Protocols

@protocol DOMEventListener <NSObject> WEBKIT_VERSION_1_3
- (void)handleEvent:(DOMEvent *)evt;
@end

@protocol DOMEventTarget <NSObject, NSCopying> WEBKIT_VERSION_1_3
- (void)addEventListener:(NSString *)type :(id <DOMEventListener>)listener :(BOOL)useCapture;
- (void)removeEventListener:(NSString *)type :(id <DOMEventListener>)listener :(BOOL)useCapture;
- (void)addEventListener:(NSString *)type listener:(id <DOMEventListener>)listener useCapture:(BOOL)useCapture AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (void)removeEventListener:(NSString *)type listener:(id <DOMEventListener>)listener useCapture:(BOOL)useCapture AVAILABLE_WEBKIT_VERSION_3_0_AND_LATER;
- (BOOL)dispatchEvent:(DOMEvent *)event;
@end

@protocol DOMNodeFilter <NSObject> WEBKIT_VERSION_1_3
- (short)acceptNode:(DOMNode *)n;
@end

@protocol DOMXPathNSResolver <NSObject> WEBKIT_VERSION_3_0
- (NSString *)lookupNamespaceURI:(NSString *)prefix;
@end
