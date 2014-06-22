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

#include "qcocoadrag.h"
#include "qmacclipboard.h"
#include "qcocoahelpers.h"

QT_BEGIN_NAMESPACE

QCocoaDrag::QCocoaDrag() :
    m_drag(0)
{
    m_lastEvent = 0;
    m_lastView = 0;
}

QCocoaDrag::~QCocoaDrag()
{
    [m_lastEvent release];
}

void QCocoaDrag::setLastMouseEvent(NSEvent *event, NSView *view)
{
    [m_lastEvent release];
    m_lastEvent = [event copy];
    m_lastView = view;
}

QMimeData *QCocoaDrag::platformDropData()
{
    if (m_drag)
        return m_drag->mimeData();

    return 0;
}

Qt::DropAction QCocoaDrag::defaultAction(Qt::DropActions possibleActions,
                                           Qt::KeyboardModifiers modifiers) const
{
    Qt::DropAction default_action = Qt::IgnoreAction;

    if (currentDrag()) {
        default_action = currentDrag()->defaultAction();
        possibleActions = currentDrag()->supportedActions();
    }

    if (default_action == Qt::IgnoreAction) {
        //This means that the drag was initiated by QDrag::start and we need to
        //preserve the old behavior
        default_action = Qt::CopyAction;
    }

    if (modifiers & Qt::ControlModifier && modifiers & Qt::AltModifier)
        default_action = Qt::LinkAction;
    else if (modifiers & Qt::AltModifier)
        default_action = Qt::CopyAction;
    else if (modifiers & Qt::ControlModifier)
        default_action = Qt::MoveAction;

#ifdef QDND_DEBUG
    qDebug("possible actions : %s", dragActionsToString(possibleActions).latin1());
#endif

    // Check if the action determined is allowed
    if (!(possibleActions & default_action)) {
        if (possibleActions & Qt::CopyAction)
            default_action = Qt::CopyAction;
        else if (possibleActions & Qt::MoveAction)
            default_action = Qt::MoveAction;
        else if (possibleActions & Qt::LinkAction)
            default_action = Qt::LinkAction;
        else
            default_action = Qt::IgnoreAction;
    }

#ifdef QDND_DEBUG
    qDebug("default action : %s", dragActionsToString(default_action).latin1());
#endif

    return default_action;
}


Qt::DropAction QCocoaDrag::drag(QDrag *o)
{
    m_drag = o;
    m_executed_drop_action = Qt::IgnoreAction;

    QPixmap pm = m_drag->pixmap();
    if (pm.isNull())
        pm = defaultPixmap();

    NSImage *nsimage = qt_mac_create_nsimage(pm);

    QMacPasteboard dragBoard((CFStringRef) NSDragPboard, QMacInternalPasteboardMime::MIME_DND);
    m_drag->mimeData()->setData(QLatin1String("application/x-qt-mime-type-name"), QByteArray("dummy"));
    dragBoard.setMimeData(m_drag->mimeData());

    NSPoint event_location = [m_lastEvent locationInWindow];
    NSPoint local_point = [m_lastView convertPoint:event_location fromView:nil];
    local_point.x -= m_drag->hotSpot().x();
    CGFloat flippedY = m_drag->pixmap().height() - m_drag->hotSpot().y();
    local_point.y += flippedY;
    NSSize mouseOffset = NSMakeSize(0.0, 0.0);
    NSPasteboard *pboard = [NSPasteboard pasteboardWithName:NSDragPboard];

    [m_lastView dragImage:nsimage
        at:local_point
        offset:mouseOffset
        event:m_lastEvent
        pasteboard:pboard
        source:m_lastView
        slideBack:YES];

    [nsimage release];

    m_drag = 0;
    return m_executed_drop_action;
}

void QCocoaDrag::setAcceptedAction(Qt::DropAction act)
{
    m_executed_drop_action = act;
}

QCocoaDropData::QCocoaDropData(NSPasteboard *pasteboard)
{
    dropPasteboard = reinterpret_cast<CFStringRef>(const_cast<const NSString *>([pasteboard name]));
    CFRetain(dropPasteboard);
}

QCocoaDropData::~QCocoaDropData()
{
    CFRelease(dropPasteboard);
}

QStringList QCocoaDropData::formats_sys() const
{
    QStringList formats;
    PasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return formats;
    }
    formats = QMacPasteboard(board, QMacInternalPasteboardMime::MIME_DND).formats();
    return formats;
}

QVariant QCocoaDropData::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    QVariant data;
    PasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return data;
    }
    data = QMacPasteboard(board, QMacInternalPasteboardMime::MIME_DND).retrieveData(mimeType, type);
    CFRelease(board);
    return data;
}

bool QCocoaDropData::hasFormat_sys(const QString &mimeType) const
{
    bool has = false;
    PasteboardRef board;
    if (PasteboardCreate(dropPasteboard, &board) != noErr) {
        qDebug("DnD: Cannot get PasteBoard!");
        return has;
    }
    has = QMacPasteboard(board, QMacInternalPasteboardMime::MIME_DND).hasFormat(mimeType);
    CFRelease(board);
    return has;
}


QT_END_NAMESPACE

