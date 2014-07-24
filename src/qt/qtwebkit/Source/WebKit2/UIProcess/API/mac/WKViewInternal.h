/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import "PluginComplexTextInputState.h"
#import "WKView.h"
#import "WebFindOptions.h"
#import <wtf/Forward.h>
#import <wtf/Vector.h>

namespace CoreIPC {
    class DataReference;
}

namespace WebCore {
    struct KeypressCommand;
    class Image;
    class SharedBuffer;
}

namespace WebKit {
    class DrawingAreaProxy;
    class FindIndicator;
    class LayerTreeContext;
    struct ColorSpaceData;
    struct EditorState;
}

@class WKFullScreenWindowController;

@interface WKView (Internal)
- (PassOwnPtr<WebKit::DrawingAreaProxy>)_createDrawingAreaProxy;
- (BOOL)_isFocused;
- (void)_processDidCrash;
- (void)_pageClosed;
- (void)_didRelaunchProcess;
- (void)_preferencesDidChange;
- (void)_toolTipChangedFrom:(NSString *)oldToolTip to:(NSString *)newToolTip;
- (void)_setCursor:(NSCursor *)cursor;
- (void)_setUserInterfaceItemState:(NSString *)commandName enabled:(BOOL)isEnabled state:(int)newState;
- (BOOL)_interpretKeyEvent:(NSEvent *)theEvent savingCommandsTo:(Vector<WebCore::KeypressCommand>&)commands;
- (void)_doneWithKeyEvent:(NSEvent *)event eventWasHandled:(BOOL)eventWasHandled;
- (bool)_executeSavedCommandBySelector:(SEL)selector;
- (void)_setIntrinsicContentSize:(NSSize)intrinsicContentSize;
- (NSRect)_convertToDeviceSpace:(NSRect)rect;
- (NSRect)_convertToUserSpace:(NSRect)rect;
- (void)_setFindIndicator:(PassRefPtr<WebKit::FindIndicator>)findIndicator fadeOut:(BOOL)fadeOut animate:(BOOL)animate;

- (void)_setAcceleratedCompositingModeRootLayer:(CALayer *)rootLayer;

- (void)_setAccessibilityWebProcessToken:(NSData *)data;

- (void)_pluginFocusOrWindowFocusChanged:(BOOL)pluginHasFocusAndWindowHasFocus pluginComplexTextInputIdentifier:(uint64_t)pluginComplexTextInputIdentifier;
- (void)_setPluginComplexTextInputState:(WebKit::PluginComplexTextInputState)pluginComplexTextInputState pluginComplexTextInputIdentifier:(uint64_t)pluginComplexTextInputIdentifier;

- (void)_setDragImage:(NSImage *)image at:(NSPoint)clientPoint linkDrag:(BOOL)linkDrag;
- (void)_setPromisedData:(WebCore::Image *)image withFileName:(NSString *)filename withExtension:(NSString *)extension withTitle:(NSString *)title withURL:(NSString *)url withVisibleURL:(NSString *)visibleUrl withArchive:(WebCore::SharedBuffer*) archiveBuffer forPasteboard:(NSString *)pasteboardName;
- (void)_updateSecureInputState;
- (void)_resetSecureInputState;
- (void)_notifyInputContextAboutDiscardedComposition;

- (WebKit::ColorSpaceData)_colorSpace;

#if ENABLE(FULLSCREEN_API)
- (BOOL)hasFullScreenWindowController;
- (WKFullScreenWindowController*)fullScreenWindowController;
- (void)closeFullScreenWindowController;
#endif

- (void)_cacheWindowBottomCornerRect;

- (NSInteger)spellCheckerDocumentTag;
- (void)handleAcceptedAlternativeText:(NSString*)text;

- (void)_setSuppressVisibilityUpdates:(BOOL)suppressVisibilityUpdates;
- (BOOL)_suppressVisibilityUpdates;

- (BOOL)_isWindowOccluded;

@end
