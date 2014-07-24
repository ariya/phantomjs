/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#import "WebHTMLRepresentation.h"

#import "DOMElementInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebArchive.h"
#import "WebBasePluginPackage.h"
#import "WebDataSourceInternal.h"
#import "WebDocumentPrivate.h"
#import "WebFrameInternal.h"
#import "WebKitNSStringExtras.h"
#import "WebKitStatisticsPrivate.h"
#import "WebNSObjectExtras.h"
#import "WebTypesInternal.h"
#import "WebView.h"
#import <Foundation/NSURLResponse.h>
#import <WebCore/Document.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/Editor.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameLoaderClient.h>
#import <WebCore/HTMLConverter.h>
#import <WebCore/HTMLFormControlElement.h>
#import <WebCore/HTMLFormElement.h>
#import <WebCore/HTMLInputElement.h>
#import <WebCore/HTMLNames.h>
#import <WebCore/HTMLTableCellElement.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/NodeTraversal.h>
#import <WebCore/Range.h>
#import <WebCore/RegularExpression.h>
#import <WebCore/RenderObject.h>
#import <WebCore/TextResourceDecoder.h>
#import <WebKit/DOMHTMLInputElement.h>
#import <wtf/Assertions.h>
#import <wtf/StdLibExtras.h>

using namespace WebCore;
using namespace HTMLNames;

@interface WebHTMLRepresentationPrivate : NSObject {
@public
    WebDataSource *dataSource;
    
    BOOL hasSentResponseToPlugin;
    BOOL includedInWebKitStatistics;

    id <WebPluginManualLoader> manualLoader;
    NSView *pluginView;
}
@end

@implementation WebHTMLRepresentationPrivate
@end

@implementation WebHTMLRepresentation

static NSMutableArray *newArrayWithStrings(const HashSet<String>& set) NS_RETURNS_RETAINED;
static NSMutableArray *newArrayWithStrings(const HashSet<String>& set)
{
    NSMutableArray *array = [[NSMutableArray alloc] initWithCapacity:set.size()];
    HashSet<String>::const_iterator end = set.end();
    for (HashSet<String>::const_iterator it = set.begin(); it != end; ++it)
        [array addObject:(NSString *)(*it)];
    return array;
}

static NSMutableArray *newArrayByConcatenatingArrays(NSArray *first, NSArray *second) NS_RETURNS_RETAINED;
static NSMutableArray *newArrayByConcatenatingArrays(NSArray *first, NSArray *second)
{
    NSMutableArray *result = [first mutableCopy];
    [result addObjectsFromArray:second];
    return result;
}

+ (NSArray *)supportedMIMETypes
{
    static __unsafe_unretained NSArray *staticSupportedMIMETypes = newArrayByConcatenatingArrays([self supportedNonImageMIMETypes], [self supportedImageMIMETypes]);
    return staticSupportedMIMETypes;
}

+ (NSArray *)supportedNonImageMIMETypes
{
    static __unsafe_unretained NSArray *staticSupportedNonImageMIMETypes = newArrayWithStrings(MIMETypeRegistry::getSupportedNonImageMIMETypes());
    return staticSupportedNonImageMIMETypes;
}

+ (NSArray *)supportedImageMIMETypes
{
    static __unsafe_unretained NSArray *staticSupportedImageMIMETypes = newArrayWithStrings(MIMETypeRegistry::getSupportedImageMIMETypes());
    return staticSupportedImageMIMETypes;
}

+ (NSArray *)unsupportedTextMIMETypes
{
    static __unsafe_unretained NSArray *staticUnsupportedTextMIMETypes = newArrayWithStrings(MIMETypeRegistry::getUnsupportedTextMIMETypes());
    return staticUnsupportedTextMIMETypes;
}

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    
    _private = [[WebHTMLRepresentationPrivate alloc] init];

    return self;
}

- (void)dealloc
{
    if (_private && _private->includedInWebKitStatistics)
        --WebHTMLRepresentationCount;

    [_private release];

    [super dealloc];
}

- (void)finalize
{
    if (_private && _private->includedInWebKitStatistics)
        --WebHTMLRepresentationCount;

    [super finalize];
}

- (void)_redirectDataToManualLoader:(id<WebPluginManualLoader>)manualLoader forPluginView:(NSView *)pluginView
{
    _private->manualLoader = manualLoader;
    _private->pluginView = pluginView;
}

- (void)setDataSource:(WebDataSource *)dataSource
{
    _private->dataSource = dataSource;

    if (!_private->includedInWebKitStatistics && [[dataSource webFrame] _isIncludedInWebKitStatistics]) {
        _private->includedInWebKitStatistics = YES;
        ++WebHTMLRepresentationCount;
    }
}

- (BOOL)_isDisplayingWebArchive
{
    return [[_private->dataSource _responseMIMEType] _webkit_isCaseInsensitiveEqualToString:@"application/x-webarchive"];
}

- (void)receivedData:(NSData *)data withDataSource:(WebDataSource *)dataSource
{
    WebFrame *webFrame = [dataSource webFrame];
    if (!webFrame)
        return;

    if (!_private->pluginView)
        [webFrame _commitData:data];

    // If the document is a stand-alone media document, now is the right time to cancel the WebKit load
    Frame* coreFrame = core(webFrame);
    if (coreFrame->document()->isMediaDocument())
        coreFrame->loader()->documentLoader()->cancelMainResourceLoad(coreFrame->loader()->client()->pluginWillHandleLoadError(coreFrame->loader()->documentLoader()->response()));

    if (_private->pluginView) {
        if (!_private->hasSentResponseToPlugin) {
            [_private->manualLoader pluginView:_private->pluginView receivedResponse:[dataSource response]];
            _private->hasSentResponseToPlugin = YES;
        }
        
        [_private->manualLoader pluginView:_private->pluginView receivedData:data];
    }
}

- (void)receivedError:(NSError *)error withDataSource:(WebDataSource *)dataSource
{
    if (_private->pluginView) {
        [_private->manualLoader pluginView:_private->pluginView receivedError:error];
    }
}

- (void)finishedLoadingWithDataSource:(WebDataSource *)dataSource
{
    WebFrame* webFrame = [dataSource webFrame];

    if (_private->pluginView) {
        [_private->manualLoader pluginViewFinishedLoading:_private->pluginView];
        return;
    }

    if (!webFrame)
        return;
    WebView *webView = [webFrame webView];
    if ([webView mainFrame] == webFrame && [webView isEditable])
        core(webFrame)->editor().applyEditingStyleToBodyElement();
}

- (BOOL)canProvideDocumentSource
{
    return [[_private->dataSource webFrame] _canProvideDocumentSource];
}

- (BOOL)canSaveAsWebArchive
{
    return [[_private->dataSource webFrame] _canSaveAsWebArchive];
}

- (NSString *)documentSource
{
    if ([self _isDisplayingWebArchive]) {            
        SharedBuffer *parsedArchiveData = [_private->dataSource _documentLoader]->parsedArchiveData();
        NSData *nsData = parsedArchiveData ? parsedArchiveData->createNSData() : nil;
        NSString *result = [[NSString alloc] initWithData:nsData encoding:NSUTF8StringEncoding];
        [nsData release];
        return [result autorelease];
    }

    Frame* coreFrame = core([_private->dataSource webFrame]);
    if (!coreFrame)
        return nil;
    Document* document = coreFrame->document();
    if (!document)
        return nil;
    TextResourceDecoder* decoder = document->decoder();
    if (!decoder)
        return nil;
    NSData *data = [_private->dataSource data];
    if (!data)
        return nil;
    return decoder->encoding().decode(reinterpret_cast<const char*>([data bytes]), [data length]);
}

- (NSString *)title
{
    return nsStringNilIfEmpty([_private->dataSource _documentLoader]->title().string());
}

- (DOMDocument *)DOMDocument
{
    return [[_private->dataSource webFrame] DOMDocument];
}

- (NSAttributedString *)attributedText
{
    // FIXME: Implement
    return nil;
}

- (NSAttributedString *)attributedStringFrom:(DOMNode *)startNode startOffset:(int)startOffset to:(DOMNode *)endNode endOffset:(int)endOffset
{
    return [WebHTMLConverter editingAttributedStringFromRange:Range::create(core(startNode)->document(), core(startNode), startOffset, core(endNode), endOffset).get()];
}

static HTMLFormElement* formElementFromDOMElement(DOMElement *element)
{
    Element* node = core(element);
    return node && node->hasTagName(formTag) ? static_cast<HTMLFormElement*>(node) : 0;
}

- (DOMElement *)elementWithName:(NSString *)name inForm:(DOMElement *)form
{
    HTMLFormElement* formElement = formElementFromDOMElement(form);
    if (!formElement)
        return nil;
    const Vector<FormAssociatedElement*>& elements = formElement->associatedElements();
    AtomicString targetName = name;
    for (unsigned i = 0; i < elements.size(); i++) {
        FormAssociatedElement* elt = elements[i];
        if (elt->name() == targetName)
            return kit(toHTMLElement(elt));
    }
    return nil;
}

static HTMLInputElement* inputElementFromDOMElement(DOMElement* element)
{
    Element* node = core(element);
    return node && isHTMLInputElement(node) ? toHTMLInputElement(node) : 0;
}

- (BOOL)elementDoesAutoComplete:(DOMElement *)element
{
    HTMLInputElement* inputElement = inputElementFromDOMElement(element);
    return inputElement
        && inputElement->isTextField()
        && !inputElement->isPasswordField()
        && inputElement->shouldAutocomplete();
}

- (BOOL)elementIsPassword:(DOMElement *)element
{
    HTMLInputElement* inputElement = inputElementFromDOMElement(element);
    return inputElement && inputElement->isPasswordField();
}

- (DOMElement *)formForElement:(DOMElement *)element
{
    HTMLInputElement* inputElement = inputElementFromDOMElement(element);
    return inputElement ? kit(inputElement->form()) : 0;
}

- (DOMElement *)currentForm
{
    return kit(core([_private->dataSource webFrame])->selection()->currentForm());
}

- (NSArray *)controlsInForm:(DOMElement *)form
{
    HTMLFormElement* formElement = formElementFromDOMElement(form);
    if (!formElement)
        return nil;
    NSMutableArray *results = nil;
    const Vector<FormAssociatedElement*>& elements = formElement->associatedElements();
    for (unsigned i = 0; i < elements.size(); i++) {
        if (elements[i]->isEnumeratable()) { // Skip option elements, other duds
            DOMElement* de = kit(toHTMLElement(elements[i]));
            if (!results)
                results = [NSMutableArray arrayWithObject:de];
            else
                [results addObject:de];
        }
    }
    return results;
}

// Either get cached regexp or build one that matches any of the labels.
// The regexp we build is of the form:  (STR1|STR2|STRN)
static RegularExpression* regExpForLabels(NSArray *labels)
{
    // All the ObjC calls in this method are simple array and string
    // calls which we can assume do not raise exceptions

    // Parallel arrays that we use to cache regExps.  In practice the number of expressions
    // that the app will use is equal to the number of locales is used in searching.
    static const unsigned int regExpCacheSize = 4;
    static NSMutableArray* regExpLabels = nil;
    DEFINE_STATIC_LOCAL(Vector<RegularExpression*>, regExps, ());
    DEFINE_STATIC_LOCAL(RegularExpression, wordRegExp, ("\\w", TextCaseSensitive));

    RegularExpression* result;
    if (!regExpLabels)
        regExpLabels = [[NSMutableArray alloc] initWithCapacity:regExpCacheSize];
    CFIndex cacheHit = [regExpLabels indexOfObject:labels];
    if (cacheHit != NSNotFound)
        result = regExps.at(cacheHit);
    else {
        String pattern("(");
        unsigned int numLabels = [labels count];
        unsigned int i;
        for (i = 0; i < numLabels; i++) {
            String label = [labels objectAtIndex:i];

            bool startsWithWordChar = false;
            bool endsWithWordChar = false;
            if (label.length() != 0) {
                startsWithWordChar = wordRegExp.match(label.substring(0, 1)) >= 0;
                endsWithWordChar = wordRegExp.match(label.substring(label.length() - 1, 1)) >= 0;
            }
            
            if (i != 0)
                pattern.append("|");
            // Search for word boundaries only if label starts/ends with "word characters".
            // If we always searched for word boundaries, this wouldn't work for languages
            // such as Japanese.
            if (startsWithWordChar)
                pattern.append("\\b");
            pattern.append(label);
            if (endsWithWordChar)
                pattern.append("\\b");
        }
        pattern.append(")");
        result = new RegularExpression(pattern, TextCaseInsensitive);
    }

    // add regexp to the cache, making sure it is at the front for LRU ordering
    if (cacheHit != 0) {
        if (cacheHit != NSNotFound) {
            // remove from old spot
            [regExpLabels removeObjectAtIndex:cacheHit];
            regExps.remove(cacheHit);
        }
        // add to start
        [regExpLabels insertObject:labels atIndex:0];
        regExps.insert(0, result);
        // trim if too big
        if ([regExpLabels count] > regExpCacheSize) {
            [regExpLabels removeObjectAtIndex:regExpCacheSize];
            RegularExpression* last = regExps.last();
            regExps.removeLast();
            delete last;
        }
    }
    return result;
}

static NSString* searchForLabelsBeforeElement(Frame* frame, NSArray* labels, Element* element, size_t* resultDistance, bool* resultIsInCellAbove)
{
    RegularExpression* regExp = regExpForLabels(labels);
    // We stop searching after we've seen this many chars
    const unsigned int charsSearchedThreshold = 500;
    // This is the absolute max we search.  We allow a little more slop than
    // charsSearchedThreshold, to make it more likely that we'll search whole nodes.
    const unsigned int maxCharsSearched = 600;
    // If the starting element is within a table, the cell that contains it
    HTMLTableCellElement* startingTableCell = 0;
    bool searchedCellAbove = false;
    
    if (resultDistance)
        *resultDistance = notFound;
    if (resultIsInCellAbove)
        *resultIsInCellAbove = false;

    // walk backwards in the node tree, until another element, or form, or end of tree
    unsigned lengthSearched = 0;
    Node* n;
    for (n = NodeTraversal::previous(element);
         n && lengthSearched < charsSearchedThreshold;
         n = NodeTraversal::previous(n))
    {
        if (n->hasTagName(formTag)
            || (n->isHTMLElement() && toElement(n)->isFormControlElement()))
        {
            // We hit another form element or the start of the form - bail out
            break;
        } else if (n->hasTagName(tdTag) && !startingTableCell) {
            startingTableCell = static_cast<HTMLTableCellElement*>(n);
        } else if (n->hasTagName(trTag) && startingTableCell) {
            NSString* result = frame->searchForLabelsAboveCell(regExp, startingTableCell, resultDistance);
            if (result && [result length] > 0) {
                if (resultIsInCellAbove)
                    *resultIsInCellAbove = true;
                return result;
            }
            searchedCellAbove = true;
        } else if (n->isTextNode() && n->renderer() && n->renderer()->style()->visibility() == VISIBLE) {
            // For each text chunk, run the regexp
            String nodeString = n->nodeValue();
            // add 100 for slop, to make it more likely that we'll search whole nodes
            if (lengthSearched + nodeString.length() > maxCharsSearched)
                nodeString = nodeString.right(charsSearchedThreshold - lengthSearched);
            int pos = regExp->searchRev(nodeString);
            if (pos >= 0) {
                if (resultDistance)
                    *resultDistance = lengthSearched;
                return nodeString.substring(pos, regExp->matchedLength());
            }
            lengthSearched += nodeString.length();
        }
    }

    // If we started in a cell, but bailed because we found the start of the form or the
    // previous element, we still might need to search the row above us for a label.
    if (startingTableCell && !searchedCellAbove) {
        NSString* result = frame->searchForLabelsAboveCell(regExp, startingTableCell, resultDistance);
        if (result && [result length] > 0) {
            if (resultIsInCellAbove)
                *resultIsInCellAbove = true;
            return result;
        }
    }
    
    return nil;
}

static NSString *matchLabelsAgainstString(NSArray *labels, const String& stringToMatch)
{
    if (stringToMatch.isEmpty())
        return nil;
    
    String mutableStringToMatch = stringToMatch;
    
    // Make numbers and _'s in field names behave like word boundaries, e.g., "address2"
    replace(mutableStringToMatch, RegularExpression("\\d", TextCaseSensitive), " ");
    mutableStringToMatch.replace('_', ' ');
    
    RegularExpression* regExp = regExpForLabels(labels);
    // Use the largest match we can find in the whole string
    int pos;
    int length;
    int bestPos = -1;
    int bestLength = -1;
    int start = 0;
    do {
        pos = regExp->match(mutableStringToMatch, start);
        if (pos != -1) {
            length = regExp->matchedLength();
            if (length >= bestLength) {
                bestPos = pos;
                bestLength = length;
            }
            start = pos + 1;
        }
    } while (pos != -1);
    
    if (bestPos != -1)
        return mutableStringToMatch.substring(bestPos, bestLength);
    return nil;
}

static NSString* matchLabelsAgainstElement(NSArray* labels, Element* element)
{
    // Match against the name element, then against the id element if no match is found for the name element.
    // See 7538330 for one popular site that benefits from the id element check.
    String resultFromNameAttribute = matchLabelsAgainstString(labels, element->getAttribute(nameAttr));
    if (!resultFromNameAttribute.isEmpty())
        return resultFromNameAttribute;
    
    return matchLabelsAgainstString(labels, element->getAttribute(idAttr));
}


- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element
{
    return [self searchForLabels:labels beforeElement:element resultDistance:0 resultIsInCellAbove:0];
}

- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element resultDistance:(NSUInteger*)outDistance resultIsInCellAbove:(BOOL*)outIsInCellAbove
{
    size_t distance;
    bool isInCellAbove;
    
    NSString *result = searchForLabelsBeforeElement(core([_private->dataSource webFrame]), labels, core(element), &distance, &isInCellAbove);
    
    if (outDistance) {
        if (distance == notFound)
            *outDistance = NSNotFound;
        else
            *outDistance = distance;
    }

    if (outIsInCellAbove)
        *outIsInCellAbove = isInCellAbove;
    
    return result;
}

- (NSString *)matchLabels:(NSArray *)labels againstElement:(DOMElement *)element
{
    return matchLabelsAgainstElement(labels, core(element));
}

@end
