/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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
#import "qnstitledframe_mac_p.h"

@interface NSThemeFrame : NSTitledFrame
{
    NSButton *toolbarButton;
    int toolbarVisibleStatus;
    NSImage *showToolbarTransitionImage;
    NSSize showToolbarPreWindowSize;
    NSButton *modeButton;
    int leftGroupTrackingTagNum;
    int rightGroupTrackingTagNum;
    char mouseInsideLeftGroup;
    char mouseInsideRightGroup;
    int widgetState;
    NSString *displayName;
}

+ (void)initialize;
+ (float)_windowBorderThickness:(unsigned int)fp8;
+ (float)_minXWindowBorderWidth:(unsigned int)fp8;
+ (float)_maxXWindowBorderWidth:(unsigned int)fp8;
+ (float)_minYWindowBorderHeight:(unsigned int)fp8;
+ (float)_windowTitlebarButtonSpacingWidth:(unsigned int)fp8;
+ (float)_windowFileButtonSpacingWidth:(unsigned int)fp8;
+ (float)_minXTitlebarWidgetInset:(unsigned int)fp8;
+ (float)_maxXTitlebarWidgetInset:(unsigned int)fp8;
+ (float)minFrameWidthWithTitle:fp8 styleMask:(unsigned int)fp12;
+ (float)_windowSideTitlebarTitleMinWidth:(unsigned int)fp8;
+ (float)_windowTitlebarTitleMinHeight:(unsigned int)fp8;
+ (float)_sideTitlebarWidth:(unsigned int)fp8;
+ (float)_titlebarHeight:(unsigned int)fp8;
+ (float)_resizeHeight:(unsigned int)fp8;
+ (char)_resizeFromEdge;
+ (struct _NSSize)sizeOfTitlebarButtons:(unsigned int)fp8;
+ (float)_contentToFrameMinXWidth:(unsigned int)fp8;
+ (float)_contentToFrameMaxXWidth:(unsigned int)fp8;
+ (float)_contentToFrameMinYHeight:(unsigned int)fp8;
+ (float)_contentToFrameMaxYHeight:(unsigned int)fp8;
+ (unsigned int)_validateStyleMask:(unsigned int)fp8;
- (struct _NSSize)_topCornerSize;
- (struct _NSSize)_bottomCornerSize;
- (void *)_createWindowOpaqueShape;
- (void)shapeWindow;
- (void)_recursiveDisplayRectIfNeededIgnoringOpacity:(NSRect)fp8 isVisibleRect:(char)fp24 rectIsVisibleRectForView:fp28 topView:(char)fp32;
- (void *)_regionForOpaqueDescendants:(NSRect)fp8 forMove:(char)fp24;
- (void)_drawFrameInterior:(NSRect *)fp8 clip:(NSRect)fp12;
- (void)_setTextShadow:(char)fp8;
- (void)_drawTitleBar:(NSRect)fp8;
- (void)_drawResizeIndicators:(NSRect)fp8;
- (void)_drawFrameRects:(NSRect)fp8;
- (void)drawFrame:(NSRect)fp8;
- contentFill;
- (void)viewDidEndLiveResize;
- (float)contentAlpha;
- (void)setThemeFrameWidgetState:(int)fp8;
- (char)constrainResizeEdge:(int *)fp8 withDelta:(struct _NSSize)fp12 elapsedTime:(float)fp20;
- (void)addFileButton:fp8;
- (void)_updateButtons;
- (void)_updateButtonState;
- newCloseButton;
- newZoomButton;
- newMiniaturizeButton;
- newToolbarButton;
- newFileButton;
- (void)_resetTitleBarButtons;
- (void)setDocumentEdited:(char)fp8;
- toolbarButton;
- modeButton;
- initWithFrame:(NSRect)fp8 styleMask:(unsigned int)fp24 owner:fp28;
- (void)dealloc;
- (void)setFrameSize:(struct _NSSize)fp8;
- (char)_canHaveToolbar;
- (char)_toolbarIsInTransition;
- (char)_toolbarIsShown;
- (char)_toolbarIsHidden;
- _toolbarView;
- _toolbar;
- (float)_distanceFromToolbarBaseToTitlebar;
- (unsigned int)_shadowFlags;
- (NSRect)frameRectForContentRect:(NSRect)fp8 styleMask:(unsigned int)fp24;
- (NSRect)contentRectForFrameRect:(NSRect)fp8 styleMask:(unsigned int)fp24;
- (struct _NSSize)minFrameSizeForMinContentSize:(struct _NSSize)fp8 styleMask:(unsigned int)fp16;
- (NSRect)contentRect;
- (NSRect)_contentRectExcludingToolbar;
- (NSRect)_contentRectIncludingToolbarAtHome;
- (void)_setToolbarShowHideResizeWeightingOptimizationOn:(char)fp8;
- (char)_usingToolbarShowHideWeightingOptimization;
- (void)handleSetFrameCommonRedisplay;
- (void)_startLiveResizeAsTopLevel;
- (void)_endLiveResizeAsTopLevel;
- (void)_growContentReshapeContentAndToolbarView:(int)fp8 animate:(char)fp12;
- (char)_growWindowReshapeContentAndToolbarView:(int)fp8 animate:(char)fp12;
- (void)_reshapeContentAndToolbarView:(int)fp8 resizeWindow:(char)fp12 animate:(char)fp16;
- (void)_toolbarFrameSizeChanged:fp8 oldSize:(struct _NSSize)fp12;
- (void)_syncToolbarPosition;
- (void)_showHideToolbar:(int)fp8 resizeWindow:(char)fp12 animate:(char)fp16;
- (void)_showToolbarWithAnimation:(char)fp8;
- (void)_hideToolbarWithAnimation:(char)fp8;
- (void)_drawToolbarTransitionIfNecessary;
- (void)drawRect:(NSRect)fp8;
- (void)resetCursorRects;
- (char)shouldBeTreatedAsInkEvent:fp8;
- (char)_shouldBeTreatedAsInkEventInInactiveWindow:fp8;
//- hitTest:(struct _NSPoint)fp8; // collides with hittest in qcocoasharedwindowmethods_mac_p.h
- (NSRect)_leftGroupRect;
- (NSRect)_rightGroupRect;
- (void)_updateWidgets;
- (void)_updateMouseTracking;
- (void)mouseEntered:fp8;
- (void)mouseExited:fp8;
- (void)_setMouseEnteredGroup:(char)fp8 entered:(char)fp12;
- (char)_mouseInGroup:fp8;
- (struct _NSSize)miniaturizedSize;
- (float)_minXTitlebarDecorationMinWidth;
- (float)_maxXTitlebarDecorationMinWidth;
- (struct _NSSize)minFrameSize;
- (float)_windowBorderThickness;
- (float)_windowTitlebarXResizeBorderThickness;
- (float)_windowTitlebarYResizeBorderThickness;
- (float)_windowResizeBorderThickness;
- (float)_minXWindowBorderWidth;
- (float)_maxXWindowBorderWidth;
- (float)_minYWindowBorderHeight;
- (float)_maxYWindowBorderHeight;
- (float)_minYTitlebarButtonsOffset;
- (float)_minYTitlebarTitleOffset;
- (float)_sideTitlebarWidth;
- (float)_titlebarHeight;
- (NSRect)_titlebarTitleRect;
- (NSRect)titlebarRect;
- (float)_windowTitlebarTitleMinHeight;
- (struct _NSSize)_sizeOfTitlebarFileButton;
- (struct _NSSize)sizeOfTitlebarToolbarButton;
- (float)_windowTitlebarButtonSpacingWidth;
- (float)_windowFileButtonSpacingWidth;
- (float)_minXTitlebarWidgetInset;
- (float)_maxXTitlebarWidgetInset;
- (float)_minXTitlebarButtonsWidth;
- (float)_maxXTitlebarButtonsWidth;
- (struct _NSPoint)_closeButtonOrigin;
- (struct _NSPoint)_zoomButtonOrigin;
- (struct _NSPoint)_collapseButtonOrigin;
- (struct _NSPoint)_toolbarButtonOrigin;
- (struct _NSPoint)_fileButtonOrigin;
- (void)_tileTitlebar;
- (NSRect)_commandPopupRect;
- (void)_resetDragMargins;
- (float)_maxYTitlebarDragHeight;
- (float)_minXTitlebarDragWidth;
- (float)_maxXTitlebarDragWidth;
- (float)_contentToFrameMinXWidth;
- (float)_contentToFrameMaxXWidth;
- (float)_contentToFrameMinYHeight;
- (float)_contentToFrameMaxYHeight;
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
- (void)_setUtilityWindow:(char)fp8;
- (char)_isUtility;
- (float)_sheetHeightAdjustment;
- (void)_setSheet:(char)fp8;
- (char)_isSheet;
- (char)_isResizable;
- (char)_isClosable;
- (char)_isMiniaturizable;
- (char)_hasToolbar;
- (NSRect)_growBoxRect;
- (void)_drawGrowBoxWithClip:(NSRect)fp8;
- (char)_inactiveButtonsNeedMask;
- (void)mouseDown:fp8;
- _displayName;
- (void)_setDisplayName:fp8;

@end
