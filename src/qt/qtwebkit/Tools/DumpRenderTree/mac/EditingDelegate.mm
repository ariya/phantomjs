/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
 
#import "config.h"
#import "EditingDelegate.h"

#import "DumpRenderTree.h"
#import "TestRunner.h"
#import <WebKit/WebKit.h>

@interface DOMNode (dumpPath)
- (NSString *)dumpPath;
@end

@implementation DOMNode (dumpPath)
- (NSString *)dumpPath
{
    DOMNode *parent = [self parentNode];
    NSString *str = [NSString stringWithFormat:@"%@", [self nodeName]];
    if (parent != nil) {
        str = [str stringByAppendingString:@" > "];
        str = [str stringByAppendingString:[parent dumpPath]];
    }
    return str;
}
@end

@interface DOMRange (dump)
- (NSString *)dump;
@end

@implementation DOMRange (dump)
- (NSString *)dump
{
    return [NSString stringWithFormat:@"range from %d of %@ to %d of %@", [self startOffset], [[self startContainer] dumpPath], [self endOffset], [[self endContainer] dumpPath]];
}
@end

@implementation EditingDelegate

- (id)init
{
    self = [super init];
    if (!self)
        return nil;
    acceptsEditing = YES;
    return self;
}

- (BOOL)webView:(WebView *)webView shouldBeginEditingInDOMRange:(DOMRange *)range
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldBeginEditingInDOMRange:%s\n", [[range dump] UTF8String]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldEndEditingInDOMRange:(DOMRange *)range
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldEndEditingInDOMRange:%s\n", [[range dump] UTF8String]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldInsertNode:(DOMNode *)node replacingDOMRange:(DOMRange *)range givenAction:(WebViewInsertAction)action
{
    static const char *insertactionstring[] = {
        "WebViewInsertActionTyped",
        "WebViewInsertActionPasted",
        "WebViewInsertActionDropped",
    };

    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldInsertNode:%s replacingDOMRange:%s givenAction:%s\n", [[node dumpPath] UTF8String], [[range dump] UTF8String], insertactionstring[action]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldInsertText:(NSString *)text replacingDOMRange:(DOMRange *)range givenAction:(WebViewInsertAction)action
{
    static const char *insertactionstring[] = {
        "WebViewInsertActionTyped",
        "WebViewInsertActionPasted",
        "WebViewInsertActionDropped",
    };

    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:%s givenAction:%s\n", [[text description] UTF8String], [[range dump] UTF8String], insertactionstring[action]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldDeleteDOMRange:(DOMRange *)range
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldDeleteDOMRange:%s\n", [[range dump] UTF8String]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldShowDeleteInterfaceForElement:(DOMHTMLElement *)element
{
    return [[element className] isEqualToString:@"needsDeletionUI"];
}

- (BOOL)webView:(WebView *)webView shouldChangeSelectedDOMRange:(DOMRange *)currentRange toDOMRange:(DOMRange *)proposedRange affinity:(NSSelectionAffinity)selectionAffinity stillSelecting:(BOOL)flag
{
    static const char *affinitystring[] = {
        "NSSelectionAffinityUpstream",
        "NSSelectionAffinityDownstream"
    };
    static const char *boolstring[] = {
        "FALSE",
        "TRUE"
    };

    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldChangeSelectedDOMRange:%s toDOMRange:%s affinity:%s stillSelecting:%s\n", [[currentRange dump] UTF8String], [[proposedRange dump] UTF8String], affinitystring[selectionAffinity], boolstring[flag]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldApplyStyle:(DOMCSSStyleDeclaration *)style toElementsInDOMRange:(DOMRange *)range
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldApplyStyle:%s toElementsInDOMRange:%s\n", [[style description] UTF8String], [[range dump] UTF8String]);
    return acceptsEditing;
}

- (BOOL)webView:(WebView *)webView shouldChangeTypingStyle:(DOMCSSStyleDeclaration *)currentStyle toStyle:(DOMCSSStyleDeclaration *)proposedStyle
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldChangeTypingStyle:%s toStyle:%s\n", [[currentStyle description] UTF8String], [[proposedStyle description] UTF8String]);
    return acceptsEditing;
}

- (void)webViewDidBeginEditing:(NSNotification *)notification
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidBeginEditing:%s\n", [[notification name] UTF8String]);
}

- (void)webViewDidChange:(NSNotification *)notification
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChange:%s\n", [[notification name] UTF8String]);
}

- (void)webViewDidEndEditing:(NSNotification *)notification
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidEndEditing:%s\n", [[notification name] UTF8String]);
}

- (void)webViewDidChangeTypingStyle:(NSNotification *)notification
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChangeTypingStyle:%s\n", [[notification name] UTF8String]);
}

- (void)webViewDidChangeSelection:(NSNotification *)notification
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChangeSelection:%s\n", [[notification name] UTF8String]);
}

- (void)setAcceptsEditing:(BOOL)newAcceptsEditing
{
    acceptsEditing = newAcceptsEditing;
}

@end
