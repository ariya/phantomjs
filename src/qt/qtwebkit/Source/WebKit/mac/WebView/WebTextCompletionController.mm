/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#import "WebTextCompletionController.h"

#import "DOMNodeInternal.h"
#import "DOMRangeInternal.h"
#import "WebFrameInternal.h"
#import "WebHTMLViewInternal.h"
#import "WebTypesInternal.h"
#import "WebView.h"
#import <WebCore/Frame.h>

@interface NSWindow (WebNSWindowDetails)
- (void)_setForceActiveControls:(BOOL)flag;
@end

using namespace WebCore;

// This class handles the complete: operation.
// It counts on its host view to call endRevertingChange: whenever the current completion needs to be aborted.

// The class is in one of two modes: Popup window showing, or not.
// It is shown when a completion yields more than one match.
// If a completion yields one or zero matches, it is not shown, and there is no state carried across to the next completion.

@implementation WebTextCompletionController

- (id)initWithWebView:(WebView *)view HTMLView:(WebHTMLView *)htmlView
{
    self = [super init];
    if (!self)
        return nil;
    _view = view;
    _htmlView = htmlView;
    return self;
}

- (void)dealloc
{
    [_popupWindow release];
    [_completions release];
    [_originalString release];
    
    [super dealloc];
}

- (void)_insertMatch:(NSString *)match
{
    // FIXME: 3769654 - We should preserve case of string being inserted, even in prefix (but then also be
    // able to revert that).  Mimic NSText.
    WebFrame *frame = [_htmlView _frame];
    NSString *newText = [match substringFromIndex:prefixLength];
    [frame _replaceSelectionWithText:newText selectReplacement:YES smartReplace:NO];
}

// mostly lifted from NSTextView_KeyBinding.m
- (void)_buildUI
{
    NSRect scrollFrame = NSMakeRect(0, 0, 100, 100);
    NSRect tableFrame = NSZeroRect;    
    tableFrame.size = [NSScrollView contentSizeForFrameSize:scrollFrame.size hasHorizontalScroller:NO hasVerticalScroller:YES borderType:NSNoBorder];
    NSTableColumn *column = [[NSTableColumn alloc] init];
    [column setWidth:tableFrame.size.width];
    [column setEditable:NO];
    
    _tableView = [[NSTableView alloc] initWithFrame:tableFrame];
    [_tableView setAutoresizingMask:NSViewWidthSizable];
    [_tableView addTableColumn:column];
    [column release];
    [_tableView setGridStyleMask:NSTableViewGridNone];
    [_tableView setCornerView:nil];
    [_tableView setHeaderView:nil];
    [_tableView setColumnAutoresizingStyle:NSTableViewUniformColumnAutoresizingStyle];
    [_tableView setDelegate:self];
    [_tableView setDataSource:self];
    [_tableView setTarget:self];
    [_tableView setDoubleAction:@selector(tableAction:)];
    
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:scrollFrame];
    [scrollView setBorderType:NSNoBorder];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [scrollView setDocumentView:_tableView];
    [_tableView release];
    
    _popupWindow = [[NSWindow alloc] initWithContentRect:scrollFrame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    [_popupWindow setAlphaValue:0.88f];
    [_popupWindow setContentView:scrollView];
    [scrollView release];
    [_popupWindow setHasShadow:YES];
    [_popupWindow setOneShot:YES];
    [_popupWindow _setForceActiveControls:YES];
    [_popupWindow setReleasedWhenClosed:NO];
}

// mostly lifted from NSTextView_KeyBinding.m
- (void)_placePopupWindow:(NSPoint)topLeft
{
    int numberToShow = [_completions count];
    if (numberToShow > 20)
        numberToShow = 20;

    NSRect windowFrame;
    NSPoint wordStart = topLeft;
    windowFrame.origin = [[_view window] convertBaseToScreen:[_htmlView convertPoint:wordStart toView:nil]];
    windowFrame.size.height = numberToShow * [_tableView rowHeight] + (numberToShow + 1) * [_tableView intercellSpacing].height;
    windowFrame.origin.y -= windowFrame.size.height;
    NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont systemFontOfSize:12.0f], NSFontAttributeName, nil];
    CGFloat maxWidth = 0;
    int maxIndex = -1;
    int i;
    for (i = 0; i < numberToShow; i++) {
        float width = ceilf([[_completions objectAtIndex:i] sizeWithAttributes:attributes].width);
        if (width > maxWidth) {
            maxWidth = width;
            maxIndex = i;
        }
    }
    windowFrame.size.width = 100;
    if (maxIndex >= 0) {
        maxWidth = ceilf([NSScrollView frameSizeForContentSize:NSMakeSize(maxWidth, 100.0f) hasHorizontalScroller:NO hasVerticalScroller:YES borderType:NSNoBorder].width);
        maxWidth = ceilf([NSWindow frameRectForContentRect:NSMakeRect(0.0f, 0.0f, maxWidth, 100.0f) styleMask:NSBorderlessWindowMask].size.width);
        maxWidth += 5.0f;
        windowFrame.size.width = std::max(maxWidth, windowFrame.size.width);
    }
    [_popupWindow setFrame:windowFrame display:NO];
    
    [_tableView reloadData];
    [_tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    [_tableView scrollRowToVisible:0];
    [self _reflectSelection];
    [_popupWindow setLevel:NSPopUpMenuWindowLevel];
    [_popupWindow orderFront:nil];    
    [[_view window] addChildWindow:_popupWindow ordered:NSWindowAbove];
}

- (void)doCompletion
{
    if (!_popupWindow) {
        NSSpellChecker *checker = [NSSpellChecker sharedSpellChecker];
        if (!checker) {
            LOG_ERROR("No NSSpellChecker");
            return;
        }

        // Get preceeding word stem
        WebFrame *frame = [_htmlView _frame];
        DOMRange *selection = kit(core(frame)->selection()->toNormalizedRange().get());
        DOMRange *wholeWord = [frame _rangeByAlteringCurrentSelection:FrameSelection::AlterationExtend
            direction:DirectionBackward granularity:WordGranularity];
        DOMRange *prefix = [wholeWord cloneRange];
        [prefix setEnd:[selection startContainer] offset:[selection startOffset]];

        // Reject some NOP cases
        if ([prefix collapsed]) {
            NSBeep();
            return;
        }
        NSString *prefixStr = [frame _stringForRange:prefix];
        NSString *trimmedPrefix = [prefixStr stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
        if ([trimmedPrefix length] == 0) {
            NSBeep();
            return;
        }
        prefixLength = [prefixStr length];

        // Lookup matches
        [_completions release];
        _completions = [checker completionsForPartialWordRange:NSMakeRange(0, [prefixStr length]) inString:prefixStr language:nil inSpellDocumentWithTag:[_view spellCheckerDocumentTag]];
        [_completions retain];
    
        if (!_completions || [_completions count] == 0) {
            NSBeep();
        } else if ([_completions count] == 1) {
            [self _insertMatch:[_completions objectAtIndex:0]];
        } else {
            ASSERT(!_originalString);       // this should only be set IFF we have a popup window
            _originalString = [[frame _stringForRange:selection] retain];
            [self _buildUI];
            NSRect wordRect = [frame _caretRectAtPosition:Position(core([wholeWord startContainer]), [wholeWord startOffset], Position::PositionIsOffsetInAnchor) affinity:NSSelectionAffinityDownstream];
            // +1 to be under the word, not the caret
            // FIXME - 3769652 - Wrong positioning for right to left languages.  We should line up the upper
            // right corner with the caret instead of upper left, and the +1 would be a -1.
            NSPoint wordLowerLeft = { NSMinX(wordRect)+1, NSMaxY(wordRect) };
            [self _placePopupWindow:wordLowerLeft];
        }
    } else {
        [self endRevertingChange:YES moveLeft:NO];
    }
}

- (void)endRevertingChange:(BOOL)revertChange moveLeft:(BOOL)goLeft
{
    if (_popupWindow) {
        // tear down UI
        [[_view window] removeChildWindow:_popupWindow];
        [_popupWindow orderOut:self];
        // Must autorelease because event tracking code may be on the stack touching UI
        [_popupWindow autorelease];
        _popupWindow = nil;

        if (revertChange) {
            WebFrame *frame = [_htmlView _frame];
            [frame _replaceSelectionWithText:_originalString selectReplacement:YES smartReplace:NO];
        } else if ([_htmlView _hasSelection]) {
            if (goLeft)
                [_htmlView moveBackward:nil];
            else
                [_htmlView moveForward:nil];
        }
        [_originalString release];
        _originalString = nil;
    }
    // else there is no state to abort if the window was not up
}

- (BOOL)popupWindowIsOpen
{
    return _popupWindow != nil;
}

// WebHTMLView gives us a crack at key events it sees. Return whether we consumed the event.
// The features for the various keys mimic NSTextView.
- (BOOL)filterKeyDown:(NSEvent *)event
{
    if (!_popupWindow)
        return NO;
    NSString *string = [event charactersIgnoringModifiers];
    if (![string length])
        return NO;
    unichar c = [string characterAtIndex:0];
    if (c == NSUpArrowFunctionKey) {
        int selectedRow = [_tableView selectedRow];
        if (0 < selectedRow) {
            [_tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow - 1] byExtendingSelection:NO];
            [_tableView scrollRowToVisible:selectedRow - 1];
        }
        return YES;
    }
    if (c == NSDownArrowFunctionKey) {
        int selectedRow = [_tableView selectedRow];
        if (selectedRow < (int)[_completions count] - 1) {
            [_tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow + 1] byExtendingSelection:NO];
            [_tableView scrollRowToVisible:selectedRow + 1];
        }
        return YES;
    }
    if (c == NSRightArrowFunctionKey || c == '\n' || c == '\r' || c == '\t') {
        // FIXME: What about backtab?
        [self endRevertingChange:NO moveLeft:NO];
        return YES;
    }
    if (c == NSLeftArrowFunctionKey) {
        [self endRevertingChange:NO moveLeft:YES];
        return YES;
    }
    if (c == 0x1B || c == NSF5FunctionKey) {
        // FIXME: F5?
        [self endRevertingChange:YES moveLeft:NO];
        return YES;
    }
    if (c == ' ' || (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) || (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7D)) {
        // FIXME: Is the above list of keys really definitive?
        // Originally this code called ispunct; aren't there other punctuation keys on international keyboards?
        [self endRevertingChange:NO moveLeft:NO];
        return NO; // let the char get inserted
    }
    return NO;
}

- (void)_reflectSelection
{
    int selectedRow = [_tableView selectedRow];
    ASSERT(selectedRow >= 0);
    ASSERT(selectedRow < (int)[_completions count]);
    [self _insertMatch:[_completions objectAtIndex:selectedRow]];
}

- (void)tableAction:(id)sender
{
    [self _reflectSelection];
    [self endRevertingChange:NO moveLeft:NO];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return [_completions count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    return [_completions objectAtIndex:row];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    [self _reflectSelection];
}

@end
