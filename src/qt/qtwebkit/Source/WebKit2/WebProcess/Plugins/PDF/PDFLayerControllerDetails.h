/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import <PDFKit/PDFKit.h>

@class CPReadingModel;

@protocol PDFLayerControllerDelegate <NSObject>

- (void)updateScrollPosition:(CGPoint)newPosition;
- (void)writeItemsToPasteboard:(NSArray *)items withTypes:(NSArray *)types;
- (void)showDefinitionForAttributedString:(NSAttributedString *)string atPoint:(CGPoint)point;
- (void)performWebSearch:(NSString *)string;
- (void)performSpotlightSearch:(NSString *)string;
- (void)openWithNativeApplication;
- (void)saveToPDF;

- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeActiveAnnotation:(PDFAnnotation *)annotation;
- (void)pdfLayerController:(PDFLayerController *)pdfLayerController clickedLinkWithURL:(NSURL *)url;
- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeContentScaleFactor:(CGFloat)scaleFactor;
- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeDisplayMode:(int)mode;
- (void)pdfLayerController:(PDFLayerController *)pdfLayerController didChangeSelection:(PDFSelection *)selection;

@end

@interface PDFLayerController : NSObject
@end

@interface PDFLayerController (Details)

@property(retain) CALayer *parentLayer;
@property(retain) PDFDocument *document;
@property(retain) id<PDFLayerControllerDelegate> delegate;

- (void)setFrameSize:(CGSize)size;

- (PDFDisplayMode)displayMode;
- (void)setDisplayMode:(PDFDisplayMode)mode;
- (void)setDisplaysPageBreaks:(BOOL)pageBreaks;

- (CGFloat)contentScaleFactor;
- (void)setContentScaleFactor:(CGFloat)scaleFactor;

- (CGFloat)deviceScaleFactor;
- (void)setDeviceScaleFactor:(CGFloat)scaleFactor;

- (CGSize)contentSize;
- (CGSize)contentSizeRespectingZoom;

- (void)snapshotInContext:(CGContextRef)context;

- (void)magnifyWithMagnification:(CGFloat)magnification atPoint:(CGPoint)point immediately:(BOOL)immediately;

- (CGPoint)scrollPosition;
- (void)setScrollPosition:(CGPoint)newPosition;
- (void)scrollWithDelta:(CGSize)delta;

- (void)mouseDown:(NSEvent *)event;
- (void)rightMouseDown:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;
- (void)mouseEntered:(NSEvent *)event;
- (void)mouseExited:(NSEvent *)event;

- (NSMenu *)menuForEvent:(NSEvent *)event;

- (NSArray *)findString:(NSString *)string caseSensitive:(BOOL)isCaseSensitive highlightMatches:(BOOL)shouldHighlightMatches;

- (PDFSelection *)currentSelection;
- (void)setCurrentSelection:(PDFSelection *)selection;
- (PDFSelection *)searchSelection;
- (void)setSearchSelection:(PDFSelection *)selection;
- (void)gotoSelection:(PDFSelection *)selection;
- (PDFSelection *)getSelectionForWordAtPoint:(CGPoint)point;

- (NSUInteger)lastPageIndex;
- (NSUInteger)currentPageIndex;
- (void)gotoNextPage;
- (void)gotoPreviousPage;

- (void)copySelection;
- (void)selectAll;

- (bool)keyDown:(NSEvent *)event;

- (void)setHUDEnabled:(BOOL)enabled;
- (BOOL)hudEnabled;

- (CGRect)boundsForAnnotation:(PDFAnnotation *)annotation;
- (void)activateNextAnnotation:(BOOL)previous;

- (void)attemptToUnlockWithPassword:(NSString *)password;

- (void)searchInDictionaryWithSelection:(PDFSelection *)selection;

// Accessibility

- (CPReadingModel *)readingModel;
- (id)accessibilityFocusedUIElement;
- (NSArray *)accessibilityAttributeNames;
- (BOOL)accessibilityIsAttributeSettable:(NSString *)attribute;
- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute;
- (NSArray *)accessibilityParameterizedAttributeNames;
- (NSString *)accessibilityRoleAttribute;
- (NSString *)accessibilityRoleDescriptionAttribute;
- (NSString *)accessibilityValueAttribute;
- (BOOL)accessibilityIsValueAttributeSettable;
- (NSString *)accessibilitySelectedTextAttribute;
- (BOOL)accessibilityIsSelectedTextAttributeSettable;
- (NSValue *)accessibilitySelectedTextRangeAttribute;
- (NSNumber *)accessibilityNumberOfCharactersAttribute;
- (BOOL)accessibilityIsNumberOfCharactersAttributeSettable;
- (NSValue *)accessibilityVisibleCharacterRangeAttribute;
- (BOOL)accessibilityIsVisibleCharacterRangeAttributeSettable;
- (NSNumber *)accessibilityLineForIndexAttributeForParameter:(id)parameter;
- (NSValue *)accessibilityRangeForLineAttributeForParameter:(id)parameter;
- (NSString *)accessibilityStringForRangeAttributeForParameter:(id)parameter;
- (NSValue *)accessibilityBoundsForRangeAttributeForParameter:(id)parameter;

@end
