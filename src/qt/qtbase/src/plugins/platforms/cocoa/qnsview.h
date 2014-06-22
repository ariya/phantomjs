/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QNSVIEW_H
#define QNSVIEW_H

#include <Cocoa/Cocoa.h>

#include <QtCore/QPointer>
#include <QtGui/QImage>
#include <QtGui/QAccessible>

QT_BEGIN_NAMESPACE
class QCocoaWindow;
class QCocoaBackingStore;
class QCocoaGLContext;
QT_END_NAMESPACE

@interface QNSView : NSView <NSTextInputClient> {
    QCocoaBackingStore* m_backingStore;
    QPoint m_backingStoreOffset;
    CGImageRef m_maskImage;
    uchar *m_maskData;
    bool m_shouldInvalidateWindowShadow;
    QWindow *m_window;
    QCocoaWindow *m_platformWindow;
    Qt::MouseButtons m_buttons;
    QString m_composingText;
    bool m_sendKeyEvent;
    QStringList *currentCustomDragTypes;
    bool m_sendUpAsRightButton;
    Qt::KeyboardModifiers currentWheelModifiers;
    bool m_subscribesForGlobalFrameNotifications;
#ifndef QT_NO_OPENGL
    QCocoaGLContext *m_glContext;
    bool m_shouldSetGLContextinDrawRect;
#endif
    NSString *m_inputSource;
}

- (id)init;
- (id)initWithQWindow:(QWindow *)window platformWindow:(QCocoaWindow *) platformWindow;
#ifndef QT_NO_OPENGL
- (void)setQCocoaGLContext:(QCocoaGLContext *)context;
#endif
- (void)flushBackingStore:(QCocoaBackingStore *)backingStore region:(const QRegion &)region offset:(QPoint)offset;
- (void)setMaskRegion:(const QRegion *)region;
- (void)invalidateWindowShadowIfNeeded;
- (void)drawRect:(NSRect)dirtyRect;
- (void)updateGeometry;
- (void)notifyWindowStateChanged:(Qt::WindowState)newState;
- (void)windowNotification : (NSNotification *) windowNotification;
- (void)notifyWindowWillZoom:(BOOL)willZoom;
- (void)viewDidHide;
- (void)viewDidUnhide;

- (BOOL)isFlipped;
- (BOOL)acceptsFirstResponder;
- (BOOL)becomeFirstResponder;
- (BOOL)hasMask;
- (BOOL)isOpaque;

- (void)convertFromScreen:(NSPoint)mouseLocation toWindowPoint:(QPointF *)qtWindowPoint andScreenPoint:(QPointF *)qtScreenPoint;

- (void)resetMouseButtons;

- (void)handleMouseEvent:(NSEvent *)theEvent;
- (void)mouseDown:(NSEvent *)theEvent;
- (void)mouseDragged:(NSEvent *)theEvent;
- (void)mouseUp:(NSEvent *)theEvent;
- (void)mouseMoved:(NSEvent *)theEvent;
- (void)mouseEntered:(NSEvent *)theEvent;
- (void)mouseExited:(NSEvent *)theEvent;
- (void)rightMouseDown:(NSEvent *)theEvent;
- (void)rightMouseDragged:(NSEvent *)theEvent;
- (void)rightMouseUp:(NSEvent *)theEvent;
- (void)otherMouseDown:(NSEvent *)theEvent;
- (void)otherMouseDragged:(NSEvent *)theEvent;
- (void)otherMouseUp:(NSEvent *)theEvent;
- (void)handleFrameStrutMouseEvent:(NSEvent *)theEvent;

- (void)handleTabletEvent: (NSEvent *)theEvent;
- (void)tabletPoint: (NSEvent *)theEvent;
- (void)tabletProximity: (NSEvent *)theEvent;

- (int) convertKeyCode : (QChar)keyCode;
+ (Qt::KeyboardModifiers) convertKeyModifiers : (ulong)modifierFlags;
- (void)handleKeyEvent:(NSEvent *)theEvent eventType:(int)eventType;
- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;
- (BOOL)performKeyEquivalent:(NSEvent *)theEvent;

- (void)registerDragTypes;
- (NSDragOperation)handleDrag:(id <NSDraggingInfo>)sender;

@end

#endif //QNSVIEW_H
