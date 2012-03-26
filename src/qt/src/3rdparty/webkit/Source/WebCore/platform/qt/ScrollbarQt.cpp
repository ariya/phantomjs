/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Staikos Computing Services Inc. <info@staikos.net>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Scrollbar.h"

#include "EventHandler.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "PlatformMouseEvent.h"
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QPainter>
#include <QStyle>

using namespace std;

namespace WebCore {

bool Scrollbar::contextMenu(const PlatformMouseEvent& event)
{
#ifndef QT_NO_CONTEXTMENU
    if (!QApplication::style()->styleHint(QStyle::SH_ScrollBar_ContextMenu))
        return true;

    bool horizontal = (m_orientation == HorizontalScrollbar);

    QMenu menu;
    QAction* actScrollHere = menu.addAction(QCoreApplication::translate("QWebPage", "Scroll here"));
    menu.addSeparator();

    QAction* actScrollTop = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Left edge") : QCoreApplication::translate("QWebPage", "Top"));
    QAction* actScrollBottom = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Right edge") : QCoreApplication::translate("QWebPage", "Bottom"));
    menu.addSeparator();

    QAction* actPageUp = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Page left") : QCoreApplication::translate("QWebPage", "Page up"));
    QAction* actPageDown = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Page right") : QCoreApplication::translate("QWebPage", "Page down"));
    menu.addSeparator();

    QAction* actScrollUp = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Scroll left") : QCoreApplication::translate("QWebPage", "Scroll up"));
    QAction* actScrollDown = menu.addAction(horizontal ? QCoreApplication::translate("QWebPage", "Scroll right") : QCoreApplication::translate("QWebPage", "Scroll down"));

    const QPoint globalPos = QPoint(event.globalX(), event.globalY());
    QAction* actionSelected = menu.exec(globalPos);

    if (actionSelected == actScrollHere) {
        // Set the pressed position to the middle of the thumb so that when we 
        // do move, the delta will be from the current pixel position of the
        // thumb to the new position
        int position = theme()->trackPosition(this) + theme()->thumbPosition(this) + theme()->thumbLength(this) / 2;
        setPressedPos(position); 
        const QPoint pos = convertFromContainingWindow(event.pos());
        moveThumb(horizontal ? pos.x() : pos.y());
    } else if (actionSelected == actScrollTop)
        scrollableArea()->scroll(horizontal ? ScrollLeft : ScrollUp, ScrollByDocument);
    else if (actionSelected == actScrollBottom)
        scrollableArea()->scroll(horizontal ? ScrollRight : ScrollDown, ScrollByDocument);
    else if (actionSelected == actPageUp)
        scrollableArea()->scroll(horizontal ? ScrollLeft : ScrollUp, ScrollByPage);
    else if (actionSelected == actPageDown)
        scrollableArea()->scroll(horizontal ? ScrollRight : ScrollDown, ScrollByPage);
    else if (actionSelected == actScrollUp)
        scrollableArea()->scroll(horizontal ? ScrollLeft : ScrollUp, ScrollByLine);
    else if (actionSelected == actScrollDown)
        scrollableArea()->scroll(horizontal ? ScrollRight : ScrollDown, ScrollByLine);
#endif // QT_NO_CONTEXTMENU
    return true;
}

}

// vim: ts=4 sw=4 et
