/* 
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2006 Apple Computer, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef MouseEventWithHitTestResults_h
#define MouseEventWithHitTestResults_h

#include "HitTestResult.h"
#include "PlatformMouseEvent.h"

namespace WebCore {

class Scrollbar;

class MouseEventWithHitTestResults {
public:
    MouseEventWithHitTestResults(const PlatformMouseEvent&, const HitTestResult&);

    const PlatformMouseEvent& event() const { return m_event; }
    const HitTestResult& hitTestResult() const { return m_hitTestResult; }
    LayoutPoint localPoint() const { return m_hitTestResult.localPoint(); }
    Scrollbar* scrollbar() const { return m_hitTestResult.scrollbar(); }
    bool isOverLink() const;
    bool isOverWidget() const { return m_hitTestResult.isOverWidget(); }
    Node* targetNode() const { return m_hitTestResult.targetNode(); }

private:
    PlatformMouseEvent m_event;
    HitTestResult m_hitTestResult;
};

} // namespace WebCore

#endif // MouseEventWithHitTestResults_h
