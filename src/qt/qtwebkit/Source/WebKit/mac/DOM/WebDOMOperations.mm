/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
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

#import "WebDOMOperationsPrivate.h"

#import "DOMDocumentInternal.h"
#import "DOMElementInternal.h"
#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebArchiveInternal.h"
#import "WebDataSourcePrivate.h"
#import "WebFrameInternal.h"
#import "WebFrameLoaderClient.h"
#import "WebFramePrivate.h"
#import "WebKitNSStringExtras.h"
#import <JavaScriptCore/APICast.h>
#import <WebCore/Document.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/HTMLInputElement.h>
#import <WebCore/HTMLParserIdioms.h>
#import <WebCore/JSElement.h>
#import <WebCore/LegacyWebArchive.h>
#import <WebCore/markup.h>
#import <WebCore/RenderTreeAsText.h>
#import <WebCore/ShadowRoot.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/DOMHTML.h>
#import <runtime/JSLock.h>
#import <runtime/JSCJSValue.h>
#import <wtf/Assertions.h>

using namespace WebCore;
using namespace JSC;

@implementation DOMElement (WebDOMElementOperationsPrivate)

+ (DOMElement *)_DOMElementFromJSContext:(JSContextRef)context value:(JSValueRef)value
{
    if (!context)
        return 0;

    if (!value)
        return 0;

    ExecState* exec = toJS(context);
    JSLockHolder lock(exec);
    return kit(toElement(toJS(exec, value)));
}

@end

class WebFrameFilter : public WebCore::FrameFilter {
public:
    WebFrameFilter(WebArchiveSubframeFilter filterBlock);
    ~WebFrameFilter();
private:
    virtual bool shouldIncludeSubframe(Frame*) const OVERRIDE;

    WebArchiveSubframeFilter m_filterBlock;
};

WebFrameFilter::WebFrameFilter(WebArchiveSubframeFilter filterBlock)
    : m_filterBlock(Block_copy(filterBlock))
{
}

WebFrameFilter::~WebFrameFilter()
{
    Block_release(m_filterBlock);
}

bool WebFrameFilter::shouldIncludeSubframe(Frame* frame) const
{
    if (!m_filterBlock)
        return true;

    return m_filterBlock(kit(frame));
}

@implementation DOMNode (WebDOMNodeOperations)

- (WebArchive *)webArchive
{
    return [[[WebArchive alloc] _initWithCoreLegacyWebArchive:LegacyWebArchive::create(core(self))] autorelease];
}

- (WebArchive *)webArchiveByFilteringSubframes:(WebArchiveSubframeFilter)webArchiveSubframeFilter
{
    WebFrameFilter filter(webArchiveSubframeFilter);
    return [[[WebArchive alloc] _initWithCoreLegacyWebArchive:LegacyWebArchive::create(core(self), &filter)] autorelease];
}

@end

@implementation DOMNode (WebDOMNodeOperationsPendingPublic)

- (NSString *)markupString
{
    return createFullMarkup(core(self));
}

- (NSRect)_renderRect:(bool *)isReplaced
{
    return NSRect(core(self)->pixelSnappedRenderRect(isReplaced));
}

@end

@implementation DOMDocument (WebDOMDocumentOperations)

- (WebFrame *)webFrame
{
    Frame* frame = core(self)->frame();
    if (!frame)
        return nil;
    return kit(frame);
}

- (NSURL *)URLWithAttributeString:(NSString *)string
{
    return core(self)->completeURL(stripLeadingAndTrailingHTMLSpaces(string));
}

@end

@implementation DOMDocument (WebDOMDocumentOperationsInternal)

- (DOMRange *)_documentRange
{
    DOMRange *range = [self createRange];

    if (DOMNode* documentElement = [self documentElement])
        [range selectNode:documentElement];

    return range;
}

@end

@implementation DOMRange (WebDOMRangeOperations)

- (WebArchive *)webArchive
{
    return [[[WebArchive alloc] _initWithCoreLegacyWebArchive:LegacyWebArchive::create(core(self))] autorelease];
}

- (NSString *)markupString
{
    return createFullMarkup(core(self));
}

@end

@implementation DOMHTMLFrameElement (WebDOMHTMLFrameElementOperations)

- (WebFrame *)contentFrame
{
    return [[self contentDocument] webFrame];
}

@end

@implementation DOMHTMLIFrameElement (WebDOMHTMLIFrameElementOperations)

- (WebFrame *)contentFrame
{
    return [[self contentDocument] webFrame];
}

@end

@implementation DOMHTMLInputElement (WebDOMHTMLInputElementOperationsPrivate)

- (void)_setAutofilled:(BOOL)autofilled
{
    toHTMLInputElement(core((DOMElement *)self))->setAutofilled(autofilled);
}

@end

@implementation DOMHTMLObjectElement (WebDOMHTMLObjectElementOperations)

- (WebFrame *)contentFrame
{
    return [[self contentDocument] webFrame];
}

@end
