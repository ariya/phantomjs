/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "PlatformUtilities.h"
#import <wtf/RetainPtr.h>

@interface InspectorBarController : NSObject {
}
@end

static bool didFinishLoad;

@implementation InspectorBarController

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

- (NSDictionary *)convertAttributes:(NSDictionary *)dictionary
{
    NSMutableDictionary *newDictionary = [dictionary mutableCopy];
    [newDictionary removeObjectForKey:NSForegroundColorAttributeName];
    return [newDictionary autorelease];
}
@end

namespace TestWebKitAPI {

TEST(WebKit1, InspectorBarTest)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<InspectorBarController> inspectorBarController = adoptNS([InspectorBarController new]);

    webView.get().frameLoadDelegate = inspectorBarController.get();
    [webView.get().mainFrame loadHTMLString:@"<body contenteditable style=\"color: green\"><u>Lorem ipsum sit amet</u></body>" baseURL:[NSURL URLWithString:@"about:blank"]];

    Util::run(&didFinishLoad);

    DOMDocument *document = webView.get().mainFrameDocument;
    [[document body] focus];
    
    EXPECT_TRUE([webView.get() respondsToSelector:@selector(typingAttributes)]);
    NSDictionary *attributes = [(id)webView.get() typingAttributes];
    [(id)[[[webView.get() mainFrame] frameView] documentView] doCommandBySelector:@selector(bold:)];
    EXPECT_FALSE([attributes isEqual:[(id)webView.get() typingAttributes]]);
    
    [webView.get() selectAll:nil];
    NSAttributedString *attrString = [(NSView <NSTextInput> *)[[[webView.get() mainFrame] frameView] documentView] attributedSubstringFromRange:NSMakeRange(0, 5)];
    attributes = [attrString attributesAtIndex:0 effectiveRange:0];
    
    EXPECT_TRUE([[attributes objectForKey:NSUnderlineStyleAttributeName] intValue] != 0);

    [webView.get() changeAttributes:inspectorBarController.get()];
    
    DOMNode *currentNode = [document body];
    while ([[currentNode firstChild] nodeType] != DOM_TEXT_NODE)
        currentNode = [currentNode firstChild];

    DOMCSSStyleDeclaration *style = [document getComputedStyle:(DOMElement *)currentNode pseudoElement:nil];
    EXPECT_WK_STREQ(@"rgb(0, 0, 0)", [style color]);
}

} // namespace TestWebKitAPI
