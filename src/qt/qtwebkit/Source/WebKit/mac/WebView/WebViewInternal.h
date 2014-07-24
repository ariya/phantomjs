/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L
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

// This header contains WebView declarations that can be used anywhere in WebKit, but are neither SPI nor API.

#import "WebPreferences.h"
#import "WebViewPrivate.h"
#import "WebTypesInternal.h"

#ifdef __cplusplus
#import <WebCore/AlternativeTextClient.h>
#import <WebCore/FindOptions.h>
#import <WebCore/FloatRect.h>
#import <WebCore/LayoutMilestones.h>
#import <WebCore/TextAlternativeWithRange.h>
#import <WebCore/WebCoreKeyboardUIMode.h>

#include <wtf/Forward.h>
#include <wtf/RetainPtr.h>

namespace WebCore {
class Element;
class Event;
class Frame;
class HistoryItem;
class KURL;
class KeyboardEvent;
class Page;
class RenderBox;
class Node;
struct DictationAlternative;
}
#endif

@class WebBasePluginPackage;
@class WebDownload;
@class WebNodeHighlight;

#ifdef __cplusplus

WebCore::FindOptions coreOptions(WebFindOptions options);

WebCore::LayoutMilestones coreLayoutMilestones(WebLayoutMilestones);
WebLayoutMilestones kitLayoutMilestones(WebCore::LayoutMilestones);

#if USE(DICTATION_ALTERNATIVES)
OBJC_CLASS NSTextAlternatives;
#endif

@interface WebView (WebViewEditingExtras)
- (BOOL)_shouldChangeSelectedDOMRange:(DOMRange *)currentRange toDOMRange:(DOMRange *)proposedRange affinity:(NSSelectionAffinity)selectionAffinity stillSelecting:(BOOL)flag;
@end

@interface WebView (AllWebViews)
+ (void)_makeAllWebViewsPerformSelector:(SEL)selector;
- (void)_removeFromAllWebViewsSet;
- (void)_addToAllWebViewsSet;
@end

@interface WebView (WebViewInternal)

+ (BOOL)shouldIncludeInWebKitStatistics;

- (WebCore::Frame*)_mainCoreFrame;
- (WebFrame *)_selectedOrMainFrame;

- (WebCore::KeyboardUIMode)_keyboardUIMode;

- (BOOL)_becomingFirstResponderFromOutside;

#if ENABLE(ICONDATABASE)
- (void)_registerForIconNotification:(BOOL)listen;
- (void)_dispatchDidReceiveIconFromWebFrame:(WebFrame *)webFrame;
#endif

#if USE(ACCELERATED_COMPOSITING)
- (BOOL)_needsOneShotDrawingSynchronization;
- (void)_setNeedsOneShotDrawingSynchronization:(BOOL)needsSynchronization;
- (void)_scheduleCompositingLayerFlush;
#endif

#if USE(GLIB)
- (void)_scheduleGlibContextIterations;
#endif

#if USE(AUTOCORRECTION_PANEL)
- (void)handleAcceptedAlternativeText:(NSString*)text;
#endif

#if USE(DICTATION_ALTERNATIVES)
- (void)_getWebCoreDictationAlternatives:(Vector<WebCore::DictationAlternative>&)alternatives fromTextAlternatives:(const Vector<WebCore::TextAlternativeWithRange>&)alternativesWithRange;
- (void)_showDictationAlternativeUI:(const WebCore::FloatRect&)boundingBoxOfDictatedText forDictationContext:(uint64_t)dictationContext;
- (void)_removeDictationAlternatives:(uint64_t)dictationContext;
- (Vector<String>)_dictationAlternatives:(uint64_t)dictationContext;
#endif

@end

#endif

// FIXME: Temporary way to expose methods that are in the wrong category inside WebView.
@interface WebView (WebViewOtherInternal)

+ (void)_setCacheModel:(WebCacheModel)cacheModel;
+ (WebCacheModel)_cacheModel;

#ifdef __cplusplus
- (WebCore::Page*)page;
- (void)_setGlobalHistoryItem:(WebCore::HistoryItem*)historyItem;
- (WTF::String)_userAgentString;
#endif

- (NSMenu *)_menuForElement:(NSDictionary *)element defaultItems:(NSArray *)items;
- (id)_UIDelegateForwarder;
- (id)_editingDelegateForwarder;
- (id)_policyDelegateForwarder;
- (void)_pushPerformingProgrammaticFocus;
- (void)_popPerformingProgrammaticFocus;
- (void)_didStartProvisionalLoadForFrame:(WebFrame *)frame;
+ (BOOL)_viewClass:(Class *)vClass andRepresentationClass:(Class *)rClass forMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins;
- (BOOL)_viewClass:(Class *)vClass andRepresentationClass:(Class *)rClass forMIMEType:(NSString *)MIMEType;
+ (void)_registerPluginMIMEType:(NSString *)MIMEType;
+ (void)_unregisterPluginMIMEType:(NSString *)MIMEType;
+ (BOOL)_canShowMIMEType:(NSString *)MIMEType allowingPlugins:(BOOL)allowPlugins;
- (BOOL)_canShowMIMEType:(NSString *)MIMEType;
+ (NSString *)_MIMETypeForFile:(NSString *)path;
- (WebDownload *)_downloadURL:(NSURL *)URL;
+ (NSString *)_generatedMIMETypeForURLScheme:(NSString *)URLScheme;
+ (BOOL)_representationExistsForURLScheme:(NSString *)URLScheme;
- (BOOL)_isPerformingProgrammaticFocus;
- (void)_mouseDidMoveOverElement:(NSDictionary *)dictionary modifierFlags:(NSUInteger)modifierFlags;
- (WebView *)_openNewWindowWithRequest:(NSURLRequest *)request;
- (void)_writeImageForElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard;
- (void)_writeLinkElement:(NSDictionary *)element withPasteboardTypes:(NSArray *)types toPasteboard:(NSPasteboard *)pasteboard;
- (void)_openFrameInNewWindowFromMenu:(NSMenuItem *)sender;
- (void)_searchWithGoogleFromMenu:(id)sender;
- (void)_searchWithSpotlightFromMenu:(id)sender;
- (void)_progressCompleted:(WebFrame *)frame;
- (void)_didCommitLoadForFrame:(WebFrame *)frame;
- (void)_didFinishLoadForFrame:(WebFrame *)frame;
- (void)_didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame;
- (void)_didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame;
- (void)_willChangeValueForKey:(NSString *)key;
- (void)_didChangeValueForKey:(NSString *)key;
- (WebBasePluginPackage *)_pluginForMIMEType:(NSString *)MIMEType;
- (WebBasePluginPackage *)_pluginForExtension:(NSString *)extension;
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
- (WebBasePluginPackage *)_videoProxyPluginForMIMEType:(NSString *)MIMEType;
#endif

- (void)setCurrentNodeHighlight:(WebNodeHighlight *)nodeHighlight;
- (WebNodeHighlight *)currentNodeHighlight;

- (void)addPluginInstanceView:(NSView *)view;
- (void)removePluginInstanceView:(NSView *)view;
- (void)removePluginInstanceViewsFor:(WebFrame*)webFrame;

- (void)_addObject:(id)object forIdentifier:(unsigned long)identifier;
- (id)_objectForIdentifier:(unsigned long)identifier;
- (void)_removeObjectForIdentifier:(unsigned long)identifier;

- (void)_setZoomMultiplier:(float)multiplier isTextOnly:(BOOL)isTextOnly;
- (float)_zoomMultiplier:(BOOL)isTextOnly;
- (float)_realZoomMultiplier;
- (BOOL)_realZoomMultiplierIsTextOnly;
- (BOOL)_canZoomOut:(BOOL)isTextOnly;
- (BOOL)_canZoomIn:(BOOL)isTextOnly;
- (IBAction)_zoomOut:(id)sender isTextOnly:(BOOL)isTextOnly;
- (IBAction)_zoomIn:(id)sender isTextOnly:(BOOL)isTextOnly;
- (BOOL)_canResetZoom:(BOOL)isTextOnly;
- (IBAction)_resetZoom:(id)sender isTextOnly:(BOOL)isTextOnly;

+ (BOOL)_canHandleRequest:(NSURLRequest *)request forMainFrame:(BOOL)forMainFrame;

- (void)_setInsertionPasteboard:(NSPasteboard *)pasteboard;

- (void)_preferencesChanged:(WebPreferences *)preferences;

#if ENABLE(VIDEO) && defined(__cplusplus)
- (void)_enterFullscreenForNode:(WebCore::Node*)node;
- (void)_exitFullscreen;
#endif

#if ENABLE(FULLSCREEN_API) && defined(__cplusplus)
- (BOOL)_supportsFullScreenForElement:(WebCore::Element*)element withKeyboard:(BOOL)withKeyboard;
- (void)_enterFullScreenForElement:(WebCore::Element*)element;
- (void)_exitFullScreenForElement:(WebCore::Element*)element;
- (void)_fullScreenRendererChanged:(WebCore::RenderBox*)renderer;
#endif

// Conversion functions between WebCore root view coordinates and web view coordinates.
- (NSPoint)_convertPointFromRootView:(NSPoint)point;
- (NSRect)_convertRectFromRootView:(NSRect)rect;

@end
