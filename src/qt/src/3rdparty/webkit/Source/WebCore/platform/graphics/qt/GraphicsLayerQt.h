/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef GraphicsLayerQt_h
#define GraphicsLayerQt_h

#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"

#if !defined(QT_NO_GRAPHICSVIEW)

namespace WebCore {

class GraphicsLayerQtImpl;

class GraphicsLayerQt : public GraphicsLayer {
    friend class GraphicsLayerQtImpl;

public:
    GraphicsLayerQt(GraphicsLayerClient*);
    virtual ~GraphicsLayerQt();

    // reimps from GraphicsLayer.h
    virtual PlatformLayer* platformLayer() const;
    virtual void setNeedsDisplay();
    virtual void setNeedsDisplayInRect(const FloatRect&);
    virtual void setParent(GraphicsLayer* layer);
    virtual void setName(const String& name);
    virtual bool setChildren(const Vector<GraphicsLayer*>&);
    virtual void addChild(GraphicsLayer*);
    virtual void addChildAtIndex(GraphicsLayer*, int index);
    virtual void addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual void addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild);
    virtual void removeFromParent();
    virtual void setMaskLayer(GraphicsLayer* layer);
    virtual void setPosition(const FloatPoint& p);
    virtual void setAnchorPoint(const FloatPoint3D& p);
    virtual void setSize(const FloatSize& size);
    virtual void setTransform(const TransformationMatrix& t);
    virtual void setChildrenTransform(const TransformationMatrix& t);
    virtual void setPreserves3D(bool b);
    virtual void setMasksToBounds(bool b);
    virtual void setDrawsContent(bool b);
    virtual void setBackgroundColor(const Color&);
    virtual void clearBackgroundColor();
    virtual void setContentsOpaque(bool b);
    virtual void setBackfaceVisibility(bool b);
    virtual void setOpacity(float opacity);
    virtual void setContentsRect(const IntRect& r);
#ifndef QT_NO_ANIMATION
    virtual bool addAnimation(const KeyframeValueList&, const IntSize& boxSize, const Animation*, const String& keyframesName, double timeOffset);
    virtual void removeAnimationsForProperty(AnimatedPropertyID);
    virtual void removeAnimationsForKeyframes(const String& keyframesName);
    virtual void pauseAnimation(const String& keyframesName, double timeOffset);
    virtual void suspendAnimations(double time);
    virtual void resumeAnimations();
#endif // QT_NO_ANIMATION
    virtual void setContentsToImage(Image*);
    virtual void setContentsNeedsDisplay();
    virtual void setContentsToMedia(PlatformLayer*);
    virtual void setContentsToCanvas(PlatformLayer*);
    virtual void setContentsBackgroundColor(const Color&);
    virtual void setContentsOrientation(CompositingCoordinatesOrientation orientation);
    virtual void distributeOpacity(float);
    virtual float accumulatedOpacity() const;
    virtual void syncCompositingState();
    virtual void syncCompositingStateForThisLayerOnly();

private:
    OwnPtr<GraphicsLayerQtImpl> m_impl;
};

}
#endif
#endif // GraphicsLayerQt_h
