/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp, qcolor_x11.cpp, qfiledialog.cpp
// and many other.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

// Private AppKit class (dumped from classdump).

#import <Cocoa/Cocoa.h>
#import "qnsframeview_mac_p.h"


@interface NSTitledFrame : NSFrameView
{
    int resizeFlags;
    id fileButton;      /* NSDocumentDragButton* */
    NSSize titleCellSize;
}

+ (float)_windowBorderThickness:(unsigned int)fp8;
+ (float)_minXWindowBorderWidth:(unsigned int)fp8;
+ (float)_maxXWindowBorderWidth:(unsigned int)fp8;
+ (float)_minYWindowBorderHeight:(unsigned int)fp8;
+ (char)_resizeFromEdge;
+ (NSRect)frameRectForContentRect:(NSRect)fp8 styleMask:(unsigned int)fp24;
+ (NSRect)contentRectForFrameRect:(NSRect)fp8 styleMask:(unsigned int)fp24;
+ (struct _NSSize)minFrameSizeForMinContentSize:(struct _NSSize)fp8 styleMask:(unsigned int)fp16;
+ (struct _NSSize)minContentSizeForMinFrameSize:(struct _NSSize)fp8 styleMask:(unsigned int)fp16;
+ (float)minFrameWidthWithTitle:fp8 styleMask:(unsigned int)fp12;
+ (struct _NSSize)_titleCellSizeForTitle:fp8 styleMask:(unsigned int)fp12;
+ (float)_titleCellHeight:(unsigned int)fp8;
+ (float)_windowTitlebarTitleMinHeight:(unsigned int)fp8;
+ (float)_titlebarHeight:(unsigned int)fp8;
+ (struct _NSSize)sizeOfTitlebarButtons:(unsigned int)fp8;
+ (float)windowTitlebarLinesSpacingWidth:(unsigned int)fp8;
+ (float)windowTitlebarTitleLinesSpacingWidth:(unsigned int)fp8;
+ (float)_contentToFrameMinXWidth:(unsigned int)fp8;
+ (float)_contentToFrameMaxXWidth:(unsigned int)fp8;
+ (float)_contentToFrameMinYHeight:(unsigned int)fp8;
+ (float)_contentToFrameMaxYHeight:(unsigned int)fp8;
- initWithFrame:(NSRect)fp8 styleMask:(unsigned int)fp24 owner:fp28;
- (void)dealloc;
- (void)setIsClosable:(char)fp8;
- (void)setIsResizable:(char)fp8;
- (void)_resetTitleFont;
- (void)_setUtilityWindow:(char)fp8;
- (char)isOpaque;
- (char)worksWhenModal;
- (void)propagateFrameDirtyRects:(NSRect)fp8;
- (void)_showDrawRect:(NSRect)fp8;
- (void)_drawFrameInterior:(NSRect *)fp8 clip:(NSRect)fp12;
- (void)drawFrame:(NSRect)fp8;
- (void)_drawFrameRects:(NSRect)fp8;
- (void)_drawTitlebar:(NSRect)fp8;
- (void)_drawTitlebarPattern:(int)fp8 inRect:(NSRect)fp12 clippedByRect:(NSRect)fp28 forKey:(char)fp44 alignment:(int)fp48;
- (void)_drawTitlebarLines:(int)fp8 inRect:(NSRect)fp12 clippedByRect:(NSRect)fp28;
- frameHighlightColor;
- frameShadowColor;
- (void)setFrameSize:(struct _NSSize)fp8;
- (void)setFrameOrigin:(struct _NSPoint)fp8;
- (void)tileAndSetWindowShape:(char)fp8;
- (void)tile;
- (void)_tileTitlebar;
- (void)setTitle:fp8;
- (char)_shouldRepresentFilename;
- (void)setRepresentedFilename:fp8;
- (void)_drawTitleStringIn:(NSRect)fp8 withColor:fp24;
- titleFont;
- (void)_drawResizeIndicators:(NSRect)fp8;
- titleButtonOfClass:(Class)fp8;
- initTitleButton:fp8;
- newCloseButton;
- newZoomButton;
- newMiniaturizeButton;
- newFileButton;
- fileButton;
- (void)_removeButtons;
- (void)_updateButtons;
- (char)_eventInTitlebar:fp8;
- (char)acceptsFirstMouse:fp8;
- (void)mouseDown:fp8;
- (void)mouseUp:fp8;
- (void)rightMouseDown:fp8;
- (void)rightMouseUp:fp8;
- (int)resizeEdgeForEvent:fp8;
- (struct _NSSize)_resizeDeltaFromPoint:(struct _NSPoint)fp8 toEvent:fp16;
- (NSRect)_validFrameForResizeFrame:(NSRect)fp8 fromResizeEdge:(int)fp24;
- (NSRect)frame:(NSRect)fp8 resizedFromEdge:(int)fp24 withDelta:(struct _NSSize)fp28;
- (char)constrainResizeEdge:(int *)fp8 withDelta:(struct _NSSize)fp12 elapsedTime:(float)fp20;
- (void)resizeWithEvent:fp8;
- (int)resizeFlags;
- (void)resetCursorRects;
- (void)setDocumentEdited:(char)fp8;
- (struct _NSSize)miniaturizedSize;
- (struct _NSSize)minFrameSize;
- (float)_windowBorderThickness;
- (float)_windowTitlebarXResizeBorderThickness;
- (float)_windowTitlebarYResizeBorderThickness;
- (float)_windowResizeBorderThickness;
- (float)_minXWindowBorderWidth;
- (float)_maxXWindowBorderWidth;
- (float)_minYWindowBorderHeight;
- (void)_invalidateTitleCellSize;
- (void)_invalidateTitleCellWidth;
- (float)_titleCellHeight;
- (struct _NSSize)_titleCellSize;
- (float)_titlebarHeight;
- (NSRect)titlebarRect;
- (NSRect)_maxTitlebarTitleRect;
- (NSRect)_titlebarTitleRect;
- (float)_windowTitlebarTitleMinHeight;
- (NSRect)dragRectForFrameRect:(NSRect)fp8;
- (struct _NSSize)sizeOfTitlebarButtons;
- (struct _NSSize)_sizeOfTitlebarFileButton;
- (float)_windowTitlebarButtonSpacingWidth;
- (float)_minXTitlebarButtonsWidth;
- (float)_maxXTitlebarButtonsWidth;
- (int)_numberOfTitlebarLines;
- (float)windowTitlebarLinesSpacingWidth;
- (float)windowTitlebarTitleLinesSpacingWidth;
- (float)_minLinesWidthWithSpace;
- (NSRect)_minXTitlebarLinesRectWithTitleCellRect:(NSRect)fp8;
- (NSRect)_maxXTitlebarLinesRectWithTitleCellRect:(NSRect)fp8;
- (float)_minXTitlebarDecorationMinWidth;
- (float)_maxXTitlebarDecorationMinWidth;
- (struct _NSPoint)_closeButtonOrigin;
- (struct _NSPoint)_zoomButtonOrigin;
- (struct _NSPoint)_collapseButtonOrigin;
- (struct _NSPoint)_fileButtonOrigin;
- (float)_maxYTitlebarDragHeight;
- (float)_minXTitlebarDragWidth;
- (float)_maxXTitlebarDragWidth;
- (float)_contentToFrameMinXWidth;
- (float)_contentToFrameMaxXWidth;
- (float)_contentToFrameMinYHeight;
- (float)_contentToFrameMaxYHeight;
- (NSRect)contentRect;
- (float)_windowResizeCornerThickness;
- (NSRect)_minYResizeRect;
- (NSRect)_minYminXResizeRect;
- (NSRect)_minYmaxXResizeRect;
- (NSRect)_minXResizeRect;
- (NSRect)_minXminYResizeRect;
- (NSRect)_minXmaxYResizeRect;
- (NSRect)_maxYResizeRect;
- (NSRect)_maxYminXResizeRect;
- (NSRect)_maxYmaxXResizeRect;
- (NSRect)_maxXResizeRect;
- (NSRect)_maxXminYResizeRect;
- (NSRect)_maxXmaxYResizeRect;
- (NSRect)_minXTitlebarResizeRect;
- (NSRect)_maxXTitlebarResizeRect;
- (NSRect)_minXBorderRect;
- (NSRect)_maxXBorderRect;
- (NSRect)_maxYBorderRect;
- (NSRect)_minYBorderRect;

@end
