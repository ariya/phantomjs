/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderEmbeddedObject_h
#define RenderEmbeddedObject_h

#include "RenderPart.h"

namespace WebCore {

class MouseEvent;
class TextRun;

// Renderer for embeds and objects, often, but not always, rendered via plug-ins.
// For example, <embed src="foo.html"> does not invoke a plug-in.
class RenderEmbeddedObject : public RenderPart {
public:
    RenderEmbeddedObject(Element*);
    virtual ~RenderEmbeddedObject();

    enum PluginUnavailabilityReason {
        PluginMissing,
        PluginCrashed,
        PluginBlockedByContentSecurityPolicy,
        InsecurePluginVersion,
    };
    void setPluginUnavailabilityReason(PluginUnavailabilityReason);
    void setPluginUnavailabilityReasonWithDescription(PluginUnavailabilityReason, const String& description);

    bool isPluginUnavailable() const { return m_isPluginUnavailable; }
    bool showsUnavailablePluginIndicator() const { return isPluginUnavailable() && !m_isUnavailablePluginIndicatorHidden; }

    void setUnavailablePluginIndicatorIsHidden(bool);

    // FIXME: This belongs on HTMLObjectElement.
    bool hasFallbackContent() const { return m_hasFallbackContent; }
    void setHasFallbackContent(bool hasFallbackContent) { m_hasFallbackContent = hasFallbackContent; }

    void handleUnavailablePluginIndicatorEvent(Event*);

    bool isReplacementObscured() const;

#if USE(ACCELERATED_COMPOSITING)
    virtual bool allowsAcceleratedCompositing() const;
#endif

protected:
    virtual void paintReplaced(PaintInfo&, const LayoutPoint&);
    virtual void paint(PaintInfo&, const LayoutPoint&);

    virtual CursorDirective getCursor(const LayoutPoint&, Cursor&) const;

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

protected:
    virtual void layout() OVERRIDE;

private:
    virtual const char* renderName() const { return "RenderEmbeddedObject"; }
    virtual bool isEmbeddedObject() const { return true; }

    void paintSnapshotImage(PaintInfo&, const LayoutPoint&, Image*);
    virtual void paintContents(PaintInfo&, const LayoutPoint&) OVERRIDE;

#if USE(ACCELERATED_COMPOSITING)
    virtual bool requiresLayer() const;
#endif

    virtual void viewCleared();

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;

    virtual bool scroll(ScrollDirection, ScrollGranularity, float multiplier, Node** stopNode);
    virtual bool logicalScroll(ScrollLogicalDirection, ScrollGranularity, float multiplier, Node** stopNode);

    void setUnavailablePluginIndicatorIsPressed(bool);
    bool isInUnavailablePluginIndicator(MouseEvent*) const;
    bool isInUnavailablePluginIndicator(const LayoutPoint&) const;
    bool getReplacementTextGeometry(const LayoutPoint& accumulatedOffset, FloatRect& contentRect, Path&, FloatRect& replacementTextRect, FloatRect& arrowRect, Font&, TextRun&, float& textWidth) const;
    LayoutRect unavailablePluginIndicatorBounds(const LayoutPoint&) const;

    virtual bool canHaveChildren() const;
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }
    
    virtual bool canHaveWidget() const { return true; }

    bool m_hasFallbackContent; // FIXME: This belongs on HTMLObjectElement.

    bool m_isPluginUnavailable;
    bool m_isUnavailablePluginIndicatorHidden;
    PluginUnavailabilityReason m_pluginUnavailabilityReason;
    String m_unavailablePluginReplacementText;
    bool m_unavailablePluginIndicatorIsPressed;
    bool m_mouseDownWasInUnavailablePluginIndicator;
    RenderObjectChildList m_children;
    String m_unavailabilityDescription;
};

inline RenderEmbeddedObject* toRenderEmbeddedObject(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isEmbeddedObject());
    return static_cast<RenderEmbeddedObject*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderEmbeddedObject(const RenderEmbeddedObject*);

} // namespace WebCore

#endif // RenderEmbeddedObject_h
